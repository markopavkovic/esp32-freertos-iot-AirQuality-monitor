#include <DHT.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>
#include <WebSocketsClient.h>

const char* ssid = "TS-fTB8";
const char* password = "296xjR4W";


const char* websocket_server_host = "192.168.1.3"; 
const uint16_t websocket_server_port = 8080;
WebSocketsClient webSocket;

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_MOSI 23
#define OLED_CLK 18
#define OLED_DC 2
#define OLED_CS 5
#define OLED_RESET 4
Adafruit_SSD1306 display(SCREEN_WIDTH,SCREEN_HEIGHT,&SPI,OLED_DC,OLED_RESET,OLED_CS);

#define BUZZ_PIN 25

#define MQ135_ANALOG_PIN 35

#define DHT_PIN 13
#define DHT_TYPE DHT11
DHT dht(DHT_PIN,DHT_TYPE);

const uint16_t TASK_STACK_SIZE=4096;
const UBaseType_t TASK_PRIORITY=1;

float current_Humidity=0;
float current_Temperature=0;
int current_AirQuality=0;

const int AIR_QUALITY_THRESHOLD=1000;

void vTaskBuzzer(void*pvParameters);
void vTaskAirQuality(void*pvParameters);
void vTaskTemperatureHumidity(void*pvParameters);
void vTaskDisplay(void*pvParameters);
void updateDisplay(float temperature,float humidity,int air_quality);

void setup(){
  Serial.begin(115200); 
  delay(1000);
  Serial.println("Pokretanje sistema...");
  pinMode(BUZZ_PIN,OUTPUT);
  analogSetAttenuation(ADC_0db);

  dht.begin();
  if(!display.begin(SSD1306_SWITCHCAPVCC)){
    Serial.println(F("Error!"));
    while(1);
  }

  WiFi.begin(ssid,password);
  while(WiFi.status()!=WL_CONNECTED){
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWi-Fi povezan!");


  webSocket.begin(websocket_server_host,websocket_server_port, "/");
  webSocket.setReconnectInterval(5000);

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("Air Quality Monitor");
  display.display();
  delay(2000);

  xTaskCreatePinnedToCore(
    vTaskBuzzer,   
    "BuzzerTask", 
    TASK_STACK_SIZE,         
    NULL,        
    TASK_PRIORITY,           
    NULL,         
    1             
  );
  xTaskCreatePinnedToCore(
    vTaskAirQuality,
    "AirQualityTask",
    TASK_STACK_SIZE,
    NULL,
    TASK_PRIORITY,
    NULL,
    1
  );
  xTaskCreatePinnedToCore(
    vTaskTemperatureHumidity,
    "DHTTask",
    TASK_STACK_SIZE,
    NULL,
    TASK_PRIORITY,
    NULL,
    1
  );
  xTaskCreatePinnedToCore(
    vTaskDisplay,
    "DisplayTask",
    TASK_STACK_SIZE,
    NULL,
    TASK_PRIORITY,
    NULL,
    1
  );
}

void loop(){
  webSocket.loop();
  vTaskDelay(pdMS_TO_TICKS(10));

}

void vTaskBuzzer(void*pvParameters){
  while(1){
    if(current_AirQuality>AIR_QUALITY_THRESHOLD){
    digitalWrite(BUZZ_PIN,HIGH);
    vTaskDelay(pdMS_TO_TICKS(200));
    digitalWrite(BUZZ_PIN,LOW);
    vTaskDelay(pdMS_TO_TICKS(200));
    }
    else{
    digitalWrite(BUZZ_PIN,LOW);
    vTaskDelay(pdMS_TO_TICKS(500));
    }
  }
}
void vTaskAirQuality(void*pvParameters){
  while(1){
    current_AirQuality=analogRead(MQ135_ANALOG_PIN);
   
   
    Serial.print("MQ135 Raw ADC: ");
    Serial.println(current_AirQuality);

   
    if(WiFi.status() == WL_CONNECTED) {
      String payload_AQ = "{\"gas\":" + String(current_AirQuality) + "}";
      webSocket.sendTXT(payload_AQ);
    }
    
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}
void vTaskTemperatureHumidity(void*pvParameters){
  while(1){
    current_Humidity=dht.readHumidity();
    current_Temperature=dht.readTemperature();
 
    if(WiFi.status() == WL_CONNECTED) {
      String payload_hum = "{\"humidity\":" + String(current_Humidity) + "}";
      webSocket.sendTXT(payload_hum);
      String payload_tmp = "{\"temperature\":" + String(current_Temperature) + "}";
      webSocket.sendTXT(payload_tmp);
    }
    vTaskDelay(pdMS_TO_TICKS(2000));
  }
}
void vTaskDisplay(void*pvParameters){
  while(1){
    updateDisplay(current_Temperature, current_Humidity, current_AirQuality);
    vTaskDelay(pdMS_TO_TICKS(500));
  }
}
void updateDisplay(float temperature,float humidity,int air_quality){
  display.clearDisplay(); 
  display.setCursor(0, 0);
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  
  display.print("Temperature: "); display.print(temperature); display.println(" C");
  display.print("Humidity:    "); display.print(humidity); display.println(" %");
  display.print("Air Quality: "); display.println(air_quality);
  
  display.display(); 
}
