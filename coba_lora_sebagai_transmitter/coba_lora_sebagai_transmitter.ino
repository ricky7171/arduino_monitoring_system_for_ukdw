/*********************
  | Custom Library
*********************/
#include "monitoring_system.h"

/*********************
  | WIFI ESP32
*********************/
#include <WiFi.h>
const char ssid[] = "bryan-poenya";
const char pass[] = "lalelilolu";
WiFiClient net;

/*********************
  | MQTT CLIENT
*********************/
#include "MQTT.h"
#include <MQTTClient.h>
MQTTClient client;

/*********************
  | EEPROM
*********************/
String strSSID;// = "laptopnya ricky";
String strPASS;// = "abcdefgh";
String gatewayId;// = "1";
String gatewayName;// = "Pos Logos";
String strBrokerAdd;// = "broker.shiftr.io";
String usernameMQTT;// = "samuelricky-skripsi-coba";
String passwordMQTT;// = "welcome3";
String nameMQTT;// = "skripsi-coba";
char buffSSID[25]; //ssid wifi
char buffPASS[10]; //password wifi
char buffBrokerAdd[25];
char buffNameMQTT[25];
char buffUsernameMQTT[50];
char buffPasswordMQTT[150];

/*********************
  | ACKNOWLEDGE TEMPORARY
*********************/
int acknowledgeTemporary[100][3]; //max 100 node and consist 3 element : room_id, sent, time
unsigned long timeAcknowledgePost = 0;

/*********************
  | TESTING KARENA GAK ADA BRYAN
*********************/
bool loopSend = false;
int timeLoopSend = 0;

void connect()
{
  //connect to wifi //TESTING ONLY
  printTextLcd("[4/6]", 0, true);
  printTextLcd("Connect Wifi & Server", 20);
  Serial.println("[4/6] Connect to WiFi...");
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(1000);
  }
  while(checkConnectionToServer() == false)
  {
    Serial.print("..");
    delay(1000);
  }
  
  Serial.println("[4/6] Connected to WiFi !");

  //connect to MQTT //TESTING ONLY
  printTextLcd("[5/6]", 0, true);
  printTextLcd("Connect MQTT", 20);
  Serial.println("[5/6] Connect to MQTT...");
  nameMQTT.toCharArray(buffNameMQTT, nameMQTT.length() + 1);
  usernameMQTT.toCharArray(buffUsernameMQTT, usernameMQTT.length() + 1);
  passwordMQTT.toCharArray(buffPasswordMQTT, passwordMQTT.length() + 1);

  while (!client.connect(buffNameMQTT, buffUsernameMQTT, buffPasswordMQTT))
  {
    delay(1000);
  }

  Serial.println("[5/6] Connected to MQTT !");
  delay(2000);
  
  //subscribe to /config-gateway
  printTextLcd("[6/6]", 0, true);
  printTextLcd("Subscribing /config-gatseeway/" + gatewayId, 20);
  Serial.println("[6/6] Subscribing to topic /config-gateway/" + gatewayId);
  String dataTopic = "/config-gateway/";
  dataTopic = dataTopic + gatewayId;
  client.subscribe(dataTopic); //TESTING ONLY
  Serial.println("[6/6] Subscribed to topic /config-gateway !");


}

void messageReceived(String &topic, String &payload)
{
  Serial.println("incoming: " + topic + " - " + payload);
  if (topic == "/config-gateway/" + gatewayId) //if message receive is config-gateway
  {
    //unsubscribe all topic
    Serial.println("client.disconnect");
    client.disconnect();
    while (!client.connect(buffSSID, buffUsernameMQTT, buffPasswordMQTT))
    {
      Serial.print(".");
      delay(1000);
    }
    //sub to config-gateway again
    String dataTopic = "/config-gateway/";
    dataTopic = dataTopic + gatewayId;
    client.subscribe(dataTopic);

    String strListNode = payload;

    //convert from strListNode to intListNode
    String temp = "";
    int idxSearch = 0;

    //split strListNode with #
    //example : 123#123#193#832#293#232
    while (true)
    {
      temp =  getValue(strListNode, '#', idxSearch); //get number in string
      if (temp == "")
      {
        break;
      }
      else
      {
        idxSearch += 1;  //update idxsearch
        //subscribe to /qrcode/(temp)
        Serial.println("subscribe to /qrcode/" + temp);
        client.subscribe("/qrcode/" + temp);
      }
    }
    printGatewayText(gatewayId, gatewayName, "connected to node", strListNode);
  }
  else if (topic.startsWith("/qrcode/")) //if message receive is qrcode
  {
    //convert id node to adl and adh
    String tempIdNode = topic.substring(8);
    int idNode = tempIdNode.toInt();
    //example : 2, AdlNode : 0, AdhNode : 2
    //example : 300, AdlNode : 1, AdhNode : 44
    byte AdlNode = idNode / 256;
    byte AdhNode = idNode % 256;

    //forward to destination node
    sendMessage(AdlNode, AdhNode, recChannel, payload, true);
    if(timeAcknowledgePost == 0)
    {
      timeAcknowledgePost = micros();
    }
    for(int i = 0;i<100;i++)
    {
      if(acknowledgeTemporary[i][0] == -1)
      {
        acknowledgeTemporary[i][0] = idNode;
        break;
      }
    }
  }


}

