// Inspired by: http://www.instructables.com/id/ESP8266-NodeMCU-v10-ESP12-E-with-Arduino-IDE/?ALLSTEPS
// 
//nodeMCU v1.0 (black) with Arduino IDE
//stream temperature data DS18B20 with 1wire on ESP8266 ESP12-E (nodeMCU v1.0)
//shin-ajaran.blogspot.com
//nodemcu pinout https://github.com/esp8266/Arduino/issues/584
#include <ESP8266WiFi.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <WiFiUdp.h>
#include <Wire.h>
#include "config.h"

#define ONE_WIRE_BUS 2  // DS18B20 on arduino pin2 corresponds to D4 on physical board

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature DS18B20(&oneWire);
WiFiUDP udp;

DeviceAddress SunForward = {0x28, 0xFF, 0xEB, 0x43, 0x90, 0x15, 0x01, 0xEF};
DeviceAddress SunReturn = {0x28, 0xFF, 0xBD, 0x7D, 0x90, 0x15, 0x01, 0xBB};

DeviceAddress TankBottom = {0x28, 0xFF, 0xC9, 0x77, 0x90, 0x15, 0x04, 0xED};
DeviceAddress TankMiddle = {0x28, 0xFF, 0x8B, 0x2C, 0x90, 0x15, 0x01, 0xF4};
DeviceAddress TankTop = {0x28, 0xFF, 0xEB, 0x75, 0x90, 0x15, 0x04, 0x91};

DeviceAddress FurnaceForward = {0x28, 0xFF, 0xF1, 0x05, 0x90, 0x15, 0x01, 0xC7};
DeviceAddress FurnaceReturn = {0x28, 0xFF, 0x32, 0x7A, 0x90, 0x15, 0x01, 0x99};

DeviceAddress Room = {0x28, 0xFF, 0x6C, 0x50, 0x90, 0x15, 0x01, 0xC5};
DeviceAddress Arduino = {0x28, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0xA2};

// https://github.com/esp8266/Arduino/issues/584#issuecomment-123715951
const int furnaceActivePin = 13;  // D7
const int solarPumpActivePin = 12; // D6
const int solarTankPumpActivePin =  14; // D5
const int avail4ActivePin =  0; // D3
const int avail5ActivePin =  16; // D0 (not connected)

void setup() {
  Wire.begin();        // join i2c bus (address optional for master)
  Serial.begin(115200);
  connectWifi();

  discoverOneWireDevices();  
}

void discoverOneWireDevices(void) {
 byte i;
 byte present = 0;
 byte data[12];
 byte addr[8];

 Serial.print("Looking for 1-Wire devices...\n\r");// "\n\r" is NewLine 
 while(oneWire.search(addr)) {
   Serial.print("\n\r\n\rFound \'1-Wire\' device with address:\n\r");
   for( i = 0; i < 8; i++) {
     Serial.print("0x");
     if (addr[i] < 16) {
       Serial.print('0');
     }
     Serial.print(addr[i], HEX);
     if (i < 7) {
       Serial.print(", ");
     }
   }
   if ( OneWire::crc8( addr, 7) != addr[7]) {
     Serial.print("CRC is not valid!\n\r");
     return;
   }
 }
 Serial.println();
 Serial.print("Done");
 oneWire.reset_search();
 return;
}

float readSensor(DeviceAddress address) {
  return DS18B20.getTempC(address);
}


void loop() {
  DS18B20.requestTemperatures();
  Wire.requestFrom(8, 12);    // request 6 bytes from slave device #8
  String solarTemp = "";  
  while (Wire.available()) { // slave may send less than requested
    char c = Wire.read(); // receive a byte as character
    solarTemp = String(solarTemp + c);
  }

  int SolarS1 = solarTemp.substring(0,4).toInt();
  int SolarS2 = solarTemp.substring(4,8).toInt();
  int SolarS3 = solarTemp.substring(8,12).toInt();

  sendDataToInflux("Room", (String) readSensor(Room), false);
  
  // Analog reads from Arduino via Wire
  sendDataToInflux("SolarS1", (String) SolarS1, false);
  sendDataToInflux("SolarS2", (String) SolarS2, false);
  sendDataToInflux("SolarS3", (String) SolarS3, false);

  // Digital reads from 230v to 3.3V PSU
  sendDataToInflux("FurnaceActive", (String) digitalRead(furnaceActivePin), false);
  sendDataToInflux("SolarActive", (String) digitalRead(solarPumpActivePin), false);
  sendDataToInflux("SolarTankPumpActive", (String) digitalRead(solarTankPumpActivePin), false);
  sendDataToInflux("AvailInput4", (String) digitalRead(avail4ActivePin), false);
  sendDataToInflux("AvailInput5", (String) digitalRead(avail5ActivePin), false);

  // D18B20 temperature sensors
  sendDataToInflux("SunForward", (String) readSensor(SunForward), false);
  sendDataToInflux("SunReturn", (String) readSensor(SunReturn), false);
  sendDataToInflux("TankTop", (String) readSensor(TankTop), false);
  sendDataToInflux("TankMiddle", (String) readSensor(TankMiddle), false);
  sendDataToInflux("TankBottom", (String) readSensor(TankBottom), false);
  sendDataToInflux("FurnaceForward", (String) readSensor(FurnaceForward), false);
  sendDataToInflux("FurnaceReturn", (String) readSensor(FurnaceReturn), true);

  Serial.println("");

  delay(1000);
}

void connectWifi()
{
  Serial.print("Connecting to "+*MY_SSID);
  WiFi.begin(MY_SSID, MY_PWD);
  while (WiFi.status() != WL_CONNECTED) {
  delay(1000);
  Serial.print(".");
  }
  
  Serial.println("");
  Serial.println("Connected");
  Serial.println("");  
}

String preparedLine = "";

void sendDataToInflux(String name, String data, boolean send) {
  // Avoid sending data if data is 85 (initializing)
  if (data == "85.00") {
    return;
  }
  
  String line = name;
  line += " value=";
  line += data;
  Serial.println(line);

  preparedLine += line + "\n";

  if (send == true) {
    //Serial.println("Sending UDP packet...");
    udp.beginPacket(influxHost, influxPort);
    udp.print(preparedLine);
    udp.endPacket();
    udp.stop();
    preparedLine = "";
  }
}

