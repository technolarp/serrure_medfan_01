/*
----------------------------------------------------------------------------
TECHNOLARP - https://technolarp.github.io/
SERRURE MEDFAN 01 - https://github.com/technolarp/serrure_medfan_01
version 1.3.0 - 12/2025
----------------------------------------------------------------------------
*/

/*
----------------------------------------------------------------------------
Pour ce montage, vous avez besoin de 
4 ou + led ws2812b
4 ou + switch reed
1 MCP23017 en i2c
----------------------------------------------------------------------------
*/

/*
----------------------------------------------------------------------------
PINOUT
D4     WS2812B
D1     I2C SCL => SCL MCP23017
D2     I2C SDA => SDA MCP23017

MCP_A0 switch reed 1
MCP_A1 switch reed 2
MCP_A2 switch reed 3
MCP_A3 switch reed 4
----------------------------------------------------------------------------
*/

/*
TODO version 1.4.0

ajouter du son
ajouter une commande d'actionneur

re-verouiller apres X secondes
scintillement tournant
*/

#include <Arduino.h>

// WIFI
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>

AsyncWebServer server(80);

// WEBSOCKET
AsyncWebSocket ws("/ws");

char bufferWebsocket[300];
bool flagBufferWebsocket = false;

// MDNS
#include <ESP8266mDNS.h>

// CONFIG
#include "config.h"
M_config aConfig;

#define BUFFERSENDSIZE 1024
char bufferToSend[BUFFERSENDSIZE];

// FASTLED
#include <technolarp_fastled.h>
M_fastled aFastled;

// MCP23017
#include "technolarp_mcp23017.h"
M_mcp23017 aMcp23017;

// STATUTS DE L OBJET
enum {
  OBJET_OUVERT = 0,
  OBJET_FERME = 1,
  OBJET_OUVERTURE = 2,
  OBJET_BLINK = 5
};

// DIVERS
bool uneFois = true;

uint32_t previousMillisReset;
uint32_t previousMillisBrightness;

bool checkTimeoutFlag = false;

uint32_t lastDebounceTime[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
uint8_t lastPinState[16] = {HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH,HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH};
uint8_t pinState[16] = {HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH,HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH};
bool pinActive[16] = {true,true,true,true,true,true,true,true,true,true,true,true,true,true,true,true};

// HEARTBEAT
uint32_t previousMillisHB;
uint32_t intervalHB;

// REFRESH
uint32_t previousMillisRefresh;
uint32_t intervalRefresh;

// FUNCTION DECLARATIONS
void serrureFermee();
void serrureOuverte();
void serrureOuverture();
void serrureBlink();
void checkReed();
void checkTimeout();
void showSparklePixel(uint8_t led);
void checkCharacter(char *toCheck, const char *allowed, char replaceChar);
void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len); 
void handleWebSocketMessage(void *arg, uint8_t *data, size_t len); 
void handleWebsocketBuffer();
void notFound(AsyncWebServerRequest *request);
void convertStrToRGB(const char * source, uint8_t* r, uint8_t* g, uint8_t* b);
void sendMaxLed();
void sendUptime();
void sendStatut();
void sendObjectConfig();
void writeObjectConfig();
void sendNetworkConfig();
void writeNetworkConfig();
uint16_t checkValeur(uint16_t valeur, uint16_t minValeur, uint16_t maxValeur);
uint8_t indexMaxValeur(uint8_t arraySize, uint8_t arrayToSearch[]);

