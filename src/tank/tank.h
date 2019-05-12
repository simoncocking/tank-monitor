#ifdef DEBUG_ESP_PORT
#define DEBUG(...) DEBUG_ESP_PORT.printf( __VA_ARGS__ )
#else
#define DEBUG(...)
#endif


#define LORA_SS_PIN     15
// Use -1 when RST is tied to the MCU RESET (for deep sleep)
#define LORA_RESET_PIN  -1
#define LORA_DIO0_PIN    5

#define USONIC_TRIG_PIN  0
#define USONIC_ECHO_PIN  4
// #define DHT_PIN         10

#define TANK_NAME       "header"
// Tank volume in litres
#define TANK_VOLUME     30000
// High water mark in mm above empty
#define HIGH_WATER_MARK 2440
// Height of the sensor above HWM
#define USONIC_HEIGHT   10

// Poll every 60 minutes normally
#define POLL_SLOW 3600e6
#ifdef DEBUG_ESP_PORT
// Poll every 5 minutes if we detect a large change
#define POLL_FAST 60e6
#else
#define POLL_FAST 300e6
#endif

// The number of samples to retain in order to determine fill/drain rate
#define SAMPLES 5

// The distance in mm over which a measurement delta will trigger fast polling
#define LEVEL_DELTA 20