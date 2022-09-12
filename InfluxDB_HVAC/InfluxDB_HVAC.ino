#define ETL_NO_STL
#define HVAC_SHIELD_DEBUG_SERIAL
#include <Arduino.h>
#include <Embedded_Template_Library.h>
#include <etl/flat_map.h>
#include <Ethernet.h>
#include "src/EthernetSimpleInfluxDB/SimpleInfluxDB.h"
#include <DallasTemperature.h>
#include "src/HVACShield/HVACShield.h"

#define INFLUX_DEBUG_SERIAL

#define SD_CS 4
#define WIZ_CS 10

const String inf_org = "Home";
const String inf_bucket = "tests";

const unsigned int fanCmdIdle = 70;
const unsigned int fanCmdPreBurst = 110;
const unsigned int fanCmdOn = 150;


int fanStatePrior;
int fanStateNow; 
unsigned int fanCommandPWM;
unsigned long fanOnTime; 
unsigned long fanOffTime; 
const unsigned long burstLength = 10000;

byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED}; // max address for ethernet shield
IPAddress controllerIP(192, 168, 1, 46);
EthernetClient client;

// INFLUX SETUP - make your own configuration file
#include "src/InfluxConfig/conf.h"
SimpleInfluxClient iClient(&client,INFLUX_API_TOKEN,INFLUX_SERVER_AND_PORT);
byte server[] = { 192, 168, 1, 25 };

HVACShield hvacShield;

void updateFanCommand()
{
  fanStateNow = hvacShield.getHVACState(FURNACE_FAN);
  Serial.print(F("Fan State"));
  Serial.println(fanStateNow);
  if(fanStateNow!=SIGNAL_HIGH)
  {
    fanCommandPWM = fanCmdIdle;
  }
  else
  {
    if(fanStateNow!=fanStatePrior)
    { 
      fanOnTime = millis();
      fanCommandPWM = fanCmdPreBurst;
    }
    else
    {
      if(millis() > (fanOnTime + burstLength))
      {
        fanCommandPWM = fanCmdOn;
      }
    }
  }  

  hvacShield.setFanCommand(fanCommandPWM);
  delay(50);
  fanStatePrior = fanStateNow;
  Serial.println(fanCommandPWM);
}


void httpResponseToSerialOut()
{
  delay(10);
  #ifdef INFLUX_DEBUG_SERIAL
  Serial.println("Response:  ");
  while (client.available()) 
  {
      char c = client.read();
      Serial.print(c);
  }
  Serial.println();
  #endif
}

void sendHVACStatus() {
    Point furnaceData("Furnace24VStatus");
    furnaceData.addField("Fan",(int)hvacShield.getHVACState(FURNACE_FAN));
    furnaceData.addField("Heating",(int)hvacShield.getHVACState(HEAT));
    furnaceData.addField("Cooling",(int)hvacShield.getHVACState(COOL));
    furnaceData.addField("Humidify",(int)hvacShield.getHVACState(HUMIDIFY));
    furnaceData.addField("Override",(int)hvacShield.getHVACState(OVERRIDE));
    iClient.sendPoint(inf_org,inf_bucket,furnaceData);
    httpResponseToSerialOut();
}

void sendBlowerData() {
    Point blowerData("BlowerStatus");
    blowerData.addField("FanRPM",(float)hvacShield.getFanSpeed());
    blowerData.addField("FanCommandPWM",(int)(100*fanCommandPWM/255));
    iClient.sendPoint(inf_org,inf_bucket,blowerData);
    httpResponseToSerialOut();
}

void sendTempData() {
    Point tempData("Temperatures");
    tempData.addField("Upstairs",hvacShield.getRemoteTemp());
    tempData.addField("GroundFloor",hvacShield.getBaseTemp());
    iClient.sendPoint(inf_org,inf_bucket,tempData);
    httpResponseToSerialOut();
}

/*
     MAIN LOGIC
*/

void setup() {
  hvacShield = HVACShield();
  Serial.begin(9600);
  Serial.println("Starting Ethernet");
  Ethernet.begin(mac, controllerIP);
  Serial.println("Ethernet Started.");
  // Set SDCard off. 
  pinMode(SD_CS, OUTPUT);
  digitalWrite(SD_CS, HIGH);
  pinMode(2, INPUT_PULLUP);
  DeviceAddress upstairsThermometer = { 0x28, 0xFF, 0xC9, 0x1D, 0x85, 0x16, 0x03, 0xCF };
  DeviceAddress groundFloorThermometer   = { 0x28, 0xFF, 0x83, 0x32, 0x85, 0x16, 0x03, 0x71 };

  hvacShield.setRemoteTempAddr(upstairsThermometer);
  hvacShield.setBaseTempAddr(groundFloorThermometer);
  

}


void loop() {
  #ifdef INFLUX_DEBUG_SERIAL
 
  #endif

  hvacShield.updateHVACStates();
  hvacShield.updateTemperatureReadings();
  hvacShield.updateFanSpeed();
  updateFanCommand();

  Serial.print(F("Fan Speed: "));
  Serial.println(hvacShield.getFanSpeed());
  delay(1000);


  if (client.connect(server, 8086)) 
  {
    
    Serial.println("----------------Connected, Starting Loop-------------------");
    if (client.available()) 
    {
     httpResponseToSerialOut();
    }
    sendHVACStatus();
    delay(500);
    sendBlowerData();
    if(!hvacShield.isTempError())
    { 
      delay(500);
      sendTempData();
    }
    else
    {
      Serial.println(F("skipping temps"));
    }
  } 
  else 
  {
    // if you couldn't make a connection:
    Serial.println("connection failed");
  }    
  // close any connection before send a new request.
  // This will free the socket on the WiFi shield
  client.stop();
  delay(5000);
}