/*
----------------------------------------------------------------------------
SETUP
----------------------------------------------------------------------------
*/
void setup() 
{
  Serial.begin(115200);
  
  // VERSION
  delay(500);
  Serial.println(F(""));
  Serial.println(F(""));
  Serial.println(F("----------------------------------------------------------------------------"));
  Serial.println(F("TECHNOLARP - https://technolarp.github.io/"));
  Serial.println(F("SERRURE MEDFAN 01 - https://github.com/technolarp/serrure_mefan_01"));
  Serial.println(F("version 1.3.0 - 12/2025"));
  Serial.println(F("----------------------------------------------------------------------------"));
  
  // I2C RESET
  aConfig.i2cReset();
  
  // CONFIG OBJET
  Serial.println(F(""));
  Serial.println(F(""));
  aConfig.mountFS();
  aConfig.listDir("/");
  aConfig.listDir("/config");
  aConfig.listDir("/www");
  
  aConfig.printJsonFile("/config/objectconfig.json");
  aConfig.readObjectConfig("/config/objectconfig.json");
  
  aConfig.printJsonFile("/config/networkconfig.json");
  aConfig.readNetworkConfig("/config/networkconfig.json");
  
  aConfig.objectConfig.activeLeds = aConfig.objectConfig.nbSegments * aConfig.objectConfig.ledParSegment;
  aConfig.writeObjectConfig("/config/objectconfig.json");
  
  // MCP23017
  aMcp23017.beginMcp23017(0);
  
  // FASTLED
  aFastled.setNbLed(aConfig.objectConfig.activeLeds);
  // animation led de depart
  aFastled.animationDepart(50, aFastled.getNbLed() * 2, CRGB::Blue);
  
  // LOOP TO WIFI CLIENT
  Serial.println(F(""));
  Serial.println(F("connecting to wifi as client"));
  
  for (uint8_t i = 0; i < WIFI_CLIENTS; i++)
  {
    if (aConfig.networkConfig.active[i] && strlen(aConfig.networkConfig.ssid[i])>0)
    {
      if (WiFi.status() != WL_CONNECTED)
      {
        Serial.print(F("ssid: "));
        Serial.print(aConfig.networkConfig.ssid[i]);
        Serial.print(F(" - delay: "));
        Serial.print(aConfig.networkConfig.wifiConnectDelay);
        Serial.println(F(" seconds"));
        
        bool ledState=true;
        bool wifiFlag=true;
        
        WiFi.disconnect(true);
        delay(500);
        WiFi.begin(aConfig.networkConfig.ssid[i], aConfig.networkConfig.password[i]);
        
        FastLED.clear();
        // Loop continuously while WiFi is not connected
        while ( (WiFi.status() != WL_CONNECTED) && (wifiFlag) )
        {
          delay(100);
          Serial.print("/");
          
          if (ledState)
          {
            aFastled.ledOn(i%aConfig.objectConfig.activeLeds, CRGB::Blue, false);
          }
          else
          {
            aFastled.ledOn(i%aConfig.objectConfig.activeLeds, CRGB::Black, false);
          }
          
          aFastled.ledShow();
          ledState = !ledState;
          
          if (millis() - previousMillisHB > (aConfig.networkConfig.wifiConnectDelay*1000) )
          {
            previousMillisHB = millis();
            wifiFlag = false;
          }
        }
      }
      
      Serial.println(F(" "));
      if (WiFi.status() == WL_CONNECTED)
      {
        Serial.print(F("connected to "));
        Serial.println(aConfig.networkConfig.ssid[i]);
        Serial.print(F("IP address: "));
        Serial.println(WiFi.localIP());
        
        // MDNS
        if (!MDNS.begin(aConfig.objectConfig.objectName))
        {
          Serial.println("Error setting up MDNS responder!");
        }
        Serial.println("mDNS responder started");
        Serial.print(F("connect on webUI admin page : http://"));
        Serial.print(aConfig.objectConfig.objectName);
        Serial.println(F(".local"));        
      }
      else
      {
        if (aConfig.networkConfig.disableSsid)
        {
          Serial.print(F("disable this ssid: "));
          Serial.println(aConfig.networkConfig.ssid[i]);
          aConfig.networkConfig.active[i]=false;
          writeNetworkConfig();
        }
        if (aConfig.networkConfig.rebootEsp)
        {
          Serial.println(F("reboot"));
          delay(1000);
          ESP.restart();
        }        
      }
    }    
  }
  Serial.println(" ");
  
  // WIFI AP MODE
  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(F("failed to connect to wifi as client, creating a wifi AP: "));
    Serial.println(aConfig.networkConfig.apName);
    
    // WiFi.mode(WIFI_AP_STA);
    WiFi.mode(WIFI_AP);
    WiFi.softAPConfig(aConfig.networkConfig.apIP,aConfig.networkConfig.apIP,aConfig.networkConfig.apNetMsk);
    bool apRC = WiFi.softAP(aConfig.networkConfig.apName, aConfig.networkConfig.apPassword);
    
    if (apRC)
    {
      Serial.println(F("AP WiFi OK"));
    }
    else
    {
      Serial.println(F("AP WiFi failed"));
    }
    
    // Print ESP soptAP IP Address
    Serial.print(F("softAPIP: "));
    Serial.println(WiFi.softAPIP());
    
    // MDNS
    if (!MDNS.begin("technolarp"))
    {
      Serial.println("Error setting up MDNS responder!");
    }
    Serial.println("mDNS responder started");
    Serial.println(F("connect on webUI admin page : http://technolarp.local"));
  }
  
  // WEB SERVER
  // Route for root / web page
  server.serveStatic("/", LittleFS, "/www/").setDefaultFile("config.html");
  server.serveStatic("/config", LittleFS, "/config/");
  server.onNotFound(notFound);
  
  // WEBSOCKET
  ws.onEvent(onEvent);
  server.addHandler(&ws);
  
  // Start server
  server.begin();
  
  // REFRESH LED/REED
  previousMillisRefresh = millis();
  intervalRefresh = 200;
  
  // RESET TIMEOUT
  previousMillisReset = millis();
  previousMillisBrightness = millis();
  
  // HEARTBEAT
  previousMillisHB = millis();
  intervalHB = 10000;
  
  // SERIAL
  Serial.println(F(""));
  Serial.println(F(""));
  Serial.println(F("START !!!"));
}
/*
----------------------------------------------------------------------------
FIN DU SETUP
----------------------------------------------------------------------------
*/


