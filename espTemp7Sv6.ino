#include <DHTesp.h>
//#include <TM1637Display.h>
#include "display.h"
#include <ESP8266WiFi.h>
#include <WiFiManager.h>
#include <WiFiClientSecure.h>
#include <time.h>
#include <UniversalTelegramBot.h>
#include "Adafruit_MQTT.h"                                  // Adafruit MQTT library
#include "Adafruit_MQTT_Client.h"                           // Adafruit MQTT library 
const char* ssid = "Dhemest";
const char* password = "3344556677";
const char* ssid1 = "WJL";
const char* password1 = "Rahasia2";
// Initialize Telegram BOT
#define BOTtoken "1428684413:AAHbLK8Vh4PHjYy25jfFMb2n0oPsriWwyo8" // your Bot Token (Get from Botfather)
// IO Adafruit key
#define AIO_USERNAME  "arifwib"
#define AIO_KEY       "aio_iKYy28DnkUESLgevliggSU8VQ2D2"
#define AIO_SERVERPORT 8883 //1883
#define AIO_SERVER      "io.adafruit.com"

//Display Variables
const int CLK = D7; //Set the CLK pin connection to the display
const int DIO = D8; //Set the DIO pin connection to the display
TM1637Display display(CLK, DIO);  
DHTesp DHT;
int t;
int h;
int displaylah;
int DTime = 1; //Delaytime for itterations, 16ms default, 4 ms now

WiFiClientSecure net_ssl;
UniversalTelegramBot bot(BOTtoken, net_ssl);
Adafruit_MQTT_Client mqtt(&net_ssl, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_KEY); 

// Web Server on port 80
WiFiServer server(80);

int botRequestDelay = 500;
unsigned long lastTimeBotRan;

unsigned long lastTimeWarning;
int botWarningDelay = 600000;

unsigned long previousMillis = 0;
const long intervalMillis = 9000;

unsigned long previousIO = 0;
const long intervalIO = 300000;

//Telegram Variables
const String idTel_AW = "213752834";
const String idTel_DNN = "286273593";
int timezone = +7; //EST is UTC-5
int dst = 0;
int i = 0;
long timer = millis() + 500;

//Smoke Variables & LED
int smokeA0 = A0;
int sensorThres = 600;

#define wfLED D2
#define wfTrigger D0
#define wfPower D4

/****************************** Feeds ***************************************/ 
//Setup a feed called 'temperature' for publishing.
// Notice MQTT paths for AIO follow the form: <username>/feeds/<feedname>   // This feed is not needed, only setup if you want to see it
//absolete const char templine_FEED[] PROGMEM = AIO_USERNAME "/feeds/temperatureline";
Adafruit_MQTT_Publish ftemperature = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/temperature"); // Setup a feed called 'temperature' for publishing.
Adafruit_MQTT_Publish ftempline = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/temperatureline"); // Setup a feed called 'templine' for publishing.
Adafruit_MQTT_Publish fhumidity = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/humidity"); 
Adafruit_MQTT_Publish fhumiline = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/humidityline");
Adafruit_MQTT_Publish fheatindex = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/heatindex"); 

Adafruit_MQTT_Publish fgasses = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/gasses"); 
Adafruit_MQTT_Publish fgassline = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/gassline"); 
Adafruit_MQTT_Publish flampindicator = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/lampindicator"); 
Adafruit_MQTT_Publish ftimetext = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/timetext");

/****************************************************/ 
// Bug workaround for Arduino 1.6.6, it seems to need a function declaration
// for some reason (only affects ESP8266, likely an arduino-builder bug).
void MQTT_connect();

