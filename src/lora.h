/**
 * Spreading factor
 * - maximise for range/reliability
 * - minimize for bandwidth
 * Default: 7, range 6-12
 */
 #define LORA_SPREAD_FACTOR 12
/**
 * Signal Bandwidth (Hz)
 * - Maximise for data rate
 * - Minimize for SNR
 * Default: 125e3
 * Supported values:
 *   7.8e3, 10.4e3, 15.6e3, 20.8e3, 31.25e3, 41.7e3, 62.5e3, 125e3, 250e3
 */
#define LORA_BANDWIDTH 125e3
/**
 * Coding Rate
 * - Maximise for reliability
 * - Minimize for data rate
 * Default: 5, range 5 - 8
 */
#define LORA_CODING_RATE 8
/**
 * Transmit Power
 * Default: 17, range 2 - 20
 */
#define LORA_TX_POWER   17

#define LORA_SYNC_WORD 0x12