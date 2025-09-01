import asyncio
from bleak import BleakScanner, BleakClient
import paho.mqtt.client as mqtt

# --- config ---
TARGET_NAME_FRAGMENT = "ShotaIoT3"  # match your BLE.setLocalName()
CHARACTERISTIC_UUID = "19B10002-E8F2-537E-4F6C-D104768A1214"  # from ble_read-write.ino
MQTT_HOST = "3.107.180.15"
MQTT_TOPIC = "ifn649/test"

BLE_ADDR = None  # set in main()

# --- BLE helpers (blocking wrapper) ---
async def _ble_write_async(addr: str, cmd: str):
    async with BleakClient(addr) as c:
        await c.write_gatt_char(CHARACTERISTIC_UUID, cmd.encode("utf-8"), response=True)
        print(f"[BLE] wrote: {cmd}")

def ble_write(cmd: str):
    asyncio.run(_ble_write_async(BLE_ADDR, cmd))

# --- MQTT callback ---
def on_message(client, userdata, msg):
    payload = msg.payload.decode(errors="ignore").strip()
    print(msg.topic, payload)
    if payload in ("LED_ON", "LED_OFF"):
        ble_write(payload)

# --- main: find BLE MAC address, then start MQTT ---
def main():
    global BLE_ADDR

    async def find_addr():
        devices = await BleakScanner.discover(timeout=5.0)
        for d in devices:
            if d.name and TARGET_NAME_FRAGMENT in d.name:
                return d.address

    BLE_ADDR = asyncio.run(find_addr())
    assert BLE_ADDR, f"BLE device with name containing '{TARGET_NAME_FRAGMENT}' not found"
    print("[BLE] using device:", BLE_ADDR)

    client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)
    client.on_message = on_message
    client.connect(MQTT_HOST, 1883)
    client.subscribe(MQTT_TOPIC)
    print("[MQTT] Subscribed; waiting for LED commands")
    client.loop_forever()

if __name__ == "__main__":
    main()
