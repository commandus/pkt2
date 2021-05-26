#include <string>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <utilstring.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>

#include <unistd.h>
#include "argtable3/argtable3.h"

#include <MQTTClient.h>

#include "platform.h"
#include "errorcodes.h"
#include "iridium.h"
#include "helper_socket.h"

#define PROGRAM_NAME		"mqtt-emitter-iridium"
#define PROGRAM_DESCRIPTION "Publish packets #8 (Iridium) to topic pkt2 tcp://127.0.0.1:1883 see proto/iridium/packet8.proto"
#define DEF_DELAY_SEC		1

#define DEF_TOPIC					"pkt2"
#define DEF_PORT					1883
#define DEF_PORT_S					"1883"
#define DEF_ADDRESS					"tcp://127.0.0.1"
#define DEF_QUEUE					"ipc:///tmp/packet.pkt2"
#define DEF_QOS						1
#define DEF_QOS_S					"1"
#define DEF_CLIENT_ID				"mqtt-receiver"
#define DEF_KEEP_ALIVE_INTERVAL		20
#define DEF_KEEP_ALIVE_INTERVAL_S	"20"
#define DEF_PUB_TIMEOUT				10000
#define DEF_PUB_TIMEOUT_S     		"10000"

typedef ALIGN struct {
	IridiumHeader header;
	IridiumHeader h_io;
	IEIOHeader io;
	IridiumHeader h_loc;
	IELocation loc;
	IridiumHeader h_payload;
	packet8_t payload;
} IRIDIUM_PAYLOAD_8;

IMEI imei = 
{
	0x30 + 3,
	0x30 + 0,
	0x30 + 0,
	0x30 + 2,
	0x30 + 3,
	0x30 + 4,
	0x30 + 0,
	0x30 + 6,
	0x30 + 0,
	0x30 + 2,
	0x30 + 3,
	0x30 + 5,
	0x30 + 3,
	0x30 + 4,
	0x30 + 0
};

gps_coord_t gpscoord_yak = 
{
	.latitude_g = 62,
	.latitude_m = 2,
	.latitude_s = 0,
	.latitude_s1 = 0,
	.latitude_ns = 'N',
	.longitude_g = 129,
	.longitude_m = 43,
	.longitude_s = 0,
	.longitude_s1 = 0,
	.longitude_ew = 'E',
	.hdop = 9,
	.pdop = 10
};

void fill_time5
(
	time5 *value, 
	time_t t
)
{
	struct tm *timeinfo;
	timeinfo = localtime(&t);
	
	value->day = timeinfo->tm_mday;
	value->month = timeinfo->tm_mon + 1;
	value->year = timeinfo->tm_year - 100;

	*(uint16_t *) value = htons(*(uint16_t *) value);

	value->hour = timeinfo->tm_hour;
	value->minute = timeinfo->tm_min;
	value->second = timeinfo->tm_sec;
}

/**
 * 01004e01001c515396683330303233343036303233353334300000dd00005701eadf03000b003e09ef81af080000000902001e0800063e01891200004e812bef15000045286321ff00c05c522084041120
 * 81 bytes
 * {"version":1,"size":78,"io":{"cddref":1364432488,"imei":"300234060235340","status":0,"sentno":221,"recvno":0,"recvtime":"2016-04-04T13:17:35","time":1459743455},
 * "location":{"lat":62.04238333,"lon":129.74680000,"cepradius":9},
 * "payload":{"type":"collar","packet":8,"gpstime":0,"gpsnavdata":0,"gpsolddata":0,"gpsencoded":0,"gpsfrommemory":0,"gpsnoformat":0,"gpsnosats":0,"gpsbadhdop":0,"satellites":6,"lat":62.02457500,"lon":129.72602500,"hdop":40,"pdop":99,"alarmlow":0,"alarmhigh":0,"battery":3.3,"temperature":-1,"r2":0,"failurepower":0,"failureeep":0,"failureclock":0,"failurecable":0,"failureint0":0,"failurewatchdog":0,"failurenoise":1,"failureworking":1,"key":21084,"time":"2016-04-04T04:17:32","utc":1459711052}}
 *
 */
