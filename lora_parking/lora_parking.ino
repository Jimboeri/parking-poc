// **********************************************************************************
// lora_parking
// sketch for in vehicle LoRa beacon
// Version 0.1
// **********************************************************************************

#include <SPI.h>
#include <RH_RF95.h>
#include <EEPROM.h>         // used to store parameters, in specific the time interval between reading transmissions

#define INITIAL_SETUP // uncomment this for an initial setup of a moteino

// define device specific settings
#define RF95_FREQ 915.0
#define led       9       // Moteinos have LEDs on D9
#define FLASH_SS  8       // and FLASH SS on D8
#define RFM95_CS  10      //wd NSS pin 10 for rf95 radio on Moteino
#define RFM95_RST 9       //wd reset not used
#define RFM95_INT 2
#define RFM95_POWER 20
#define BUFFER_LEN 50

#define SERIAL_BAUD 115200
#define LENGTH_PARK_REF 10
#define LED_DURATION 400

// ***********************************************************
// EEPROM Parameter offsets
#define PARK_NODE 101
#define PARK_REF 110
//#define TXDELAY 140

// *************************************************************
// General variable declarations

char radioBuffer[BUFFER_LEN];     // char array used to receive data
char sendBuffer[BUFFER_LEN];      // char array used to send data
long int timeCtr = 1000L;         // var used to store millis value so led can be non-blocking
char park_ref[] = "TEST";         // var to hold rego
int park_node = 1001;           // var to hold node number
int txDelay = 3;                  // var hold seconds between transmissions
int strIndex = 0;                 // var used for string manipulation
int inRSSI = 0;                   // var for incoming signal strength

// **************************************************************
// Declarations for serial comms
String inputString = "";         // a string to hold incoming data
boolean stringComplete = false;  // whether the string is complete
boolean valid_input = false;
String serialInput = "";
String sRef = "";
int iNode = 0;

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
  Serial.println("Parking module start");

  // if this is an initial set up, store defaults in EEPROM, otherwise read them
#ifdef INITIAL_SETUP
  EEPROM.put(PARK_NODE, park_node);                     // initial set up node
  EEPROM.put(PARK_REF, park_ref);     // initial set up gateway
  //EEPROM.put(TXDELAY, txDelay);               // initial set up gateway
  Serial.println("Initial setup - values stored in EEPROM");
#endif
  EEPROM.get(PARK_NODE, park_node);                     // set up node
  EEPROM.get(PARK_REF, park_ref);     // registration
  //EEPROM.get(TXDELAY, txDelay);               // transmit delay
  Serial.print("Node: ");
  Serial.print(park_node);
  Serial.print(" and parking reference : ");
  Serial.println(park_ref);
  //Serial.print("Seconds delay between transmissions : ");
  //Serial.println(txDelay);

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

      if (radioBuffer[0] == 'V')
      {
        Serial.print("Vehicle message received from node ");
        // move the input to a string for easier management
        String radioIn = radioBuffer;

        // the string starts "V," after that is the vehicle node number. Find the posn of the next comma
        strIndex = radioIn.indexOf(',', 2);
        // Store the vehicle node
        String inNode = radioIn.substring(2, strIndex);
        Serial.print(inNode);
        Serial.print(" with registration ");

        String inReg = radioIn.substring(strIndex + 1, len);
        Serial.print(inReg);
        Serial.print(" RSSI is ");

        inRSSI = rf95.lastRssi();
        Serial.println(inRSSI);

        send_radio_msg(inReg, inNode, inRSSI);

      }
    }
  }
  if ((timeCtr + LED_DURATION) < millis())
  {
    digitalWrite(led, LOW);
  }
  //process any serial input
  serialEvent();            // call the function
  if (stringComplete)       // keep all serial processing in one place
    process_serial();
}

//*******************************************************************************************
// Generic routine for sending a radio message
// the vehicle registration is the only parameter
int strLen = 0;
char cRego[8];
char cVehNode[8];
void send_radio_msg(String rego, String inNode, int inRSSI)
{
  // make a char array for the rego
  strLen = rego.length();
  //char cRego[strLen];
  rego.toCharArray(cRego, strLen + 1);
  Serial.print("Rego to send is ");
  Serial.println(cRego);

  // make a char array for the vehicle node
  strLen = inNode.length();
  inNode.toCharArray(cVehNode, strLen + 1);
  Serial.print("Vehicle node to send is ");
  Serial.print(cVehNode);
  Serial.print(" original was ");
  Serial.println(inNode);

  // sprintf creates the string to transmit
  sprintf(sendBuffer, "P,%s,%s,%d,%d,%s", cVehNode, cRego, inRSSI, park_node, park_ref);
  Serial.print("Radio transmission should be ");
  Serial.println(sendBuffer);
  Serial.println("******************************************************************************");
  // in the send command '(uint8_t *) sendBuffer' casts the proper data type
  // example taken from thread https://lowpowerlab.com/forum/rf-range-antennas-rfm69-library/sending-first-packet-with-rfm95-lora-module/

  //jw, the sketch seems to restart as soon as the message is sent, need to fix this
  rf95.send((uint8_t *) sendBuffer, sizeof(sendBuffer));
  rf95.waitPacketSent();

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
    //serialInput.toUpperCase();
    Serial.println("Test start");
    Serial.println(serialInput);
    Serial.print("Parking reference update - ");

    serialInput.toCharArray(park_ref, serialInput.length());
    EEPROM.put(PARK_REF, park_ref);     // update reference
    Serial.println(park_ref);
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
      park_node = iNode;
      EEPROM.put(PARK_NODE, park_node);                     // update node
      Serial.println(park_node);
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

