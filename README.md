# parking-poc

This repository is to hold files and code related to a proof of concept for location of parked vehicles for an organisation

Components include:
LoRa_car - a sketch running on a lora enabled moteino which broadcasts its registration on a regular basis

LoRa_parking - a sketch runningon another lora motino listening for vehicle broadcasts. I received a new message is sent to a gateway
      lora moteino with te registration and RSSI (signal strength) received from LoRa_car
	
LoRa_gateway - anothe LoRa enabled moteino that listens for messages from the parking moteinos. Sends the data over a serial port to
      NodeRed on a raspberry Pi
	
NodeRed running on a Raspberry Pi that is WiFi connected. Receives the serial message from the Moteino and structures a MQTT message
      on CloudMqtt
	
CloudMqtt - a general internet available MQTT broker used for this PoC

NodeRed on (another) Raspberry Pi. not in the parking garage. Recives vehicle updates from CloudMqtt and stores info in a database (?????)
