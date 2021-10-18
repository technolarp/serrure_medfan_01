/*
   ----------------------------------------------------------------------------
   TECHNOLARP - https://technolarp.github.io/
   SERRURE MEDFAN 01 - https://github.com/technolarp/serrure_medfan_01
   version 1.0 - 10/2021
   ----------------------------------------------------------------------------
*/

/*
   ----------------------------------------------------------------------------
   Pour ce montage, vous avez besoin de 
   4 ou + led neopixel
   4 ou + switch reed
   1 MCP23017 en i2c
   ----------------------------------------------------------------------------
*/

/*
   ----------------------------------------------------------------------------
   PINOUT
   D0     NEOPIXEL
   D1     I2C SCL => SCL PCF8574
   D2     I2C SDA => SDA PCF8574
   ----------------------------------------------------------------------------
*/

#include <Arduino.h>

/*
add check sur code
add scintillement
reecrire technolarp_fastled
*/

// WIFI
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>

AsyncWebServer server(80);

// WEBSOCKET
AsyncWebSocket ws("/ws");

// FASTLED
#include <FastLED.h>
#define NUM_LEDS 50
#define DATA_PIN D0
CRGB leds[NUM_LEDS];

// MCP23017
#include "technolarp_mcp23017.h"
M_mcp23017 aMcp23017(0);

// CONFIG
#include "config.h"
M_config aConfig;

// CODE ACTUEL DE LA SERRURE
uint8_t codeSerrureActuel[10] = {0,0,0,0,0,0,0,0,0,0};


// STATUTS DE LA SERRURE
enum {
  SERRURE_OUVERTE = 0,
  SERRURE_FERMEE = 1,
  SERRURE_BLOQUEE = 2,
  SERRURE_ERREUR = 3,
  SERRURE_OUVERTURE = 4,
  SERRURE_BLINK = 5
};

// DIVERS
bool uneFois = true;
bool blinkUneFois = true;
bool blinkflag = true;
uint8_t cptBlink=0;
bool checkTimeoutFlag = false;

