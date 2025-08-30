#include "src/spi_handler/master_spi_handler.h"
#include "src/spi_handler/slave_spi_handler.h"
#include <Wire.h>
#include "src/i2c_handler/i2c_handler.h"
#include "src/can_handler/can_handler.h"
#include "src/temperature_handler/temperature_handler.h" // Include the new temperature handler


// --- I2C Pin Definitions ---
const int I2C_SDA_PIN = 21;
const int I2C_SCL_PIN = 22;

// --- Role & UART Definitions ---
Role currentRole = UNKNOWN; // This is now the definitive declaration
#define MASTER_SLAVE_PIN 34
#define UART_SERIAL Serial1
#define UART_TX_PIN 17
#define UART_RX_PIN 16
#define UART_BAUD_RATE 115200

MasterSpiHandler* masterHandler = nullptr;
SlaveSpiHandler* slaveHandler = nullptr;
bool master_last_power_state = false;

// ####################################################################
// #                 TEMPERATURE SENSOR FUNCTION                      #
// ####################################################################
//
// All temperature functions have been moved to temperature_handler.cpp
//
// ####################################################################


// ####################################################################
// #                       MAIN LOGIC & LOOPS                         #
// ####################################################################

void slave_loop() {
  if (UART_SERIAL.available() > 0) {
    String command = UART_SERIAL.readStringUntil('\n');
    command.trim();

    if (command.startsWith("START_CAN_TEST ")) {
        int num_messages = command.substring(15).toInt();
        run_can_communication_test(num_messages);
    } else if (command == "GET_CAN_RESULTS") {
        char buf[100];
        snprintf(buf, sizeof(buf), "CAN_RESULTS:%d,%d,%d,%d", testResults.tx_ok, testResults.tx_fail, testResults.rx_ok, testResults.crosstalk);
        UART_SERIAL.println(buf);
    } else if (command == "CHECK_SPI_ADC") {
      AdcReadings r = slaveHandler->readAllAdcValues();
      char buf[100];
      snprintf(buf, sizeof(buf), "DATA:%.4f,%.4f,%.4f,%.4f", r.cic_v, r.cic_i, r.vcan_v, r.vcan_i);
      UART_SERIAL.println(buf);
    } else if (command == "READ_I2C_VOLTAGE_B") {
      float v = get_i2c_voltage();
      char buf[50];
      snprintf(buf, sizeof(buf), "I2C_VOLTAGE_B:%.4f", v);
      UART_SERIAL.println(buf);
    } else if (command.startsWith("SET_I2C_CURRENT ")) {
      uint16_t val = command.substring(16).toInt();
      set_i2c_load_current(val);
    } else if (command == "READ_TEMP") {
      float temp = get_temperature();
      char buf[50];
      snprintf(buf, sizeof(buf), "TEMP_B:%.2f", temp);
      UART_SERIAL.println(buf);
    }
  }
}

