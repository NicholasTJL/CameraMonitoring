#include "esp_camera.h"
#include <WiFi.h>
#include <ArduinoWebsockets.h>
#define CAMERA_MODEL_AI_THINKER
#include "camera_pins.h"

const char* ssid = "dlinkmic";
const char* password = "12KwongMin";
const char* websocket_server_host = "192.168.0.105";
const uint16_t websocket_server_port = 8888;

using namespace websockets;
WebsocketsClient client;
bool isWebSocketConnected;

void onEventsCallback(WebsocketsEvent event, String data){
  if(event == WebsocketsEvent::ConnectionOpened){
    Serial.println("Connection Opened");
    isWebSocketConnected = true;
  }else if(event == WebsocketsEvent::ConnectionClosed){
    Serial.println("Connection Closed");
    isWebSocketConnected = false;
    webSocketConnect();
  }
}

void setup() {
  isWebSocketConnected = false;
  Serial.begin(115200);
  Serial.println();

  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 10000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size = FRAMESIZE_VGA;
  config.jpeg_quality = 40;
  config.fb_count = 2;

  // camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }
 

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
  client.onEvent(onEventsCallback);
  webSocketConnect();
}

void webSocketConnect() {
    Serial.println("Attempting WebSocket connection...");
    int attempts = 0;
    const int maxAttempts = 10;  // Maximum number of connection attempts

    while (!client.connect(websocket_server_host, websocket_server_port, "/")) {
        delay(500);
        Serial.print(".");
        attempts++;
        
        if (attempts >= maxAttempts) {
            Serial.println("\nWebSocket connection failed after 10 attempts!");
            Serial.println("Check server address and port:");
            Serial.print("Server: ");
            Serial.println(websocket_server_host);
            Serial.print("Port: ");
            Serial.println(websocket_server_port);
            Serial.println("Restarting ESP32...");
            delay(1000);
            ESP.restart();  // Restart the ESP32 if connection fails
            return;
        }
    }
    Serial.println("\nWebSocket Connected!");
}

void loop() {

  if(client.available()){
    client.poll();
  }
  
  if(!isWebSocketConnected) return;
  
  camera_fb_t *fb = esp_camera_fb_get();
  if(!fb){
    Serial.println("Camera capture failed");
    esp_camera_fb_return(fb);
    return;
  }

  if(fb->format != PIXFORMAT_JPEG){
    Serial.println("Non-JPEG data not implemented");
    return;
  }
  
  fb->buf[12] = 0x01; //FIRST CAM
  //fb->buf[12] = 0x02; //SECOND CAM 5640
  //fb->buf[12] = 0x03; //THIRD CAM
  //fb->buf[12] = 0x04; //FOURTH CAM
  
  client.sendBinary((const char*) fb->buf, fb->len);
  esp_camera_fb_return(fb);
}
