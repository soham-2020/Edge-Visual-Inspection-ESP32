#!/usr/bin/env python3
"""Publish a synthetic motor/status event and wait for the broker PUBACK.

Set MQTT_HOST, MQTT_USERNAME, and MQTT_PASSWORD in the environment first.
This verifies broker connectivity and message publishing; it does not verify
Node-RED rendering or any physical ESP32-CAM behavior.
"""
import json
import os
import sys
import time

import paho.mqtt.client as mqtt

host = os.environ.get("MQTT_HOST")
if not host:
    sys.exit("Set MQTT_HOST to the Mosquitto broker hostname or LAN IP.")

port = int(os.environ.get("MQTT_PORT", "1883"))
username = os.environ.get("MQTT_USERNAME")
password = os.environ.get("MQTT_PASSWORD")
topic = os.environ.get("MQTT_TOPIC", "motor/status")
payload = {
    "status": "anomaly",
    "confidence": 94.8,
    "dsp_ms": 4,
    "classification_ms": 643,
}

client = mqtt.Client(
    mqtt.CallbackAPIVersion.VERSION2,
    client_id=f"visual-inspection-smoke-{int(time.time())}",
    protocol=mqtt.MQTTv311,
)
if username:
    client.username_pw_set(username, password or "")

try:
    client.connect(host, port, keepalive=30)
    client.loop_start()
    info = client.publish(topic, json.dumps(payload), qos=1, retain=False)
    info.wait_for_publish(timeout=10)
    if not info.is_published():
        raise TimeoutError("broker did not acknowledge the publish within 10 seconds")
    print(f"PASS: published synthetic JSON to {host}:{port} topic {topic}")
    print(json.dumps(payload))
except Exception as exc:
    sys.exit(f"FAIL: MQTT smoke test: {exc}")
finally:
    client.loop_stop()
    client.disconnect()
