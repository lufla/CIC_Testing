#include "channel_b.h"
#include "../config/config.h"
#include "../cic_control/cic_adc.h"
#include <Wire.h>
#include <SPI.h>
#include <CAN.h>

// This file runs on the SLAVE.
namespace ChannelB {

    void initialize() {
        Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
        pinMode(CH_B_IO_EXPANDER_CS_PIN, OUTPUT);
        digitalWrite(CH_B_IO_EXPANDER_CS_PIN, HIGH);
        SPI.begin(IO_EXPANDER_SCLK_PIN, IO_EXPANDER_MISO_PIN, IO_EXPANDER_MOSI_PIN, -1);

        // Initialize the ADC for this channel
        CIC_ADC::initialize(CH_B_IO_EXPANDER_CS_PIN);
    }

    void setVCAN(byte bitPattern) {
        SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0));
        digitalWrite(CH_B_IO_EXPANDER_CS_PIN, LOW);
        SPI.transfer(0x40);
        SPI.transfer(0x0A);
        SPI.transfer(bitPattern);
        digitalWrite(CH_B_IO_EXPANDER_CS_PIN, HIGH);
        SPI.endTransaction();

        StaticJsonDocument<128> response;
        response["status"] = "success";
        response["channel"] = "B";
        response["action"] = "set_vcan";
        response["pattern_sent"] = String(bitPattern, HEX);
        serializeJson(response, SLAVE_SERIAL);
        SLAVE_SERIAL.println();
    }

    void readVCAN() {
        // Use the new module to read the VCAN voltage
        float voltage = CIC_ADC::readChannel(CH_B_IO_EXPANDER_CS_PIN, "VCAN");

        StaticJsonDocument<128> response;
        response["status"] = "success";
        response["channel"] = "B";
        response["data"]["voltage_measured"] = voltage;
        serializeJson(response, SLAVE_SERIAL);
        SLAVE_SERIAL.println();
    }
    
    void setCurrent(float current) {
        StaticJsonDocument<128> response;
        response["status"] = "success";
        response["channel"] = "B";
        response["action"] = "set_current";
        response["current_set"] = current;
        serializeJson(response, SLAVE_SERIAL);
        SLAVE_SERIAL.println();
    }

    void initCAN(long baudrate) {
        CAN.setPins(CH_B_CAN_RX_PIN, CH_B_CAN_TX_PIN);
        if (!CAN.begin(baudrate)) {
            SLAVE_SERIAL.println("{\"status\":\"error\", \"channel\":\"B\", \"message\":\"CAN start failed\"}");
        } else {
            SLAVE_SERIAL.println("{\"status\":\"success\", \"channel\":\"B\", \"message\":\"CAN started\"}");
        }
    }

    void executeCommand(JsonDocument& doc) {
        const char* command = doc["command"];

        if (strcmp(command, "read_voltage") == 0) readVCAN();
        else if (strcmp(command, "read_voltage") == 0) readVCAN();
        else if (strcmp(command, "set_current") == 0) setCurrent(doc["current"]);
        else if (strcmp(command, "init_can") == 0) initCAN(doc["baudrate"]);
    }
}
