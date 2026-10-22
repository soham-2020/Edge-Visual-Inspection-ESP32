# Edge Visual Inspection System — TinyML on ESP32-CAM

An industrial visual anomaly detection prototype using an AI Thinker ESP32-CAM, an Edge Impulse image classifier, MQTT, Mosquitto, and a Node-RED dashboard.

Clone this repository:

```sh
git clone https://github.com/soham-2020/Edge-Visual-Inspection-ESP32.git
cd Edge-Visual-Inspection-ESP32
```

> **Performance results:** The project reports 87.8% overall accuracy, 94.8% anomaly recall, 78.4% normal recall, an anomaly F1 score of 0.90, about 643 ms inference per frame, and about 280 KB RAM use. These are the project's documented measurements; this repository update has not independently reproduced them.

## Repository contents

```text
.
├── esp32cam_firmware1.ino        # ESP32-CAM inference and MQTT publisher
├── secrets.h.example             # Local Wi-Fi/MQTT settings template
├── model/README.md               # Model details and export instructions
├── dashboard/
│   ├── node-red-flow.json        # Importable Dashboard 1 flow
│   └── README.md                 # Dashboard install/configuration
├── mosquitto/security.conf       # Broker authentication configuration
├── scripts/mqtt_smoke_test.py    # Hardware-free synthetic MQTT publish
└── requirements-smoke-test.txt
```

## Requirements

### Hardware

- AI Thinker ESP32-CAM with OV2640 camera and working PSRAM.
- USB-to-TTL adapter (such as CP2102), suitable 5 V power, and jumper wires.
- For flashing: connect GPIO0 to GND, upload, then disconnect GPIO0 from GND and reset.

### Arduino IDE and board configuration

1. Install Arduino IDE 2.x.
2. In **Boards Manager**, install **esp32 by Espressif Systems**.
3. Select **Tools → Board → esp32 → AI Thinker ESP32-CAM**.
4. Select the serial port for the USB-to-TTL adapter. For upload, use 115200 baud if the adapter/board is unreliable at a higher speed.
5. Enable PSRAM if the installed board package exposes that option. Select a partition scheme with enough app space for the exported model (the model is documented as under 1 MB; “Huge APP” is a suitable choice where available).

The firmware also uses these libraries:

| Library | Installation |
|---|---|
| Edge Impulse generated Arduino library for this project | Export and install as described below |
| PubSubClient by Nick O'Leary | Arduino Library Manager |
| ArduinoJson 6.x | Arduino Library Manager |
| WiFi and esp_camera | Included with the Espressif ESP32 board package |

Use ArduinoJson 6.x because the sketch uses `StaticJsonDocument`. The generated Edge Impulse header is not included in this repository: it is project-specific and must be exported from the Edge Impulse project.

## Obtain and install the Edge Impulse model library

1. Sign in to Edge Impulse and open the project that generated this model.
2. Open **Deployment**, select **Arduino library**, and build/download the library ZIP. Choose the int8 deployment that matches the model used for the reported results.
3. In Arduino IDE, choose **Sketch → Include Library → Add .ZIP Library…** and select the downloaded ZIP.
4. Confirm that the installed library provides `MetalMind-project-1_inferencing.h`, which the sketch includes.
5. In the Edge Impulse project, verify the input dimensions and camera sensor type match the firmware's camera image classifier. Re-export after any model/project changes.

The Edge Impulse project/export is not committed here, so a clone alone cannot compile the inference sketch until the matching library is obtained and installed.

## Configure credentials

The sketch includes a local `secrets.h` file, which is ignored by Git.

```sh
cp secrets.h.example secrets.h
```

Edit `secrets.h` and set:

- `WIFI_SSID` and `WIFI_PASSWORD` for a 2.4 GHz Wi-Fi network.
- `MQTT_SERVER` to the Mosquitto computer's LAN IP address.
- `MQTT_USERNAME` and `MQTT_PASSWORD` to a broker account.
- `MQTT_PORT` (1883 by default).

