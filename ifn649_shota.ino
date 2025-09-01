#include <ArduinoBLE.h>
#include "DHT.h"
// Define the pin and sensor type
#define DHTPIN 2 // Use digital pin
#define DHTTYPE DHT11 // sensor
DHT dht(DHTPIN, DHTTYPE); // generate instance

// Custom Service and Characteristic UUIDs
#define SERVICE_UUID        "19B10000-E8F2-537E-4F6C-D104768A1214"
#define DHT_CHARACTERISTIC_UUID "19B10001-E8F2-537E-4F6C-D104768A1214"
#define LED_CHARACTERISTIC_UUID "19B10002-E8F2-537E-4F6C-D104768A1214"

BLEService customService(SERVICE_UUID);

// DHT permission settings
// read and notify temperature data
BLECharacteristic dhtCharacteristic(
  DHT_CHARACTERISTIC_UUID,
  BLERead | BLENotify, 20
);

// Write permission for receiving LED commands.
BLECharacteristic ledCharacteristic(
  LED_CHARACTERISTIC_UUID,
  BLEWrite, 10 // ensures no unnecessary data is included
);

// Timer
unsigned long lastSend = 0;
const unsigned long sendIntervalMs = 2000; // send data every 2 seconds

void setup() {
  Serial.begin(9600);
  // Set the built-in LED pin (D13 on Nano 33 IoT) as an OUTPUT
  // This allows us to control the LED by writing HIGH (on) or LOW (off)
  pinMode(LED_BUILTIN, OUTPUT);
  //Initialize DHT
  dht.begin();

  if (!BLE.begin()) {
    Serial.println("Failed to initialize BLE!");
    while (true);
  }

  BLE.setDeviceName("ShotaIoT3");
  BLE.setLocalName("ShotaIoT3");

  // Add service and characteristic
  BLE.setAdvertisedService(customService);
  customService.addCharacteristic(dhtCharacteristic);
  customService.addCharacteristic(ledCharacteristic);
  BLE.addService(customService); // Enable the BLE service

  // Initialize the data characteristic
  dhtCharacteristic.writeValue("Ready");

  BLE.advertise();
  Serial.println("BLE advertising started...");
}

void loop() {
  // Poll BLE to process incoming requests
  BLE.poll();

  // Conection start monitoring.
  static bool wasConnected = false;
  bool isConnected = BLE.connected();
  if (isConnected && !wasConnected) {
    Serial.println("Central connected");
  } else if (!isConnected && wasConnected) {
    Serial.println("Central disconnected");
    BLE.advertise();
  }
  wasConnected = isConnected;

  // Upstream Data Sending
  unsigned long now = millis();
  if (isConnected && now - lastSend >= sendIntervalMs) {
    lastSend = now;

    // Read temperature as Celsius (default)
    float temp = dht.readTemperature();
    // Read humidity
    float humidity = dht.readHumidity();

    // Error handling. Check if sensor returned NaN
    if (isnan(temp) || isnan(humidity)) {
      Serial.println("Failed to read from DHT sensor!");
    } else {
      //output
      Serial.print("Temperature: ");
      Serial.print(temp, 1); // display with 1 decimal place
      Serial.print(" °C\tHumidity: ");
      Serial.print(humidity, 1); // display with 1 decimal place
      Serial.println(" %");
  
      char msg[21]; // set BLE buffer to 21 (20 characters plus null terminator)
      int len = snprintf(msg, sizeof(msg), "T:%.1f,H%.1f", temp, humidity); // e.g., "T:25.5,H:60.1"
      if (len > 0 && len < sizeof(msg)) {
        // send only if the string is generated correctly
        dhtCharacteristic.writeValue((uint8_t*)msg, len);
      }
    }
  }
  // Donwstream: LED command
  if (ledCharacteristic.written()) {
    // TODO:
    int len = ledCharacteristic.valueLength();
    // avoid buffer overflow.
    if (len > 10) len = 10;
    uint8_t raw[11]; // max 10 bytes + null terminator '\0'
    ledCharacteristic.readValue(raw, len);
    raw[len] = '\0'; // null terminator

    String command = (char*)raw;
    command.trim(); // removes trailing spaces and newlines
    command.toUpperCase(); // convert to uppercase (e.g, led_on -> LED_ON)

    Serial.print("Received command: ");
    Serial.println(command);

    if (command == "LED_ON" || command == "ON") {
      digitalWrite(LED_BUILTIN, HIGH); // turn on LED 
      Serial.println("Turning LED ON");
    } else if (command == "LED_OFF" || command == "OFF") {
      digitalWrite(LED_BUILTIN, LOW);  // turn off LED
      Serial.println("Turning LED OFF");
    } else {
      // error handling
      Serial.println("Unknown command");
    }
  }
}
