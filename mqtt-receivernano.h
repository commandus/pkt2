#include <nanomsg/nn.h>
#include <MQTTClient.h>

#include "mqtt-receiver-config.h"

class MQTT_Env {
public:
	Config *config;
	MQTTClient client;
	int nano_socket;
	
	MQTT_Env
	(
		Config *aconfig,
		MQTTClient aclient,
		int ananosocket
	);
};

int mqtt_receiever_nano(Config *config);
int stop(Config *config);
int reload(Config *config);