void fill_random_packet(
		IRIDIUM_PAYLOAD_8 *p,
		int idx
)
{
	p->header.id = 1;
	p->header.size = htons(78);

	p->h_io.id = 1;
	p->h_io.size = htons(28);

	p->io.cdrref = htonl(1364432488);
	memmove(&p->io.imei, &imei, 15);
	p->io.recvno = htons(0);
	p->io.sentno = htons(idx);
	time_t t = time(NULL);
	p->io.recvtime = htonl(t);
	p->io.status = 0;

	p->h_loc.id = 3;
	p->h_loc.size = htons(11);

	p->loc.direwi = 0;
	p->loc.dirnsi = 0;
	p->loc.dirformat = 0;
	p->loc.dirreserved = 0;
	p->loc.lat = 62;
	p->loc.lat1000 = htons(4);
	p->loc.lon = 129;
	p->loc.lon1000 = htons(74);
	p->loc.cepradius = htonl(9);

	p->h_payload.id = 2;
	p->h_payload.size = htons(30);

	p->payload.packettype = 0x08;
	p->payload.gpsolddata = 0;		//< 1)GPS old data, GPS not read
	p->payload.gpsencoded = 0;		//< GPS data encoded
	p->payload.gpsfrommemory = 0;	//< data got from the memory
	p->payload.gpsnoformat = 0;		//< memory is not formatted
	p->payload.gpsnosats = 0;		//< No visible GPS salellites
	p->payload.gpsbadhdop = 0;		//< GPS HDOP = 99 or > 50
	p->payload.gpstime = 0;			//< 1 бит 1-0  GPS.valid=0x01 или 0;     GPS.time_valid=0b0000 0010 или 0;
	p->payload.gpsnavdata = 0;		//< 1 бит 1-0  GPS.valid=0x01 или 0;     GPS.time_valid=0b0000 0010 или 0;
	p->payload.sats = 5;
	memmove(&(p->payload.coord), &gpscoord_yak, sizeof(gps_coord_t));    //< 3) 16 байт (или 10 байт??? ) hex ?
	p->payload.bat = 32;			//< 19) байт бортовое напряжение fixed point (32=3.2 вольта) (от 2.0 до 4.0)
	p->payload.alarmlow = 0;		//< bit 6 < 2V, bit 7 > 4V
	p->payload.alarmhigh = 0;		//< bit 6 < 2V, bit 7 > 4V
	p->payload.temperature = 25;	//< 20) 1 байт температура
	p->payload.r2 = 0;		        //< 21) 1 байт номер пакета?? - он же индекс в таблице кодировки
	p->payload.failurepower = 0;	//< 22) device status power loss
	p->payload.failureeep = 0;		//< EEPROM failure
	p->payload.failureclock = 0;	//< clock not responding
	p->payload.failurecable = 0;	//< MAC not responding
	p->payload.failureint0 = 0;		//< clock int failure
	p->payload.failurewatchdog = 0;	//< software failure
	p->payload.failurenoise = 0;	//< blocks in memory found
	p->payload.failureworking = 0;	//< device was
	p->payload.key = htons(256);	// 23) 2 байт     volatile unsigned int packet_key;  младшие 16 бит
	fill_time5(&(p->payload.time), t);	// 25 5 байт
	std::cerr << *(uint16_t *) &(p->payload.time) << std::endl;
}

bool cont;

void signalHandler(int signal)
{
	switch(signal)
	{
	case SIGINT:
		cont = false;
		std::cerr << MSG_INTERRUPTED;
		break;
	case SIGHUP:
		std::cerr << MSG_RELOAD_CONFIG_REQUEST << " nothing to do";
		break;
	default:
		break;
	}
}

void setSignalHandler(int signal)
{
	struct sigaction action;
	memset(&action, 0, sizeof(struct sigaction));
	action.sa_handler = &signalHandler;
	sigaction(signal, &action, NULL);
}

