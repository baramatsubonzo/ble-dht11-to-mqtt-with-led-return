#include <ArduinoBLE.h>

// Custom Service and Characteristic UUIDs
#define SERVICE_UUID        "19B10000-E8F2-537E-4F6C-D104768A1214"
#define DHT_CHARACTERISTIC_UUID "19B10001-E8F2-537E-4F6C-D104768A1214"
#define LED_CHARACTERISTIC_UUID "19B10002-E8F2-537E-4F6C-D104768A1214"

BLEService customService(SERVICE_UUID);

// MEMO: DHT用の権限設定。温度データの読み取りと通知。
BLECharacteristic dhtCharacteristic(
  DHT_CHARACTERISTIC_UUID,
  BLERead | BLENotify, 10 // 余計なゴミがつかないようにする
);

// MEMO: LEDコマンド受信用。書き込み権限。
BLECharacteristic ledCharacteristic(
  LED_CHARACTERISTIC_UUID,
  BLEWrite, 10 // 余計なゴミがつかないようにする
);

// MEMO: Timer
unsigned long lastSend = 0;
const unsigned long sendIntervalMs = 2000; // send data every 2 seconds

void setup() {
  Serial.begin(9600);
  //TODO:
  pinMode(LED_BUILTIN, OUTPUT);

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
  BLE.addService(customService); // MEMO: BLEサービスへの有効化

  // Initialize the data characteristic
  dhtCharacteristic.writeValue("Ready");

  BLE.advertise();
  Serial.println("BLE advertising started...");
}

void loop() {
  // Poll BLE to process incoming requests
  BLE.poll();

  // MEMO: Conection start monitoring.
  static bool wasConnected = false;
  bool isConnected = BLE.connected();
  if (isConnected && !wasConnected) {
    Serial.println("Central connected");
  } else if (!isConnected && wasConnected) {
    Serial.println("Central disconnected");
    BLE.advertise();
  }
  wasConnected = isConnected;

  // MEMO: Upstream Data Sending
  unsigned long now = millis();
  if (isConnected && now - lastSend >= sendIntervalMs) {
    lastSend = now;

    // TODO: Dummy
    float temperature = 77.7; 
    String tempStr = String(temperature);

    dhtCharacteristic.writeValue(tempStr.c_str());
      // TODO: Dummy
    Serial.print("Updated BLE value to: ");
    Serial.println(tempStr);  
  }

  if (ledCharacteristic.written()) {
    // TODO:
    int len = ledCharacteristic.valueLength();
    if (len > 10) len = 10;
    uint8_t raw[11];                     // +1でヌル終端
    ledCharacteristic.readValue(raw, len);
    raw[len] = '\0';

    String command = (char*)raw;
    command.trim();
    command.toUpperCase();

    Serial.print("Received command: ");
    Serial.println(command);

    if (command == "LED_ON") {
      digitalWrite(LED_BUILTIN, HIGH); // LEDを点灯
      Serial.println("Turning LED ON");
    } else if (command == "LED_OFF") {
      digitalWrite(LED_BUILTIN, LOW);  // LEDを消灯
      Serial.println("Turning LED OFF");
    } else {
      Serial.println("Unknown command");
    }
  }
}
