#include <LittleFS.h>
#include <ArduinoJson.h> // arduino json v6  // https://github.com/bblanchon/ArduinoJson

// to upload config file : https://github.com/earlephilhower/arduino-esp8266littlefs-plugin/releases
#define SIZE_ARRAY 21
#define NB_COULEURS 3
#define MAX_SIZE_CODE 10
#define JSONBUFFERSIZE 2048

#include <IPAddress.h>
#include <FastLED.h>

class M_config
{
  public:
  // structure stockage
  struct OBJECT_CONFIG_STRUCT
  {
    uint16_t objectId;
    uint16_t groupId;
      
    char objectName[SIZE_ARRAY];
    
    uint8_t activeLeds;
    uint8_t brightness;
    uint16_t intervalScintillement;
    uint16_t scintillementOnOff;
    uint8_t nbSegments;
    uint8_t ledParSegment;
    
    uint8_t statutActuel;
    uint8_t statutPrecedent;

    uint8_t indexCode;
    uint8_t tailleCode;
    uint32_t timeoutReset;
    uint32_t debounceTime;

    CRGB couleurs[NB_COULEURS];

    // DEFINITION DU CODE
    uint8_t code[MAX_SIZE_CODE];
  };
  
  // creer une structure
  OBJECT_CONFIG_STRUCT objectConfig;

  struct NETWORK_CONFIG_STRUCT
  {
    IPAddress apIP;
    IPAddress apNetMsk;
    char apName[SIZE_ARRAY];
    char apPassword[SIZE_ARRAY];
  };
  
  // creer une structure
  NETWORK_CONFIG_STRUCT networkConfig;
  
  
  M_config()
  {
  }

  void readObjectConfig(const char * filename)
  {
    // lire les données depuis le fichier littleFS
    // Open file for reading
    File file = LittleFS.open(filename, "r");
    if (!file) 
    {
      Serial.println(F("Failed to open file for reading"));
      return;
    }
  
    StaticJsonDocument<JSONBUFFERSIZE> doc;
    
    // Deserialize the JSON document
    DeserializationError error = deserializeJson(doc, file);
    if (error)
    {
      Serial.println(F("Failed to deserialize file in read object"));
      Serial.println(error.c_str());
    }
    else
    {
      // Copy values from the JsonObject to the Config
      objectConfig.objectId = doc["objectId"];
      objectConfig.groupId = doc["groupId"];
      
      objectConfig.brightness = doc["brightness"];
      objectConfig.intervalScintillement = doc["intervalScintillement"];
      objectConfig.scintillementOnOff = doc["scintillementOnOff"];
      objectConfig.nbSegments = doc["nbSegments"];      
      objectConfig.ledParSegment = doc["ledParSegment"];
      objectConfig.activeLeds = objectConfig.nbSegments * objectConfig.ledParSegment;
      
      objectConfig.statutActuel = doc["statutActuel"];
      objectConfig.statutPrecedent = doc["statutPrecedent"];
      
      objectConfig.tailleCode = doc["tailleCode"];
      objectConfig.indexCode = doc["indexCode"];
      objectConfig.timeoutReset = doc["timeoutReset"];  
      objectConfig.debounceTime = doc["debounceTime"]; 

      if (doc.containsKey("couleurs"))
      {
        JsonArray couleurArray=doc["couleurs"];
        
        for (uint8_t i=0;i<NB_COULEURS;i++)
        {
          JsonArray rgbArray=couleurArray[i];

          objectConfig.couleurs[i].red = rgbArray[0];
          objectConfig.couleurs[i].green = rgbArray[1];
          objectConfig.couleurs[i].blue =  rgbArray[2];
        }        
      }

      if (doc.containsKey("code"))
      {
        JsonArray codesArray = doc["code"];
        
        for (uint8_t i=0;i<MAX_SIZE_CODE;i++)
        {
          objectConfig.code[i] = codesArray[i];
        }
      }
      
      // read object name
      if (doc.containsKey("objectName"))
      { 
        strlcpy(  objectConfig.objectName,
                  doc["objectName"],
                  SIZE_ARRAY);
      }
    }
      
    // Close the file (File's destructor doesn't close the file)
    file.close();
  }

