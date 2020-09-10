//=========================================== SETUP VARIABLE =================================================
/*********************
  | Hardware
*********************/
#include "Arduino.h"
#include <SoftwareSerial.h>
#include <HardwareSerial.h>
#include <HTTPClient.h>

/*********************
  | SERVER
*********************/
String serverName = "http://192.168.43.140:8000";


/*********************
  | SCREEN TFT ILI9225 176*220
*********************/
#include "SPI.h"
#include "TFT_22_ILI9225.h"
#include <../fonts/FreeSans9pt7b.h>
#include <../fonts/FreeSans12pt7b.h>
#define TFT_RST 15    // RST P17
#define TFT_RS  2    // RS P16
#define TFT_CLK 18    // CLK P18
#define TFT_SDI 23    // SDA P23
#define TFT_CS  5     // CS P5
#define TFT_LED 0     // 0 if wired to +5V directly
#define TFT_BRIGHTNESS 200 // Initial brightness of TFT backlight (optional)
// Use hardware SPI (faster - on Uno: 13-SCK, 12-MISO, 11-MOSI)
TFT_22_ILI9225 TFTscreen = TFT_22_ILI9225(TFT_RST, TFT_RS, TFT_CS, TFT_LED, TFT_BRIGHTNESS);
int16_t x=0, y=0, width, height; //position;
String strTextDisplay;


/*********************
  | QR CODE
*********************/
#include <qrcode.h>
QRCode qrcode;
uint8_t vers = 3;
uint8_t pixel = 5;
uint8_t offset_x = 15;
uint8_t offset_y = 5;
uint8_t borderWidth = 5;
char dataCharArray[100];
String strQrCode="";

/*********************
  | EEPROM
*********************/
#include "EEPROM.h"
#define EEPROM_SIZE 128


/*********************
  | LORA
*********************/
HardwareSerial loraSerial(1);

//setting pin
int pinM0 = 0;
int pinM1 = 4;
int pinRX = 16;
int pinTX = 17;

//my address
byte myAdl;// = 0x01;     //decimal : 0
byte myAdh;// = 0x01;     //decimal : 0





/*********************
  | EVALUATION
*********************/
unsigned long timePeriodTestMillis;
unsigned long timePeriodTestMicros;

// INFO
// Default Parameter : {0xC2, 0x0, 0x0, 0x1A, 0x17, 0x44} => {194, 0, 0, 26, 23, 68} => {Head, Adh, Adl, Sped, Channel, Option}
// Fixed transmission : 194 0 0 26 23 196

//============================================================================================================

//=========================================== FUNCTION =======================================================

void setupGraphicLCD(){
  TFTscreen.begin();
  TFTscreen.setOrientation(0);
  TFTscreen.setBacklightBrightness(128);
}

void printTextLcd(String text, int heightFromTop, bool needClear = false)
{
  if(needClear)
    TFTscreen.clear();
    
  TFTscreen.setFont(Terminal6x8);
  TFTscreen.drawText(10, heightFromTop, text);
}

void printGatewayText(String id, String name, String additionalText1 = "", String additionalText2 = ""){
  TFTscreen.clear();
  TFTscreen.setGFXFont(&FreeSans9pt7b);
  String gatewayId = "G - " + id; // Create string object
  TFTscreen.getGFXTextExtent(gatewayId, x, y, &width, &height); // Get string extents
  x = 10; y = height;
  TFTscreen.drawGFXText(x, y, gatewayId, COLOR_GREEN); // Print string
  TFTscreen.drawGFXText(x, y + 20, name, COLOR_GREEN); // Print string
  TFTscreen.setFont(Terminal6x8);
  TFTscreen.drawText(10, 50, additionalText1);
  TFTscreen.drawText(10, 70, additionalText2);
}

void printNodeText(String id, String name, String additionalText1 = "", String additionalText2 = ""){
  TFTscreen.clear();
  TFTscreen.setGFXFont(&FreeSans9pt7b);
  String gatewayId = "N - " + id; // Create string object
  TFTscreen.getGFXTextExtent(gatewayId, x, y, &width, &height); // Get string extents
  x = 10; y = height;
  TFTscreen.drawGFXText(x, y, gatewayId, COLOR_GREEN); // Print string
  TFTscreen.drawGFXText(x, y + 20, name, COLOR_GREEN); // Print string
  TFTscreen.setFont(Terminal6x8);
  TFTscreen.drawText(10, 50, additionalText1);
  TFTscreen.drawText(10, 70, additionalText2);
}