void setup()
{
  setupGraphicLCD();
  // put your setup code here, to run once:
  Serial.begin(9600);
  Serial.setTimeout(10);
  Serial.println("start setup.... Please wait until 6 process is finish !");
  

  //====== testing section =======

  //==============================

  
  //setting pin M0 & M1 to OUTPUT
  Serial.println("[1/6] Set pin M0, M1, flag LED to output");
  pinMode(pinM0, OUTPUT);
  pinMode(pinM1, OUTPUT);
  pinMode(32, OUTPUT);
  digitalWrite(32, LOW);

  printTextLcd("[1/6]", 0, true);
  printTextLcd("Set pin...", 20);
  Serial.println("[1/6] Set pin M0, M1, flag LED is done !");

  
  //Connect to EEPROM from serial
  Serial.println("[2/6] Connect to EEPROM");
  connectEEPROM();
  Serial.println("[2/6] Connected to EEPROM ! Waiting serial data in 10 seconds");
  
  updateEEPROMFromSerial();


  //read data from eeprom
  
  //strSSID#strPASS#gatewayID#Broker_Add#usernameMQTT#passwordMQTT#gatewayNameMQTT>
  //DESKTOP-P4FL5H1_9931#patrol1234#1#broker.shiftr.io#904e4807#cfdc8ca761caadf9#QR_Display1>
  //iphone#abcdefgh#4#broker.shiftr.io#samuelricky-skripsi-coba#sukukata123#skripsi-coba>
  //laptopnya ricky#abcdefgh#4#broker.shiftr.io#samuelricky-skripsi-coba#sukukata123#skripsi-coba>
  
  printTextLcd("[2/6]", 0, true);
  printTextLcd("Read data from EEPROM", 20);
  Serial.println("[2/6] Read data from EEPROM");
  //EEPROM.writeString(0,"DESKTOP-P4FL5H1_9931#patrol1234#1#broker.shiftr.io#904e4807#cfdc8ca761caadf9#QR_Display1");
  //EPROM.commit();
  String readData = EEPROM.readString(0);
  Serial.println(readData);

  strSSID = getValue(readData, '#', 0); Serial.print("SSID     : "); Serial.println(strSSID);
  strPASS = getValue(readData, '#', 1); Serial.print("PASS SSID: "); Serial.println(strPASS);
  gatewayId = getValue(readData, '#', 2);  Serial.print("Gateway ID number: "); Serial.println(gatewayId);
  gatewayName = getValue(readData, '#', 3);  Serial.print("Gateway Name: "); Serial.println(gatewayName);
  strBrokerAdd = getValue(readData, '#', 4); Serial.print("Broker Address: "); Serial.println(strBrokerAdd);
  usernameMQTT = getValue(readData, '#', 5); Serial.print("username      : "); Serial.println(usernameMQTT);
  passwordMQTT = getValue(readData, '#', 6); Serial.print("password      : "); Serial.println(passwordMQTT);
  nameMQTT = getValue(readData, '#', 7); Serial.print("client name   : "); Serial.println(nameMQTT);

  Serial.println("[2/6] Finish read data from EEPROM ! Set Wifi username & pass and set broker mqtt client");

  strSSID.toCharArray(buffSSID, (strSSID.length() + 1));
  strPASS.toCharArray(buffPASS, (strPASS.length() + 1));

  WiFi.begin(buffSSID, buffPASS); //TESTING ONLY

  // Note: Local domain names (e.g. "Computer.local" on OSX) are not supported by Arduino.
  // You need to set the IP address directly.
  //client.begin("192.168.43.182", net); //mosquitto broker
  strBrokerAdd.toCharArray(buffBrokerAdd, (strBrokerAdd.length() + 1));
  //TESTING ONLY
  client.begin(buffBrokerAdd, net);
  client.onMessage(messageReceived);

  
  //setup lora
  printTextLcd("[3/6]", 0, true);
  printTextLcd("Setup LoRa & QRCode (10 sec)", 20);
  Serial.println("[3/6] Setup LoRa (wait 10 sec)");
  loraSerial.begin(9600, SERIAL_8N1, pinRX, pinTX);
  loraSerial.setTimeout(200);
  
  
  
  clearLoraSerial();
  clearSerial();
  
  //get adl and adh
  int intGatewayId = gatewayId.toInt();
  myAdl = intGatewayId / 256;
  myAdh = intGatewayId % 256;

  //set sleep mode in LoRa & settting parameter lora
  setupParameterLoRa();

  //clear acknowledgeTemporary with -1
  for(int i = 0;i<100;i++)
  {
    acknowledgeTemporary[i][0] = -1;
    acknowledgeTemporary[i][1] = -1;
    acknowledgeTemporary[i][2] = -1;
  }
  Serial.println("This LoRa address : " + String(myAdl) + " " + String(myAdh));
  //Serial.println("[3/6] Setup LoRa & Setup QRCode has finished !");

  delay(5000);

  connect();
  digitalWrite(32, HIGH);
  printGatewayText(gatewayId, gatewayName);
}

