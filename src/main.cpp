#include <Arduino.h>

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <Wire.h>
#include "DHT.h"
#include <Stepper.h>
#include "RTClib.h" 
#include <Adafruit_SSD1306.h>

//-----------------------------------------------------------//

const int DHTPIN = 15;          //chân cảm biến DHT22
const int LED_PIN = 13;         //chân Led
const int photoresistorPin = 33;//chân cảm biến ánh sáng
const int trigPin = 5;          //chân cảm biến siêu âm 
const int echoPin = 18;
#define SOUND_SPEED 0.034       //xác định tốc độ âm thanh tính bằng cm/uS
#define CM_TO_INCH 0.393701
long duration;
float distanceCm;
float distanceInch;
float lux;
const int pir = 19;             //chân cảm biến hồng ngoại thụ động
float stepmax = 600;            //chân  motor
Stepper Stepper1(stepmax, 12, 14, 27, 26);
int i = 0;
float step1;
float steptarget1 = 500; 
float deltaT = 5;
float speed1;
const int Buzzer = 23;          //chân buzzer
const int LED_PIN2 = 25;        //chân led vàng
const float GAMMA = 0.7;
const float RL10 = 33;
#define Pump 4                  //Relay kích mức cao bằng chân số 4, sử dụng relay để bật tắt bơm
int hourAlarm = 12, minuteAlarm = 0;  //Set thời gian hẹn giờ tưới nước (mặc định 12.00 giờ)
int timePumpRun = 1;                  //Set thời gian bật bơm tưới 1 phút
bool statusRelay = false;
int PIR;

//-----------------------------------------------------------//

const char* ssid     =  "Wokwi-GUEST";
const char* password = "";
// REPLACE with your Domain name and URL path or IP address with path
const char* serverName = "http://esp-weather-station.000webhostapp.com/esp-post-data.php";
String apiKeyValue = "tPmAT5Ab3j7F9";
String sensorName = "BME280";
String sensorLocation = "Office";
#define SEALEVELPRESSURE_HPA (1013.25)

//define RTC
RTC_DS3231 rtc;
char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321
DHT dht(DHTPIN, DHTTYPE);
float Temperature, Humidity;
//WiFiClient client;
//Oled DS1306
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
//-----------------------------------------------------------//

void setup() {
  Serial.begin(115200);
  //khai báo DHT22
  Serial.println(F("DHTxx test!"));
  dht.begin();
  //khai báo Led đỏ
  pinMode(LED_PIN, OUTPUT);
  //khai báo Led vàng
  pinMode(LED_PIN2, OUTPUT);
  //khai báo cảm biến ánh sáng
  pinMode(photoresistorPin, INPUT);
  //khai báo cảm biến siêu âm
  pinMode(trigPin, OUTPUT); // Sets the trigPin as an Output
  pinMode(echoPin, INPUT); // Sets the echoPin as an Input
  //khai báo cảm biến cảm biến hồng ngoại thụ động
  pinMode(pir,INPUT);
  //khai báo motor
  speed1 = ((steptarget1 * 60) / (deltaT * stepmax));
  Stepper1.setSpeed(speed1);
  //khai báo buzzer
  pinMode(Buzzer, OUTPUT);
// Khai bao RTC
  #ifndef ESP8266
  while (!Serial); // wait for serial port to connect. Needed for native USB
#endif

  if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    Serial.flush();
    while (1) delay(10);
  }

  if (rtc.lostPower()) {
    Serial.println("RTC lost power, let's set the time!");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    // January 21, 2014 at 3am you would call:
    //rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
  }
  // rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  // January 21, 2014 at 3am you would call:
  //rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
//Khởi động màn hình oled
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;); // Don't proceed, loop forever
  }

  display.clearDisplay();
  display.setTextSize(1);           
  display.setTextColor(SSD1306_WHITE);      
  display.setCursor(0,0); 
  display.println("Connecting to WiFi...");
  display.display();
  delay(1000);
// Khởi động kết nối wifi
  WiFi.begin(ssid, password);
  Serial.println("Connecting");
  while(WiFi.status() != WL_CONNECTED) { 
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to WiFi network with IP Address: ");
  Serial.println(WiFi.localIP());
}


void dht_sensor();
void RTC_function();
void displayOled();
void photoresistor();
void getHttp();
void gerak(float k) {
  step1 = (steptarget1) * (k / (1000));
  Stepper1.step(step1);
}