uint32_t lastDebounceTime[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
uint8_t lastPinState[16] = {HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH,HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH};
uint8_t pinState[16] = {HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH,HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH};
bool pinActive[16] = {true,true,true,true,true,true,true,true,true,true,true,true,true,true,true,true};

uint32_t previousMillisHB;
uint32_t currentMillisHB;
uint32_t intervalHB;

// REFRESH
uint32_t previousMillisRefresh;
uint32_t currentMillisRefresh;
uint32_t intervalRefresh;

uint32_t previousMillisReset;
uint32_t currentMillisReset;

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
  Serial.println(F("version 1.0 - 10/2021"));
  Serial.println(F("----------------------------------------------------------------------------"));
  
  // CONFIG OBJET
  Serial.println(F(""));
  Serial.println(F(""));
  aConfig.mountFS();
  aConfig.listDir("/");
  aConfig.listDir("/config");
  aConfig.listDir("/www");
  
  aConfig.printJsonFile("/config/objectconfig.txt");
  aConfig.readObjectConfig("/config/objectconfig.txt");

  aConfig.printJsonFile("/config/networkconfig.txt");
  aConfig.readNetworkConfig("/config/networkconfig.txt");

  aConfig.objectConfig.activeLeds = aConfig.objectConfig.nbSegments * aConfig.objectConfig.ledParSegment;
  aConfig.writeObjectConfig("/config/objectconfig.txt");

  // MCP23017
  aMcp23017.setDebounceDelay(300);
  
  // LED RGB
  FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, NUM_LEDS);
  FastLED.setBrightness(aConfig.objectConfig.brightness);

  // animation led de depart
  FastLED.clear();
  for (int i = 0; i < aConfig.objectConfig.activeLeds * 2; i++)
  {
    leds[i % aConfig.objectConfig.activeLeds]=CRGB::Blue;
    FastLED.show();
    delay(50);
    leds[i % aConfig.objectConfig.activeLeds]=CRGB::Black;
    FastLED.show();
  }
  FastLED.show();
  FastLED.clear();

  // check code
  //checkCode();

  // WIFI
  WiFi.disconnect(true);
  
  // AP MODE
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAPConfig(aConfig.networkConfig.apIP, aConfig.networkConfig.apIP, aConfig.networkConfig.apNetMsk);
  WiFi.softAP(aConfig.networkConfig.apName, aConfig.networkConfig.apPassword);
  
  /*
  // CLIENT MODE POUR DEBUG
  const char* ssid = "MYDEBUG";
  const char* password = "pppppp";
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  if (WiFi.waitForConnectResult() != WL_CONNECTED) 
  {
    Serial.printf("WiFi Failed!\n");        
  }
  */
  
  // WEB SERVER
  // Print ESP Local IP Address
  Serial.print(F("localIP: "));
  Serial.println(WiFi.localIP());
  Serial.print(F("softAPIP: "));
  Serial.println(WiFi.softAPIP());

  // Route for root / web page
  server.serveStatic("/", LittleFS, "/www/").setDefaultFile("config.html").setTemplateProcessor(processor);
  server.serveStatic("/config", LittleFS, "/config/");
  server.onNotFound(notFound);

  // WEBSOCKET
  ws.onEvent(onEvent);
  server.addHandler(&ws);

  // Start server
  server.begin();
  
  // REFRESH
  currentMillisRefresh = millis();
  previousMillisRefresh = currentMillisRefresh;
  intervalRefresh = 200;

  // RESET TIMEOUT
  currentMillisReset = millis();
  previousMillisReset = currentMillisReset;
  //intervalReset = 5000;
  
  // HEARTBEAT
  currentMillisHB = millis();
  previousMillisHB = currentMillisHB;
  intervalHB = 20000;

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
  // avoid watchdog reset
  yield();
  
  // WEBSOCKET
  ws.cleanupClients();

  // gerer le statut de la serrure
  switch (aConfig.objectConfig.statutSerrureActuel)
  {
    case SERRURE_FERMEE:
      // la serrure est fermee
      serrureFermee();
      break;

    case SERRURE_OUVERTE:
      // la serrure est ouverte
      serrureOuverte();
      break;

    case SERRURE_BLOQUEE:
      // la serrure est bloquee
      serrureBloquee();
      break;

    case SERRURE_ERREUR:
      // un code incorrect a ete entrer
      serrureErreur();
      break;

    case SERRURE_OUVERTURE:
      // animation de changement d'etat
      serrureOuverture();
      break;
      
    case SERRURE_BLINK:
      // blink led pour identification
      serrureBlink();
      break;
      
    default:
      // nothing
      break;
  }

  // HEARTBEAT
  currentMillisHB = millis();
  if(currentMillisHB - previousMillisHB > intervalHB)
  {
    previousMillisHB = currentMillisHB;
    
    Serial.println("heartbeat");
  }
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

    Serial.print(F("SERRURE FERMEE"));
    Serial.println();
  }

  checkReed();
  checkTimeout();
}

void serrureOuverte()
{
  if (uneFois)
  {
    uneFois = false;
    
    Serial.print(F("SERRURE OUVERTE"));
    Serial.println();
  }

  checkReed();
  checkTimeout();
}

void serrureBloquee()
{
  
}

void serrureErreur()
{
  
}

void serrureOuverture()
{
  if (uneFois)
  {
    //uneFois = false;
    
    Serial.print(F("SERRURE OUVERTURE"));
    Serial.println();

    delay(500);

    aConfig.objectConfig.indexCode=0;

    for (uint8_t k=0;k<6;k++)
    {
      FastLED.clear();
      for (uint8_t i=0;i<aConfig.objectConfig.activeLeds;i++)
      {
        leds[i]=aConfig.objectConfig.couleurs[2];
      }
      FastLED.show();
      delay(75);
      FastLED.clear();
      FastLED.show();
      delay(75);
    }
    FastLED.clear();
    FastLED.show();
    
    // changement etat serrure
    aConfig.objectConfig.statutSerrureActuel=aConfig.objectConfig.statutSerrurePrecedent;
    aConfig.objectConfig.statutSerrureActuel=!aConfig.objectConfig.statutSerrureActuel;
    sendStatutSerrure();
  }
}