int main(int argc, char **argv)
{
    // Signal handler
    setSignalHandler(SIGINT);

   	struct arg_str *a_topic = arg_str0("t", "topic", "<MQTT topic>", "MQTT topic name. Default " DEF_TOPIC);
	struct arg_str *a_broker_address = arg_str0("a", "broker", "<host name/address>", "MQTT broker address. Default " DEF_ADDRESS);
	struct arg_int *a_broker_port = arg_int0("p", "port", "<number>", "TCP port to listen. Default " DEF_PORT_S);
	struct arg_int *a_qos = arg_int0("q", "qos", "<0..2>", "0- at most once, 1- at least once, 2- exactly once. Default " DEF_QOS_S);
	struct arg_str *a_client_id = arg_str0("c", "client", "<id>", "MQTT client identifier string. Default " DEF_CLIENT_ID);
	struct arg_int *a_keep_alive_interval = arg_int0("k", "keepalive", "<seconds>", "Keep alive interval. Default " DEF_KEEP_ALIVE_INTERVAL_S);
	struct arg_int *a_pub_timeout = arg_int0("t", "timeout", "<ms>", "Publication timeout. Default " DEF_PUB_TIMEOUT_S);

	struct arg_int *a_delay = arg_int0("d", "delay", "<seconds>", "Delay after send packet. Default 1 (0- no delay).");
	struct arg_lit *a_verbosity = arg_litn("v", "verbosity", 0, 2, "Verbosity level");
	struct arg_lit *a_help = arg_lit0("h", "help", "Show this help");
	struct arg_end *a_end = arg_end(20);

    void* argtable[] = {a_topic, a_broker_address, a_broker_port, a_qos, a_client_id, a_keep_alive_interval,
		a_delay, a_verbosity, a_help, a_end};
	// verify the argtable[] entries were allocated successfully
	if (arg_nullcheck(argtable) != 0)
	{
		arg_freetable(argtable, sizeof(argtable) / sizeof(argtable[0]));
		exit(ERRCODE_COMMAND);
	}
	// Parse the command line as defined by argtable[]
	int nerrors = arg_parse(argc, argv, argtable);
	// special case: '--help' takes precedence over error reporting
	if ((a_help->count) || nerrors)
	{
			if (nerrors)
					arg_print_errors(stderr, a_end, PROGRAM_NAME);
			printf("Usage: %s\n", PROGRAM_NAME);
			arg_print_syntax(stdout, argtable, "\n");
			printf("%s\n", PROGRAM_DESCRIPTION);
			arg_print_glossary(stdout, argtable, "  %-25s %s\n");
			arg_freetable(argtable, sizeof(argtable) / sizeof(argtable[0]));
			exit(ERRCODE_PARSE_COMMAND);
	}

	std::string broker_address;
	int broker_port;
	int qos;
	std::string topic;
	std::string client_id;
	int keep_alive_interval;
	int pub_timeout;
	int delay;
    int verbosity;
	

	// MQTT
	if (a_broker_address->count)
		broker_address = *a_broker_address->sval;
	else
		broker_address = DEF_ADDRESS;

	if (a_broker_port->count)
		broker_port = *a_broker_port->ival;
	else
		broker_port = DEF_PORT;

	if (a_qos->count)
		qos = *a_qos->ival;
	else
		qos = DEF_QOS;

	if (a_topic->count)
		topic = *a_topic->sval;
	else
		topic = DEF_TOPIC;
	
	if (a_client_id->count)
		client_id = *a_client_id->sval;
	else
		client_id = DEF_CLIENT_ID;

	if (a_keep_alive_interval->count)
		keep_alive_interval = *a_keep_alive_interval->ival;
	else
		keep_alive_interval = DEF_KEEP_ALIVE_INTERVAL;

	
	if (a_pub_timeout->count)
		pub_timeout = *a_pub_timeout->ival;
	else
		pub_timeout = DEF_PUB_TIMEOUT;
	
	if (a_delay->count)
		delay = *a_delay->ival;
	else
		delay = DEF_DELAY_SEC;
	verbosity = a_verbosity->count;

	arg_freetable(argtable, sizeof(argtable) / sizeof(argtable[0]));

	IRIDIUM_PAYLOAD_8 pkt;
	int count = 0;
	cont = true;
	int sock;

	MQTTClient client;
	MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
	

	std::stringstream ss;
	ss << broker_address << ":" << broker_port;
	MQTTClient_create(&client, ss.str().c_str(), client_id.c_str(), MQTTCLIENT_PERSISTENCE_NONE, NULL);
	conn_opts.keepAliveInterval = keep_alive_interval;
	conn_opts.cleansession = 1;

	int r;
	if ((r = MQTTClient_connect(client, &conn_opts)) != MQTTCLIENT_SUCCESS)
	{
		std::cerr << "Failed to connect, return code " << r;
		exit(r);
	}
	
	while (cont)
	{
		fill_random_packet(&pkt, count);

		MQTTClient_message pubmsg = MQTTClient_message_initializer;
		pubmsg.qos = qos;
		pubmsg.retained = 1;
		pubmsg.payloadlen = sizeof(pkt);
		pubmsg.payload = &pkt;

		MQTTClient_deliveryToken token;
		MQTTClient_publishMessage(client, topic.c_str(), &pubmsg, &token);
		
		std::cerr << "Waiting for up to " << (int)(pub_timeout / 1000) << " seconds for publication on topic " << topic 
			<< " for client " << client_id << ".." << std::endl;
		r = MQTTClient_waitForCompletion(client, token, pub_timeout);
		std::cerr << "Message delivered, token: " << token << std::endl;
		
		if (verbosity >= 1)
		{
			std::cerr << std::setw(6) << count;
			if (verbosity >= 2)
			{
				std::cerr << " " << pkt2utilstring::hexString(&pkt, sizeof(IRIDIUM_PAYLOAD_8));
			}
			std::cerr << std::endl;
		}
		SLEEP(delay);
		count++;
	}
	
	MQTTClient_disconnect(client, 10000);
	MQTTClient_destroy(&client);
}