Do not commit or share `secrets.h`. Rotate a credential if it has ever been published. The firmware and broker use plain MQTT on port 1883; keep the broker on a trusted LAN and firewall the port from the public internet. Add TLS before using an untrusted network.

## Run Mosquitto

Install Mosquitto and its command-line clients on the computer that will host the broker. Create the password file once:

```sh
sudo mosquitto_passwd -c /etc/mosquitto/passwd mqttuser
```

Copy `mosquitto/security.conf` into Mosquitto's configuration directory (commonly `/etc/mosquitto/conf.d/security.conf` on Debian/Ubuntu), then restart the service:

```sh
sudo systemctl restart mosquitto
sudo systemctl status mosquitto
```

The configuration listens on port 1883, requires authentication, and disables anonymous access. Permit port 1883 only from the trusted local network. The ESP32 connects to the host's LAN IP; Node-RED can use `127.0.0.1` when it runs on the same host.

## Install and configure Node-RED

Follow the [dashboard setup guide](dashboard/README.md). Import `dashboard/node-red-flow.json`, then edit its MQTT broker node with the host and credentials. The flow listens on `motor/status`; the firmware publishes JSON containing `status`, `confidence`, `dsp_ms`, and `classification_ms`. The Node-RED Dashboard 1 UI is normally available at `http://localhost:1880/ui`.

## Flash the firmware

1. Install the Edge Impulse ZIP, PubSubClient, and ArduinoJson as described above.
2. Copy `secrets.h.example` to `secrets.h` and add your local settings.
3. Open `esp32cam_firmware1.ino` from the repository root in Arduino IDE.
4. Select **AI Thinker ESP32-CAM** and the serial port. Connect GPIO0 to GND and upload.
5. Disconnect GPIO0 from GND, press Reset, then open Serial Monitor at 115200 baud.

## Hardware-free MQTT smoke test

This test sends one synthetic event with the same topic and JSON keys used by the firmware. It requires a reachable authenticated Mosquitto broker and Python, but no ESP32 or camera.

```sh
python3 -m venv .venv
. .venv/bin/activate
python -m pip install -r requirements-smoke-test.txt
export MQTT_HOST=192.168.1.10
export MQTT_PORT=1883
export MQTT_USERNAME=mqttuser
export MQTT_PASSWORD='your-broker-password'
python scripts/mqtt_smoke_test.py
```

Set `MQTT_HOST` to the broker's LAN IP. A successful run waits for the broker's QoS 1 acknowledgment and prints `PASS`. With the flow deployed, check that the status text shows `anomaly` and the gauge shows `94.8`. This verifies the MQTT connection and publish path; it is not firmware compilation, model validation, or physical hardware testing.

## Verification status

- **Repository and flow configuration:** checked for file paths, JSON validity, topic `motor/status`, the displayed `status`/`confidence` fields, and declared Dashboard 1 dependency.
- **Synthetic MQTT publish:** run the smoke test above against your broker to verify connectivity and publishing without hardware.
- **Firmware compilation:** not verified from a clean clone because the required Edge Impulse Arduino export is not part of this repository. No ESP32 Arduino toolchain or model export was available during this repository review.
- **Hardware:** not tested here. Camera capture, inference timing, Wi-Fi behavior, and on-device memory use require the target board and matching model export.

## Reported model metrics

| Metric | Project-reported value |
|---|---:|
| Overall accuracy | 87.8% |
| Anomaly recall | 94.8% |
| Normal recall | 78.4% |
| Anomaly F1 score | 0.90 |
| Model | MobileNetV2, 96×96, int8 |
| Inference time | About 643 ms/frame |
| RAM use | About 280 KB of 520 KB |
| Model size | Under 1 MB |

Dataset documented by the project: Casting Product Quality Inspection, 7,284 images. Metrics are retained from the project documentation and have not been independently reproduced by this repository change.

## References

- [Edge Impulse documentation](https://docs.edgeimpulse.com/)
- [Casting Product dataset](https://www.kaggle.com/datasets/ravirajsinh45/real-life-industrial-dataset-of-casting-product)
- [Mosquitto documentation](https://mosquitto.org/documentation/)
- [Node-RED documentation](https://nodered.org/docs/)
