/*
 * Edge Impulse camera classifier with Wi-Fi and MQTT publishing.
 * Copy secrets.h.example to secrets.h and add local credentials before compiling.
 */
#include <MetalMind-project-1_inferencing.h>
#include "edge-impulse-sdk/dsp/image/image.hpp"
#include "esp_camera.h"
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "secrets.h"

#ifndef MQTT_PORT
#define MQTT_PORT 1883
#endif

WiFiClient espClient;
PubSubClient client(espClient);

#define CAMERA_MODEL_AI_THINKER
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

#define EI_CAMERA_RAW_FRAME_BUFFER_COLS 320
#define EI_CAMERA_RAW_FRAME_BUFFER_ROWS 240
#define EI_CAMERA_FRAME_BYTE_SIZE 3

static bool debug_nn = false;
static bool is_initialised = false;
static uint8_t *snapshot_buf = nullptr;

static camera_config_t camera_config = {
    .pin_pwdn = PWDN_GPIO_NUM,
    .pin_reset = RESET_GPIO_NUM,
    .pin_xclk = XCLK_GPIO_NUM,
    .pin_sscb_sda = SIOD_GPIO_NUM,
    .pin_sscb_scl = SIOC_GPIO_NUM,
    .pin_d7 = Y9_GPIO_NUM,
    .pin_d6 = Y8_GPIO_NUM,
    .pin_d5 = Y7_GPIO_NUM,
    .pin_d4 = Y6_GPIO_NUM,
    .pin_d3 = Y5_GPIO_NUM,
    .pin_d2 = Y4_GPIO_NUM,
    .pin_d1 = Y3_GPIO_NUM,
    .pin_d0 = Y2_GPIO_NUM,
    .pin_vsync = VSYNC_GPIO_NUM,
    .pin_href = HREF_GPIO_NUM,
    .pin_pclk = PCLK_GPIO_NUM,
    .xclk_freq_hz = 20000000,
    .ledc_timer = LEDC_TIMER_0,
    .ledc_channel = LEDC_CHANNEL_0,
    .pixel_format = PIXFORMAT_JPEG,
    .frame_size = FRAMESIZE_QVGA,
    .jpeg_quality = 12,
    .fb_count = 1,
    .fb_location = CAMERA_FB_IN_PSRAM,
    .grab_mode = CAMERA_GRAB_WHEN_EMPTY,
};

bool ei_camera_init(void);
bool ei_camera_capture(uint32_t img_width, uint32_t img_height, uint8_t *out_buf);
static int ei_camera_get_data(size_t offset, size_t length, float *out_ptr);

void setup_wifi() {
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.print("Connecting to Wi-Fi");
    const unsigned long started = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - started < 30000) {
        delay(500);
        Serial.print(".");
    }
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("\nWi-Fi connection timed out; restarting in 5 seconds.");
        delay(5000);
        ESP.restart();
    }
    Serial.print("\nWi-Fi connected, IP: ");
    Serial.println(WiFi.localIP());
}

void reconnect_mqtt() {
    while (!client.connected()) {
        Serial.print("Connecting to MQTT...");
        if (client.connect("ESP32CAM", MQTT_USERNAME, MQTT_PASSWORD)) {
            Serial.println("connected");
        } else {
            Serial.print("failed, state=");
            Serial.println(client.state());
            delay(3000);
        }
    }
}

void setup() {
    Serial.begin(115200);
    delay(100);
    Serial.println("\nEdge Impulse visual inspection");

    setup_wifi();
    client.setServer(MQTT_SERVER, MQTT_PORT);

    if (!ei_camera_init()) {
        Serial.println("Camera initialization failed; stopping.");
        while (true) delay(1000);
    }
    Serial.println("Camera initialized");
    delay(2000);
}

