# ESP8266-based wireless water tank level monitor

This is a wireless water tank level monitor, based on the NodeMCU / ESP12a dev-kit microcontroller, using the Arduino toolchain.

The water level is measured using a HC-SR04 ultrasonic range-finder, (optionally) temperature-compensated with measurements from a DHT11 one-wire humidity/temperature sensor.

Measurements are transmitted over LoRa packet radio, and on reception injected into a MQTT message bus.

## License

