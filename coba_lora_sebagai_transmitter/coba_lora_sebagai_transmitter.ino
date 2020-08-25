/*********************
  | Custom Library
*********************/
#include "monitoring_system.h"

/*********************
  | WIFI ESP32
*********************/
#include <WiFi.h>
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
String strSSID;
String strPASS;
String gatewayId;
String myChannel;
String gatewayName;
String strBrokerAdd;
String usernameMQTT;
String passwordMQTT;
String nameMQTT;
char buffSSID[25]; 
char buffPASS[10]; 
char buffBrokerAdd[25];
char buffNameMQTT[25];
char buffUsernameMQTT[50];
char buffPasswordMQTT[150];

/*********************
  | ACKNOWLEDGE TEMPORARY
*********************/
int acknowledgeTemporary[100][3]; //max 100 node and consist 3 element : room_id, sent, time
unsigned long timeAcknowledgePost = 0;



void connect()
{
  connectWifi();
  connectMqtt();
  subscribeConfigGateway();
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

    //start tracking acknowledge from node
//    if(timeAcknowledgePost == 0)
//    {
//      timeAcknowledgePost = millis();
//    }
  }
  else if (topic.startsWith("/qrcode/")) //if message receive is qrcode
  {
    //convert id node to adl and adh
    String tempIdNode = topic.substring(8);
    int idNode = tempIdNode.toInt();
    //example : 2, AdhNode : 0, AdlNode : 2
    //example : 300, AdhNode : 1, AdlNode : 44
    byte AdhNode = idNode / 256;
    byte AdlNode = idNode % 256;

    //forward to destination node
    sendMessage(AdhNode, AdlNode, myChannel.toInt(), payload, true);
    
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

  //setup pin
  Serial.println("[1/6] Set pin M0, M1, flag LED to output");
  printTextLcd("[1/6]", 0, true);
  printTextLcd("Set pin...", 20);
  setupPin();
  Serial.println("[1/6] Set pin M0, M1, flag LED is done !");

  //connect EEPROM
  printTextLcd("[2/6]", 0, true);
  printTextLcd("Connect to EEPROM", 20);
  Serial.println("[2/6] Connect to EEPROM");
  connectEEPROM();
  Serial.println("[2/6] Connected to EEPROM ! Waiting serial data in 10 seconds");

  updateEEPROMFromSerial();


  //read data from eeprom

  //example EEPROM : 
  //strSSID#strPASS#gatewayID#myChannel#gatewayName#Broker_Add#usernameMQTT#passwordMQTT#gatewayNameMQTT>
  //ricky#abcdefgh#3#23#Didaktos#broker.shiftr.io#samuelricky-skripsi-coba#sukukata123#skripsi-coba>

  updateSettingFromEEPROM();

  printTextLcd("[3/6]", 0, true);
  printTextLcd("Setup LoRa & QRCode (10 sec)", 20);
  Serial.println("[3/6] Setup LoRa (wait 10 sec)");
  setupLoRa(gatewayId.toInt());
  
  clearAcknowledge();

  connect();
  digitalWrite(32, HIGH);
  printGatewayText(gatewayId, gatewayName);
}