void setup(){
  // put your setup code here, to run once:
  Serial.begin(9600);

  net_ssl.setInsecure();
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  Serial.print("Connecting Wifi: ");
  WiFi.begin(ssid, password);
  delay(5000);
  configTime(timezone * 3600, dst * 0, "pool.ntp.org", "time.nist.gov"); //Time server
  /*
  while (!time(nullptr)) { //Wait for connection to get actual time
      delay(1000);
      showTime(0,false, 4, 0);    //digit 4
  } */
  DHT.setup(5, DHTesp::DHT11); // Connect DHT sensor to GPIO 05/D1;  
  display.setBrightness(0x0a);  //set the display to medium brightness
  //jika tdk konek wifi, coba alternatif wifi
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WIFI 1 not Connected, trying other 2...");  
    WiFi.begin(ssid1, password1);
    delay(3000);
    configTime(timezone * 3600, dst * 0, "pool.ntp.org", "time.nist.gov"); //Time server
  }

  showTime(display, 3000, 01, 23);  
  delay(100);
 
  pinMode(wfTrigger, INPUT); //D0 /GPIO16 / LED buttom
  pinMode(wfPower, OUTPUT); //D4 /GPIO2 /LED Blue Up
  pinMode(smokeA0, INPUT);
  //set AP modem if not connected
  if(digitalRead(wfTrigger) == HIGH and WiFi.status() != WL_CONNECTED) {
    digitalWrite(wfPower, LOW);
    WiFiManager wifiManager;
    //wifiManager.autoConnect("AutoConnectAP");
    wifiManager.resetSettings();
    wifiManager.autoConnect("TS Servers Sensor - AW");
    Serial.println("connected :)");
  }

  // Starting the web server
  server.begin();
  Serial.println("Web server running. Waiting for the ESP IP...");
}

