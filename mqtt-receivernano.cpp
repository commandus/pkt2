#include <iostream>
#include <string.h>
#include <stdio.h>

#include <nanomsg/bus.h>
#include <glog/logging.h>

#if defined(_WIN32) || defined(_WIN64)
#else
#endif

#include "platform.h"
#include "mqtt-receivernano.h"
#include "errorcodes.h"
#include "input-packet.h"
#include "helper_socket.h"
#include "utilstring.h"

MQTT_Env::MQTT_Env
(
	Config *aconfig,
	MQTTClient aclient,
	int ananosocket
) 
	: config(aconfig), client(aclient), nano_socket(ananosocket)
{
	
}

int do_connect
(
	Config *config,
	MQTTClient &client
)
{
	int r;
	MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
	conn_opts.keepAliveInterval = config->keep_alive_interval;
	// 0- dirty, please send me old messages
	conn_opts.cleansession = 0;

	if ((r = MQTTClient_connect(client, &conn_opts)) != MQTTCLIENT_SUCCESS)
	{
		LOG(ERROR) << ERR_MQTT_CONNECT_FAIL << r;
		return ERRCODE_MQTT_CONNECT_FAIL;
	}

	const char ** topics = (const char **) malloc(config->topics.size() * sizeof(char *));
	int *qos = (int*) malloc(config->topics.size() * sizeof(int *));
	
	for (int i = 0; i < config->topics.size(); i++)
	{
		topics[i] = config->topics[i].c_str();
		qos[i] = config->qos;
	}

	MQTTClient_subscribeMany(client, config->topics.size(), (char * const*) topics, qos);
	// MQTTClient_subscribe(client, config->topics[0].c_str(), config->qos);
	free(topics);
	free(qos);
	return ERR_OK;
}

void cb_delivered
(
	void *context, 
	MQTTClient_deliveryToken token
)
{
	if (!context)
		return;
	MQTT_Env *env = (MQTT_Env*) context;
    LOG(INFO) << MSG_MQTT_DELIVERED << token;
}

int cb_message_arrived
(
	void *context, 
	char *topicName, 
	int topicLen, 
	MQTTClient_message *message
)
{
	if (!context)
		return 1;
	MQTT_Env *env = (MQTT_Env*) context;
	
	LOG(INFO) << MSG_MQTT_ARRIVED << topicName << ", " << message->payloadlen;
	
	InputPacket packet('T', message->payloadlen);
	packet.setLength(message->payloadlen);
	memmove(packet.data(), message->payload, message->payloadlen);
	
	// send message to the nano queue
	int bytes = nn_send(env->nano_socket, packet.get(), packet.size, 0);
	// flush
	SEND_FLUSH(100);	// BUGBUG 0 - nn_send 
	if (bytes != packet.size)
	{
		if (bytes < 0)
			LOG(ERROR) << ERR_NN_SEND << " " << errno << ": " << nn_strerror(errno);
		else
			LOG(ERROR) << ERR_NN_SEND << bytes << ",  payload " << packet.length;
	}
	else
	{
		if (env->config->verbosity >= 1)
		{
			LOG(INFO) << MSG_NN_SENT_SUCCESS << env->config->message_url << " data: " << bytes << " bytes: " << pkt2utilstring::hexString(packet.get(), packet.size);
			if (env->config->verbosity >= 2)
			{
				std::cerr << MSG_NN_SENT_SUCCESS << env->config->message_url << " data: " << bytes << " bytes: " << pkt2utilstring::hexString(packet.get(), packet.size)
					<< " payload: " << pkt2utilstring::hexString(packet.data(), packet.length) << " bytes: " << packet.length
					<< std::endl;
			}
		}
	}

	MQTTClient_freeMessage(&message);
	MQTTClient_free(topicName);
	return 1;
}

void cb_connection_lost
(
	void *context, 
	char *cause
)
{
	if (!context)
		return;
	MQTT_Env *env = (MQTT_Env*) context;

	std::stringstream ss;
	for (int i = 0; i < env->config->topics.size(); i++)
	{
		ss << env->config->topics[i] << std::endl;
	}

	std::string scause;
	if (cause)
		scause = cause;
	else
		scause = "unknown";
	
	LOG(ERROR) << ERR_MQTT_CONNECTION_LOST << scause << ". Config: " 
		<< env->config->getBrokerAddress() 
		<< " client: " << env->config->client_id
		<< " topics: " << ss.str();
	env->config->stop_request = 2;
}

/**
* Return:  0- success
*          1- can not listen port
*          2- invalid nano socket URL
*          3- buffer allocation error
*          4- send error, re-open 
*/
int mqtt_receiever_nano
(
	Config *config
)
{
START:	
	LOG(INFO) << MSG_START;
	config->stop_request = 0;

	int nano_socket = nn_socket(AF_SP, NN_BUS);
	WAIT_CONNECTION(1); // wait for connections
	int timeout = 100;
	int r = nn_setsockopt(nano_socket, NN_SOL_SOCKET, NN_RCVTIMEO, &timeout, sizeof(timeout));
	if (r < 0)
	{
		LOG(ERROR) << ERR_NN_SET_SOCKET_OPTION << config->message_url << " " << errno << ": " << nn_strerror(errno);
		return ERRCODE_NN_SET_SOCKET_OPTION;
	}

	int eoid = nn_connect(nano_socket, config->message_url.c_str());
    if (eoid < 0)
    {
        LOG(ERROR) << ERR_NN_CONNECT << config->message_url << " " << errno << ": " << nn_strerror(errno);
		return ERRCODE_NN_CONNECT;
    }

	if (config->verbosity >= 2)
	{
		LOG(INFO) << MSG_NN_BIND_SUCCESS << config->message_url << " with time out: " << timeout;
	}

	// MQTT init
	MQTTClient client;
	MQTT_Env env(config, client, nano_socket);
	
	MQTTClient_create(&client, config->getBrokerAddress().c_str(), config->client_id.c_str(), MQTTCLIENT_PERSISTENCE_NONE, NULL);
	MQTTClient_setCallbacks(client, &env, cb_connection_lost, cb_message_arrived, cb_delivered);
	
	do_connect(config, client);

	while (!config->stop_request)
	{
		SLEEP(1);
	}
	r = nn_shutdown(nano_socket, eoid);
	
	MQTTClient_disconnect(client, 10000);
	MQTTClient_destroy(&client);

	LOG(INFO) << MSG_STOP;
	if (config->stop_request == 2)
		goto START;

	if (r < 0)
	{
		LOG(ERROR) << ERR_NN_SHUTDOWN << " " << errno << ": " << nn_strerror(errno);
		return ERRCODE_NN_SHUTDOWN;
	}
	return ERR_OK;
}

/**
* @param config configuration
* @return 0- success
*        1- config is not initialized yet
*/
int stop(Config *config)
{
	if (!config)
		return ERRCODE_NO_CONFIG;
	config->stop_request = 1;
	return ERR_OK;
}

int reload(Config *config)
{
	if (!config)
		return ERRCODE_NO_CONFIG;
	LOG(ERROR) << MSG_RELOAD_BEGIN;
	config->stop_request = 2;
	return ERR_OK;
}