void stop() {
  step1 = step1;
}
const char* serverTime = "http://esp-weather-station.000webhostapp.com/esp-time-duration.php"; // Replace with your server URL

//-----------------------------------------------------------//

//hàm đọc dữ liệu từ dht22
void dht_sensor()
{
  Humidity = dht.readHumidity();
  // Read temperature as Celsius (the default)
  Temperature = dht.readTemperature();
  // Read temperature as Fahrenheit (isFahrenheit = true)
  float f = dht.readTemperature(true);

  // Check if any reads failed and exit early (to try again).
  if (isnan(Humidity) || isnan(Temperature) || isnan(f)) {
    Serial.println(F("Failed to read from DHT sensor!"));
    return;
  }

  // Compute heat index in Fahrenheit (the default)
  float hif = dht.computeHeatIndex(f, Humidity);
  // Compute heat index in Celsius (isFahreheit = false)
  float hic = dht.computeHeatIndex(Temperature, Humidity, false);

  Serial.print(F("Humidity: "));
  Serial.print(Humidity);
  Serial.print(F("%  Temperature: "));
  Serial.print(Temperature);
  Serial.print(F("°C "));
  
}

void RTC_function()
{
    DateTime now = rtc.now();

    Serial.print(now.year(), DEC);
    Serial.print('/');
    Serial.print(now.month(), DEC);
    Serial.print('/');
    Serial.print(now.day(), DEC);
    Serial.print(" (");
    Serial.print(daysOfTheWeek[now.dayOfTheWeek()]);
    Serial.print(") ");
    Serial.print(now.hour(), DEC);
    Serial.print(':');
    Serial.print(now.minute(), DEC);
    Serial.print(':');
    Serial.print(now.second(), DEC);
    Serial.println();
}
void displayOled()
{
  display.clearDisplay();
  display.setCursor(0,0); 
  display.println("H: " + String(Humidity) +" %");
  display.setCursor(65,0); 
  display.println("T: " + String(Temperature)+" C");
  display.setCursor(0,20); 
  display.println("D: " + String(distanceCm)+" cm");
  display.setCursor(75,20); 
  display.println("Pir: " + String(digitalRead(pir))); 
  display.setCursor(0,40); 
  if(statusRelay)
  {
  display.println("Bom bat "); 
  }
  else
  {display.println("Bom tat ");}
  display.display();
  display.setCursor(0,60);
  display.println("Light Intensity: " + String(lux) + "lux");
}

void photoresistor(){
    // Read value from photoresistor
  int lightIntensity = analogRead(photoresistorPin);
  float voltage = lightIntensity / 4096. * 3.3;
  float resistance = 2000 * voltage / (1 - voltage / 3.3);
  lux = pow(RL10 * 1e3 * pow(10, GAMMA) / resistance, (1 / GAMMA));
  if (lux < 500) {
     digitalWrite(LED_PIN2, HIGH);
    
  } else {
     digitalWrite(LED_PIN2, LOW);
    
  }

}
void getHttp() {
  //Check WiFi connection status
  if(WiFi.status()== WL_CONNECTED){
    String url = "http://esp-weather-station.000webhostapp.com/esp-post-data.php?api_key=" + apiKeyValue + "&sensor=" + sensorName
                          + "&location=" + sensorLocation + "&value1=" + String(Humidity)
                          + "&value2=" + String(Temperature) + "&value3=" + String(distanceCm) + "&value4=" + String(lux) + "&value5=" + String(PIR)+"";
    HTTPClient http; 
    http.begin(url);
    
    int httpCode = http.GET();
  if (httpCode == HTTP_CODE_OK) {
    Serial.println("Data pushed successfully");
  } else {
    Serial.println("Push error: " + String(httpCode));
  }

  http.end();
  delay(2000);
  }
}