/*
----------------------------------------------------------------------------
LOOP
----------------------------------------------------------------------------
*/
void loop() 
{
  // AVOID WATCHDOG
  yield();
  
  // WEBSOCKET
  ws.cleanupClients();
  
  // FASTLED
  aFastled.updateAnimation();
  
  // CONTROL BRIGHTNESS
  aFastled.controlBrightness(aConfig.objectConfig.brightness);
  
  // gerer le statut de la serrure
  switch (aConfig.objectConfig.statutActuel)
  {
    case OBJET_FERME:
    // la serrure est fermee
    serrureFermee();
    break;
    
    case OBJET_OUVERT:
    // la serrure est ouverte
    serrureOuverte();
    break;
    
    case OBJET_OUVERTURE:
    // animation de changement d'etat
    serrureOuverture();
    break;
    
    case OBJET_BLINK:
    // blink led pour identification
    serrureBlink();
    break;
    
    default:
    // nothing
    break;
  }
  
  // traiter le buffer du websocket
  if (flagBufferWebsocket)
  {
    flagBufferWebsocket = false;
    handleWebsocketBuffer();
  }
  
  // HEARTBEAT
  if(millis() - previousMillisHB > intervalHB)
  {
    previousMillisHB = millis();
    
    // envoyer l'uptime
    sendUptime();
  }
  
  // MDNS
  MDNS.update();
}
/*
----------------------------------------------------------------------------
FIN DU LOOP
----------------------------------------------------------------------------
*/


/*
----------------------------------------------------------------------------
FONCTIONS ADDITIONNELLES
----------------------------------------------------------------------------
*/
void serrureFermee()
{
  if (uneFois)
  {
    uneFois = false;
    
    Serial.println(F("SERRURE FERMEE"));
  }
  
  checkReed();
  checkTimeout();
}

void serrureOuverte()
{
  if (uneFois)
  {
    uneFois = false;
    
    Serial.println(F("SERRURE OUVERTE"));
  }
  
  checkReed();
  checkTimeout();
}

void serrureOuverture()
{
  if (uneFois)
  {
    uneFois = false;
    Serial.println(F("SERRURE OUVERTURE"));
    
    aConfig.objectConfig.indexCode=0;
    
    aFastled.animationBlink02Start(75, 1550, aConfig.objectConfig.couleurs[2], CRGB::Black);
  }
  
  // fin de l'animation blink
  if(!aFastled.isAnimActive()) 
  {
    uneFois = true;
    
    // inverser le statut de la serrure
    aConfig.objectConfig.statutActuel=!aConfig.objectConfig.statutPrecedent;
    
    writeObjectConfig();
    sendObjectConfig();
    
    Serial.println(F("END OUVERTURE"));
  }
}

void serrureBlink()
{
  if (uneFois)
  {
    uneFois = false;
    Serial.println(F("SERRURE BLINK"));
    
    sendStatut();
    
    aFastled.animationBlink02Start(100, 3000, CRGB::Blue, CRGB::Black);
  }
  
  // fin de l'animation blink
  if(!aFastled.isAnimActive()) 
  {
    uneFois = true;
    
    aConfig.objectConfig.statutActuel = aConfig.objectConfig.statutPrecedent;
    
    writeObjectConfig();
    sendObjectConfig();
    
    Serial.println(F("END SERRURE "));
  }
}

