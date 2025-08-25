#include "cic_adc.h"
#include <SPI.h>

namespace CIC_ADC {

    // Helper function to manage SPI transactions
    void executeSPITransaction(int cs_pin, uint8_t* data, size_t len) {
        SPI.beginTransaction(SPISettings(2000000, MSBFIRST, SPI_MODE0));
        digitalWrite(cs_pin, LOW);
        for (size_t i = 0; i < len; ++i) {
            data[i] = SPI.transfer(data[i]);
        }
        digitalWrite(cs_pin, HIGH);
        SPI.endTransaction();
    }

    // Simplified initialization based on the python script's startup sequence
    void initialize(int cs_pin) {
        // 1. Wake up ADC with 15 dummy bytes + 0xFE
        uint8_t wakeup_cmd[16];
        for(int i=0; i<15; i++) wakeup_cmd[i] = 0xFF;
        wakeup_cmd[15] = 0xFE;
        executeSPITransaction(cs_pin, wakeup_cmd, 16);
        delay(10);

        // 2. Reset the ADC
        uint8_t reset_cmd[] = {0x03, 0x00, 0x00, 0x80};
        executeSPITransaction(cs_pin, reset_cmd, 4);
        delay(100);
        reset_cmd[3] = 0x00; // Clear reset bit
        executeSPITransaction(cs_pin, reset_cmd, 4);
        delay(10);

        // 3. Configure for 4 CSRs setups
        uint8_t config_cmd[] = {0x03, 0x00, 0x30, 0x00};
        executeSPITransaction(cs_pin, config_cmd, 4);
        delay(10);

        // NOTE: For a full implementation, the write_csrs and calibration
        // steps from the python script would be added here.
        // For this example, we assume default calibration is sufficient.
    }

    float readChannel(int cs_pin, const char* channel_name) {
        uint8_t cmd_byte = 0;
        // Map channel name to the ADC command byte from the python script
        // self.__address_byte = [0x80, 0x88, 0x90, 0x98, 0x80]
        // self.channel_value = ("UH", "IMON", "VCAN", "Temperature")
        if (strcmp(channel_name, "VCAN") == 0)      cmd_byte = 0x90; // Channel 2
        else if (strcmp(channel_name, "IMON") == 0) cmd_byte = 0x88; // Channel 1
        else if (strcmp(channel_name, "UH") == 0)   cmd_byte = 0x80; // Channel 0
        else if (strcmp(channel_name, "TEMP") == 0) cmd_byte = 0x98; // Channel 3
        else return -999.0; // Invalid channel

        uint8_t read_cmd[] = {cmd_byte, 0x00, 0x00, 0x00, 0x00};

        // The ADC needs multiple cycles to return a valid reading for the correct channel
        float result = -1.0;
        for (int i=0; i < 5; i++) {
            executeSPITransaction(cs_pin, read_cmd, 5);
            // The channel ID is in the upper bits of the last byte
            int response_channel = (read_cmd[4] >> 4) & 0x03;
            if (response_channel == 2 && strcmp(channel_name, "VCAN") == 0) { // Found VCAN
                int16_t raw_val = (read_cmd[2] << 8) | read_cmd[3];
                // V = (raw * V_ref / 2^16) -> V = raw * (2.5 / 65536) -> V = raw * 0.00003814697
                // The python script divides by 1000 for some channels, we assume VCAN is direct voltage
                result = (float)raw_val * 0.00003814697;
                break; // Exit loop once we have the correct channel reading
            }
            delay(50); // Wait for next conversion
        }
        return result;
    }
}