void loop() {
// Lấy thời gian 
  RTC_function();

if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;

    http.begin(serverTime); // Specify the URL
    int httpCode = http.GET(); // Make the request

    if (httpCode > 0) { // Check for the returning code
      String payload = http.getString();
      Serial.println("Response: " + payload);

      // Split the data
      int separatorIndex = payload.indexOf('/');
      if (separatorIndex != -1) {
        String time = payload.substring(0, separatorIndex);
        String durationStr = payload.substring(separatorIndex + 1);

        Serial.println("Time: " + time);
        Serial.println("Duration: " + durationStr);

        // Further split the time into hours and minutes
        int colonIndex1 = time.indexOf(':');
        int colonIndex2 = time.indexOf(':', colonIndex1 + 1);

        if (colonIndex1 != -1 && colonIndex2 != -1) {
          String hoursStr = time.substring(0, colonIndex1);
          String minutesStr = time.substring(colonIndex1 + 1, colonIndex2);
          int duration = durationStr.toInt();

          int hours = hoursStr.toInt();
          int minutes = minutesStr.toInt();

          Serial.print("Hours: ");
          Serial.println(hours);
          Serial.print("Minutes: ");
          Serial.println(minutes);
          Serial.print("Duration: ");
          Serial.println(duration);

          // Get current time from RTC
          DateTime now = rtc.now();
          int currentHours = now.hour();
          int currentMinutes = now.minute();

          Serial.print("Current Hours: ");
          Serial.println(currentHours);
          Serial.print("Current Minutes: ");
          Serial.println(currentMinutes);

          // Calculate the end time
          int endMinutes = minutes + duration;
          int endHours = hours + (endMinutes / 60);
          endMinutes = endMinutes % 60;

          Serial.print("End Hours: ");
          Serial.println(endHours);
          Serial.print("End Minutes: ");
          Serial.println(endMinutes);
          
          bool isWithinTimeFrame = true;
          // Check if current time is within start time and end time
           if (    currentHours < hours 
                || currentHours > endHours 
                || (currentHours == hours && currentMinutes <= minutes) 
                || (currentHours == endHours && currentMinutes >= endMinutes)
              )
            {
              isWithinTimeFrame = false;
            }

          if (isWithinTimeFrame) {
              Serial.println("Current time is within the scheduled time frame.");
              digitalWrite(Pump, HIGH);
              display.setCursor(0, 40);
              statusRelay = true;
          } else {
              Serial.println("Current time is not within the scheduled time frame.");
              digitalWrite(Pump, LOW);
              display.setCursor(0, 40);
              statusRelay = false;
          }
          
        } else {
          Serial.println("Invalid time format");
        }
      } else {
        Serial.println("Invalid format");
      }
    } else {
      Serial.println("Error on HTTP request");
    }

    http.end(); // Free the resources
  } else {
    Serial.println("WiFi not connected");
  }

//Đọc giá trị từ cảm biến dht22
  dht_sensor();
  // Read value from photoresistor
  int lightIntensity = analogRead(photoresistorPin);
  float voltage = lightIntensity / 4096. * 3.3;
  float resistance = 2000 * voltage / (1 - voltage / 3.3);
  lux = pow(RL10 * 1e3 * pow(10, GAMMA) / resistance, (1 / GAMMA));
  if (lux < 500) {
     digitalWrite(LED_PIN2, HIGH);
  } else {
     digitalWrite(LED_PIN2, LOW);
  }

  // Control LED based on temperature and humidity
  if (Temperature > 35 || Temperature < 12 || Humidity > 70 || Humidity < 40) {
     digitalWrite(LED_PIN, HIGH);
    
  } else {
     digitalWrite(LED_PIN, LOW);
    
  }

  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  // Sets the trigPin on HIGH state for 10 micro seconds
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  
  // Reads the echoPin, returns the sound wave travel time in microseconds
  long duration = pulseIn(echoPin, HIGH);
  
  // Calculate the distance
  distanceCm = duration * SOUND_SPEED / 2;

  if (distanceCm < 200) {
    // Nếu khoảng cách nhỏ hơn 200cm, motor sẽ chạy theo chiều thuận
    Stepper1.step(steptarget1);
    step1 += steptarget1; // Cập nhật vị trí hiện tại của motor
  } else {
    // Nếu khoảng cách lớn hơn hoặc bằng 200cm, motor sẽ quay ngược lại và dừng lại ở vị trí 0
    Stepper1.step(-step1); // Di chuyển motor về vị trí 0 ngay lập tức
    delay(100); // Đợi một lát để motor dừng lại
    stop(); // Dừng motor và đặt vị trí hiện tại về 0
  }
   
  PIR = digitalRead(pir);
  if (PIR == HIGH) {
    // Phát ra tiếng từ buzzer với tần số 1000Hz trong 100ms
    digitalWrite(Buzzer, HIGH);
    delay(100);
    
  } else {
    // Tắt tiếng từ buzzer
    digitalWrite(Buzzer, LOW);
  }
  Serial.print(" Pir: " + String(PIR));
  Serial.println("  Distance: " +String(distanceCm));
  Serial.println("Light Intensity: " + String(lux) + "lux");
  //pushData(); 
  //sendHttp();
  getHttp();
  displayOled();
}
//---------------------//
