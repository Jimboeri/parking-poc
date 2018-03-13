// **********************************************************************************
// WD Version 0.3
// nrf8001 Bluetooth that might work with RFM95 9.15mhz LoRa
// **********************************************************************************

#include <SPI.h>            //included in Arduino IDE (www.arduino.cc)
#include <RH_RF95.h>
#include <EEPROM.h>         // used to store parameters, in specific the time interval between reading transmissions

#define INITIAL_SETUP // uncomment this for an initial setup of a moteino

#define RF95_FREQ 915.0
#define led       9       // Moteinos have LEDs on D9
#define FLASH_SS  8       // and FLASH SS on D8
#define RFM95_CS  10      //wd NSS pin 10 for rf95 radio on Moteino
#define RFM95_RST 9       //wd reset not used
#define RFM95_INT 2

#define SERIAL_BAUD 115200
#define LENGTH_REGO 6

// ***********************************************************
// EEPROM Parameter offsets
#define NODE 101
#define REGISTRATION 110

// *************************************************************
// General variable declarations

char sendBuffer[50];                  // char array used to send data
long int timeCtr = 0L;                // var used to store millis value so led can be non-blocking
char registration[] = "TEST01";       // var to hold rego
int node = 1;                         // var to hold node number

//************************************ Load Drivers ****************************************
// wd load driver instance of the radio and name it "rf95"
//RH_RF95 rf95;
//RH_RF95 rf95(RFM95_CS, RFM95_INT);
RH_RF95 rf95(RFM95_CS, RFM95_INT);


//************************************ SETUP START ******************************************
void setup()
{
  // Enable the led as an output
  pinMode(led, OUTPUT);  // initialize digital pin LED_BUILTIN as an output.

  Serial.begin(SERIAL_BAUD);
  Serial.println("Car module start");

  // if this is an initial set up, store defaults in EEPROM, otherwise read them
#ifdef INITIAL_SETUP
  EEPROM.put(NODE, node);              // temp to set up node
  EEPROM.put(REGISTRATION, registration);        // temp to set up gateway
#endif
  EEPROM.get(NODE, node);              // temp to set up node
  EEPROM.get(REGISTRATION, registration);        // temp to set up gateway

  Serial.print("Node: ");
  Serial.print(node);
  Serial.print(" and registration : ");
  Serial.println(registration);

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

  // If you are using RFM95/96/97/98 modules which uses the PA_BOOST transmitter pin, then
  // you can set transmitter powers from 5 to 23 dBm:
  rf95.setTxPower(5, false);
  //wd rf95.setModemConfig(RH_RF95::Bw31_25Cr48Sf512); //configure modem for long range?
  //wd rf95.printRegisters();  //Print all the RFM95 register values
}


// wd variables
//wd char BLEstr[10];
//char buffer[50];

//char* Nodestr = "AKCCBLE"; //first chracter in string, node ID
//char* BLEstr = "Test";     //BLE chracter in string
//char* Endstr = "";     //last chracter in string
//byte sendLen;

//uint8_t buf[RH_RF95_MAX_MESSAGE_LEN]; //wd keep outside loop to avoid replys in klingon

//************************************ Start Loop ************************************************************
void loop()
{


  //wd rf95.send((uint8_t *) buffer, sendLen); // wd works kind of
  //rf95.send(buffer, sendLen);
  send_radio_msg("AAA555");


  // wd flash the led at the end of the loop.
  digitalWrite(led, HIGH);
  delay(100);
  digitalWrite(led, LOW);
  delay(6000);
  //
}

//*******************************************************************************************
// Generic routine for sending a radio message
// the vehicle registration is the only parameter

void send_radio_msg(char rego[])
{
  // sprintf creates the string to transmit
  sprintf(sendBuffer, "%s,%s", "V", rego);
  Serial.print("Send Data: ");
  Serial.print(sendBuffer);
  Serial.println();

  // in the send command '(uint8_t *) sendBuffer' casts the proper data type
  // example taken from thread https://lowpowerlab.com/forum/rf-range-antennas-rfm69-library/sending-first-packet-with-rfm95-lora-module/
  if (rf95.send((uint8_t *) sendBuffer, strlen(sendBuffer)))
    Serial.println("Sent OK");
  else
    Serial.println("Send error");

}

