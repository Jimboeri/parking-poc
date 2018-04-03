// **********************************************************************************
// lora_car
// sketch for in vehicle LoRa beacon
// Version 0.3
// **********************************************************************************

#include <SPI.h>            //included in Arduino IDE (www.arduino.cc)
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
#define RFM95_POWER 5

#define SERIAL_BAUD 115200
#define LENGTH_REGO 6
#define LED_DURATION 400

// ***********************************************************
// EEPROM Parameter offsets
#define NODE 101
#define REGISTRATION 110
#define TXDELAY 120

// *************************************************************
// General variable declarations

char sendBuffer[50];                  // char array used to send data
long int timeCtr = 1000L;                // var used to store millis value so led can be non-blocking
char registration[] = "VAN002";       // var to hold rego
int node = 12;                         // var to hold node number
int txDelay = 15;                     // var hold seconds between transmissions

// **************************************************************
// Declarations for serial comms
String inputString = "";         // a string to hold incoming data
boolean stringComplete = false;  // whether the string is complete
boolean valid_input = false;
String serialInput = "";
String sRego = "";
int iNode = 0;

//************************************ Load Drivers ****************************************
//  load driver instance of the radio and name it "rf95"
RH_RF95 rf95(RFM95_CS, RFM95_INT);


//************************************ SETUP START ******************************************
void setup()
{
  // Enable the led as an output
  pinMode(led, OUTPUT);  // initialize digital pin LED_BUILTIN as an output.

  // setup serial channel
  Serial.begin(SERIAL_BAUD);
  Serial.println("Car module start");

  // if this is an initial set up, store defaults in EEPROM, otherwise read them
#ifdef INITIAL_SETUP
  EEPROM.put(NODE, node);                     // initial set up node
  EEPROM.put(REGISTRATION, registration);     // initial set up gateway
  EEPROM.put(TXDELAY, txDelay);               // initial set up gateway
  Serial.println("Initial setup - values stored in EEPROM");
#endif
  EEPROM.get(NODE, node);                     // set up node
  EEPROM.get(REGISTRATION, registration);     // registration
  EEPROM.get(TXDELAY, txDelay);               // transmit delay
  Serial.print("Node: ");
  Serial.print(node);
  Serial.print(" and registration : ");
  Serial.println(registration);
  Serial.print("Seconds delay between transmissions : ");
  Serial.println(txDelay);

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
  rf95.setTxPower(RFM95_POWER, false);
  Serial.print("Set power level to: "); Serial.println(RFM95_POWER);
  //wd rf95.setModemConfig(RH_RF95::Bw31_25Cr48Sf512); //configure modem for long range?

  Serial.println("Setup complete");
  Serial.println();

}


//************************************ Start Loop ************************************************************
void loop()
{

  // Serial.println(millis());
  // check to see if time counter has been exceeded
  if (millis() > (timeCtr + (txDelay * 1000)))
  {
    //Serial.println("Time to send a message");
    send_radio_msg(registration);
    timeCtr = millis();
    digitalWrite(led, HIGH);
  }

  // turn the led off a second after transmission
  if (millis() > (timeCtr + LED_DURATION))
  {
    digitalWrite(led, LOW);
  }

  // this covers situations where the millis counter comes back to 0, every 3 months or so
  if (millis() < (timeCtr - 1))
  {
    timeCtr = millis();
  }
  //process any serial input
  serialEvent();            // call the function
  if (stringComplete)       // keep all serial processing in one place
    process_serial();

}

//*******************************************************************************************
// Generic routine for sending a radio message
// the vehicle registration is the only parameter

void send_radio_msg(char rego[])
{
  // sprintf creates the string to transmit
  sprintf(sendBuffer, "%s,%d,%s", "V", node, rego);
  Serial.print("Send Data: ");
  Serial.print(sendBuffer);
  Serial.println();

  // in the send command '(uint8_t *) sendBuffer' casts the proper data type
  // example taken from thread https://lowpowerlab.com/forum/rf-range-antennas-rfm69-library/sending-first-packet-with-rfm95-lora-module/
  if (!rf95.send((uint8_t *) sendBuffer, strlen(sendBuffer)))
    Serial.println("Send error");

}

// *************************************************************************************************
void serialEvent() {
  while (Serial.available()) { // keep on reading while info is available
    // get the new byte:
    char inByte = Serial.read();

    // add it to the inputString:
    if ((inByte >= 65 && inByte <= 90) || (inByte >= 97 && inByte <= 122) || (inByte >= 48 && inByte <= 57) || inByte == 43 || inByte == 44 || inByte == 45 || inByte == 46 || inByte == 61 || inByte == 63) {
      inputString.concat(inByte);
    }
    if (inByte == 10 || inByte == 13) {
      // user hit enter or such like
      Serial.println(inputString);
      stringComplete = true;
    }
  }
}

// ************************************************************************************************
void process_serial()
{
  valid_input = false;

  // A serial input starting with 'R-' is a registration update
  if (inputString.substring(0, 2) == "R-")
  {
    serialInput = inputString.substring(2);
    serialInput.toUpperCase();
    // Make sure it is at least 6 characters
    while (serialInput.length() < LENGTH_REGO)
    {
      serialInput.concat(" ");
    }
    // Reduce length if too long
    sRego = serialInput.substring(0, LENGTH_REGO);
    Serial.print("Rego update - ");

    sRego.toCharArray(registration, LENGTH_REGO + 1);
    EEPROM.put(REGISTRATION, registration);     // update registration
    Serial.println(registration);
    valid_input = true;
  }

  // A serial input starting with 'N-' is a node number update
  if (inputString.substring(0, 2) == "N-")
  {
    serialInput = inputString.substring(2);
    iNode = serialInput.toInt();

    // 0 if return if string is invalid
    if (iNode != 0)
    {
      Serial.print("Node number update - ");
      node = iNode;
      EEPROM.put(NODE, node);                     // update node
      Serial.println(node);
      valid_input = true;
    }
  }

  if (!valid_input)
  {
    Serial.println("Invalid serial input");
    Serial.println("To update the registration number, enter R-XXXNNN where XXXNNN is the registration number");
    Serial.println("To update the node number, enter N-nnnn where nnnn is the node nuber number");
    Serial.println("Thanks from the Auckland Council IoT team");
    Serial.println();
  }

  // clear the string:
  inputString = "";
  stringComplete = false;
}

