// **********************************************************************************
// lora_monitor
// sketch for in vehicle LoRa beacon
// Version 0.3
// **********************************************************************************

#include <SPI.h>
#include <RH_RF95.h>

// define device specific settings
#define RF95_FREQ 915.0
#define led       9       // Moteinos have LEDs on D9
#define FLASH_SS  8       // and FLASH SS on D8
#define RFM95_CS  10      //wd NSS pin 10 for rf95 radio on Moteino
#define RFM95_RST 9       //wd reset not used
#define RFM95_INT 2

#define SERIAL_BAUD 115200
#define LED_DURATION 400
#define BUFFER_LEN 50

// *************************************************************
// General variable declarations

char radioBuffer[BUFFER_LEN];                  // char array used to send data
long int ledCtr = 1L;                  // var used to store millis value so led can be non-blocking
uint8_t len = BUFFER_LEN;

//************************************ Load Drivers ****************************************
//  load driver instance of the radio and name it "rf95"
RH_RF95 rf95(RFM95_CS, RFM95_INT);


//************************************ SETUP START ******************************************
void setup()
{
  // Rocket Scream Mini Ultra Pro with the RFM95W only:
  // Ensure serial flash is not interfering with radio communication on SPI bus
  //  pinMode(4, OUTPUT);
  //  digitalWrite(4, HIGH);

  // Enable the led as an output
  pinMode(led, OUTPUT);  // initialize digital pin LED_BUILTIN as an output.

  // setup serial channel
  Serial.begin(SERIAL_BAUD);
  Serial.println("LoRa monitor start");

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
  Serial.println("Setup complete");
  Serial.println();

}

//************************************ Start Loop ************************************************************
void loop()
{
  if (rf95.available())
  {
 
    uint8_t len = sizeof(radioBuffer);
    if (rf95.recv((uint8_t *) radioBuffer, &len))
    {
      digitalWrite(led, HIGH);
      ledCtr = millis();
      
      for (int i = 0; i < 50; i++)
      {
        if (radioBuffer[i] == 93)
        {
          Serial.print(" in ");
          Serial.print(i + 1);
          Serial.print(" characters : ");
          break;
        }
        Serial.print(char(radioBuffer[i]));
      }

      Serial.print(" RSSI: ");
      Serial.println(rf95.lastRssi(), DEC);

    }
    else
    {
      Serial.println("recv failed");
    }
  }
  
  // turn the led off a second after transmission
  if (millis() > (ledCtr + LED_DURATION))
  {
    digitalWrite(led, LOW);
  }
  
}


