# System Architecture
```mermaid
flowchart LR
    DHT["DHT-11 Sensor"]
    LED["Onboard LED"]

    subgraph Arduino["Arduino Nano 33 IoT"]
		BLE_Char1["dhtCharacteristic"]
        BLE_Char2["ledCharacteristic"]
    end

    subgraph Pi["Raspberry Pi"]
        BLE_Read["upstream.py"]
        BLE_Write["downstream.py"]
    end

    subgraph EC2["AWS EC2 (MQTT Broker)"]
        Broker["Mosquitto"]
    end

	subgraph Phone["Smartphone"]
		 MyMQTTApp
	end

	DHT -->|Temperature & Humidity| BLE_Char1
    Arduino --> |BLE Read/Notify| Pi
    Pi --> |Publish| EC2
    EC2 --> |Data| Phone
    Phone --> |LED_ON/OFF| EC2
    EC2 --> |Commands| Pi
    Pi --> |BLE Write| BLE_Char2
    BLE_Char2 --> LED

```

# Preparation

- [ ] Check the public IP address of your EC2 instance

- [ ] Open two terminals on the Raspberry Pi

    - [ ] Terminal 1: Upstream (read temperature and humidity)

    - [ ] Terminal 2: Downstream (control LED ON/OFF)

- [ ] In the smartphone app (MyMQTT), update the EC2 public IP address

- [ ] In the smartphone app (MyMQTT), update the topics

- [ ] Make sure the Raspberry Pi is accessible via VNC Viewer

# Demo Steps
## Upstream (DHT11 → Arduino → BLE → Pi → MQTT → EC2)

On the smartphone app MyMQTT, subscribe to the topic

Connect the Arduino and start the upstream bridge on the Pi

Verify that the temperature and humidity data from Arduino appears in the app

## Downstream (Smartphone → MQTT → Pi → BLE → Arduino)

From the smartphone app, publish LED_ON to the topic

The orange LED on the Arduino will turn on

From the smartphone app, publish LED_OFF to the ifn649 topic

The orange LED on the Arduino will turn off