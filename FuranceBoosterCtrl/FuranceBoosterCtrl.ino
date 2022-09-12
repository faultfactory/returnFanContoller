#define ETL_NO_STL
#include <Arduino.h>
#include <Embedded_Template_Library.h>
#include <etl/flat_map.h>

#define FAN_PWM_OUTPUT 5
#define OPTO_PIN_HEAT 9
#define OPTO_PIN_COOL 8
#define OPTO_PIN_FAN 7
#define OPTO_PIN_HUM 6
#define FANSPEED_INPUT_PIN 3
#define SSR_OUT_PIN A5
#define SD_CS = 4;
#define WIZ_CS = 10;

float fanSpeed;
int fanCmdIdle = 70;
int fanCmdPreBurst = 110; //Lower to gradually increase speed vs overshoot. 
int fanCmdOn = 150;

int fanStatePrior;
int fanStateNow; 
int fanCommandPWM;
unsigned long fanOnTime; 
unsigned long fanOffTime; 
const unsigned long burstLength = 10000;
bool tempError = false;

volatile unsigned long prevTime = 0;
volatile unsigned long edgeTime = 0;
volatile unsigned long pulseCounter = 0;
volatile bool newPulse = false;

byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };    
byte myIp[] = { 192, 168 , 1 , 19 };

enum HVACState
{
   UNKNOWN,
   SIGNAL_LOW,
   SIGNAL_HIGH
};

enum HVACSignal
{
   FURNACE_FAN,
   HEAT,
   COOL,
   HUMIDIFY
};

etl::flat_map<HVACSignal,HVACState,4> stateMap;
etl::flat_map<HVACSignal,bool,4> activeMap;
etl::flat_map<HVACSignal,int,4> pinMap;



HVACState getHVACState(HVACSignal signal)
{
   if(activeMap[signal])
   {
       int pinState = digitalRead(pinMap[signal]);
       if(pinState == LOW)
       {
           return SIGNAL_HIGH;
       }
       if(pinState == HIGH)
       {
           return SIGNAL_LOW;
       }
       return UNKNOWN;
   }
   else
   {
       return UNKNOWN;
   }
   return UNKNOWN;
}

void updateHVACStates()
{
  for(auto& sig : stateMap)
  {
    sig.second = getHVACState(sig.first);
  }
}

void updateFanSpeed()
{
  unsigned long timing = calculatePulseTiming();
  Serial.print("timing: ");
  Serial.println(timing);
  if(timing == 0)
  {
    fanSpeed = 0.0f;
  }
  else
  {
    fanSpeed = 60.0f * 1000000.0f/(float(timing));
  }
}

void measureTimingViaISR()
{
  noInterrupts();
  prevTime = 0;
  edgeTime = 0;
  pulseCounter = 0;
  newPulse = false;
  interrupts();
  auto now = millis();
  auto start = now;
  while (now < start + 1000)
  {
    now = millis();
  }
  Serial.print("Pulse Counter:  ");
  Serial.println(pulseCounter);
}

unsigned long calculatePulseTiming()
{
  measureTimingViaISR();
  // Serial.println(prevTime);
  // Serial.println(edgeTime);
  // Serial.println(pulseCounter);
  if (newPulse == false || pulseCounter == 1)
  {
    return 0;
  }
  else
  {
    noInterrupts(); 
    unsigned long deltaTime = edgeTime - prevTime;
    pulseCounter = 1;
    newPulse = false;
    interrupts();
    return deltaTime;
  }
}

int leftOfDecimal(float value)
{
  int leftValues = 1; 
    while(value/10 > 0)
    {
      value /=10;
      leftValues++;
    }
    return leftValues;
}



void updateFanCommand()
{
  fanStateNow = stateMap[FURNACE_FAN];
  Serial.print("Fan State");
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

  analogWrite(FAN_PWM_OUTPUT,fanCommandPWM);
  delay(50);
  fanStatePrior = fanStateNow;
  Serial.println(fanCommandPWM);
}

void setup()
{ 
  Serial.begin(9600);
  
  pinMode(2, INPUT_PULLUP);
  pinMode(SSR_OUT_PIN, OUTPUT);
  
  // Set status for opto input pins
  pinMode(pinMap[FURNACE_FAN],INPUT_PULLUP);
  pinMode(pinMap[HEAT],INPUT_PULLUP);
  pinMode(pinMap[COOL],INPUT_PULLUP);
  pinMode(pinMap[HUMIDIFY],INPUT_PULLUP);
  // Set status for SSR output Pin
  digitalWrite(SSR_OUT_PIN, LOW);  
  // Set SDCard off. 
  
  // Attach interrupt to pin for measuring Fanspeed
  pinMode(FANSPEED_INPUT_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(FANSPEED_INPUT_PIN), ISR1, FALLING);

  
   stateMap.insert(etl::pair<HVACSignal,HVACState>{FURNACE_FAN,UNKNOWN});
   stateMap.insert(etl::pair<HVACSignal,HVACState>{HEAT,UNKNOWN});
   stateMap.insert(etl::pair<HVACSignal,HVACState>{COOL,UNKNOWN});
   stateMap.insert(etl::pair<HVACSignal,HVACState>{OVERRIDE,UNKNOWN});
   
   activeMap.insert(etl::pair<HVACSignal,bool>{FURNACE_FAN,true});
   activeMap.insert(etl::pair<HVACSignal,bool>{HEAT,true});
   activeMap.insert(etl::pair<HVACSignal,bool>{COOL,true});
   activeMap.insert(etl::pair<HVACSignal,bool>{OVERRIDE,false});

   pinMap.insert(etl::pair<HVACSignal,int>{FURNACE_FAN,OPTO_PIN_FAN});
   pinMap.insert(etl::pair<HVACSignal,int>{HEAT,OPTO_PIN_HEAT});
   pinMap.insert(etl::pair<HVACSignal,int>{COOL,OPTO_PIN_COOL});
   pinMap.insert(etl::pair<HVACSignal,int>{OVERRIDE,OPTO_PIN_HUM});
  
}

void loop()
{
   
  updateHVACStates();
  updateFanSpeed();
  updateFanCommand();

  Serial.print("Fan Speed: ");
  Serial.println(fanSpeed);
  delay(5000);

}


void ISR1()
{
  prevTime = edgeTime;
  edgeTime = micros();
  pulseCounter++;
  newPulse = true;
}
  
