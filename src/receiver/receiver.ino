#include <SPI.h>
#include <LoRa.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <PubSubClient.h>
#include <PubSubClientTools.h>
#include <ArduinoJson.h>
#include "receiver.h"
#include "lora.h"

PubSubClient mqtt_client;
PubSubClientTools mqtt(mqtt_client);
WiFiClient client;
WiFiUDP udp;
NTPClient ntp(udp, NTP_SERVER, 0, NTP_INTERVAL);
//LoRaClass LoRa;

void setup() {
	Serial.begin(115200);
	DEBUG("Setup LoRa\n");
	LoRa.setPins(LORA_SS_PIN, LORA_RESET_PIN, LORA_DIO0_PIN);
	LoRa.begin(433e6);
	LoRa.setTxPower(LORA_TX_POWER);
	LoRa.setSpreadingFactor(LORA_SPREAD_FACTOR);
	LoRa.setSignalBandwidth(LORA_BANDWIDTH);
	LoRa.setCodingRate4(LORA_CODING_RATE);
	LoRa.setSyncWord(LORA_SYNC_WORD);
	LoRa.enableCrc();
	
	DEBUG("Setup WiFi\n");
	WiFi.mode(WIFI_STA);
	WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
	while (WiFi.status() != WL_CONNECTED) {
		delay(500);
	}
	
	mqtt_client.setClient(client);
	mqtt_client.setServer(MQTT_BROKER, MQTT_PORT);
	ntp.begin();
}

StaticJsonDocument<300> json;
const char* json_keys[] = { "dst", "tmp", "hum", "lvl", "ltr", "pct", "vcc" };
const char* mqtt_keys[] = { "distance", "temp", "humidity", "level", "litres", "percent", "vcc" };
void loop() {
	if (int bytes = LoRa.parsePacket()) {
		ntp.update();
		DEBUG("Received packet (%d bytes)\n", bytes);
		DeserializationError err = deserializeJson(json, LoRa);
		if (err) {
			DEBUG(String("Error decoding JSON: " + String(err.c_str()) + "\n").c_str());
			return;
		}
		if (! mqtt_client.connected()) {
			DEBUG("Reconnecting MQTT\n");
			if (!mqtt_client.connect("TankReceiver")) {
				DEBUG("Error reconnecting to MQTT\n");
				return;
			}
		}
		mqtt.setPublishPrefix(String("tank/" + json["tnk"].as<String>() + "/"));
		for (int i = 0; i < 7; i++) {
			if (json[json_keys[i]] != NULL) {
				publish(mqtt_keys[i], json[json_keys[i]].as<String>(), true);
			}
		}
		publish("rssi",  String(LoRa.packetRssi()),  true);
		publish("snr",   String(LoRa.packetSnr()),   true);
		publish("epoch", String(ntp.getEpochTime()), true);

	}
}

bool publish(String topic, String value, bool retain) {
	if (!mqtt.publish(topic, value, retain)) {
		DEBUG(String("Unable to publish " + topic + " to MQTT\n").c_str());
	}
}