void checkReed()
{
  int16_t lastPin=-1;
  
  for (uint8_t i=0;i<aConfig.objectConfig.nbSegments;i++)
  {
    pinState[i]=aMcp23017.readPin(i);
    
    if (pinState[i]==0)
    {
      lastDebounceTime[i]=millis();
    }
    
    if ( (pinState[i] != lastPinState[i]) && (pinActive[i]==true) )
    {
      lastPinState[i]=pinState[i];      
      pinActive[i]=false;
      
      if (pinState[i]==0 )
      {
        lastPin=i;
        Serial.print("last pin: ");
        Serial.println(lastPin);
      }
    }
    
    uint32_t currentTime = millis();
    if ( (pinActive[i]==false) && ((currentTime - lastDebounceTime[i]) > aConfig.objectConfig.debounceTime) )
    {
      lastDebounceTime[i]=currentTime;
      pinActive[i]=true;
    }
  }
  
  if (lastPin>-1)
  {
    if (aConfig.objectConfig.code[aConfig.objectConfig.indexCode]==lastPin)
    {
      aConfig.objectConfig.indexCode++;
    }
    else
    {
      aConfig.objectConfig.indexCode=0;
      
      for (uint8_t i=0;i<aConfig.objectConfig.nbSegments;i++)
      {
        lastDebounceTime[i]=0;
        pinActive[i]=true;
      }
    }
    
    checkTimeoutFlag = true;
    previousMillisReset = millis();
    
    showSparklePixel(lastPin);
    
    previousMillisReset=millis();
  }
  
  if (aConfig.objectConfig.indexCode==aConfig.objectConfig.tailleCode)
  {
    aConfig.objectConfig.statutPrecedent=aConfig.objectConfig.statutActuel;
    aConfig.objectConfig.statutActuel=OBJET_OUVERTURE;
    
    sendStatut();
    
    uneFois=true;
    previousMillisRefresh=0;
    Serial.println("code OK");
  }
  
  // refresh leds
  if(millis() - previousMillisRefresh > intervalRefresh)
  {
    previousMillisRefresh = millis();
    
    for (uint8_t i=0;i<aConfig.objectConfig.nbSegments;i++)
    {
      for (uint8_t j=0;j<aConfig.objectConfig.ledParSegment;j++)
      {
        if (aConfig.objectConfig.statutActuel==OBJET_FERME)
        {
          aFastled.ledOn(i*aConfig.objectConfig.ledParSegment+j, aConfig.objectConfig.couleurs[0], false);
        }
        else if (aConfig.objectConfig.statutActuel==OBJET_OUVERT)
        {
          aFastled.ledOn(i*aConfig.objectConfig.ledParSegment+j, aConfig.objectConfig.couleurs[1], false);
        }
      }
    }
    
    for (uint8_t i=0;i<aConfig.objectConfig.indexCode;i++)
    {
      for (uint8_t j=0;j<aConfig.objectConfig.ledParSegment;j++)
      {
        aFastled.ledOn(aConfig.objectConfig.code[i]*aConfig.objectConfig.ledParSegment+j, aConfig.objectConfig.couleurs[2], false);
      }
    }
    aFastled.ledShow();
  }
}

void checkTimeout()
{
  // reset si timeout
  if (checkTimeoutFlag)
  {
    if( millis() - previousMillisReset > aConfig.objectConfig.timeoutReset)
    {
      previousMillisReset = millis();
      
      Serial.println(F("timeout"));    
      uneFois = true;
      aConfig.objectConfig.indexCode=0;
      
      checkTimeoutFlag = false;
    }
  }
}

void showSparklePixel(uint8_t led)
{
  for (uint8_t j=0;j<aConfig.objectConfig.ledParSegment;j++)
  {
    aFastled.ledOn(led*aConfig.objectConfig.ledParSegment+j, CRGB::White, false);
  }
  aFastled.ledShow();
  
  previousMillisRefresh = millis();
}

// index of max value in an array
uint8_t indexMaxValeur(uint8_t arraySize, uint8_t arrayToSearch[])
{
  uint8_t indexMax=0;
  uint8_t currentMax=0;
  
  for (uint8_t i=0;i<arraySize;i++)
  {
    if (arrayToSearch[i]>=currentMax)
    {
      currentMax = arrayToSearch[i];
      indexMax = i;
    }
  }
  
  return(indexMax);
}

void checkCharacter(char *toCheck, const char *allowed, char replaceChar)
{
  for (uint8_t i = 0; i < strlen(toCheck); i++)
  {
    if (!strchr(allowed, toCheck[i]))
    {
      toCheck[i]=replaceChar;
    }
    Serial.print(toCheck[i]);
  }
  Serial.println("");
}

