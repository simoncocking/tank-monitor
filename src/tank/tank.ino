#include <ESP8266WiFi.h>
#include <SPI.h>
#include <LoRa.h>
#include <HCSR04.h>
#include <ArduinoJson.h>
#include <EEPROM.h>
#include "lora.h"
#include "tank.h"
#ifdef DHT_PIN
#include <DHTesp.h>
DHTesp dht;
#endif

ADC_MODE(ADC_VCC);

/**
 * TODO
 *
 * - add transistors to power down DHT and ultrasonic sensors when
 *   in deep sleep
 **/

void setup() {
	UltraSonicDistanceSensor usonic(USONIC_TRIG_PIN, USONIC_ECHO_PIN);
	delay(1000);
	WiFi.mode(WIFI_OFF);
	#ifdef DEBUG_ESP_PORT
		Serial.begin(115200);
	#endif
	setup_lora();
	setup_dht();

	#ifdef DHT_PIN
	// Poll temperature
	DEBUG("Measure temperature\n");
	TempAndHumidity env = dht.getTempAndHumidity();
	#endif

	// Poll distance
	DEBUG("Measure distance\n");
	uint16_t distance = (uint16_t)(usonic.measureDistanceCm() * 10.0); // mm

	// Send data
	const uint16_t level = HIGH_WATER_MARK + USONIC_HEIGHT - distance;
	const double percent = (double)level / (double)HIGH_WATER_MARK;
	StaticJsonDocument<300> json;
	json["tnk"] = TANK_NAME;
	json["dst"] = distance;
	#ifdef DHT_PIN
	json["tmp"] = env.temperature;
	json["hum"] = env.humidity;
	#endif
	json["lvl"] = level;
	json["ltr"] = (int)(TANK_VOLUME * percent);
	json["pct"] = (int)(percent * 100);
	json["vcc"] = ESP.getVcc();
	#ifdef DEBUG_ESP_PORT
		serializeJson(json, Serial);
		Serial.println("");
	#endif
	DEBUG("Beginning TX\n");
	if (LoRa.beginPacket()) {
		serializeJson(json, LoRa);
		LoRa.endPacket();
		DEBUG("TX complete\n");
	} else {
		DEBUG("Error sending LoRa packet\n");
	}

	// Put the radio to sleep
	LoRa.sleep();
	uint64_t poll = sleep_length(distance);
	DEBUG("Sleeping for %ds\n", (int)(poll / 1e6));
	ESP.deepSleep(poll);
}

void setup_lora() {
	DEBUG("Setup LoRa\n");
	// Set up
	LoRa.setPins(LORA_SS_PIN, LORA_RESET_PIN, LORA_DIO0_PIN);
	LoRa.begin(433e6);
	LoRa.setTxPower(LORA_TX_POWER);
	LoRa.setSpreadingFactor(LORA_SPREAD_FACTOR);
	LoRa.setSignalBandwidth(LORA_BANDWIDTH);
	LoRa.setCodingRate4(LORA_CODING_RATE);
	LoRa.enableCrc();
}

void setup_dht() {
#ifdef DHT_PIN
	DEBUG("Setup DHT\n");
	dht.setup(DHT_PIN, DHTesp::DHT11);
#endif
}

uint64_t sleep_length(uint16_t distance) {
	EEPROM.begin(2*SAMPLES);
	// Read the last `n` samples
	uint16_t samples[SAMPLES];
	uint16_t min = distance, max = distance;
	for (int i = 0; i < SAMPLES; i++) {
		samples[i] = (EEPROM.read(2*i) << 8) | (EEPROM.read(2*i+1));
		DEBUG("Sample %d = %d\n", i, samples[i]);
		if (samples[i] > max) max = samples[i];
		if (samples[i] < min) min = samples[i];
	}
	// Discard the oldest sample
	for (int i = 0; i < SAMPLES - 1; i++) {
		EEPROM.write(2*i,   (uint8_t)(samples[i+1] >> 8));
		EEPROM.write(2*i+1, (uint8_t)(samples[i+1] & 0x00FF));
	}
	// Write the current sample
	EEPROM.write(2*(SAMPLES-1),   (uint8_t)(distance >> 8));
	EEPROM.write(2*(SAMPLES-1)+1, (uint8_t)(distance & 0x00FF));
	EEPROM.end();

	uint64_t poll = POLL_SLOW;
	// If there's an outlier, increase the poll frequency
	if (max - min > LEVEL_DELTA) {
		poll = POLL_FAST;
	}
	return poll;	
}

// Don't use loop() - we put the MCU into deep sleep after
// each cycle
void loop() {}