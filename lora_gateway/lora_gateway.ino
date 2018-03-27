// **********************************************************************************
// lora_gateway
// sketch for gateway lora to Raspberry Pi
// Version 0.1
// **********************************************************************************

#include <SPI.h>
#include <RH_RF95.h>
#include <EEPROM.h>         // used to store parameters, in specific the time interval between reading transmissions

//#define INITIAL_SETUP // uncomment this for an initial setup of a moteino

// define device specific settings
#define RF95_FREQ 915.0
#define led       9       // Moteinos have LEDs on D9
#define FLASH_SS  8       // and FLASH SS on D8
#define RFM95_CS  10      //wd NSS pin 10 for rf95 radio on Moteino
#define RFM95_RST 9       //wd reset not used
#define RFM95_INT 2
#define RFM95_POWER 8
#define BUFFER_LEN 50

#define SERIAL_BAUD 115200
#define LENGTH_PARK_REF 10
#define LED_DURATION 400

// *************************************************************
// General variable declarations

char radioBuffer[BUFFER_LEN];     // char array used to receive data
long int timeCtr = 1000L;         // var used to store millis value so led can be non-blocking
int txDelay = 3;                  // var hold seconds between transmissions
int strIndex = 0;                 // var used for string manipulation
int prevIndex = 0;                // var used for string manipulation
int inRSSI = 0;                   // var for incoming signal strength
char quote = 22;

//************************************ Load Drivers ****************************************
//  load driver instance of the radio and name it "rf95"
RH_RF95 rf95(RFM95_CS, RFM95_INT);

//************************************ SETUP START ******************************************
void setup()
{
  // Enable the led as an output
  pinMode(led, OUTPUT);

  // setup serial channel
  Serial.begin(SERIAL_BAUD);
  Serial.println("Gateway module start");

  pinMode(RFM95_RST, OUTPUT);
  digitalWrite(RFM95_RST, HIGH);
  // manual radio reset
  digitalWrite(RFM95_RST, LOW);
  delay(10);
  digitalWrite(RFM95_RST, HIGH);
  delay(10);

  //wd initialise radio
  if (!rf95.init())
    Serial.println("LoRa init failed");

  // Defaults after init are 434.0MHz, modulation GFSK_Rb250Fd250, +13dbM
  if (!rf95.setFrequency(RF95_FREQ)) {
    Serial.println("setFrequency failed");
    //  while (1);
  }
  Serial.print("Set Freq to: "); Serial.println(RF95_FREQ);

  rf95.setTxPower(RFM95_POWER, false);
  Serial.print("Set power level to: "); Serial.println(RFM95_POWER);
}

//************************************ Start Loop ************************************************************
void loop()
{
  // Initialise the radio buffer with space
  strncpy(radioBuffer, "                                               ", 50);

  if (rf95.available())
  {
    // Should be a message for us now
    uint8_t len = sizeof(radioBuffer);
    if (rf95.recv((uint8_t *) radioBuffer, &len))
    {
      digitalWrite(led, HIGH);
      timeCtr = millis();

      if (radioBuffer[0] == 'P')
      {
        //Serial.println(radioBuffer);
        //Serial.print("Parking message received from node ");
        // move the input to a string for easier management
        String radioIn = radioBuffer;

        Serial.print("{\x22vehicleNode\x22:");
        // Start with the vehicle node number
        // the string starts "P," after that is the parking node number. Find the posn of the next comma
        strIndex = radioIn.indexOf(',', 2);
        // Store the vehicle node
        String inNode = radioIn.substring(2, strIndex);
        Serial.print(inNode);

        // Now the vehicl registration
        // the string the string continues with comma, then the registration number
        prevIndex = strIndex;
        strIndex = radioIn.indexOf(',', prevIndex + 1);
        String inReg = radioIn.substring(prevIndex + 1, strIndex);
        //Serial.print(" with registration ");
        Serial.print(",\x22vehicleRegistration\x22:\x22");
        Serial.print(inReg);

        // next the parking node number
        // the string the string continues with comma, then the parking sensor node number
        prevIndex = strIndex;
        strIndex = radioIn.indexOf(',', prevIndex + 1);
        String parkNode = radioIn.substring(prevIndex + 1, strIndex);
        //Serial.print(" from parking sensor ");
        Serial.print("\x22,\x22parkingNode\x22:");
        Serial.print(parkNode);

        // next the parking reference
        // the string the string continues with comma, then the parking sensor node number
        prevIndex = strIndex;
        strIndex = radioIn.indexOf(',', prevIndex + 1);
        String parkRef = radioIn.substring(prevIndex + 1, strIndex);
        //Serial.print(" reference ");
        Serial.print(",\x22parkingReference\x22:\x22");
        Serial.print(parkRef);

        //Serial.print(" RSSI is ");
        Serial.print("\x22,\x22RSSI\x22:");
        inRSSI = rf95.lastRssi();
        Serial.print(inRSSI);
        Serial.println('}');

      }
    }
  }
  if ((timeCtr + LED_DURATION) < millis())
  {
    digitalWrite(led, LOW);
  }
}


