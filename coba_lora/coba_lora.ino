/*********************
  | Custom Library
*********************/
#include "monitoring_system.h"

/***************************************************************
  | DATA QRCODE
***************************************************************/
String currentDataQRCode = "";
unsigned long timerHandlingNoData;

String nodeID;
String nodeName;

void setup()
{

  setupGraphicLCD();
  
  //setting serial
  Serial.begin(9600);
  Serial.setTimeout(0);
  Serial.println("start setup.... Please wait until 4 process is finish !");

  //====== testing section =======

  //==============================


  //set pin M0 & M1 to OUTPUT 
  printTextLcd("[1/4]", 0, true);
  printTextLcd("Set pin...", 20);
  Serial.println("[1/4] Set pin M0, M1, flag LED to output");
  pinMode(pinM0, OUTPUT);
  pinMode(pinM1, OUTPUT);
  pinMode(32, OUTPUT);
  digitalWrite(32, LOW);
  Serial.println("[1/4] Set pin M0, M1, flag LED is done !");

  //Connect to EEPROM from serial
  printTextLcd("[2/4]", 0, true);
  printTextLcd("Connect EEPROM", 20);
  Serial.println("[2/4] Connect to EEPROM");
  connectEEPROM();
  Serial.println("[2/4] Connected to EEPROM ! Waiting serial data in 10 seconds");

  updateEEPROMFromSerial();


  //nodeID#nodeName>
  //1#Ruang D.1.2>

  Serial.println("[2/4] Read data from EEPROM");
  String readData = EEPROM.readString(0);
  Serial.println(readData);

  nodeID = getValue(readData, '#', 0); Serial.print("NodeID     : "); Serial.println(nodeID);
  nodeName = getValue(readData, '#', 1); Serial.print("nodeName: "); Serial.println(nodeName);

  Serial.println("[2/4] Finish read data from EEPROM !");

  


  //setup lora
  printTextLcd("[3/4]", 0, true);
  printTextLcd("Setup Lora (10 sec)", 20);
  Serial.println("[3/4] Setup LoRa... (wait 10 sec)");
  loraSerial.begin(9600, SERIAL_8N1, pinRX, pinTX);
  
  loraSerial.setTimeout(10);
  clearLoraSerial();
  clearSerial();
  //loraSerial.flush();
  //Serial.flush();
  

  //get adl and adh
  int intNodeId = nodeID.toInt();
  myAdl = intNodeId / 256;
  myAdh = intNodeId % 256;

  //set sleep mode in LoRa & setting parameter lora
  setupParameterLoRa();
  Serial.println("This LoRa address : " + String(myAdl) + " " + String(myAdh));
  Serial.println("[3/4] Setup LoRa has finished !");

  //setup qrcode
  printTextLcd("[4/4]", 0, true);
  printTextLcd("Setup QRCode", 20);
  Serial.println("[4/4] Setup QRCode...");
  printNodeText(nodeID, nodeName);
  digitalWrite(32, HIGH);
  Serial.println("[4/4] Setup QRCode is finished !");

  
  
  
}

void loop()
{
  //if, for 1.1 minutes no data is received, then the qrcode is replaced by "No Shift Found"
  if(millis() - timerHandlingNoData >= 90000)
  {
    printNodeText(nodeID, nodeName, "no shift found");
    timerHandlingNoData = millis();
  }

  if (loraSerial.available())
  {
    
    Serial.println("ada incoming message");

    
    String incomingString = "";
    //read incoming string
    incomingString = loraSerial.readString();
    
    //evaluation purpase

      
  
    bool fromDevice = checkMessageFromLoRaOrNot(incomingString);

    if (fromDevice == true)
    {
      //get sender from string message (3 byte)
      //format message : (WITH ACK OR NOT (1 / 0)) 11 11 (myADDRESS LOW) (myADDRESS HIGH) (myCHANNEL) (MESSAGE)
      
      byte addressLowSender = incomingString.charAt(3);
      byte addressHighSender = incomingString.charAt(4);
      byte channelSender = incomingString.charAt(5);
      String message = incomingString.substring(6);
      Serial.println("message : " + message);

      //send ack to sender
      if (incomingString.charAt(0) == 1) //if must send ack to sender
      {
        sendMessage(addressLowSender, addressHighSender, channelSender, "Acknowledge", false);
      }
      currentDataQRCode = incomingString.substring(6);
      timerHandlingNoData = millis();
      printQR(currentDataQRCode, nodeName);
      
      
    }
    else
    {
      Serial.println("undefined type message");
      Serial.println("dalam string : ");
      Serial.println(incomingString);
      Serial.println("dalam hexa : ");
      for (int i = 0; i < incomingString.length(); i++)
      {
        Serial.print(incomingString[i], HEX);
        Serial.println(" ");
      }
    }
    
    

    delay(10);
  }

  if (Serial.available())
  {
    String input = Serial.readString();
    if (input.startsWith("param"))
    {
      Serial.println("lihat param : ");
      byte cmd[] = {0xC1, 0xC1, 0xC1};
      loraSerial.write(cmd, sizeof(cmd));
    }
    else if (input.startsWith("set param:"))
    {
      readParamFromSerialAndSave(input);
    }
    else if (input.startsWith("kirim pesan:"))
    {
      //get low address input
      String stringMessage = input.substring(13);
      String addressReceiver = input.substring(12,13);
      Serial.println("kirim pesan ke adh: " + addressReceiver);
      Serial.println("dengan pesan : " + stringMessage);
      sendMessage(0, addressReceiver.toInt(), recChannel, stringMessage, true);
    }
    else if (input.startsWith("mode normal"))
    {
      setMode("normal");
    }
    else if (input.startsWith("mode sleep"))
    {
      setMode("sleep");
    }
  }

  
}

/*
   ==== FUNCTIONS ====
*/



/*
   Message to other device
*/
void sendMessage(byte adl, byte adh, byte channel, String msg, bool withAck)
{
  //format message : (WITH ACK OR NOT (1 / 0)) 11 11 (myADDRESS LOW) (myADDRESS HIGH) (myCHANNEL) (MESSAGE)
  //11 11 is a sign that it is a message from another device
  Serial.println("siap-siap kirim pesan !");
  byte cmd[100] = {adl, adh, channel, withAck, 11, 11, myAdl, myAdh, myChannel};
  for (int i = 0; i < msg.length(); i++)
  {
    cmd[i + 9] = msg.charAt(i);
  }
//  for (int i = 0; i < msg.length() + 9; i++)
//  {
//    Serial.println(cmd[i]);
//  }
  
  loraSerial.write(cmd, sizeof(cmd));
  
}
