# Node-RED dashboard

The checked-in `node-red-flow.json` is a Node-RED Dashboard 1 flow. It subscribes to `motor/status`, parses the JSON fields `status` and `confidence`, and sends them to the status text and confidence gauge. The flow declares `node-red-dashboard` 3.6.6.

## Install and import

1. Install Node.js and Node-RED using the official Node-RED instructions.
2. In the Node-RED user directory, install the dashboard module:
   ```sh
   npm install node-red-dashboard@3.6.6
   ```
3. Start Node-RED, open its editor (normally `http://localhost:1880`), then use **Menu → Import** to import this file.
4. Open the MQTT broker configuration node and set:
   - **Server**: `127.0.0.1` if Mosquitto runs on the same computer as Node-RED; otherwise use the broker host's LAN IP.
   - **Port**: `1883`.
   - **Username/password**: the credentials created for Mosquitto.
5. Deploy. The Dashboard 1 UI is normally at `http://localhost:1880/ui`.

The exported flow intentionally contains no broker password. Configure the broker node after import. The ESP32 must use the Mosquitto host's LAN IP, not `127.0.0.1`, because loopback on the ESP32 refers to the ESP32 itself.

## Verify without an ESP32

With Mosquitto running and the broker credentials configured in Node-RED, follow the repository's [MQTT smoke test](../README.md#hardware-free-mqtt-smoke-test). It publishes a synthetic `motor/status` message. Confirm the dashboard status and confidence gauge update.

If importing into a clean Node-RED installation, ensure the dashboard module is installed. The flow is for Dashboard 1; it is not a FlowFuse Dashboard 2 flow.
