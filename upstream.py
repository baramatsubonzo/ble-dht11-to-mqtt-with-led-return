import asyncio
from bleak import BleakScanner, BleakClient
import paho.mqtt.client as mqtt

TARGET_NAME_FRAGMENT = "ShotaIoT3"
CHARACTERISTIC_UUID = "19B10001-E8F2-537E-4F6C-D104768A1214"

MQTT_HOST = "13.238.128.64"
MQTT_PORT = 1883
MQTT_TOPIC = "ifn649/dht"

# Keep MQTT client global so connection stays alive for continuous streaming.
# If inside function, it would stop after one read.
mqtt_client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)
mqtt_client.connect(MQTT_HOST, MQTT_PORT, keepalive=30)
mqtt_client.loop_start()  # For continuous data collection

def notification_handler(sender, data: bytearray):
    # called every time Arduino sends new temp/humidity data via Notify
    try:
        decoded = data.decode("utf-8").strip()
    except Exception:
        decoded = str(data)
    print(f"[BLE Notify] {decoded}")
    mqtt_client.publish(MQTT_TOPIC, decoded)
    print(f"[MQTT] Published: {decoded}")

async def scan_and_connect():
    while True: # Loop for reconnection attempts
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
            await asyncio.sleep(5)
            continue

        print(f"Found target: {target_device.name} ({target_device.address})")

        try:
            # Connect to the discovered Arduino address
            async with BleakClient(target_device.address) as ble_client:
                if not ble_client.is_connected:
                    print("Failed to connect.")
                    await asyncio.sleep(5)
                    continue

                print("Connected successfully. Starting Notify subscription...")
                await ble_client.start_notify(CHARACTERISTIC_UUID, notification_handler)

                try:
                    # keep alive while notifications stream in
                    while ble_client.is_connected:
                        await asyncio.sleep(1)
                finally:
                    await ble_client.stop_notify(CHARACTERISTIC_UUID)
        except Exception as e:
            print(f"[BLE] Error: {e}")

        print("Disconnected. Reconnecting in 5s...")
        await asyncio.sleep(5)

if __name__ == "__main__":
    try:
        asyncio.run(scan_and_connect())
    except KeyboardInterrupt:
        print("Stopped by user.")
    finally:
        mqtt_client.loop_stop()
        mqtt_client.disconnect()