uint16_t checkValeur(uint16_t valeur, uint16_t minValeur, uint16_t maxValeur)
{
  return(min(max(valeur,minValeur), maxValeur));
}

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) 
{
  switch (type) 
  {
    case WS_EVT_CONNECT:
    Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
    // send config value to html
    sendObjectConfig();
    sendNetworkConfig();
    
    // send volatile info
    sendMaxLed();
    
    sendUptime();
    sendStatut();
    break;
    
    case WS_EVT_DISCONNECT:
    Serial.printf("WebSocket client #%u disconnected\n", client->id());
    break;
    
    case WS_EVT_DATA:
    handleWebSocketMessage(arg, data, len);
    break;
    
    case WS_EVT_PING:
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
    break;
  }
}

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) 
{
  AwsFrameInfo *info = (AwsFrameInfo*)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) 
  {
    data[len] = 0;
    sprintf(bufferWebsocket,"%s\n", (char*)data);
    Serial.print(len);
    Serial.print(bufferWebsocket);
    flagBufferWebsocket = true;
  }
}

void handleWebsocketBuffer()
{
  JsonDocument doc;
  
  DeserializationError error = deserializeJson(doc, bufferWebsocket);
  if (error)
  {
    Serial.println(F("Failed to deserialize buffer"));
  }
  else
  {
    // write config or not
    bool writeObjectConfigFlag = false;
    bool sendObjectConfigFlag = false;
    bool writeNetworkConfigFlag = false;
    bool sendNetworkConfigFlag = false;
    
    // **********************************************
    // modif object config
    // **********************************************
    if (doc["new_objectName"].is<const char*>())
    {
      strlcpy(aConfig.objectConfig.objectName, doc["new_objectName"],sizeof(aConfig.objectConfig.objectName));
      
      // lowercase
      for (uint8_t i = 0; i < sizeof(aConfig.objectConfig.objectName); i++)
      {
        aConfig.objectConfig.objectName[i] = tolower(aConfig.objectConfig.objectName[i]);
      }
      
      // check character
      char const * listeCheck = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-";
      checkCharacter(aConfig.objectConfig.objectName, listeCheck, '-');
      
      writeObjectConfigFlag = true;
      sendObjectConfigFlag = true;
    }
    
    if (doc["new_objectId"].is<unsigned short>()) 
    {
      uint16_t tmpValeur = doc["new_objectId"];
      aConfig.objectConfig.objectId = checkValeur(tmpValeur,1,1000);
      
      writeObjectConfigFlag = true;
      sendObjectConfigFlag = true;
    }
    
    if (doc["new_groupId"].is<unsigned short>()) 
    {
      uint16_t tmpValeur = doc["new_groupId"];
      aConfig.objectConfig.groupId = checkValeur(tmpValeur,1,1000);
      
      writeObjectConfigFlag = true;
      sendObjectConfigFlag = true;
    }
    
    if (doc["new_activeLeds"].is<unsigned short>()) 
    {
      FastLED.clear(); 
      
      uint16_t tmpValeur = doc["new_activeLeds"];
      aConfig.objectConfig.activeLeds = checkValeur(tmpValeur,1,NB_LEDS_MAX);
      uneFois=true;
      
      writeObjectConfigFlag = true;
      sendObjectConfigFlag = true;
    }
    
    if (doc["new_brightness"].is<unsigned short>())
    {
      uint16_t tmpValeur = doc["new_brightness"];
      aConfig.objectConfig.brightness = checkValeur(tmpValeur,0,255);
      FastLED.setBrightness(aConfig.objectConfig.brightness);
      uneFois=true;
      
      writeObjectConfigFlag = true;
      sendObjectConfigFlag = true;
    }
    
    if (doc["new_intervalScintillement"].is<unsigned short>())
    {
      uint16_t tmpValeur = doc["new_intervalScintillement"];
      aConfig.objectConfig.intervalScintillement = checkValeur(tmpValeur,0,1000);
      aFastled.setIntervalControlBrightness(aConfig.objectConfig.intervalScintillement);
      
      writeObjectConfigFlag = true;
      sendObjectConfigFlag = true;
    }
    
    if (doc["new_scintillementOnOff"].is<unsigned short>())
    {
      uint16_t tmpValeur = doc["new_scintillementOnOff"];
      aConfig.objectConfig.scintillementOnOff = checkValeur(tmpValeur,0,1);
      aFastled.setControlBrightness(aConfig.objectConfig.scintillementOnOff);
      
      if (aConfig.objectConfig.scintillementOnOff == 0)
      {
        FastLED.setBrightness(aConfig.objectConfig.brightness);
      }
      
      writeObjectConfigFlag = true;
      sendObjectConfigFlag = true;
    }
    
    if (doc["new_couleurs"].is<JsonVariant>()) 
    {
      JsonArray newCouleur = doc["new_couleurs"];
      
      uint8_t i = newCouleur[0];
      char newColorStr[8];
      strncpy(newColorStr, newCouleur[1], 8);
      
      uint8_t r;
      uint8_t g;
      uint8_t b;
      
      convertStrToRGB(newColorStr, &r, &g, &b);
      aConfig.objectConfig.couleurs[i].red=r;
      aConfig.objectConfig.couleurs[i].green=g;
      aConfig.objectConfig.couleurs[i].blue=b;
      
      writeObjectConfigFlag = true;
      sendObjectConfigFlag = true;
    }
    
    if (doc["new_nbSegments"].is<unsigned short>())
    {
      uint16_t tmpValeur = doc["new_nbSegments"];
      aConfig.objectConfig.nbSegments = checkValeur(tmpValeur,1,10);
      aConfig.objectConfig.activeLeds = aConfig.objectConfig.nbSegments * aConfig.objectConfig.ledParSegment;
      aFastled.setNbLed(aConfig.objectConfig.activeLeds);
      aFastled.allLedOff(false);
      
      uneFois=true;
      
      writeObjectConfigFlag = true;
      sendObjectConfigFlag = true;
    }
    
    if (doc["new_ledParSegment"].is<unsigned short>())
    {
      uint16_t tmpValeur = doc["new_ledParSegment"];
      aConfig.objectConfig.ledParSegment = checkValeur(tmpValeur,1,5);
      aConfig.objectConfig.activeLeds = aConfig.objectConfig.nbSegments * aConfig.objectConfig.ledParSegment;
      aFastled.setNbLed(aConfig.objectConfig.activeLeds);
      aFastled.allLedOff(false);
      
      uneFois=true;
      
      writeObjectConfigFlag = true;
      sendObjectConfigFlag = true;
    }
    
    if (doc["new_tailleCode"].is<unsigned short>())
    {
      uint16_t tmpValeur = doc["new_tailleCode"];
      aConfig.objectConfig.tailleCode = checkValeur(tmpValeur,1,10);
      uneFois=true;
      
      writeObjectConfigFlag = true;
      sendObjectConfigFlag = true;
    }
    
    if (doc["new_code"].is<JsonVariant>())
    {
      JsonArray newCodeToSet = doc["new_code"];
      
      uint8_t nouvellePosition = newCodeToSet[0];
      uint8_t nouvelleValeur = newCodeToSet[1];
      
      aConfig.objectConfig.code[nouvellePosition]=nouvelleValeur;
      
      writeObjectConfigFlag = true;
      sendObjectConfigFlag = true;
    }
    
    if ( doc["new_resetCode"].is<unsigned short>() && doc["new_resetCode"]==1 )
    {
      for (uint8_t i=0;i<MAX_SIZE_CODE;i++)
      {
        aConfig.objectConfig.code[i] = i;
      }
      
      writeObjectConfigFlag = true;
      sendObjectConfigFlag = true;
    }
    
    if ( doc["new_aleatCode"].is<unsigned short>() && doc["new_aleatCode"]==1 )
    {
      uint8_t tabAleatoire[aConfig.objectConfig.nbSegments];
      for (uint8_t i=0;i<aConfig.objectConfig.nbSegments;i++)
      {
        tabAleatoire[i]=random(10,50);
      }
      
      for (uint8_t i=0;i<aConfig.objectConfig.nbSegments;i++)
      {
        uint8_t indexAleat = indexMaxValeur(aConfig.objectConfig.nbSegments, tabAleatoire);
        tabAleatoire[indexAleat]=0;
        aConfig.objectConfig.code[i]=indexAleat;
      }
      
      writeObjectConfigFlag = true;
      sendObjectConfigFlag = true;
    }
    
    if (doc["new_timeoutReset"].is<unsigned short>())
    {
      uint16_t tmpValeur = doc["new_timeoutReset"];
      aConfig.objectConfig.timeoutReset = checkValeur(tmpValeur,1,30000);
      uneFois=true;
      
      writeObjectConfigFlag = true;
      sendObjectConfigFlag = true;
    }
    
    if (doc["new_debounceTime"].is<unsigned short>())
    {
      uint16_t tmpValeur = doc["new_debounceTime"];
      aConfig.objectConfig.debounceTime = checkValeur(tmpValeur,50,1000);
      
      writeObjectConfigFlag = true;
      sendObjectConfigFlag = true;
    }
    
    if (doc["new_statutActuel"].is<unsigned short>())
    {
      aConfig.objectConfig.statutPrecedent=aConfig.objectConfig.statutActuel;
      
      uint16_t tmpValeur = doc["new_statutActuel"];
      aConfig.objectConfig.statutActuel=tmpValeur;
      
      uneFois=true;
      
      writeObjectConfigFlag = true;
      sendObjectConfigFlag = true;
    }
    
    // **********************************************
    // modif network config
    if (doc["new_active"].is<JsonVariant>())
    {
      JsonArray newActive = doc["new_active"];
      
      uint8_t x = checkValeur(newActive[0], 0, WIFI_CLIENTS - 1);
      uint8_t y = checkValeur(newActive[1], 0, 1);
      
      aConfig.networkConfig.active[x] = y;
      
      writeNetworkConfigFlag = true;
      sendNetworkConfigFlag = true;
      
      // update statut
      uneFois = true;
    }
    
    if (doc["new_ssid"].is<JsonVariant>())
    {
      JsonArray newSsid = doc["new_ssid"];
      
      uint8_t x = checkValeur(newSsid[0], 0, WIFI_CLIENTS - 1);
      strlcpy(aConfig.networkConfig.ssid[x], newSsid[1], sizeof(aConfig.networkConfig.ssid[x]));
      
      writeNetworkConfigFlag = true;
      sendNetworkConfigFlag = true;
      
      // update statut
      uneFois = true;
    }
    
    if (doc["new_password"].is<JsonVariant>())
    {
      JsonArray newPassword = doc["new_password"];
      
      uint8_t x = checkValeur(newPassword[0], 0, WIFI_CLIENTS - 1);
      strlcpy(aConfig.networkConfig.password[x], newPassword[1], sizeof(aConfig.networkConfig.password[x]));
      
      writeNetworkConfigFlag = true;
      sendNetworkConfigFlag = true;
      
      // update statut
      uneFois = true;
    }
    
    if (doc["new_wifiConnectDelay"].is<unsigned short>())
    {
      uint16_t tmpValeur = doc["new_wifiConnectDelay"];
      aConfig.networkConfig.wifiConnectDelay = checkValeur(tmpValeur, 1, 255);
      
      writeNetworkConfigFlag = true;
      sendNetworkConfigFlag = true;
    }
    
    if (doc["new_disableSsid"].is<unsigned short>())
    {
      uint16_t tmpValeur = doc["new_disableSsid"];
      aConfig.networkConfig.disableSsid = checkValeur(tmpValeur, 0, 1);
      
      writeNetworkConfigFlag = true;
      sendNetworkConfigFlag = true;
    }
    
    if (doc["new_rebootEsp"].is<unsigned short>())
    {
      uint16_t tmpValeur = doc["new_rebootEsp"];
      aConfig.networkConfig.rebootEsp = checkValeur(tmpValeur, 0, 1);
      
      writeNetworkConfigFlag = true;
      sendNetworkConfigFlag = true;
    }
    
    if (doc["new_apName"].is<const char*>())
    {
      strlcpy(aConfig.networkConfig.apName, doc["new_apName"], sizeof(aConfig.networkConfig.apName));
      
      // uppercase
      for (uint8_t i = 0; i < sizeof(aConfig.objectConfig.objectName); i++)
      {
        aConfig.networkConfig.apName[i] = toupper(aConfig.networkConfig.apName[i]);
      }
      
      // check for unsupported char
      char const * listeCheck = "ABCDEFGHIJKLMNOPQRSTUVWYXZ0123456789_-";
      checkCharacter(aConfig.networkConfig.apName, listeCheck, 'A');      
      
      writeNetworkConfigFlag = true;
      sendNetworkConfigFlag = true;
    }
    
    if (doc["new_apPassword"].is<const char*>()) 
    {
      strlcpy(  aConfig.networkConfig.apPassword, doc["new_apPassword"], sizeof(aConfig.networkConfig.apPassword));
      
      writeNetworkConfigFlag = true;
      sendNetworkConfigFlag = true;
    }
    
    if (doc["new_apIP"].is<const char*>()) 
    {
      char newIPchar[16] = "";
      
      strlcpy(  newIPchar, doc["new_apIP"], sizeof(newIPchar));
      
      IPAddress newIP;
      if (newIP.fromString(newIPchar))
      {
        Serial.println("valid IP");
        aConfig.networkConfig.apIP = newIP;
        
        writeNetworkConfigFlag = true;
      }
      
      sendNetworkConfigFlag = true;
    }
    
    if (doc["new_apNetMsk"].is<const char*>()) 
    {
      char newNMchar[16] = "";
      
      strlcpy(newNMchar, doc["new_apNetMsk"], sizeof(newNMchar));
      
      IPAddress newNM;
      if (newNM.fromString(newNMchar)) 
      {
        Serial.println("valid netmask");
        aConfig.networkConfig.apNetMsk = newNM;
        
        writeNetworkConfigFlag = true;
      }
      
      sendNetworkConfigFlag = true;
    }
    
    // actions sur le esp8266
    if ( doc["new_restart"].is<unsigned char>() && doc["new_restart"]==1 )
    {
      Serial.println(F("RESTART RESTART RESTART"));
      ESP.restart();
    }
    
    if ( doc["new_refresh"].is<unsigned char>() && doc["new_refresh"]==1 )
    {
      Serial.println(F("REFRESH"));
      
      sendObjectConfigFlag = true;
      sendNetworkConfigFlag = true;
    }
    
    if ( doc["new_defaultObjectConfig"].is<unsigned char>() && doc["new_defaultObjectConfig"]==1 )
    {
      aConfig.writeDefaultObjectConfig("/config/objectconfig.json");
      Serial.println(F("reset to default object config"));
      
      aFastled.allLedOff();
      aFastled.setNbLed(aConfig.objectConfig.activeLeds);          
      aFastled.setControlBrightness(aConfig.objectConfig.scintillementOnOff);
      aFastled.setIntervalControlBrightness(aConfig.objectConfig.intervalScintillement);
      
      sendObjectConfigFlag = true;
      uneFois = true;
    }
    
    if ( doc["new_defaultNetworkConfig"].is<unsigned char>() && doc["new_defaultNetworkConfig"]==1 )
    {
      aConfig.writeDefaultNetworkConfig("/config/networkconfig.json");
      Serial.println(F("reset to default network config"));          
      
      sendNetworkConfigFlag = true;
    }
    
    // modif config
    // write object config
    if (writeObjectConfigFlag)
    {
      writeObjectConfig();
      
      // update statut
      uneFois = true;
    }
    
    // resend object config
    if (sendObjectConfigFlag)
    {
      sendObjectConfig();
    }
    
    // write network config
    if (writeNetworkConfigFlag)
    {
      writeNetworkConfig();
    }
    
    // resend network config
    if (sendNetworkConfigFlag)
    {
      sendNetworkConfig();
    }
  }
  
  // clear json buffer
  doc.clear();
}