  void writeObjectConfig(const char * filename)
  { 
    // Delete existing file, otherwise the configuration is appended to the file
    LittleFS.remove(filename);
    
    // Open file for writing
    File file = LittleFS.open(filename, "w");
    if (!file) 
    {
      Serial.println(F("Failed to create file"));
      return;
    }

    // Allocate a temporary JsonDocument
    DynamicJsonDocument doc(JSONBUFFERSIZE);

    doc["objectName"] = objectConfig.objectName;
    
    doc["objectId"] = objectConfig.objectId;
    doc["groupId"] = objectConfig.groupId;
    
    doc["activeLeds"] = objectConfig.ledParSegment*objectConfig.nbSegments;
    doc["brightness"] = objectConfig.brightness;
    doc["intervalScintillement"] = objectConfig.intervalScintillement;
    doc["scintillementOnOff"] = objectConfig.scintillementOnOff;
    doc["ledParSegment"] = objectConfig.ledParSegment;
    doc["nbSegments"] = objectConfig.nbSegments;
    
    doc["statutActuel"] = objectConfig.statutActuel;
    doc["statutPrecedent"] = objectConfig.statutPrecedent;
    
    doc["tailleCode"] = objectConfig.tailleCode;
    doc["indexCode"] = objectConfig.indexCode;
    doc["timeoutReset"] = objectConfig.timeoutReset;
    doc["debounceTime"] = objectConfig.debounceTime;

    JsonArray codeArray = doc.createNestedArray("code");

    for (uint8_t i=0;i<MAX_SIZE_CODE;i++)
    {
      codeArray.add(objectConfig.code[i]);
    }

    JsonArray couleurArray = doc.createNestedArray("couleurs");

    for (uint8_t i=0;i<NB_COULEURS;i++)
    {
      JsonArray couleur_x = couleurArray.createNestedArray();
      
      couleur_x.add(objectConfig.couleurs[i].red);
      couleur_x.add(objectConfig.couleurs[i].green);
      couleur_x.add(objectConfig.couleurs[i].blue);
    }

    // Serialize JSON to file
    if (serializeJson(doc, file) == 0) 
    {
      Serial.println(F("Failed to write to file"));
    }

    // Close the file (File's destructor doesn't close the file)
    file.close();
  }

  void writeDefaultObjectConfig(const char * filename)
  {
    objectConfig.objectId = 1;
    objectConfig.groupId = 1;

    objectConfig.activeLeds = 8;
    objectConfig.brightness = 80;
    objectConfig.intervalScintillement = 50;
    objectConfig.scintillementOnOff = 0;
    objectConfig.nbSegments = 4;
    objectConfig.ledParSegment = 2; 
       
    objectConfig.statutActuel = 1;
    objectConfig.statutPrecedent = 1;
    
    objectConfig.tailleCode = 4;
    objectConfig.indexCode = 0;
    objectConfig.timeoutReset = 5000;
    objectConfig.debounceTime = 300;

    objectConfig.couleurs[0].red = 255;
    objectConfig.couleurs[0].green = 0;
    objectConfig.couleurs[0].blue =  0;
    
    objectConfig.couleurs[1].red = 0;
    objectConfig.couleurs[1].green = 255;
    objectConfig.couleurs[1].blue =  0;

    objectConfig.couleurs[2].red = 255;
    objectConfig.couleurs[2].green = 255;
    objectConfig.couleurs[2].blue =  0;
    
    strlcpy( objectConfig.objectName,
             "serrure_magique_1",
             SIZE_ARRAY);
    
    for (uint8_t i=0;i<MAX_SIZE_CODE;i++)
    {
      objectConfig.code[i]=i;
    }
    
    
    writeObjectConfig(filename);
  }
 
  void readNetworkConfig(const char * filename)
  {
    // lire les données depuis le fichier littleFS
    // Open file for reading
    File file = LittleFS.open(filename, "r");
    if (!file) 
    {
      Serial.println(F("Failed to open file for reading"));
      return;
    }
  
    StaticJsonDocument<JSONBUFFERSIZE> doc;
    
    // Deserialize the JSON document
    DeserializationError error = deserializeJson(doc, file);
    if (error)
    {
      Serial.println(F("Failed to deserialize file in read network "));
      Serial.println(error.c_str());
    }
    else
    {
      // Copy values from the JsonObject to the Config
      if (doc.containsKey("apIP"))
      { 
        JsonArray apIP = doc["apIP"];
        
        networkConfig.apIP[0] = apIP[0];
        networkConfig.apIP[1] = apIP[1];
        networkConfig.apIP[2] = apIP[2];
        networkConfig.apIP[3] = apIP[3];
      }

      if (doc.containsKey("apNetMsk"))
      { 
        JsonArray apNetMsk = doc["apNetMsk"];
        
        networkConfig.apNetMsk[0] = apNetMsk[0];
        networkConfig.apNetMsk[1] = apNetMsk[1];
        networkConfig.apNetMsk[2] = apNetMsk[2];
        networkConfig.apNetMsk[3] = apNetMsk[3];
      }
          
      if (doc.containsKey("apName"))
      { 
        strlcpy(  networkConfig.apName,
                  doc["apName"],
                  SIZE_ARRAY);
      }

      if (doc.containsKey("apPassword"))
      { 
        strlcpy(  networkConfig.apPassword,
                  doc["apPassword"],
                  SIZE_ARRAY);
      }
    }
      
    // Close the file (File's destructor doesn't close the file)
    file.close();
  }
  