void serrureBlink()
{
  if (blinkUneFois)
  {
    blinkUneFois = false;
    
    Serial.print(F("SERRURE BLINK"));
    Serial.println();

    cptBlink=0;
    currentMillisRefresh = millis();
    previousMillisRefresh=currentMillisRefresh;
  }

  currentMillisRefresh = millis();
  if(currentMillisRefresh - previousMillisRefresh > intervalRefresh)
  {
    previousMillisRefresh = currentMillisRefresh;

    if (blinkflag)
    {
      for (uint8_t i=0;i<aConfig.objectConfig.activeLeds;i++)
      {
        leds[i]=CRGB::Blue;
      }
    }
    else
    {
      for (uint8_t i=0;i<aConfig.objectConfig.activeLeds;i++)
      {
        leds[i]=CRGB::Black;
      }
    }
    FastLED.show();    
    
    cptBlink++;
    blinkflag=!blinkflag;
  }

  if (cptBlink==20)
  {
    aConfig.objectConfig.statutSerrureActuel=aConfig.objectConfig.statutSerrurePrecedent;
    sendStatutSerrure();
    uneFois=true;
  }
}

void checkReed()
{  
  int8_t lastPin=-1;
  
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
    if ( (pinActive[i]==false) && ((currentTime - lastDebounceTime[i]) > 1000) )
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
      //Serial.println("wrong");

      for (uint8_t i=0;i<aConfig.objectConfig.nbSegments;i++)
      {
        lastDebounceTime[i]=0;
        pinActive[i]=true;
      }
    }

    checkTimeoutFlag = true;
    currentMillisReset = millis();
    previousMillisReset = currentMillisReset;
    
    showSparklePixel(lastPin);

    previousMillisReset=millis();
  }

  if (aConfig.objectConfig.indexCode==aConfig.objectConfig.tailleCode)
  {
    aConfig.objectConfig.statutSerrurePrecedent=aConfig.objectConfig.statutSerrureActuel;
    aConfig.objectConfig.statutSerrureActuel=SERRURE_OUVERTURE;

    sendStatutSerrure();
    
    uneFois=true;
    previousMillisRefresh=0;
    Serial.println("code OK");
  }
  
  // refresh leds
  currentMillisRefresh = millis();
  if(currentMillisRefresh - previousMillisRefresh > intervalRefresh)
  {
    previousMillisRefresh = currentMillisRefresh;
    
    for (uint8_t i=0;i<aConfig.objectConfig.nbSegments;i++)
    {
      for (uint8_t j=0;j<aConfig.objectConfig.ledParSegment;j++)
      {
        if (aConfig.objectConfig.statutSerrureActuel==SERRURE_FERMEE)
        {
          //Serial.println(i*aConfig.objectConfig.ledParSegment+j);
          leds[i*aConfig.objectConfig.ledParSegment+j]=aConfig.objectConfig.couleurs[0];
        }
        else if (aConfig.objectConfig.statutSerrureActuel==SERRURE_OUVERTE)
        {
          leds[i*aConfig.objectConfig.ledParSegment+j]=aConfig.objectConfig.couleurs[1];
        }
      }
    }
    
    for (uint8_t i=0;i<aConfig.objectConfig.indexCode;i++)
    {
      for (uint8_t j=0;j<aConfig.objectConfig.ledParSegment;j++)
      {
        leds[aConfig.objectConfig.code[i]*aConfig.objectConfig.ledParSegment+j]=aConfig.objectConfig.couleurs[2];
      }
    }
    FastLED.show();
  }
}