void loop()
{
  client.loop();
  
  //FOR TESTING
  if(loopSend)
  {
    if(millis() - timeLoopSend > 3000) //if 3 second, then send data from gateway to node 
    {
      timeLoopSend = millis();
      sendMessage(0, 2, recChannel, "s2m8QRwElljEQYCujPawn3Vf0UbPfZYTJICody13/00=", true);
    } 
  }
//  else
//  {
//    loopSend = true;
//    timeLoopSend = millis();
//  }
  
  if(micros() - timeAcknowledgePost > 5000000 && timeAcknowledgePost != 0) //if 5 second, then send to server
  {
    Serial.println("cek timeacknowledgePost");
     for(int i = 0;i<100;i++)
     {
      if(acknowledgeTemporary[i][0] != -1)
      {
        String roomidData = String(acknowledgeTemporary[i][0]);
        String sentData = String(acknowledgeTemporary[i][1]);
        String timeData = String(acknowledgeTemporary[i][2]);
        if(sentData == "-1") //if not any acknowledge from node (failed send)
        {
          logAcknowledge(roomidData,"0","0");  
        }
        else
        {
          logAcknowledge(roomidData,sentData,timeData); //TESTING ONLY
        }
        acknowledgeTemporary[i][0] = -1;
        acknowledgeTemporary[i][1] = -1;
        acknowledgeTemporary[i][2] = -1;
        
      }
      timeAcknowledgePost = 0;
     }
  }
//  if (!client.connected()) TESTING ONLY
//  {
//    connect();
//  }

  if (loraSerial.available())
  {
    Serial.println("ada incoming message");

    Serial.println("difference time : ");
    int differenceTimeMillis = millis() - timePeriodTestMillis;
    int differenceTimeMicros = micros() - timePeriodTestMicros;
    Serial.println(differenceTimeMillis);

        
    String incomingString = "";

    //read incoming string
    incomingString = loraSerial.readString();

    int sizeIncoming = incomingString.length();

    bool fromDevice = checkMessageFromLoRaOrNot(incomingString);

    if (fromDevice == true)
    {
      //get sender from string message (3 byte)
      //format message : (WITH ACK OR NOT (1 / 0)) 11 11 (myADDRESS LOW) (myADDRESS HIGH) (myCHANNEL) (MESSAGE)
      byte addressLowSender = incomingString.charAt(3);
      byte addressHighSender = incomingString.charAt(4);
      byte channelSender = incomingString.charAt(5);
      String message = incomingString.substring(6);
      Serial.println(message);
      int idNode = (addressLowSender * 256) + addressHighSender;
      for(int i = 0;i<100;i++)
      {
        if(acknowledgeTemporary[i][0] == idNode)
        {
          acknowledgeTemporary[i][1] = 1;
          acknowledgeTemporary[i][2] = differenceTimeMicros;
          break;
        }
      }
      
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
    else if (input.startsWith("loopsend"))
    {
      loopSend = true;
      timeLoopSend = millis();
    }
    else if (input.startsWith("kirim pesan:")) //EVALUATION PURPOSE !!!
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
      Serial.println("Mode normal !");
      digitalWrite(pinM0, LOW); //M0
      digitalWrite(pinM1, LOW); //M1
    }
    else if (input.startsWith("mode sleep"))
    {
      Serial.println("Mode sleep !");
      digitalWrite(pinM0, HIGH); //M0
      digitalWrite(pinM1, HIGH); //M1
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
  timePeriodTestMillis = millis();
  timePeriodTestMicros = micros();
  loraSerial.write(cmd, sizeof(cmd));
}