void loop()
{
  client.loop();
  
//  if(millis() - timeAcknowledgePost > 5000 && timeAcknowledgePost != 0) //if 5 second, then send all acknowledge to server
//  {
//    sendAllAcknowledgeToServer();
//  }

  if (loraSerial.available())
  {
    Serial.println("incoming message !!");

//    Serial.println("difference time : ");
//    int differenceTimeMillis = millis() - timePeriodTestMillis;
//    int differenceTimeMicros = micros() - timePeriodTestMicros;
//    Serial.println(differenceTimeMillis);

        
    String incomingString = "";

    //read incoming string
    incomingString = loraSerial.readString();

    int sizeIncoming = incomingString.length();

    bool fromDevice = checkMessageFromLoRaOrNot(incomingString);

    if (fromDevice == true)
    {
      //get sender from string message (3 byte)
      //format message : (WITH ACK OR NOT (1 / 0)) 11 11 (myADDRESS HIGH) (myADDRESS LOW) (myCHANNEL) (MESSAGE)
      byte addressHighSender = incomingString.charAt(3);
      byte addressLowSender = incomingString.charAt(4);
      byte channelSender = incomingString.charAt(5);
      String message = incomingString.substring(6);
      Serial.println(message);
      int idNode = (addressHighSender * 256) + addressLowSender;
//      for(int i = 0;i<100;i++)
//      {
//        if(acknowledgeTemporary[i][0] == idNode)
//        {
//          acknowledgeTemporary[i][1] = 1;
//          acknowledgeTemporary[i][2] = differenceTimeMicros;
//          break;
//        }
//      }
      
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
      String stringMessage = input.substring(14);
      String addressHighReceiver = input.substring(12,13);
      String addressLowReceiver = input.substring(13,14);
      Serial.println("kirim pesan ke adh: " + addressHighReceiver);
      Serial.println("kirim pesan ke adl: " + addressLowReceiver);
      Serial.println("dengan pesan : " + stringMessage);
      sendMessage(addressHighReceiver.toInt(), addressLowReceiver.toInt(), myChannel.toInt(), stringMessage, true);
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
   ================ FUNCTIONS ================
*/

/*
   Message to other device
*/
void sendMessage(byte adh, byte adl, byte channel, String msg, bool withAck)
{
  //format message : (WITH ACK OR NOT (1 / 0)) 11 11 (myADDRESS HIGH) (myADDRESS LOW) (myCHANNEL) (MESSAGE)
  //11 11 is a sign that it is a message from another device
  Serial.println("siap-siap kirim pesan !");

  byte cmd[100] = {adh, adl, channel, withAck, 11, 11, myAdh, myAdl, (myChannel.toInt())};
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




/*
  Update setting from EEPROM
*/
void updateSettingFromEEPROM(){
  Serial.println("[2/6] Read data from EEPROM");
  printTextLcd("[2/6]", 0, true);
  printTextLcd("Read data from EEPROM", 20);
  String readData = EEPROM.readString(0);
  Serial.println(readData);

  strSSID = getValue(readData, '#', 0); Serial.print("SSID     : "); Serial.println(strSSID);
  strPASS = getValue(readData, '#', 1); Serial.print("PASS SSID: "); Serial.println(strPASS);
  gatewayId = getValue(readData, '#', 2);  Serial.print("Gateway ID number: "); Serial.println(gatewayId);
  myChannel = getValue(readData, '#', 3);  Serial.print("My Channel: "); Serial.println(myChannel);
  gatewayName = getValue(readData, '#', 4);  Serial.print("Gateway Name: "); Serial.println(gatewayName);
  strBrokerAdd = getValue(readData, '#', 5); Serial.print("Broker Address: "); Serial.println(strBrokerAdd);
  usernameMQTT = getValue(readData, '#', 6); Serial.print("username      : "); Serial.println(usernameMQTT);
  passwordMQTT = getValue(readData, '#', 7); Serial.print("password      : "); Serial.println(passwordMQTT);
  nameMQTT = getValue(readData, '#', 8); Serial.print("client name   : "); Serial.println(nameMQTT);

  Serial.println("[2/6] Finish read data from EEPROM ! Set Wifi username & pass and set broker mqtt client");
  printTextLcd("[2/6]", 0, true);
  printTextLcd("Set wifi & mqtt account", 20);

  strSSID.toCharArray(buffSSID, (strSSID.length() + 1));
  strPASS.toCharArray(buffPASS, (strPASS.length() + 1));

  WiFi.begin(buffSSID, buffPASS); 

  strBrokerAdd.toCharArray(buffBrokerAdd, (strBrokerAdd.length() + 1));
  
  client.begin(buffBrokerAdd, net);
  client.onMessage(messageReceived);
}



void clearAcknowledge(){
  //clear acknowledgeTemporary with -1
  for(int i = 0;i<100;i++)
  {
    acknowledgeTemporary[i][0] = -1;
    acknowledgeTemporary[i][1] = -1;
    acknowledgeTemporary[i][2] = -1;
  }
  Serial.println("This LoRa address : " + String(myAdh) + " " + String(myAdl));

  delay(5000);
}


/*
  Connect to WiFi
*/
void connectWifi()
{
  printTextLcd("[4/6]", 0, true);
  printTextLcd("Connect Wifi & Server", 20);
  printTextLcd("SSID : " + strSSID, 40);
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
}

/*
  Connect to MQTT
*/
void connectMqtt()
{
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
}

/*
  Subscribe to topic /config-gateway
  to get some id of LoRa nodes that 
  connected to this gateway
*/
void subscribeConfigGateway()
{
  //subscribe to /config-gateway
  printTextLcd("[6/6]", 0, true);
  printTextLcd("Subscribing /config-gatseeway/" + gatewayId, 20);
  Serial.println("[6/6] Subscribing to topic /config-gateway/" + gatewayId);
  String dataTopic = "/config-gateway/";
  dataTopic = dataTopic + gatewayId;
  client.subscribe(dataTopic); //TESTING ONLY
  Serial.println("[6/6] Subscribed to topic /config-gateway !");
}

/*
  Send all acknowledge LoRa Node to server
  acknowledge consist of :
  - roomId or nodeId
  - fail or success of data transmission
  - response time of data transmission
*/
void sendAllAcknowledgeToServer(){
  timeAcknowledgePost = 0;
  for(int i = 0;i<100;i++)
  {
    if(acknowledgeTemporary[i][0] != -1)
    {
      String roomidData = String(acknowledgeTemporary[i][0]);
      String sentData = String(acknowledgeTemporary[i][1]);
      String timeData = String(acknowledgeTemporary[i][2]);
      if(sentData == "-1") //if not any acknowledge from node (failed send)
      {
        //logAcknowledge(roomidData,"0","0");  
      }
      else
      {
        //logAcknowledge(roomidData,sentData,timeData); 
      }
      acknowledgeTemporary[i][0] = -1;
      acknowledgeTemporary[i][1] = -1;
      acknowledgeTemporary[i][2] = -1;
      
    }
  }
}
