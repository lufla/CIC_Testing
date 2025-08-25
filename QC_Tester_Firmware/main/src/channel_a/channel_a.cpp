
#include "channel_a.h"
#include "../config/config.h"
#include "../cic_control/cic_adc.h"
#include <Wire.h>
#include <SPI.h>
#include <CAN.h>

namespace ChannelA {

    void initialize() {
        Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
        pinMode(CH_B_IO_EXPANDER_CS_PIN, OUTPUT);
        digitalWrite(CH_B_IO_EXPANDER_CS_PIN, HIGH);
        SPI.begin(IO_EXPANDER_SCLK_PIN, IO_EXPANDER_MISO_PIN, IO_EXPANDER_MOSI_PIN, -1);

        // Initialize the ADC for this channel
        CIC_ADC::initialize(CH_B_IO_EXPANDER_CS_PIN);
    }

    void setVCAN(byte bitPattern) {
        // SPI communication logic to control MCP23S08
        SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0));
        digitalWrite(CH_A_IO_EXPANDER_CS_PIN, LOW);
        SPI.transfer(0x40); // Device address write
        SPI.transfer(0x0A); // Address of GPIO Register (OLAT register might be better)
        SPI.transfer(bitPattern);
        digitalWrite(CH_A_IO_EXPANDER_CS_PIN, HIGH);
        SPI.endTransaction();

        StaticJsonDocument<128> response;
        response["status"] = "success";
        response["channel"] = "A";
        response["action"] = "set_vcan";
        response["pattern_sent"] = String(bitPattern, HEX);
        serializeJson(response, Serial);
        Serial.println();
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
        // I2C communication to set current via MCP4726
        StaticJsonDocument<128> response;
        response["status"] = "success";
        response["channel"] = "A";
        response["action"] = "set_current";
        response["current_set"] = current;
        serializeJson(response, Serial);
        Serial.println();
    }

    void initCAN(long baudrate) {
        CAN.setPins(CH_A_CAN_RX_PIN, CH_A_CAN_TX_PIN);
        if (!CAN.begin(baudrate)) {
            Serial.println("{\"status\":\"error\", \"channel\":\"A\", \"message\":\"CAN start failed\"}");
        } else {
            Serial.println("{\"status\":\"success\", \"channel\":\"A\", \"message\":\"CAN started\"}");
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