void printQR(String strData, String nodeName)
{ 
  TFTscreen.clear();

  if(strData!=""){
      /**********************
      | QR Code             |
    **********************/
    strData.toCharArray(dataCharArray, sizeof(dataCharArray));
    // Print to TFT
    uint8_t qrcodeData[qrcode_getBufferSize(3)];
    qrcode_initText(&qrcode, qrcodeData, vers, 0, dataCharArray); // your text in last parameter, e.g. "Hello World"

    //make border
    TFTscreen.fillRectangle((offset_x - borderWidth), (offset_y - borderWidth), ((qrcode.size * pixel) + offset_x + borderWidth), offset_y, COLOR_WHITE); //square1 horizontal atas
    TFTscreen.fillRectangle((offset_x - borderWidth), offset_y, offset_x, ((qrcode.size * pixel) + offset_y), COLOR_WHITE); //square2 vertical kiri
    TFTscreen.fillRectangle((offset_x - borderWidth), ((qrcode.size * pixel) + offset_y), ((qrcode.size * pixel) + offset_x + borderWidth), ((qrcode.size * pixel) + offset_y + borderWidth), COLOR_WHITE); //square3 horizontal bawah
    TFTscreen.fillRectangle(((qrcode.size * pixel) + offset_x), offset_y, ((qrcode.size * pixel) + offset_x + borderWidth), ((qrcode.size * pixel) + offset_y), COLOR_WHITE); //square4

    for (uint8_t y = 0; y < qrcode.size; y++) { //vertical
      for (uint8_t x = 0; x < qrcode.size; x++) { //horizontal
        if (!qrcode_getModule(&qrcode, x, y)) {
          uint16_t x1 = (x * pixel) + offset_x;
          uint16_t y1 = (y * pixel) + offset_y;
          uint16_t x2 = (x1 + pixel - 1);
          uint16_t y2 = (y1 + pixel - 1);
          TFTscreen.fillRectangle(x1, y1, x2, y2, COLOR_WHITE);
        }
      }
    }
  }

  strTextDisplay = nodeName; // Create string object
  TFTscreen.setGFXFont(&FreeSans9pt7b);
  TFTscreen.getGFXTextExtent(strTextDisplay, x, y, &width, &height); // Get string extents
  x = 10;
  y = 160 + height; // Set y position to string height plus shift down 10 pixels
  TFTscreen.drawGFXText(x, y, strTextDisplay, COLOR_WHITE); // Print string

  strTextDisplay = "UKDW"; // Create string object
  TFTscreen.getGFXTextExtent(strTextDisplay, x, y, &width, &height); // Get string extents
  x = 10;
  y += height + 5; // Set y position to string height plus shift down 10 pixels
  TFTscreen.drawGFXText(x, y, strTextDisplay, COLOR_ORANGE); // Print string

  strTextDisplay = "Patrolee System"; // Create string object
  TFTscreen.setGFXFont(&FreeSans9pt7b);
  TFTscreen.getGFXTextExtent(strTextDisplay, x, y, &width, &height); // Get string extents
  x = 10;
  y += height + 5; // Set y position to string height plus shift down 10 pixels
  TFTscreen.drawGFXText(x, y, strTextDisplay, COLOR_CYAN); // Print string
    
  
}


void connectEEPROM()
{
  

  if (!EEPROM.begin(200)) {
      Serial.println("Failed to initialise EEPROM");
      Serial.println("Restarting...");
      delay(1000);
      ESP.restart();
  }

  
}

void updateEEPROMFromSerial()
{
    byte counter = 0;
    while (!Serial.available())
    {
        Serial.print('.');
        delay(1000);
        counter++;
        if (counter >= 10) break;
    }

    Serial.println("");
    String outMessage = "";
    while (Serial.available() > 0) {
        char inChar = Serial.read();
        if (inChar == '>') {
        Serial.println("Recieve serial data");
        Serial.println(outMessage);
        Serial.println("Write to EEPROM");
        EEPROM.writeString(0, outMessage);
        EEPROM.commit();
        Serial.println("Writing data success");
        break;
        }
        else
        outMessage.concat(inChar);
    }
}

void clearSerial()
{
  byte w = 0;

  for (int i = 0; i < 10; i++)
  {
    while (Serial.available() > 0)
    {
      char k = Serial.read();
      w++;
      delay(1);
    }
    delay(1);
  }
}

void clearLoraSerial()
{
  byte w = 0;

  for (int i = 0; i < 10; i++)
  {
    while (loraSerial.available() > 0)
    {
      char k = loraSerial.read();
      w++;
      delay(1);
    }
    delay(1);
  }
}


void setupPin(){
  pinMode(pinM0, OUTPUT);
  pinMode(pinM1, OUTPUT);
  pinMode(32, OUTPUT);
  digitalWrite(32, LOW);
}

/*
Setup Parameter LoRa
*/
void setupParameterLoRa()
{
    digitalWrite(pinM0, HIGH); //M0
    digitalWrite(pinM1, HIGH); //M1
    delay(5000);
    byte paramByte[6] = {194, myAdh, myAdl, 26, 23, 196}; //{HEAD,ADDH,ADDL,SPED,CHAN,OPTION}
    loraSerial.write(paramByte, sizeof(paramByte));
    //set normal mode in LoRa
    delay(5000);
    digitalWrite(pinM0, LOW); //M0
    digitalWrite(pinM1, LOW); //M1
}

