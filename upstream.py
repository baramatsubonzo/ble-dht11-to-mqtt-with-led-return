import asyncio
from bleak import BleakScanner, BleakClient
import paho.mqtt.client as mqtt

TARGET_NAME_FRAGMENT = "ShotaIoT3"
CHARACTERISTIC_UUID = "19B10001-E8F2-537E-4F6C-D104768A1214"

MQTT_HOST = "13.238.128.64"
MQTT_PORT = 1883
MQTT_TOPIC = "ifn649/dht"

async def scan_and_connect():
    mqtt_client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)
    mqtt_client.connect(MQTT_HOST, MQTT_PORT, keepalive=30)

    try:
        print("Scanning for BLE devices...")
        # Asynchronously scan for nearby devices for up to 5 seconds
        devices = await BleakScanner.discover(timeout=5.0)

        target_device = None
        for d in devices:
            if d.name and TARGET_NAME_FRAGMENT in d.name:
                target_device = d
                break

        if not target_device:
            print(f"No device found with name containing '{TARGET_NAME_FRAGMENT}'.")
            return

        print(f"Found target: {target_device.name} ({target_device.address})")

        # Connect to the discovered Arduino address
        async with BleakClient(target_device.address) as ble_client:
            if ble_client.is_connected:
                print("Connected successfully.")
                try:
                    # read_gatt_char is an async I/O call (may take hundreds of ms to seconds)
                    value = await ble_client.read_gatt_char(CHARACTERISTIC_UUID) # Read the dhtCharacteristic on the Arduino side
                    print(f"Raw value: {value}")
                    try:
                        decoded = value.decode('utf-8')
                        print("Decoded value:", decoded)
                    except Exception:
                        decoded = None

                    # --- Publish to MQTT ---
                    payload = decoded if decoded is not None else value
                    mqtt_client.publish(MQTT_TOPIC, payload)
                    print(f"Published to MQTT topic '{MQTT_TOPIC}'.")
                except Exception as e:
                    print("Failed to read characteristic:", e)
            else:
                print("Failed to connect.")
    finally:
        # Cleanly disconnect MQTT
        try:
            mqtt_client.disconnect()
        except Exception:
            pass

asyncio.run(scan_and_connect())