  void writeNetworkConfig(const char * filename)
  {
    // Delete existing file, otherwise the configuration is appended to the file
    LittleFS.remove(filename);
  
    // Open file for writing
    File file = LittleFS.open(filename, "w");
    if (!file) 
    {
      Serial.println(F("Failed to create file"));
      return;
    }

    // Allocate a temporary JsonDocument
    StaticJsonDocument<JSONBUFFERSIZE> doc;

    doc["apName"] = networkConfig.apName;
    doc["apPassword"] = networkConfig.apPassword;

    StaticJsonDocument<128> docIp;
    JsonArray arrayIp = docIp.to<JsonArray>();
    arrayIp.add(networkConfig.apIP[0]);
    arrayIp.add(networkConfig.apIP[1]);
    arrayIp.add(networkConfig.apIP[2]);
    arrayIp.add(networkConfig.apIP[3]);

    StaticJsonDocument<128> docNetMask;
    JsonArray arrayNetMask = docNetMask.to<JsonArray>();
    arrayNetMask.add(networkConfig.apNetMsk[0]);
    arrayNetMask.add(networkConfig.apNetMsk[1]);
    arrayNetMask.add(networkConfig.apNetMsk[2]);
    arrayNetMask.add(networkConfig.apNetMsk[3]);
    
    doc["apIP"]=arrayIp;
    doc["apNetMsk"]=arrayNetMask;

    // Serialize JSON to file
    if (serializeJson(doc, file) == 0) 
    {
      Serial.println(F("Failed to write to file"));
    }
    
    // Close the file (File's destructor doesn't close the file)
    file.close();
  }
  
  void writeDefaultNetworkConfig(const char * filename)
  {
    strlcpy(  networkConfig.apName,
              "SERRURE_MAGIQUE_1",
              SIZE_ARRAY);
    
    strlcpy(  networkConfig.apPassword,
              "",
              SIZE_ARRAY);
  
    networkConfig.apIP[0]=192;
    networkConfig.apIP[1]=168;
    networkConfig.apIP[2]=1;
    networkConfig.apIP[3]=1;
  
    networkConfig.apNetMsk[0]=255;
    networkConfig.apNetMsk[1]=255;
    networkConfig.apNetMsk[2]=255;
    networkConfig.apNetMsk[3]=0;
      
    writeNetworkConfig(filename);
  }
  
  void stringJsonFile(const char * filename, char * target, uint16_t targetReadSize)
  {
    // Open file for reading
    File file = LittleFS.open(filename, "r");
    if (!file) 
    {
      Serial.println(F("Failed to open file for reading"));
    }
    else
    {
      uint16_t cptRead = 0;
      while ( (file.available()) && (cptRead<targetReadSize) )
      {
        target[cptRead] = file.read();
        cptRead++;
      }

      if (cptRead<targetReadSize)
      {
        target[cptRead] = '\0';
      }
      else
      {
        target[targetReadSize] = '\0';
      }
    }
    
    // Close the file (File's destructor doesn't close the file)
    file.close();
  }

  void mountFS()
  {
    Serial.println(F("Mount LittleFS"));
    if (!LittleFS.begin())
    {
      Serial.println(F("LittleFS mount failed"));
      return;
    }
  }

  void printJsonFile(const char * filename)
  {
    // Open file for reading
    File file = LittleFS.open(filename, "r");
    if (!file) 
    {
      Serial.println(F("Failed to open file for reading"));
    }
      
    StaticJsonDocument<JSONBUFFERSIZE> doc;
    //DynamicJsonDocument doc(JSONBUFFERSIZE);
    
    // Deserialize the JSON document
    DeserializationError error = deserializeJson(doc, file);
    if (error)
    {
      Serial.println(F("Failed to deserialize file in print object"));
      Serial.println(error.c_str());
    }
    else
    {
      //serializeJsonPretty(doc, Serial);
      serializeJson(doc, Serial);
      Serial.println();
    }
    
    // Close the file (File's destructor doesn't close the file)
    file.close();
  }
  
  void listDir(const char * dirname)
  {
    Serial.printf("Listing directory: %s", dirname);
    Serial.println();
  
    Dir root = LittleFS.openDir(dirname);
  
    while (root.next())
    {
      File file = root.openFile("r");
      Serial.print(F("  FILE: "));
      Serial.print(root.fileName());
      Serial.print(F("  SIZE: "));
      Serial.print(file.size());
      Serial.println();
      file.close();
    }
    Serial.println();
  }

