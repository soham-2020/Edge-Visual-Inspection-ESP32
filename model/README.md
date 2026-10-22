# Edge Impulse model export

The firmware expects the Edge Impulse Arduino library to provide `MetalMind-project-1_inferencing.h`. This generated library is not committed because it is tied to the Edge Impulse project export.

## Install the matching export

1. Open the Edge Impulse project that produced this model.
2. Open **Deployment**, choose **Arduino library**, select an int8 quantized deployment, and build/download its ZIP.
3. Install the ZIP in Arduino IDE with **Sketch → Include Library → Add .ZIP Library…**.
4. Confirm the generated header name matches the include in `esp32cam_firmware1.ino`. If the project slug changed, update the include to the generated header name.
5. Confirm the export is a camera image classifier and its input dimensions are compatible with the firmware.

## Project-reported model details

These figures are retained from the project documentation; they have not been independently reproduced as part of the repository changes.

| Item | Project-reported value |
|---|---:|
| Architecture | MobileNetV2, 96×96, int8 |
| Overall accuracy | 87.8% |
| Anomaly recall | 94.8% |
| Normal recall | 78.4% |
| Anomaly F1 score | 0.90 |
| Inference time | About 643 ms on ESP32 at 240 MHz |
| RAM use | About 280 KB of 520 KB |
| Model size | Under 1 MB |
| Dataset | Casting Product Quality Inspection |
| Dataset size | 7,284 labeled normal/anomaly images |

Dataset source: [Casting Product Quality Inspection on Kaggle](https://www.kaggle.com/datasets/ravirajsinh45/real-life-industrial-dataset-of-casting-product).