void master_loop() {
  if (Serial.available() > 0) {
    char cmdBuffer[50];
    int bytesRead = Serial.readBytesUntil('\n', cmdBuffer, sizeof(cmdBuffer) - 1);
    cmdBuffer[bytesRead] = '\0';

    int setting;
    uint16_t dacValue;
    int num_messages;

    if (sscanf(cmdBuffer, "RUN_CAN_TEST %d", &num_messages) == 1) {
        // 1. Command slave to start its test
        UART_SERIAL.printf("START_CAN_TEST %d\n", num_messages);

        // 2. Run our test simultaneously
        run_can_communication_test(num_messages);

        // 3. Request results from slave
        delay(100);
        while(UART_SERIAL.available() > 0) { UART_SERIAL.read(); } // Clear UART buffer
        UART_SERIAL.println("GET_CAN_RESULTS");

        bool received = false;
        unsigned long start_time = millis();
        while(millis() - start_time < 2000 && !received) {
            if(UART_SERIAL.available() > 0) {
                String response = UART_SERIAL.readStringUntil('\n');
                int s_tx_ok, s_tx_fail, s_rx_ok, s_crosstalk;
                if(sscanf(response.c_str(), "CAN_RESULTS:%d,%d,%d,%d", &s_tx_ok, &s_tx_fail, &s_rx_ok, &s_crosstalk) == 4) {
                    received = true;
                    Serial.println("CAN_TEST_PROGRESS: Received results from slave.");

                    // 4. Final validation
                    bool pass = (testResults.tx_fail == 0 && testResults.rx_ok >= num_messages && testResults.crosstalk == 0 &&
                                 s_tx_fail == 0 && s_rx_ok >= num_messages && s_crosstalk == 0);
                    const char* result_str = pass ? "PASS" : "FAIL";

                    Serial.printf("CAN_TEST_FINAL:%s:Master(tx_ok:%d,tx_fail:%d,rx_ok:%d,crosstalk:%d) Slave(tx_ok:%d,tx_fail:%d,rx_ok:%d,crosstalk:%d)\n",
                        result_str,
                        testResults.tx_ok, testResults.tx_fail, testResults.rx_ok, testResults.crosstalk,
                        s_tx_ok, s_tx_fail, s_rx_ok, s_crosstalk);
                }
            }
        }
        if(!received) Serial.println("CAN_TEST_FINAL:FAIL:No results response from slave.");

    } else if (strcmp(cmdBuffer, "READ_TEMP") == 0) {
        float master_temp = get_temperature();

        while(UART_SERIAL.available() > 0) { UART_SERIAL.read(); } // Clear UART buffer
        UART_SERIAL.println("READ_TEMP");

        bool received = false;
        float slave_temp = 99.00; // Default fail value
        unsigned long start_time = millis();
        while(millis() - start_time < 2000 && !received) {
            if(UART_SERIAL.available() > 0) {
                String response = UART_SERIAL.readStringUntil('\n');
                float temp_val;
                if(sscanf(response.c_str(), "TEMP_B:%f", &temp_val) == 1) {
                    slave_temp = temp_val;
                    received = true;
                }
            }
        }
        Serial.printf("TEMPERATURES:Master=%.2f,Slave=%.2f\n", master_temp, slave_temp);

    } else if (sscanf(cmdBuffer, "SET_VCAN_VOLTAGE %d", &setting) == 1) {
      bool current_power_state = (setting & 0x3) == 0x3;
      masterHandler->setVcanPower('A', (byte)setting);
      masterHandler->setVcanPower('B', (byte)setting);
      if (master_last_power_state && !current_power_state) { delay(300); } else { delay(100); }
      float v_a = masterHandler->readVcanVoltage('A');
      float v_b = masterHandler->readVcanVoltage('B');
      char buffer[50];
      snprintf(buffer, sizeof(buffer), "VCAN_DATA:%.4f,%.4f", v_a, v_b);
      Serial.println(buffer);
      master_last_power_state = current_power_state;
    } else if (sscanf(cmdBuffer, "SET_I2C_CURRENT %hu", &dacValue) == 1) {
      set_i2c_load_current(dacValue);
      UART_SERIAL.println(cmdBuffer);
      Serial.println("ACK_CURRENT_SET");
    } else if (strcmp(cmdBuffer, "READ_MASTER_SPI") == 0) {
      float v_a = masterHandler->readVcanVoltage('A');
      float i_a = masterHandler->readVcanCurrent('A');
      char buffer[50];
      snprintf(buffer, sizeof(buffer), "MASTER_SPI:%.4f,%.4f", v_a, i_a);
      Serial.println(buffer);
    } else if (strcmp(cmdBuffer, "READ_I2C_VOLTAGE_A") == 0) {
      float v = get_i2c_voltage();
      char buf[50];
      snprintf(buf, sizeof(buf), "I2C_VOLTAGE_A:%.4f", v);
      Serial.println(buf);
    } else {
      UART_SERIAL.println(cmdBuffer);
    }
  }

  if (UART_SERIAL.available() > 0) {
    String response = UART_SERIAL.readStringUntil('\n');
    response.trim();
    Serial.println(response);
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\nESP32 Unified Firmware Initializing...");
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
  init_temperature_sensor(); // Initialize the temperature sensor

  pinMode(MASTER_SLAVE_PIN, INPUT_PULLUP);
  delay(10);
  if (digitalRead(MASTER_SLAVE_PIN) == LOW) {
    currentRole = MASTER;
    pinMode(MASTER_SLAVE_PIN, INPUT_PULLDOWN);
    Serial.println("Role: MASTER");
    masterHandler = new MasterSpiHandler();
    masterHandler->begin();
    Serial.println("Master hardware initialized.");
  } else {
    currentRole = SLAVE;
    Serial.println("Role: SLAVE");
    slaveHandler = new SlaveSpiHandler();
    slaveHandler->begin();
  }
  UART_SERIAL.begin(UART_BAUD_RATE, SERIAL_8N1, UART_RX_PIN, UART_TX_PIN);
}

void loop() {
  if (currentRole == MASTER) master_loop();
  else if (currentRole == SLAVE) slave_loop();
}

