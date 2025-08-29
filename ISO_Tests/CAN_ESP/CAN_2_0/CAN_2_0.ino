/*
 * ESP32 TWAI (CAN) Communication Example using the official driver
 *
 * This sketch demonstrates how to receive CAN frames on an ESP32 using the
 * official Espressif TWAI driver, which is a stable alternative to CAN.h.
 * This approach is more robust than direct register manipulation and includes
 * automatic bus-off recovery.
 *
 * Hardware Setup:
 * - ESP32-WROOM-32E or similar ESP32 module.
 * - A functional CAN transceiver (e.g., SN65HVD230, MCP2551) connected to the ESP32.
 * - ESP32 GPIO 5 (TX) -> Transceiver's TXD/CANTX pin.
 * - ESP32 GPIO 2 (RX) -> Transceiver's RXD/CANRX pin.
 * - The CAN transceiver is then connected to the CAN bus (CAN_H, CAN_L).
 *
 * Author: Gemini
 * Date: August 29, 2025
 */

// Include the official ESP-IDF driver for the TWAI (CAN) peripheral
#include "driver/twai.h"

// --- Configuration ---
const int CAN_TX_PIN = 5;
const int CAN_RX_PIN = 2;

/**
 * @brief Initializes the TWAI driver.
 */
void twai_init() {
  // 1. Configure general TWAI settings
  twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(
      (gpio_num_t)CAN_TX_PIN, 
      (gpio_num_t)CAN_RX_PIN, 
      TWAI_MODE_NORMAL
  );
  // Disable alerts, as we will be polling for errors manually
  g_config.alerts_enabled = TWAI_ALERT_NONE;

  // 2. Configure timing for 125 kbps
  twai_timing_config_t t_config = TWAI_TIMING_CONFIG_125KBITS();

  // 3. Configure the acceptance filter to accept all messages
  twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

  // 4. Install the TWAI driver
  esp_err_t install_err = twai_driver_install(&g_config, &t_config, &f_config);
  if (install_err == ESP_OK) {
    Serial.println("TWAI driver installed successfully.");
  } else {
    Serial.printf("Failed to install TWAI driver. Error: %s\n", esp_err_to_name(install_err));
    return;
  }

  // 5. Start the TWAI driver
  esp_err_t start_err = twai_start();
  if (start_err == ESP_OK) {
    Serial.println("TWAI driver started.");
  } else {
    Serial.printf("Failed to start TWAI driver. Error: %s\n", esp_err_to_name(start_err));
  }
}

/**
 * @brief Prints a received TWAI/CAN frame to the Serial monitor.
 * @param message The received message structure.
 */
void print_frame(const twai_message_t* message) {
  Serial.printf("RX Frame | ID: 0x%03lX, ", message->identifier);
  Serial.printf("Ext: %d, RTR: %d, DLC: %d, Data: ", message->extd, message->rtr, message->data_length_code);
  for (int i = 0; i < message->data_length_code; i++) {
    Serial.printf("%02X ", message->data[i]);
  }
  Serial.println();
}

// --- Arduino Sketch ---
void setup() {
  Serial.begin(115200);
  while (!Serial); // Wait for serial connection
  Serial.println("ESP32 TWAI (CAN) Receiver using Official Driver");
  
  twai_init();
}

void loop() {
  // Check for received messages
  twai_message_t rx_message;
  esp_err_t status = twai_receive(&rx_message, pdMS_TO_TICKS(100)); // Wait up to 100ms

  if (status == ESP_OK) {
    // A message was received successfully
    print_frame(&rx_message);
  } else if (status == ESP_ERR_TIMEOUT) {
    // No message received within the timeout period, this is normal
  } else {
    // An error occurred during receive
    Serial.printf("Error receiving CAN frame: %s\n", esp_err_to_name(status));

    // You can check the bus status if needed, though the driver handles recovery
    uint32_t alerts;
    twai_read_alerts(&alerts, 0);
    if (alerts & TWAI_ALERT_BUS_OFF) {
        Serial.println("Bus-off alert detected, driver is recovering.");
    }
  }
}