void loop() {
    if (!client.connected()) reconnect_mqtt();
    client.loop();

    if (ei_sleep(5) != EI_IMPULSE_OK) return;

    const size_t buffer_size = EI_CAMERA_RAW_FRAME_BUFFER_COLS *
                               EI_CAMERA_RAW_FRAME_BUFFER_ROWS *
                               EI_CAMERA_FRAME_BYTE_SIZE;
    snapshot_buf = (uint8_t *)malloc(buffer_size);
    if (snapshot_buf == nullptr) {
        Serial.println("ERR: Could not allocate image buffer");
        delay(1000);
        return;
    }

    if (!ei_camera_capture(EI_CLASSIFIER_INPUT_WIDTH,
                          EI_CLASSIFIER_INPUT_HEIGHT, snapshot_buf)) {
        Serial.println("Camera capture failed");
        free(snapshot_buf);
        snapshot_buf = nullptr;
        return;
    }

    ei::signal_t signal;
    signal.total_length = EI_CLASSIFIER_INPUT_WIDTH * EI_CLASSIFIER_INPUT_HEIGHT;
    signal.get_data = &ei_camera_get_data;

    ei_impulse_result_t result = {0};
    const EI_IMPULSE_ERROR err = run_classifier(&signal, &result, debug_nn);
    if (err != EI_IMPULSE_OK) {
        Serial.printf("Classifier failed: %d\n", err);
        free(snapshot_buf);
        snapshot_buf = nullptr;
        return;
    }

    float confidence = 0.0f;
    const char *status = "unknown";
    for (uint16_t i = 0; i < EI_CLASSIFIER_LABEL_COUNT; ++i) {
        if (result.classification[i].value > confidence) {
            confidence = result.classification[i].value;
            status = ei_classifier_inferencing_categories[i];
        }
    }

    Serial.printf("Prediction: %s (%.2f%%)\n", status, confidence * 100.0f);

    StaticJsonDocument<200> doc;
    doc["status"] = status;
    doc["confidence"] = confidence * 100.0f;
    doc["dsp_ms"] = result.timing.dsp;
    doc["classification_ms"] = result.timing.classification;

    char payload[200];
    serializeJson(doc, payload, sizeof(payload));
    if (client.publish("motor/status", payload)) {
        Serial.printf("Published: %s\n", payload);
    } else {
        Serial.println("MQTT publish failed");
    }

    free(snapshot_buf);
    snapshot_buf = nullptr;
}

bool ei_camera_init(void) {
    if (is_initialised) return true;

    const esp_err_t err = esp_camera_init(&camera_config);
    if (err != ESP_OK) {
        Serial.printf("Camera init failed with error 0x%x\n", err);
        return false;
    }

    sensor_t *sensor = esp_camera_sensor_get();
    if (sensor && sensor->id.PID == OV3660_PID) {
        sensor->set_vflip(sensor, 1);
        sensor->set_brightness(sensor, 1);
        sensor->set_saturation(sensor, 0);
    }
    is_initialised = true;
    return true;
}

bool ei_camera_capture(uint32_t img_width, uint32_t img_height, uint8_t *out_buf) {
    if (!is_initialised) {
        Serial.println("Camera is not initialized");
        return false;
    }

    camera_fb_t *fb = esp_camera_fb_get();
    if (!fb) {
        Serial.println("Camera capture failed");
        return false;
    }

    const bool converted = fmt2rgb888(fb->buf, fb->len, PIXFORMAT_JPEG, out_buf);
    esp_camera_fb_return(fb);
    if (!converted) {
        Serial.println("JPEG to RGB conversion failed");
        return false;
    }

    if (img_width != EI_CAMERA_RAW_FRAME_BUFFER_COLS ||
        img_height != EI_CAMERA_RAW_FRAME_BUFFER_ROWS) {
        ei::image::processing::crop_and_interpolate_rgb888(
            out_buf,
            EI_CAMERA_RAW_FRAME_BUFFER_COLS,
            EI_CAMERA_RAW_FRAME_BUFFER_ROWS,
            out_buf,
            img_width,
            img_height);
    }
    return true;
}

static int ei_camera_get_data(size_t offset, size_t length, float *out_ptr) {
    size_t pixel_index = offset * 3;
    for (size_t i = 0; i < length; ++i) {
        out_ptr[i] = (snapshot_buf[pixel_index + 2] << 16) |
                     (snapshot_buf[pixel_index + 1] << 8) |
                      snapshot_buf[pixel_index];
        pixel_index += 3;
    }
    return 0;
}

#if !defined(EI_CLASSIFIER_SENSOR) || EI_CLASSIFIER_SENSOR != EI_CLASSIFIER_SENSOR_CAMERA
#error "The exported Edge Impulse model must use a camera sensor"
#endif
