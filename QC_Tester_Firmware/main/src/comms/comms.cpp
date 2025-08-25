#include "comms.h"
#include <ArduinoJson.h>
#include "../config/config.h"
#include "../channel_a/channel_a.h"
#include "../channel_b/channel_b.h"
#include "../cic_control/cic_adc.h" // Include ADC module for direct access

// Master: Handles commands from Python script
void handlePCCommand(String cmd) {
    StaticJsonDocument<256> doc;
    DeserializationError error = deserializeJson(doc, cmd);

    if (error) {
        Serial.println("{\"status\":\"error\", \"message\":\"JSON deserialize failed\"}");
        return;
    }

    const char* command = doc["command"];

    // *** NEW: Command to read both voltages at once ***
    if (strcmp(command, "read_all_voltages") == 0) {
        // 1. Read local channel A voltage directly
        float voltage_a = CIC_ADC::readChannel(CH_A_IO_EXPANDER_CS_PIN, "VCAN");

        // 2. Send command to slave to read channel B voltage
        SLAVE_SERIAL.println("{\"command\":\"read_voltage\", \"channel\":\"B\"}");

        // 3. Wait for slave's response
        long startTime = millis();
        String slaveResponse = "";
        while (!SLAVE_SERIAL.available() && (millis() - startTime < 1000)) { // 1s timeout
            delay(10);
        }
        if (SLAVE_SERIAL.available()) {
            slaveResponse = SLAVE_SERIAL.readStringUntil('\n');
        }

        // 4. Parse slave's response and combine results
        StaticJsonDocument<128> slaveDoc;
        float voltage_b = -1.0;
        if (slaveResponse.length() > 0) {
            deserializeJson(slaveDoc, slaveResponse);
            voltage_b = slaveDoc["data"]["voltage_measured"];
        }

        // 5. Send combined JSON back to PC
        StaticJsonDocument<256> finalResponse;
        finalResponse["status"] = "success";
        finalResponse["command"] = "read_all_voltages";
        finalResponse["data"]["voltage_a"] = voltage_a;
        finalResponse["data"]["voltage_b"] = voltage_b;
        serializeJson(finalResponse, Serial);
        Serial.println();
        return; // Command handled
    }

    // --- Original logic for single-channel commands ---
    const char* channel = doc["channel"];

    if (strcmp(channel, "A") == 0) {
        ChannelA::executeCommand(doc);
    } else if (strcmp(channel, "B") == 0) {
        // Forward command to Slave
        SLAVE_SERIAL.println(cmd);

        // Wait for slave response and forward it to PC
        long startTime = millis();
        while (!SLAVE_SERIAL.available() && (millis() - startTime < 2000)) { // 2s timeout
            delay(10);
        }
        if (SLAVE_SERIAL.available()) {
            String response = SLAVE_SERIAL.readStringUntil('\n');
            Serial.println(response);
        } else {
            Serial.println("{\"status\":\"error\", \"message\":\"No response from slave\"}");
        }
    }
}

// Slave: Handles commands from Master
void handleMasterCommand(String cmd) {
    StaticJsonDocument<256> doc;
    DeserializationError error = deserializeJson(doc, cmd);

    if (error) {
        SLAVE_SERIAL.println("{\"status\":\"error\", \"channel\":\"B\", \"message\":\"JSON deserialize failed\"}");
        return;
    }
    
    // On the slave, all commands are for Channel B
    ChannelB::executeCommand(doc);
}
