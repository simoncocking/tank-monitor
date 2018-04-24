require "config"
-- Keep the linter quiet
local gpio  = gpio
local uart  = uart
local wifi  = wifi
local mqtt  = mqtt
local tmr   = tmr
local dht   = dht
local sjson = sjson
local MQTT  = MQTT
local WIFI_CONFIG = WIFI_CONFIG

local DEBUG           = true
-- Pin assignments
local RADIO_MODE_PIN  = 1
local USONIC_TRIG_PIN = 2
local DHT_PIN         = 4
local USONIC_ECHO_PIN = 5
-- General config
local TIMER_INTERVAL  = 5 -- seconds
local RADIO_UART      = 0
local RADIO_BPS       = 115200
local RADIO_CHANNEL   = 100
local TANK_NAME       = "header"
local TANK_VOLUME     = 30000 -- litres
local HIGH_WATER_MARK = 2400  -- mm
local USONIC_HEIGHT   = 300   -- mm above HWM

local function do_in(delay, func)
	tmr.create():alarm(delay, tmr.ALARM_SINGLE, func)
end

local function sleep_radio()
	gpio.write(RADIO_MODE_PIN, gpio.LOW)
	do_in(1, function()
		uart.write(RADIO_UART, "AT+SLEEP\n")
		gpio.write(RADIO_MODE_PIN, gpio.HIGH)
	end)
end

local function wake_radio(and_then, ...)
	gpio.write(RADIO_MODE_PIN, gpio.LOW)
	do_in(1, function()
		gpio.write(RADIO_MODE_PIN, gpio.HIGH)
		and_then(...)
	end)
end

local function setup_radio()
	uart.setup(RADIO_UART, RADIO_BPS, 8, uart.PARITY_NONE, uart.STOPBITS_1)
	gpio.mode(RADIO_MODE_PIN, gpio.OUTPUT, gpio.PULLUP)
	gpio.write(RADIO_MODE_PIN, gpio.LOW)
	do_in(1, function()
		uart.write(RADIO_UART, "AT+C"..RADIO_CHANNEL.."\n")
		uart.write(RADIO_UART, "AT+FU3\n")
		uart.write(RADIO_UART, "AT+P8\n")
		do_in(1, function()
			gpio.write(RADIO_MODE_PIN, gpio.HIGH)
		end)
	end)
end

local function setup_usonic()
	gpio.mode(USONIC_TRIG_PIN, gpio.OUTPUT, gpio.FLOAT)
	gpio.mode(USONIC_ECHO_PIN, gpio.INT,  gpio.FLOAT)
	gpio.write(USONIC_TRIG_PIN, gpio.LOW)
end

local function setup()
	if DEBUG then
		wifi.setmode(wifi.STATION)
		wifi.sta.disconnect()
		wifi.sta.config(WIFI_CONFIG)
		wifi.sta.autoconnect(1)
		MQTT.client = mqtt.Client(MQTT.client_id, 120)
		wifi.eventmon.register(wifi.eventmon.STA_GOT_IP, function()
			MQTT.client:connect(MQTT.host, MQTT.port, 0)
		end)
	else
		wifi.setmode(wifi.NULLMODE)
	end
	setup_radio()
	setup_usonic()
end

local function temperature()
	for _ = 1, 10 do
		local status, temp = dht.read11(DHT_PIN)
		if status == dht.OK or status == dht.ERROR_CHECKSUM then
			return temp
		end
		tmr.delay(1000)
	end
	return 20.0
end

local function transmit(distance, temp)
	local level = HIGH_WATER_MARK + USONIC_HEIGHT - distance
	local percent = level / HIGH_WATER_MARK
	local litres = TANK_VOLUME * percent
	if DEBUG then
		MQTT.client:publish("tank/"..TANK_NAME.."/distance",    ""..distance, 0, 1)
		MQTT.client:publish("tank/"..TANK_NAME.."/temperature", ""..temp, 0, 1)
		MQTT.client:publish("tank/"..TANK_NAME.."/level",       ""..level, 0, 1)
		MQTT.client:publish("tank/"..TANK_NAME.."/litres",      ""..litres, 0, 1)
		MQTT.client:publish("tank/"..TANK_NAME.."/percent",     ""..percent * 100, 0, 1)
	end
	local json = sjson.encode({
		tank     = TANK_NAME,
		distance = distance,
		temp     = temp,
		level    = level,
		litres   = litres,
		percent  = percent * 100
	})
	wake_radio(function()
		uart.write(RADIO_UART, json.."\n")
		do_in(1, sleep_radio)
	end)
end

local function ping()
	local echo = 0
	gpio.trig(USONIC_ECHO_PIN, "both", function(level, when)
		echo = when - echo -- µs
		if level == gpio.LOW then
			gpio.trig(USONIC_ECHO_PIN, "none")
			local t = temperature()
			local c = (331.3 + 0.606 * t) / 1000 -- mm/µs
			local d = echo / 2 * c -- mm
			transmit(d, t)
		end
	end)
	gpio.write(USONIC_TRIG_PIN, gpio.HIGH)
	tmr.delay(10)
	gpio.write(USONIC_TRIG_PIN, gpio.LOW)
end

setup()
tmr:create().alarm(TIMER_INTERVAL * 1000, tmr.ALARM_AUTO, ping)