/*
Setup LoRa
*/
void setupLoRa(int id){
  //setup lora
  loraSerial.begin(9600, SERIAL_8N1, pinRX, pinTX);
  loraSerial.setTimeout(200);
  
  
  
  clearLoraSerial();
  clearSerial();
  
  //get adl and adh
  myAdh = id / 256;
  myAdl = id % 256;

  //set sleep mode in LoRa & settting parameter lora
  setupParameterLoRa();

  
}



bool checkMessageFromLoRaOrNot(String incomingString)
{
    int sizeIncoming = incomingString.length();
  
    bool fromDevice = false;
    int sizeMessage = incomingString.length();
    if (sizeMessage > 5) //check if incoming byte is message from other LoRa
    {
        if (incomingString.charAt(1) == 11 && incomingString.charAt(2) == 11) //if (11 & 11)
        {
            fromDevice = true;
        }
    }
    return fromDevice;
}

void readParamFromSerialAndSave(String input)
{
    byte paramByte[6];
    int idxParamByte = 0;

    //read param
    String param = input.substring(10);

    String temp = "";
    int idxSearch = 0;

    //split param with space
    //123 123 193 832 293 232
    Serial.println("result : ");
    while (true)
    {
        temp = param.substring(idxSearch, param.indexOf(" ", idxSearch)); //127
        idxSearch = param.indexOf(" ", idxSearch) + 1;
        int intTemp = temp.toInt();
        //byte byteTemp = intTemp;
        paramByte[idxParamByte] = intTemp;
        //Serial.println(paramByte[idxParamByte ]);
        idxParamByte += 1;
        if (idxParamByte == 5)
        {
            break;
        }
    }
    temp = param.substring(param.lastIndexOf(" "));
    int intTempLast = temp.toInt();
    paramByte[idxParamByte] = intTempLast;

    Serial.println("cek param yang akan disave");
    Serial.println(sizeof(paramByte));
    Serial.println("ini isinya :");
    for (int i = 0; i < 6; i++)
    {
        Serial.print("i : ");
        Serial.println(i);

        Serial.println(paramByte[i]);
    }
    Serial.println("=== done ===");
    //set param lora with paramByte
    loraSerial.write(paramByte, sizeof(paramByte));
}

void setMode(String mode)
{
  if (mode == "normal")
  {
    //set normal mode in LoRa
    Serial.println("Mode normal !");
    digitalWrite(pinM0, LOW); //M0
    digitalWrite(pinM1, LOW); //M1
  }
  else if (mode == "sleep")
  {
    //set sleep mode in LoRa
    Serial.println("Mode sleep !");
    digitalWrite(pinM0, HIGH); //M0
    digitalWrite(pinM1, HIGH); //M1
  }
}

bool checkConnectionToServer()
{
  return true;
  // Serial.println("checking connection to server " + serverName);
  // HTTPClient http;
  // String serverPath = serverName + "/api/acknowledges/testConnection";
  // http.begin(serverPath);
  // int httpResponseCode = http.GET();
  // String payload = http.getString();
  // Serial.println(httpResponseCode);
  // if(httpResponseCode == 200)
  // {
  //   return true;
  // }
  // else{
  //   return false;
  // }

}
void logAcknowledge(String room_id, String sent, String time)
{
  Serial.println("lockAcknowledge");
  Serial.println("room_id : " + room_id + " sent : " + sent + " time : " + time);
  HTTPClient http;

  String serverPath = serverName + "/api/acknowledges";

  http.begin(serverPath.c_str());
  http.addHeader("Content-Type", "application/json");

  //String httpRequestData = "room_id=" + room_id + "&sent=" + sent + "&time" + time;
  String httpRequestData = "{\"room_id\":\"" + room_id +"\",\"sent\":\"" + sent + "\",\"time\":\"" + time + "\"}";
  
  // Send HTTP GET request
  int httpResponseCode = http.POST(httpRequestData);
  String payload = http.getString();
  Serial.println(payload);
  if (httpResponseCode>0) {
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
  }
  else {
    Serial.print("Cannot send report to server. Response code :");
    Serial.println(httpResponseCode);
  }
  // Free resources
//  http.end();
}


String getValue(String data, char separator, int index)
{
  int found = 0;
  int strIndex[] = {0, -1};
  int maxIndex = data.length() - 1;

  for (int i = 0; i <= maxIndex && found <= index; i++) {
    if (data.charAt(i) == separator || i == maxIndex) {
      found++;
      strIndex[0] = strIndex[1] + 1;
      strIndex[1] = (i == maxIndex) ? i + 1 : i;
    }
  }

  return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}


//============================================================================================================