void loop()
{
  unsigned long currentMillis = millis();
  
  //cek telegeram
  if (currentMillis > lastTimeBotRan + botRequestDelay)  {
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    while(numNewMessages) {
      Serial.println("got response");
      handleNewMessages(numNewMessages);
      numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    }
    lastTimeBotRan = millis();
  }
  
  //print status
  if (!time(nullptr)) {Serial.println("get time failed");  }
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WIFI not Connected");  
    digitalWrite(wfPower, LOW);
    digitalWrite(wfLED, LOW);
    delay(600);
    digitalWrite(wfPower, HIGH);
  }
  else { 
    Serial.print("Connected, IP address: "); 
    Serial.println(WiFi.localIP()); 
    digitalWrite(wfPower, HIGH);
    digitalWrite(wfLED, HIGH);
  }

  // Listenning for new clients
  WiFiClient client = server.available();
  if (client) {
    Serial.println("New client");
    // bolean to locate when the http request ends
    boolean blank_line = true;
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        
        if (c == '\n' && blank_line) {
            t = DHT.getTemperature();
            h = DHT.getHumidity();
            // Computes temperature values in Celsius + Fahrenheit and Humidity
            float hic = DHT.computeHeatIndex(t, h, false);       
          
            if (isnan(h) || isnan(t) || t==2147483647 || h==2147483647) {
             Serial.println("Failed to read from DHT sensor!");
             t = 0; h = 0;
             displaylah = 0;
             }
            else{
             displaylah = (t * 100) + h;  //geser t ke depan 2 digit
             }
            client.println("HTTP/1.1 200 OK");
            client.println("Content-Type: text/html");
            client.println("Connection: close");
            client.println();
            // your actual web page that displays temperature and humidity
            client.println("<!DOCTYPE HTML>");
            client.println("<html>");
            client.println("<head></head><body><h1>ESP8266 - Temperature and Humidity</h1><h3>Temperature in Celsius: ");
            client.println(t);
            client.println("*C</h3><h3>Humidity: ");
            client.println(h);
            client.println("%</h3><h3>");
            client.println("</body></html>");     
            break;
        }
        if (c == '\n') {
          // when starts reading a new line
          blank_line = true;
        }
        else if (c != '\r') {
          // when finds a character on the current line
          blank_line = false;
        }
      }
    }
    // closing the client connection
    delay(1);
    client.stop();
    Serial.println("Client disconnected.");
  }
  time_t now;
  struct tm * timeinfo;
  time(&now);
  timeinfo = localtime(&now);  
  /*
  dig1 = timeinfo->tm_hour/10; //Abusing integer math a bit to get the digits
  dig2 = timeinfo->tm_hour%10;
  dig3 = timeinfo->tm_min/10;
  dig4 = timeinfo->tm_min%10;
  */
  //try to lower LCD brightness
  if ((timeinfo->tm_hour > 18) and (timeinfo->tm_hour < 6)) {
    display.setBrightness(0x01);
  } else {
    display.setBrightness(0x09);
  }
    
  t = DHT.getTemperature();
  h = DHT.getHumidity();
  // Computes temperature values in Celsius + Fahrenheit and Humidity
  float hic = DHT.computeHeatIndex(t, h, false);       

  if (isnan(h) || isnan(t) || t==2147483647 || h==2147483647) {
   Serial.println("Failed to read from DHT sensor!");
   t = 0; h = 0;
   displaylah = 0;
   }
  else{
   displaylah = (t * 100) + h;  //geser t ke depan 2 digit
   }
  //display.showNumberDecEx(displaylah,0b01000000);
  Serial.print(timeinfo->tm_hour);
  Serial.print(":");
  Serial.println(timeinfo->tm_min);
  Serial.print("Temperature = ");
  Serial.println(t);
  Serial.print("Humidity = "); 
  Serial.println(h);
  if (currentMillis - previousMillis >= intervalMillis) {
    // save the last time
    previousMillis = currentMillis;
    Serial.println("LCD status menampilkan suhu celcius");
    showTemp(display,t);
    delay(1500);
    Serial.println("LCD status menampilkan humidity");
    showHumi(display,h);
    delay(1500);
    showTime(display, 5000, timeinfo->tm_hour, timeinfo->tm_min);
    //int digit_All = (timeinfo->tm_hour*100) + (timeinfo->tm_min);
    //display.showNumberDecEx(digit_All,0b01000000);
    //delay(5000);
  }
  
  //send data to IO.AdaFruit Dashboard
  if (currentMillis - previousIO >= intervalIO) {
    // save the last time
    previousIO = currentMillis;
    MQTT_connect();                                       // Run Procedure to connect to Adafruit IO MQTT     
    if (t > 0 ) {
      ftemperature.publish(t);
      ftempline.publish(t);
      fhumidity.publish(h);
      fhumiline.publish(h);
      fheatindex.publish(hic);
    }
    int analogSensor = analogRead(smokeA0);
    if (analogSensor > 0 ) {
      fgasses.publish(analogSensor);
      fgassline.publish(analogSensor);
    }
    ftimetext.publish((timeinfo->tm_hour*100)+timeinfo->tm_min);
    flampindicator.publish(0);
  }

  //cek semua sensor and send alarm to idTelelgram 
  if (millis() > lastTimeWarning + botWarningDelay)  {
    digitalWrite(wfLED, HIGH);
    //cek suhu
    if (t > 28) {
      lastTimeWarning = millis();
      String warningtext = "Warning, **Temperature** reach " + String(t) + " *C\n";
      warningtext += "Humidity " + String(h) + " %\n";
      warningtext += "This message will be sent again in 10 minutes *(if the conditions persists)*\n";
      Serial.println(warningtext);
      bot.sendMessage(idTel_AW, warningtext, "Markdown");
      bot.sendMessage(idTel_DNN, warningtext, "Markdown");
      digitalWrite(wfLED, LOW);
      flampindicator.publish(t);
    }
    //cek gas asap
    int analogSensor = analogRead(smokeA0);
    Serial.print("Pin A0: ");
    Serial.println(analogSensor);
    // Checks if it has reached the threshold value
    if (analogSensor > sensorThres) {
      lastTimeWarning = millis();
      String warningtext = "Warning, kadar **gas / asap** melebihi ambang batas ( ~~MQ-2~~ )\n";
      Serial.println(warningtext);
      bot.sendMessage(idTel_AW, warningtext, "Markdown");
      bot.sendMessage(idTel_DNN, warningtext, "Markdown");
      digitalWrite(wfLED, LOW);
      flampindicator.publish(analogSensor);
    }
  }
}