void notFound(AsyncWebServerRequest *request)
{
  request->send(404, "text/plain", "Not found");
}

void convertStrToRGB(const char * source, uint8_t* r, uint8_t* g, uint8_t* b)
{ 
  uint32_t  number = (uint32_t) strtol( &source[1], NULL, 16);
  
  // Split them up into r, g, b values
  *r = number >> 16;
  *g = number >> 8 & 0xFF;
  *b = number & 0xFF;
}

void sendMaxLed()
{
  char toSend[20];
  snprintf(toSend, 20, "{\"maxLed\":%i}", NB_LEDS_MAX);
  
  ws.textAll(toSend);
}

void sendUptime()
{
  uint32_t now = millis() / 1000;
  uint16_t days = now / 86400;
  uint16_t hours = (now%86400) / 3600;
  uint16_t minutes = (now%3600) / 60;
  uint16_t seconds = now % 60;
  
  char toSend[100];
  snprintf(toSend, 100, "{\"uptime\":\"%id %ih %im %is\"}", days, hours, minutes, seconds);
  
  ws.textAll(toSend);
}

void sendStatut()
{
  char toSend[100];
  snprintf(toSend, 100, "{\"statutActuel\":%i}", aConfig.objectConfig.statutActuel); 
  
  ws.textAll(toSend);
}

void sendObjectConfig()
{
  aConfig.stringJsonFile("/config/objectconfig.json", bufferToSend, BUFFERSENDSIZE);
  ws.textAll(bufferToSend);
}

void writeObjectConfig()
{
  aConfig.writeObjectConfig("/config/objectconfig.json");
}

void sendNetworkConfig()
{
  aConfig.stringJsonFile("/config/networkconfig.json", bufferToSend, BUFFERSENDSIZE);
  ws.textAll(bufferToSend);
}

void writeNetworkConfig()
{
  aConfig.writeNetworkConfig("/config/networkconfig.json");
}
/*
----------------------------------------------------------------------------
FIN DES FONCTIONS ADDITIONNELLES
----------------------------------------------------------------------------
*/