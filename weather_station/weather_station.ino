#include <Wire.h> //I2C needed for sensors
#include "SparkFunMPL3115A2.h" //Pressure sensor - Search "SparkFun MPL3115" and install from Library Manager
#include "SparkFun_Si7021_Breakout_Library.h" //Humidity sensor - Search "SparkFun Si7021" and install from Library Manager

#include <SoftwareSerial.h>

#include "arduino_secrets.h"

String ssid = SSID;
String password = PASSWORD; 

MPL3115A2 myPressure; //Create an instance of the pressure sensor
Weather myHumidity;//Create an instance of the humidity sensor

SoftwareSerial wifiSerial(9, 10);      // RX, TX for ESP8266

//Hardware pin definitions
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
const byte STAT_BLUE = 7;
const byte STAT_GREEN = 8;

//Global Variables
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
long lastSecond; //The millis counter to see when a second rolls by

bool DEBUG = true;   //show more logs
int responseTime = 1000; //communication timeout

String wifiStatus = "";

String server = "HOST_NAME";
int port = 3000;

void setup() {
  Serial.begin(9600);
  wifiSerial.begin(9600);

  pinMode(STAT_BLUE, OUTPUT); //Status LED Blue
  pinMode(STAT_GREEN, OUTPUT); //Status LED Green

  sendToWifi("AT+RST",responseTime,DEBUG);
  sendToWifi("AT+CWMODE=1",responseTime,DEBUG); // configure as access point
  sendToWifi("AT+CIFSR",responseTime,DEBUG); // get ip address

  while (wifiStatus.indexOf("WIFI CONNECTED") < 0) {
    Serial.print("Attempting to connect to network: ");
    Serial.println(ssid);    
    String cmd = "AT+CWJAP=\"" +ssid+"\",\"" + password + "\"";
    wifiStatus = sendToWifi(cmd,10000,DEBUG);
  }

  Serial.println("You're connected to the network");
  digitalWrite(STAT_GREEN, HIGH); //Blink stat LED

  //Configure the pressure sensor
  myPressure.begin(); // Get sensor online
  myPressure.setModeBarometer(); // Measure pressure in Pascals from 20 to 110 kPa
  myPressure.setOversampleRate(7); // Set Oversample to the recommended 128
  myPressure.enableEventFlags(); // Enable all three pressure and temp event flags

  //Configure the humidity sensor
  myHumidity.begin();

  lastSecond = millis();

  Serial.println("Weather Shield online!");

  // sendDataToServer(13.1, 13.1);
  while (wifiSerial.available() > 0) {
    wifiSerial.read();
  }
}

void loop() {
  if (millis() - lastSecond >= 5000) {
    digitalWrite(STAT_BLUE, HIGH); //Blink stat LED

    lastSecond += 5000;

    //Check Humidity Sensor
    float humidity = myHumidity.getRH();

    if (humidity == 998) {
      Serial.println("I2C communication to sensors is not working. Check solder connections.");

      //Try re-initializing the I2C comm and the sensors
      myPressure.begin(); 
      myPressure.setModeBarometer();
      myPressure.setOversampleRate(7);
      myPressure.enableEventFlags();
      myHumidity.begin();
    } else {
      Serial.print("Humidity = ");
      Serial.print(humidity);
      Serial.print("%,");
      float temp_h = myHumidity.getTempF();
      Serial.print(" temp_h = ");
      Serial.print(temp_h, 2);
      Serial.print("F,");

      //Check Pressure Sensor
      float pressure = myPressure.readPressure();
      Serial.print(" Pressure = ");
      Serial.print(pressure);
      Serial.print("Pa,");

      //Check tempf from pressure sensor
      float tempf = myPressure.readTempF();
      Serial.print(" temp_p = ");
      Serial.print(tempf, 2);
      Serial.print("F,");

      Serial.println();
      sendDataToServer(temp_h, humidity);
    }

    digitalWrite(STAT_BLUE, LOW); //Turn off stat LED
  }

  delay(responseTime);
}

/*
* Name: sendToWifi
* Description: Function used to send data to ESP8266.
* Params: command - the data/command to send; timeout - the time to wait for a response; debug - print to Serial window?(true = yes, false = no)
* Returns: The response from the esp8266 (if there is a reponse)
*/
String sendToWifi(String command, const int timeout, boolean debug){
  String response = "";
  wifiSerial.println(command); // send the read character to the esp8266
  long int time = millis();
  while( (time+timeout) > millis())
  {
    while(wifiSerial.available())
    {
    // The esp has data so display its output to the serial window 
    char c = wifiSerial.read(); // read the next character.
    response+=c;
    }  
  }
  if(debug)
  {
    Serial.println(response);
  }
  return response;
}

void sendDataToServer(float temperature, float humidity) {
  wifiSerial.println("AT+CIPSTART=\"TCP\",\"" + server + "\",port");
  if( wifiSerial.find("OK")) {
    Serial.println("TCP connection ready");
  }
  delay(1000);
  String data = "{\"temperature\":" + String(temperature) + ", \"humidity\":" + String(humidity) + "}";
  String request =
  "POST /temperature HTTP/1.1\r\nHost: " + server + "\r\n" +
  "Accept: *" + "/" + "*\r\n" +
  "Content-Length: " + data.length() + "\r\n" +
  "Content-Type: application/json\r\n" +
  "\r\n" + data + "\r\n";
  String sendCmd = "AT+CIPSEND=";//determine the number of caracters to be sent.
  wifiSerial.print(sendCmd);
  wifiSerial.println(request.length() );
  delay(500);
  if(wifiSerial.find(">")) { Serial.println("Sending.."); wifiSerial.print(request); }
  if( wifiSerial.find("SEND OK")) { Serial.println("Packet sent"); }
  while (wifiSerial.available()) {
    String tmpResp = wifiSerial.readString();
    Serial.println(tmpResp);
  }
  wifiSerial.println("AT+CIPCLOSE");
}
