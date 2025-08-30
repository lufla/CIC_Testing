#include "src/spi_handler/master_spi_handler.h"
#include "src/spi_handler/slave_spi_handler.h"
// Enum to define the role of the ESP32.
enum Role { UNKNOWN, MASTER, SLAVE };
Role currentRole = UNKNOWN;

// --- SHARED PIN & UART DEFINITIONS ---
#define MASTER_SLAVE_PIN 34
#define UART_SERIAL Serial1
#define UART_TX_PIN 17
#define UART_RX_PIN 16
#define UART_BAUD_RATE 115200

// Pointers to the role-specific handlers. Only one will be initialized.
MasterSpiHandler* masterHandler = nullptr;
SlaveSpiHandler* slaveHandler = nullptr;

// A variable to track the last power state, used only by the master.
bool master_last_power_state = false;

// ####################################################################
// #                       MAIN LOGIC & LOOPS                         #
// ####################################################################

void slave_loop() {
  if (UART_SERIAL.available() > 0) {
    String command = UART_SERIAL.readStringUntil('\n');
    command.trim();
    if (command == "CHECK_SPI_ADC") {
      // Use the handler to get ADC readings
      AdcReadings readings = slaveHandler->readAllAdcValues();

      // Format and send the data over UART
      char buffer[100];
      snprintf(buffer, sizeof(buffer), "DATA:%.4f,%.4f,%.4f,%.4f",
               readings.cic_v, readings.cic_i, readings.vcan_v, readings.vcan_i);
      UART_SERIAL.println(buffer);
    }
  }
}

void master_loop() {
  if (Serial.available() > 0) {
    char cmdBuffer[50];
    int bytesRead = Serial.readBytesUntil('\n', cmdBuffer, sizeof(cmdBuffer) - 1);
    cmdBuffer[bytesRead] = '\0'; // Null-terminate the string

    int setting;
    // Use sscanf to reliably parse the command
    if (sscanf(cmdBuffer, "SET_VCAN_VOLTAGE %d", &setting) == 1) {
      bool current_power_state = (setting & 0x03) == 0x03;

      // Use the handler to set the power on both channels
      masterHandler->setVcanPower('A', (byte)setting);
      masterHandler->setVcanPower('B', (byte)setting);

      // Apply delay after setting voltage to allow settling
      if (master_last_power_state && !current_power_state) {
        delay(300); // Longer delay when turning power OFF
      } else {
        delay(100); // Standard delay for other cases
      }

      // Use the handler to read the voltages
      float v_a = masterHandler->readVcanVoltage('A');
      float v_b = masterHandler->readVcanVoltage('B');

      // Format and send the response
      char buffer[50];
      snprintf(buffer, sizeof(buffer), "VCAN_DATA:%.4f,%.4f", v_a, v_b);
      Serial.println(buffer);

      master_last_power_state = current_power_state;

    } else {
      // If it's not a voltage command, forward it to the slave
      UART_SERIAL.println(cmdBuffer);
    }
  }

  // Check for and print any response from the slave
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

  // Determine the role (Master or Slave) based on a pin state
  pinMode(MASTER_SLAVE_PIN, INPUT_PULLUP);
  delay(10);
  if (digitalRead(MASTER_SLAVE_PIN) == LOW) {
    currentRole = MASTER;
    pinMode(MASTER_SLAVE_PIN, INPUT_PULLDOWN);
    Serial.println("Role: MASTER");

    // Initialize the Master handler, which configures all master-specific hardware
    masterHandler = new MasterSpiHandler();
    masterHandler->begin();

    Serial.println("Master hardware initialized.");

  } else {
    currentRole = SLAVE;
    Serial.println("Role: SLAVE");

    // Initialize the Slave handler, which configures all slave-specific hardware
    slaveHandler = new SlaveSpiHandler();
    slaveHandler->begin();
  }

  // Start UART communication for Master-Slave link
  UART_SERIAL.begin(UART_BAUD_RATE, SERIAL_8N1, UART_RX_PIN, UART_TX_PIN);
}

void loop() {
  // Call the appropriate loop function based on the determined role
  if (currentRole == MASTER) {
    master_loop();
  } else if (currentRole == SLAVE) {
    slave_loop();
  }
}