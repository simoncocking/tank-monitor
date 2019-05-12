#ifdef DEBUG_ESP_PORT
#define DEBUG(...) DEBUG_ESP_PORT.printf( __VA_ARGS__ )
#else
#define DEBUG(...)
#endif

#define LORA_SS_PIN     15
// Use -1 when RST is tied to the MCU RESET (for deep sleep)
#define LORA_RESET_PIN  -1
#define LORA_DIO0_PIN    5

#define WIFI_SSID     "Taj Garage"
#define WIFI_PASSWORD "Tallarook"
#define MQTT_BROKER   "10.0.0.3"
#define MQTT_PORT     1883
#define NTP_SERVER    "10.0.0.1"
#define NTP_INTERVAL  3600e3
