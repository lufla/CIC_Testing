/*
 * ESP32 TWAI (CAN) Communication Example using the official driver
 *
 * This sketch acts as a "requester" to communicate with the provided
 * Arduino/MCP2515 "responder" code. It sends a request frame with ID 0x581
 * and listens for a response frame with ID 0x701.
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
const int CAN_TX_PIN = 0; //5
const int CAN_RX_PIN = 2; //2
const uint8_t NODE_ID_TO_REQUEST = 1; // The node ID we want to query

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
 * @brief Prints a TWAI/CAN frame to the Serial monitor.
 * @param message The message structure to print.
 * @param direction A string indicating if it was "TX" or "RX".
 */
void print_frame(const twai_message_t* message, const char* direction) {
  Serial.printf("%s Frame | ID: 0x%03lX, ", direction, message->identifier);
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
  Serial.println("ESP32 TWAI (CAN) Requester using Official Driver");
  
  twai_init();
}

unsigned long last_request_time = 0;
const long request_interval = 2000; // Send a request every 2 seconds

void loop() {
  // Periodically send a request frame
  if (millis() - last_request_time >= request_interval) {
    last_request_time = millis();

    // 1. Create the request message
    twai_message_t tx_message;
    tx_message.identifier = 0x580 + NODE_ID_TO_REQUEST;
    tx_message.extd = 0; // Standard frame
    tx_message.rtr = 0; // Not a remote frame
    tx_message.data_length_code = 0; // No data needed for this request
    
    // 2. Queue the message for transmission
    esp_err_t tx_status = twai_transmit(&tx_message, pdMS_TO_TICKS(1000));
    
    if (tx_status == ESP_OK) {
      print_frame(&tx_message, "TX");
    } else {
      Serial.printf("Failed to send request: %s\n", esp_err_to_name(tx_status));
    }
  }

  // Check for received messages (the response)
  twai_message_t rx_message;
  esp_err_t rx_status = twai_receive(&rx_message, pdMS_TO_TICKS(10)); // Check briefly

  if (rx_status == ESP_OK) {
    // A message was received successfully
    print_frame(&rx_message, "RX");
  } else if (rx_status == ESP_ERR_TIMEOUT) {
    // No message received, this is normal
  } else {
    // An error occurred during receive
    Serial.printf("Error receiving CAN frame: %s\n", esp_err_to_name(rx_status));
  }
}

