#include "src/spi_handler/master_spi_handler.h"
#include "src/spi_handler/slave_spi_handler.h"
#include <Wire.h>
#include "src/i2c_handler/i2c_handler.h"
#include "driver/twai.h" // ESP-IDF CAN Driver

// --- I2C Pin Definitions ---
const int I2C_SDA_PIN = 21;
const int I2C_SCL_PIN = 22;

// --- CAN Pin Definitions ---
#define CAN_A_TX_PIN 0
#define CAN_A_RX_PIN 2
#define CAN_B_TX_PIN 5
#define CAN_B_RX_PIN 4

// --- Role & UART Definitions ---
enum Role { UNKNOWN, MASTER, SLAVE };
Role currentRole = UNKNOWN;
#define MASTER_SLAVE_PIN 34
#define UART_SERIAL Serial1
#define UART_TX_PIN 17
#define UART_RX_PIN 16
#define UART_BAUD_RATE 115200

MasterSpiHandler* masterHandler = nullptr;
SlaveSpiHandler* slaveHandler = nullptr;
bool master_last_power_state = false;

// ####################################################################
// #                         CAN TEST FUNCTIONS                       #
// ####################################################################

/**
 * @brief Robustly installs and starts the TWAI driver for a specific channel.
 * Ensures the driver is fully stopped and uninstalled before re-initializing
 * to prevent race conditions and version compatibility issues.
 * @param channel 'A' or 'B'.
 * @return esp_err_t ESP_OK on success, ESP_FAIL on failure.
 */
esp_err_t twai_install_for_channel(char channel) {
    twai_status_info_t status;
    // If getting the status returns ESP_OK, it means the driver is installed and needs to be handled.
    if (twai_get_status_info(&status) == ESP_OK) {
        // If the driver is running, it must be stopped before uninstalling.
        if (status.state == TWAI_STATE_RUNNING) {
            if (twai_stop() != ESP_OK) {
                Serial.println("CAN_TEST_FINAL:FAIL:Failed to stop driver");
                return ESP_FAIL;
            }
        }
        // Now that the driver is stopped, it's safe to uninstall.
        if (twai_driver_uninstall() != ESP_OK) {
            Serial.println("CAN_TEST_FINAL:FAIL:Driver uninstall failed");
            return ESP_FAIL;
        }
    }
    // A brief delay allows hardware resources to be fully released.
    delay(50);

    // Now, with a clean slate, configure and install the driver for the selected channel.
    gpio_num_t tx_pin = (channel == 'A') ? (gpio_num_t)CAN_A_TX_PIN : (gpio_num_t)CAN_B_TX_PIN;
    gpio_num_t rx_pin = (channel == 'A') ? (gpio_num_t)CAN_A_RX_PIN : (gpio_num_t)CAN_B_RX_PIN;
    
    twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(tx_pin, rx_pin, TWAI_MODE_NORMAL);
    g_config.alerts_enabled = TWAI_ALERT_NONE;
    twai_timing_config_t t_config = TWAI_TIMING_CONFIG_125KBITS();
    twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

    if (twai_driver_install(&g_config, &t_config, &f_config) != ESP_OK) {
        Serial.println("CAN_TEST_FINAL:FAIL:Driver install failed");
        return ESP_FAIL;
    }
    
    if (twai_start() != ESP_OK) {
        Serial.println("CAN_TEST_FINAL:FAIL:Driver start failed");
        return ESP_FAIL;
    }
    return ESP_OK;
}


/**
 * @brief Performs a comprehensive CAN communication and crosstalk test.
 * @param num_messages The number of messages to send and verify on each channel.
 * @return true if the test passes, false otherwise.
 */
bool run_can_crosstalk_test(int num_messages) {
    Serial.println("CAN_TEST_PROGRESS: Starting CAN test...");
    
    for (int i = 0; i < num_messages; i++) {
        twai_message_t tx_message;
        tx_message.extd = 0;
        tx_message.rtr = 0;
        tx_message.data_length_code = 1;
        tx_message.data[0] = i % 256;

        // --- Test Channel A and check for crosstalk on B ---
        if (twai_install_for_channel('A') != ESP_OK) return false;
        tx_message.identifier = 0x581;
        if (twai_transmit(&tx_message, pdMS_TO_TICKS(100)) != ESP_OK) {
            Serial.println("CAN_TEST_FINAL:FAIL:TX failed on Channel A");
            return false;
        }
        twai_message_t rx_a;
        if (twai_receive(&rx_a, pdMS_TO_TICKS(200)) != ESP_OK || rx_a.identifier != 0x701) {
            Serial.println("CAN_TEST_FINAL:FAIL:No/Wrong response on Channel A");
            return false;
        }

        if (twai_install_for_channel('B') != ESP_OK) return false;
        twai_message_t rx_b_xtalk;
        if (twai_receive(&rx_b_xtalk, pdMS_TO_TICKS(20)) == ESP_OK) {
            Serial.printf("CAN_TEST_FINAL:FAIL:Crosstalk! Got 0x%lX on B when sending on A\n", rx_b_xtalk.identifier);
            return false;
        }
        
        // --- Test Channel B and check for crosstalk on A ---
        if (twai_install_for_channel('B') != ESP_OK) return false;
        tx_message.identifier = 0x582;
        if (twai_transmit(&tx_message, pdMS_TO_TICKS(100)) != ESP_OK) {
            Serial.println("CAN_TEST_FINAL:FAIL:TX failed on Channel B");
            return false;
        }
        twai_message_t rx_b;
        if (twai_receive(&rx_b, pdMS_TO_TICKS(200)) != ESP_OK || rx_b.identifier != 0x702) {
            Serial.println("CAN_TEST_FINAL:FAIL:No/Wrong response on Channel B");
            return false;
        }
        
        if (twai_install_for_channel('A') != ESP_OK) return false;
        twai_message_t rx_a_xtalk;
        if (twai_receive(&rx_a_xtalk, pdMS_TO_TICKS(20)) == ESP_OK) {
            Serial.printf("CAN_TEST_FINAL:FAIL:Crosstalk! Got 0x%lX on A when sending on B\n", rx_a_xtalk.identifier);
            return false;
        }

        if ((i + 1) % (num_messages / 10 < 1 ? 1 : num_messages / 10) == 0) {
             Serial.printf("CAN_TEST_PROGRESS: %d/%d messages checked.\n", i + 1, num_messages);
        }
    }
    
    // Final cleanup
    twai_status_info_t status;
    if (twai_get_status_info(&status) == ESP_OK && status.state != TWAI_STATE_STOPPED) {
      twai_stop();
      twai_driver_uninstall();
    }
    return true;
}


// ####################################################################
// #                       MAIN LOGIC & LOOPS                         #
// ####################################################################

void slave_loop() {
  if (UART_SERIAL.available() > 0) {
    String command = UART_SERIAL.readStringUntil('\n');
    command.trim();

    if (command == "CHECK_SPI_ADC") {
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

    if (sscanf(cmdBuffer, "SET_VCAN_VOLTAGE %d", &setting) == 1) {
      bool current_power_state = (setting & 0x03) == 0x03;
      masterHandler->setVcanPower('A', (byte)setting);
      masterHandler->setVcanPower('B', (byte)setting);

      if (master_last_power_state && !current_power_state) {
        delay(300);
      } else {
        delay(100);
      }
      
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
    } else if (sscanf(cmdBuffer, "RUN_CAN_TEST %d", &num_messages) == 1) {
        if (run_can_crosstalk_test(num_messages)) {
            Serial.println("CAN_TEST_FINAL:PASS");
        }
        // Failure messages are printed inside the function
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