void checkTimeout()
{
  // reset si timeout
  if (checkTimeoutFlag)
  {
    currentMillisReset = millis();
    if(currentMillisReset - previousMillisReset > aConfig.objectConfig.timeoutReset)
    {
      previousMillisReset = currentMillisReset;
  
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
    //leds[aConfig.objectConfig.code[led]*aConfig.objectConfig.ledParSegment+j]=CRGB::White;
    leds[led*aConfig.objectConfig.ledParSegment+j]=CRGB::White;
  }
    
  FastLED.show();
  currentMillisRefresh = millis();
  previousMillisRefresh = currentMillisRefresh;
}

// index of max value in an array
int indexMaxValeur(uint8_t arraySize, uint8_t arrayToSearch[])
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

/*
   ----------------------------------------------------------------------------
   FIN DES FONCTIONS ADDITIONNELLES
   ----------------------------------------------------------------------------
*/

void sendUptime()
{
  uint32_t now = millis() / 1000;
  uint16_t days = now / 86400;
  uint16_t hours = (now%86400) / 3600;
  uint16_t minutes = (now%3600) / 60;
  uint16_t seconds = now % 60;
    
  String toSend = "{\"uptime\":\"";
  toSend+= String(days) + String("d ") + String(hours) + String("h ") + String(minutes) + String("m ") + String(seconds) + String("s");
  toSend+= "\"}";

  ws.textAll(toSend);
  Serial.println(toSend);
}

void sendStatutSerrure()
{
  String toSend = "{\"statutSerrureActuel\":";
  toSend+= aConfig.objectConfig.statutSerrureActuel;
  toSend+= "}";

  ws.textAll(toSend);
}

void convertStrToRGB(String source, uint8_t* r, uint8_t* g, uint8_t* b)
{
  uint32_t  number = (uint32_t) strtol( &source[1], NULL, 16);
  
  // Split them up into r, g, b values
  *r = number >> 16;
  *g = number >> 8 & 0xFF;
  *b = number & 0xFF;
}

void checkCharacter(char* toCheck, char* allowed, char replaceChar)
{
  for (int i = 0; i < strlen(toCheck); i++)
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

String processor(const String& var)
{
  if (var == "MAXLEDS")
  {
    return String(NUM_LEDS);
  }

  if (var == "APNAME")
  {
    return String(aConfig.networkConfig.apName);
  }

  if (var == "OBJECTNAME")
  {
    return String(aConfig.objectConfig.objectName);
  }
   
  return String();
}


void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) 
{
   switch (type) 
    {
      case WS_EVT_CONNECT:
        Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
        // send config value to html
        ws.textAll(aConfig.stringJsonFile("/config/objectconfig.txt"));
        ws.textAll(aConfig.stringJsonFile("/config/networkconfig.txt"));

        // send volatile info
        sendUptime();
    
        break;
        
      case WS_EVT_DISCONNECT:
        Serial.printf("WebSocket client #%u disconnected\n", client->id());
        break;
        
      case WS_EVT_DATA:
        handleWebSocketMessage(arg, data, len);
        break;
        
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
    char buffer[100];
    data[len] = 0;
    sprintf(buffer,"%s\n", (char*)data);
    Serial.print(buffer);
    
    StaticJsonDocument<256> doc;
    DeserializationError error = deserializeJson(doc, buffer);
    if (error)
    {
      Serial.println(F("Failed to deserialize buffer"));
    }
    else
    {
        // write config or not
        bool writeObjectConfig = false;
        bool sendObjectConfig = false;
        bool writeNetworkConfig = false;
        bool sendNetworkConfig = false;
        
        // modif object config
        if (doc.containsKey("new_objectName"))
        {
          strlcpy(  aConfig.objectConfig.objectName,
                    doc["new_objectName"],
                    sizeof(aConfig.objectConfig.objectName));
  
          writeObjectConfig = true;
          sendObjectConfig = true;
        }
  
        if (doc.containsKey("new_objectId")) 
        {
          uint16_t tmpValeur = doc["new_objectId"];
          aConfig.objectConfig.objectId = checkValeur(tmpValeur,1,1000);
  
          writeObjectConfig = true;
          sendObjectConfig = true;
        }
  
        if (doc.containsKey("new_groupId")) 
        {
          uint16_t tmpValeur = doc["new_groupId"];
          aConfig.objectConfig.groupId = checkValeur(tmpValeur,1,1000);
          
          writeObjectConfig = true;
          sendObjectConfig = true;
        }
  
        if (doc.containsKey("new_activeLeds")) 
        {
          FastLED.clear(); 
          
          uint16_t tmpValeur = doc["new_activeLeds"];
          aConfig.objectConfig.activeLeds = checkValeur(tmpValeur,1,NUM_LEDS);
          uneFois=true;
          
          writeObjectConfig = true;
          sendObjectConfig = true;
        }
        
        if (doc.containsKey("new_brightness"))
        {
          uint16_t tmpValeur = doc["new_brightness"];
          aConfig.objectConfig.brightness = checkValeur(tmpValeur,0,255);
          FastLED.setBrightness(aConfig.objectConfig.brightness);
          uneFois=true;
          
          writeObjectConfig = true;
          sendObjectConfig = true;
        }

        if (doc.containsKey("new_couleurs")) 
        {
          JsonArray newCouleur = doc["new_couleurs"];

          uint8_t i = newCouleur[0];
          String newColorStr = newCouleur[1];
          
          uint8_t r;
          uint8_t g;
          uint8_t b;
          
          convertStrToRGB(newColorStr,&r, &g, &b);
          aConfig.objectConfig.couleurs[i].red=r;
          aConfig.objectConfig.couleurs[i].green=g;
          aConfig.objectConfig.couleurs[i].blue=b;
          //convertStrToRGB(newColorStr,&aConfig.objectConfig.couleurs[i].red, &aConfig.objectConfig.couleurs[i].green, &aConfig.objectConfig.couleurs[i].blue);
          uneFois=true;
          
          writeObjectConfig = true;
          sendObjectConfig = true;
        }
  
        if (doc.containsKey("new_nbSegments"))
        {
          uint16_t tmpValeur = doc["new_nbSegments"];
          aConfig.objectConfig.nbSegments = checkValeur(tmpValeur,0,20);
          aConfig.objectConfig.activeLeds = aConfig.objectConfig.nbSegments * aConfig.objectConfig.ledParSegment;
          
          uneFois=true;
          
          writeObjectConfig = true;
          sendObjectConfig = true;
        }

        if (doc.containsKey("new_ledParSegment"))
        {
          uint16_t tmpValeur = doc["new_ledParSegment"];
          aConfig.objectConfig.ledParSegment = checkValeur(tmpValeur,0,10);
          aConfig.objectConfig.activeLeds = aConfig.objectConfig.nbSegments * aConfig.objectConfig.ledParSegment;
          
          uneFois=true;
          
          writeObjectConfig = true;
          sendObjectConfig = true;
        }

        if (doc.containsKey("new_tailleCode"))
        {
          uint16_t tmpValeur = doc["new_tailleCode"];
          aConfig.objectConfig.tailleCode = checkValeur(tmpValeur,1,10);
          uneFois=true;
          
          writeObjectConfig = true;
          sendObjectConfig = true;
        }

        if (doc.containsKey("new_code"))
        {
          JsonArray newCodeToSet = doc["new_code"];
        
          uint8_t nouvellePosition = newCodeToSet[0];
          uint8_t nouvelleValeur = newCodeToSet[1];
          
          aConfig.objectConfig.code[nouvellePosition]=nouvelleValeur;
          
          writeObjectConfig = true;
          sendObjectConfig = true;
        }

        if ( doc.containsKey("new_resetCode") && doc["new_resetCode"]==1 )
        {
          for (uint8_t i=0;i<MAX_SIZE_CODE;i++)
          {
            aConfig.objectConfig.code[i] = i;
          }
          
          writeObjectConfig = true;
          sendObjectConfig = true;
        }

        if ( doc.containsKey("new_aleatCode") && doc["new_aleatCode"]==1 )
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

          writeObjectConfig = true;
          sendObjectConfig = true;
        }

        if (doc.containsKey("new_timeoutReset"))
        {
          uint16_t tmpValeur = doc["new_timeoutReset"];
          aConfig.objectConfig.timeoutReset = checkValeur(tmpValeur,1,30000);
          uneFois=true;
          
          writeObjectConfig = true;
          sendObjectConfig = true;
        }

        if (doc.containsKey("new_statutSerrure"))
        {
          uint16_t tmpValeur = doc["new_statutSerrure"];
          aConfig.objectConfig.statutSerrurePrecedent=aConfig.objectConfig.statutSerrureActuel;
          aConfig.objectConfig.statutSerrureActuel=checkValeur(tmpValeur,0,5);
    
          uneFois=true;
          
          writeObjectConfig = true;
          sendObjectConfig = true;
        }
        
        // modif network config
        if (doc.containsKey("new_apName")) 
        {
          strlcpy(  aConfig.networkConfig.apName,
                    doc["new_apName"],
                    sizeof(aConfig.networkConfig.apName));
  
          // check for unsupported char
          checkCharacter(aConfig.networkConfig.apName, "ABCDEFGHIJKLMNOPQRSTUVWYZ", 'A');
          
          writeNetworkConfig = true;
          sendNetworkConfig = true;
        }
  
        if (doc.containsKey("new_apPassword")) 
        {
          strlcpy(  aConfig.networkConfig.apPassword,
                    doc["new_apPassword"],
                    sizeof(aConfig.networkConfig.apPassword));
  
          writeNetworkConfig = true;
        }
  
        if (doc.containsKey("new_apIP")) 
        {
          char newIPchar[16] = "";
  
          strlcpy(  newIPchar,
                    doc["new_apIP"],
                    sizeof(newIPchar));
  
          IPAddress newIP;
          if (newIP.fromString(newIPchar)) 
          {
            Serial.println("valid IP");
            aConfig.networkConfig.apIP = newIP;
  
            writeNetworkConfig = true;
          }
          
          sendNetworkConfig = true;
        }
  
        if (doc.containsKey("new_apNetMsk")) 
        {
          char newNMchar[16] = "";
  
          strlcpy(  newNMchar,
                    doc["new_apNetMsk"],
                    sizeof(newNMchar));
  
          IPAddress newNM;
          if (newNM.fromString(newNMchar)) 
          {
            Serial.println("valid netmask");
            aConfig.networkConfig.apNetMsk = newNM;
  
            writeNetworkConfig = true;
          }
  
          sendNetworkConfig = true;
        }
        
        // actions sur le esp8266
        if ( doc.containsKey("new_restart") && doc["new_restart"]==1 )
        {
          Serial.println(F("RESTART RESTART RESTART"));
          ESP.restart();
        }
  
        if ( doc.containsKey("new_refresh") && doc["new_refresh"]==1 )
        {
          Serial.println(F("REFRESH"));

          sendObjectConfig = true;
          sendNetworkConfig = true;
        }

        if ( doc.containsKey("new_defaultObjectConfig") && doc["new_defaultObjectConfig"]==1 )
        {
          Serial.println(F("reset to default object config"));
          aConfig.writeDefaultObjectConfig("/config/objectconfig.txt");
          
          sendObjectConfig = true;
          uneFois = true;
        }

        if ( doc.containsKey("new_defaultNetworkConfig") && doc["new_defaultNetworkConfig"]==1 )
        {
          Serial.println(F("reset to default network config"));
          aConfig.writeDefaultNetworkConfig("/config/networkconfig.txt");
          
          sendNetworkConfig = true;
        }

        if ( doc.containsKey("new_blink") && doc["new_blink"]==1 )
        {
          Serial.println(F("BLINK"));
          blinkUneFois = true;
          aConfig.objectConfig.statutSerrurePrecedent = aConfig.objectConfig.statutSerrureActuel;
          aConfig.objectConfig.statutSerrureActuel = SERRURE_BLINK;
          sendStatutSerrure();
        }
  
        // modif config
        // write object config
        if (writeObjectConfig)
        {
          aConfig.writeObjectConfig("/config/objectconfig.txt");
          //aConfig.printJsonFile("/config/objectconfig.txt");
        }
  
        // resend object config
        if (sendObjectConfig)
        {
          ws.textAll(aConfig.stringJsonFile("/config/objectconfig.txt"));
        }
  
        // write network config
        if (writeNetworkConfig)
        {
          aConfig.writeNetworkConfig("/config/networkconfig.txt");
          //aConfig.printJsonFile("/config/networkconfig.txt");
        }
  
        // resend network config
        if (sendNetworkConfig)
        {
          ws.textAll(aConfig.stringJsonFile("/config/networkconfig.txt"));
        }
    }
 
    // clear json buffer
    doc.clear();
  }
}

void notFound(AsyncWebServerRequest *request)
{
    request->send(404, "text/plain", "Not found");
}
