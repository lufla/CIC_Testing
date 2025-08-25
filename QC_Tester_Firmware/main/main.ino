// QC_Tester_Firmware.ino
//TODO generate a requirements file and a read me fot the installation of versions etc.
//TOO make seperate file for reading the ADC adn DACs

/*
  CIC Quality Control Tester Firmware

  HOW TO USE:
  1. Open this file in the Arduino IDE.
  2. Create new tabs for each of the other files (.h and .cpp).
     - config.h
     - roles.h
     - comms.h & comms.cpp
     - channel_a.h & channel_a.cpp
     - channel_b.h & channel_b.cpp
     - slave_flasher.h & slave_flasher.cpp
  3. Go to Sketch > Include Library > Manage Libraries... and install the following:
     - "Adafruit BusIO"
     - "Adafruit MCP23017" (Note: We need MCP23S08, but this library might be adaptable or you may need a specific one)
     - "Adafruit ADS1X15"
     - "MCP4725" by Rob Tillaart
     - "CAN" by Sandeep Mistry
     - "ArduinoJson" by Benoit Blanchon
  4. Select your ESP32 board from the Tools menu.
  5. Compile and upload.
*/
//TODO include here and in the arduino sheeld the libarys localy, so that they dont get lost in the future
//Manual changes made in ESP32SJA1000.h (CAN), because an old libary was not available anymore

#include <Arduino.h>
#include <ArduinoJson.h>
#include "src/config/config.h"
#include "src//roles/roles.h"
#include "src/comms/comms.h"
#include "src/channel_a/channel_a.h"
#include "src/channel_b/channel_b.h"

Role currentRole = UNDEFINED;

void setup() {
    // Start PC communication
    Serial.begin(PC_SERIAL_BAUD);
    while (!Serial) { delay(10); }

    Serial.println("QC Tester Firmware Initialized and Ready.");

    // Determine role
    pinMode(ROLE_SELECT_PIN, INPUT_PULLUP);
    delay(100); // Allow pin to stabilize
    if (digitalRead(ROLE_SELECT_PIN) == LOW) {
        currentRole = SLAVE;
    } else {
        currentRole = MASTER;
    }

    if (currentRole == MASTER) {
        // Initialize communication with Slave
        SLAVE_SERIAL.begin(SLAVE_SERIAL_BAUD, SERIAL_8N1, SLAVE_RX_PIN, SLAVE_TX_PIN);
        ChannelA::initialize();
    } else { // SLAVE
        // The slave's primary Serial is connected to the Master's SLAVE_SERIAL pins.
        SLAVE_SERIAL.begin(PC_SERIAL_BAUD); // Slaves use their main serial to talk to master
        ChannelB::initialize();
    }
}

void loop() {
    if (currentRole == MASTER) {
        // Master listens for commands from the Python script
        if (Serial.available()) {
            String incoming = Serial.readStringUntil('\n');
            handlePCCommand(incoming);
        }
        // Master also listens for any messages from the slave
        if (SLAVE_SERIAL.available()) {
            String slaveMsg = SLAVE_SERIAL.readStringUntil('\n');
            // Forward slave messages to PC for logging/debugging
            Serial.println(slaveMsg);
        }

    } else { // SLAVE
        // Slave listens for commands from the Master on its main Serial port
        if (SLAVE_SERIAL.available()) {
            String incoming = SLAVE_SERIAL.readStringUntil('\n');
            handleMasterCommand(incoming);
        }
    }
}