void MQTT_connect() {
  if (mqtt.connected()) { return; }                     // Stop and return to Main Loop if already connected to Adafruit IO
  Serial.print("Connecting to MQTT... ");
  mqtt.connect();
  if (mqtt.connected()) {
    Serial.println("MQTT Connected!");
  }else {
    Serial.println("MQTT Disconnect !");
  }
/* di block karena menggunakan jk restart akan input wifi conn lg
  int8_t ret;
  uint8_t retries = 3;
  while ((ret = mqtt.connect()) != 0) {                 // Connect to Adafruit, Adafruit will return 0 if connected
       Serial.println(mqtt.connectErrorString(ret));   // Display Adafruits response
       Serial.println("Retrying MQTT...");
       mqtt.disconnect();
       delay(5000);                                     // wait X seconds
       retries--;
       if (retries == 0) {                              // basically die and wait for WatchDogTimer to reset me                                                          
         while (1);         
       }
  }
  Serial.println("MQTT Connected!");
  delay(1000);
*/  
}

void handleNewMessages(int numNewMessages) {
  Serial.println("handleNewMessages");
  Serial.println(String(numNewMessages));

  for (int i=0; i<numNewMessages; i++) {
    String chat_id = String(bot.messages[i].chat_id);
    String text = bot.messages[i].text;

    String from_name = bot.messages[i].from_name;
    if (from_name == "") from_name = "Guest";

    if (text == "/suhu") {
      //sensors.requestTemperatures();
      t = DHT.getTemperature();
      //h = DHT.getHumidity();
      //float hic = DHT.computeHeatIndex(t, h, false);
      //displaylah = (t * 100) + h;  //geser t ke depan 2 digit
      //bot.sendMessage(m.chat_id, String(printTemperature(Thermometer1))); 
      bot.sendMessage(chat_id, "Temperature : " + String(t) + " *C"); 
      //bot.sendMessage(m.chat_id, "Humidity : " + String(h) + " %"); 
    }
    if (text == "/humi") {
      //sensors.requestTemperatures();
      t = DHT.getTemperature();
      h = DHT.getHumidity();
      float hic = DHT.computeHeatIndex(t, h, false);
      //bot.sendMessage(m.chat_id, String(printTemperature(Thermometer1))); 
      bot.sendMessage(chat_id, "Humidity : " + String(h) + " %"); 
      bot.sendMessage(chat_id, "HeatIndex : " + String(hic) + " *C"); 
    }

    //cek suara
    if (text == "/sound") {
      bot.sendMessage(chat_id, "Sorry, esp8266 only has 1 Analog and being used by smoke detector"); 
    }

    //cek gass
    if (text == "/gass") {
      int analogSensor = analogRead(smokeA0);
      bot.sendMessage(chat_id, "Gas level is " + String(analogSensor), "");
    }
        
    if (text == "/status") {
      bot.sendMessage(chat_id, "semua akan baik-baik saja", "");
    }

    if (text == "/lcdhigh") {
      display.setBrightness(0x0f);  //set the display to maximum brightness
      bot.sendMessage(chat_id, "The Led MAXimum brightness", "");
    }

    if (text == "/lcdlow") {
      display.setBrightness(0x00);  //set the diplay to minimum brightness
      bot.sendMessage(chat_id, "The Led LOW brighness", "");
    }

    if (text == "/lcdON") {
      display.setBrightness(0x08, true);  //set the diplay to ON and medium brightness 
      bot.sendMessage(chat_id, "The Led ON", "");
    }
    if (text == "/lcdOFF") {
      display.setBrightness(0x00, false);  //set the diplay to OFF 
      bot.sendMessage(chat_id, "The Led OFF", "");
    }

    if (text == "/start" || text == "/help") {
      String welcome = "Hello, " + from_name + ". I'm ESP8266, controlled by: \n";
      welcome += "/suhu : Temperatur\n";
      welcome += "/humi : Humidity\n";
      welcome += "/sound : Loudness\n";
      welcome += "/gass : Gasses\n";
      welcome += "/status : Returns status of us\n";
      welcome += "/lcdHIGH : to set the display to maximum brightness\n";
      welcome += "/lcdLOW : to set the diplay to minimum brightness\n";
      welcome += "/lcdON : to switch ON display and set the diplay to medium brightness\n";
      welcome += "/lcdOFF : to switch OFF display\n";
      bot.sendMessage(chat_id, welcome, "Markdown");
    }
  }
}