  // I2C RESET
  void i2cReset()
  {
    uint8_t rtn = I2C_ClearBus(); // clear the I2C bus first before calling Wire.begin()
    if (rtn != 0)
    {
      Serial.println(F("I2C bus error. Could not clear"));
      if (rtn == 1) 
      {
        Serial.println(F("SCL clock line held low"));
      }
      else if (rtn == 2)
      {
        Serial.println(F("SCL clock line held low by slave clock stretch"));
      }
      else if (rtn == 3) 
      {
        Serial.println(F("SDA data line held low"));
      }
    }
    else
    { 
      // bus clear
      Serial.println(F("I2C bus clear"));
    }
  }

  /**
   * This routine turns off the I2C bus and clears it
   * on return SCA and SCL pins are tri-state inputs.
   * You need to call Wire.begin() after this to re-enable I2C
   * This routine does NOT use the Wire library at all.
   *
   * returns 0 if bus cleared
   *         1 if SCL held low.
   *         2 if SDA held low by slave clock stretch for > 2sec
   *         3 if SDA held low after 20 clocks.
   *         
   * from: 
   * https://github.com/esp8266/Arduino/issues/1025
   * http://www.forward.com.au/pfod/ArduinoProgramming/I2C_ClearBus/index.html
   */
  uint8_t I2C_ClearBus()
  {
  #if defined(TWCR) && defined(TWEN)
    TWCR &= ~(_BV(TWEN)); //Disable the Atmel 2-Wire interface so we can control the SDA and SCL pins directly
  #endif
    pinMode(SDA, INPUT_PULLUP); // Make SDA (data) and SCL (clock) pins Inputs with pullup.
    pinMode(SCL, INPUT_PULLUP);
  
    delay(500);  // Wait 2.5 secs. This is strictly only necessary on the first power
    // up of the DS3231 module to allow it to initialize properly,
    // but is also assists in reliable programming of FioV3 boards as it gives the
    // IDE a chance to start uploaded the program
    // before existing sketch confuses the IDE by sending Serial data.
  
    boolean SCL_LOW = (digitalRead(SCL) == LOW); // Check is SCL is Low.
    if (SCL_LOW)
    { //If it is held low Arduno cannot become the I2C master. 
      return 1; //I2C bus error. Could not clear SCL clock line held low
    }
  
    boolean SDA_LOW = (digitalRead(SDA) == LOW);  // vi. Check SDA input.
    uint8_t clockCount = 20; // > 2x9 clock
  
    while (SDA_LOW && (clockCount > 0)) 
    { //  vii. If SDA is Low,
      clockCount--;
      // Note: I2C bus is open collector so do NOT drive SCL or SDA high.
      pinMode(SCL, INPUT); // release SCL pullup so that when made output it will be LOW
      pinMode(SCL, OUTPUT); // then clock SCL Low
      delayMicroseconds(10); //  for >5uS
      pinMode(SCL, INPUT); // release SCL LOW
      pinMode(SCL, INPUT_PULLUP); // turn on pullup resistors again
      // do not force high as slave may be holding it low for clock stretching.
      delayMicroseconds(10); //  for >5uS
      // The >5uS is so that even the slowest I2C devices are handled.
      SCL_LOW = (digitalRead(SCL) == LOW); // Check if SCL is Low.
      uint8_t counter = 20;
      while (SCL_LOW && (counter > 0)) 
      {  //  loop waiting for SCL to become High only wait 2sec.
        counter--;
        delay(100);
        SCL_LOW = (digitalRead(SCL) == LOW);
      }
      if (SCL_LOW)
      { // still low after 2 sec error
        return 2; // I2C bus error. Could not clear. SCL clock line held low by slave clock stretch for >2sec
      }
      SDA_LOW = (digitalRead(SDA) == LOW); //   and check SDA input again and loop
    }
    if (SDA_LOW)
    { // still low
      return 3; // I2C bus error. Could not clear. SDA data line held low
    }
  
    // else pull SDA line low for Start or Repeated Start
    pinMode(SDA, INPUT); // remove pullup.
    pinMode(SDA, OUTPUT);  // and then make it LOW i.e. send an I2C Start or Repeated start control.
    // When there is only one I2C master a Start or Repeat Start has the same function as a Stop and clears the bus.
    /// A Repeat Start is a Start occurring after a Start with no intervening Stop.
    delayMicroseconds(10); // wait >5uS
    pinMode(SDA, INPUT); // remove output low
    pinMode(SDA, INPUT_PULLUP); // and make SDA high i.e. send I2C STOP control.
    delayMicroseconds(10); // x. wait >5uS
    pinMode(SDA, INPUT); // and reset pins as tri-state inputs which is the default state on reset
    pinMode(SCL, INPUT);
    return 0; // all ok
  }
};
