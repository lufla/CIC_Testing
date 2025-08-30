#include <Arduino.h>
#include <SPI.h>
#include "driver/twai.h"

// ################# ROLE & UART DEFINITIONS #################
enum Role { UNKNOWN, MASTER, SLAVE };
Role currentRole = UNKNOWN;
const int MASTER_SLAVE_PIN = 34; // Input only pin

// ################# PIN DEFINITIONS & CONSTANTS #################
// --- Shared Control Pin ---
const int CH_SEL_PIN = 32;
// --- C_SPI (Power Control) System ---
const int C_CS_PIN = 5;
SPIClass c_spi(VSPI);
// --- MCP23S08 I/O Expander Constants ---
const byte MCP23S08_IODIR = 0x00;
const byte MCP23S08_GPIO = 0x09;
const byte MCP23S08_WRITE_OPCODE = 0x40;
// --- TWAI (CAN) System ---
const int CAN_TX_PIN = 0;
const int CAN_RX_PIN = 2;
// --- UART Between ESP32s ---
const int UART_TX_PIN = 17;
const int UART_RX_PIN = 16;
HardwareSerial SerialPort(2); // Use Serial2 for inter-ESP communication

// ################# GLOBAL TEST VARIABLES #################
int tx_success = 0;
int tx_fail = 0;
int rx_count = 0;

// ################# POWER CONTROL FUNCTIONS #################
// These functions are required to turn on the CAN transceivers
void selectControlDevice(char channel) {
    digitalWrite(CH_SEL_PIN, (channel == 'A' || channel == 'a') ? HIGH : LOW);
    delayMicroseconds(10);
    digitalWrite(C_CS_PIN, LOW);
    delayMicroseconds(5);
}

void deselectControlDevice() {
    digitalWrite(C_CS_PIN, HIGH);
    delayMicroseconds(5);
}

void setupIoExpander(char channel) {
    selectControlDevice(channel);
    c_spi.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0));
    c_spi.transfer(MCP23S08_WRITE_OPCODE);
    c_spi.transfer(MCP23S08_IODIR);
    c_spi.transfer(0x00); // Set all pins to be outputs
    c_spi.endTransaction();
    deselectControlDevice();
}

void setVcanPower(char channel, byte setting) {
    selectControlDevice(channel);
    c_spi.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0));
    c_spi.transfer(MCP23S08_WRITE_OPCODE);
    c_spi.transfer(MCP23S08_GPIO);
    c_spi.transfer(setting); // 0x03 for ON
    c_spi.endTransaction();
    deselectControlDevice();
}

void initialize_vcan_power() {
    Serial.println("Initializing VCAN Power...");
    pinMode(CH_SEL_PIN, OUTPUT);
    pinMode(C_CS_PIN, OUTPUT);
    digitalWrite(C_CS_PIN, HIGH);
    c_spi.begin();
    setupIoExpander('A');
    setupIoExpander('B');
    setVcanPower('A', 0x03);
    setVcanPower('B', 0x03);
    delay(500); // Wait for power to stabilize
    Serial.println("VCAN Power ON.");
}

// ################# CAN FUNCTIONS #################
void twai_init() {
  twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT((gpio_num_t)CAN_TX_PIN, (gpio_num_t)CAN_RX_PIN, TWAI_MODE_NORMAL);
  twai_timing_config_t t_config = TWAI_TIMING_CONFIG_125KBITS();
  twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();
  if (twai_driver_install(&g_config, &t_config, &f_config) != ESP_OK || twai_start() != ESP_OK) {
    if (currentRole == MASTER) Serial.println("FATAL: Failed to start TWAI driver.");
    while(1); // Halt on failure
  }
  if (currentRole == MASTER) Serial.println("TWAI driver started successfully.");
}

// Helper function to print frame details for debugging
void print_frame_details(const twai_message_t* message) {
    if (currentRole == MASTER) {
        Serial.printf("RX | ID: 0x%03lX, DLC: %d, Data: ", message->identifier, message->data_length_code);
        for (int i = 0; i < message->data_length_code; i++) {
            Serial.printf("%02X ", message->data[i]);
        }
        Serial.println();
    }
}

// ** MODIFIED to implement request-response protocol **
void run_can_test(int num_messages) {
    // Reset counters for the new test
    tx_success = 0;
    tx_fail = 0;
    rx_count = 0;
    unsigned long last_send = 0;
    const unsigned long SEND_INTERVAL = 100;

    // Define CAN IDs based on role, according to the provided can_handler files
    // Master sends requests for Node 1, Slave for Node 2
    uint32_t my_request_to_send = (currentRole == MASTER) ? 0x581 : 0x582;
    uint32_t expected_response_to_my_request = (currentRole == MASTER) ? 0x701 : 0x702;
    uint32_t request_id_from_other = (currentRole == MASTER) ? 0x582 : 0x581;
    uint32_t my_response_to_send = (currentRole == MASTER) ? 0x702 : 0x701;

    if (currentRole == MASTER) Serial.printf("Master starting test: %d requests...\n", num_messages);

    for (int i = 0; i < num_messages; i++) {
        // This inner loop runs for the SEND_INTERVAL duration.
        // It handles receiving messages and responding to requests.
        while (millis() - last_send < SEND_INTERVAL) {
            twai_message_t rx_msg;
            if (twai_receive(&rx_msg, 0) == ESP_OK) {
                // Check if this is a request FOR ME from the other device
                if (rx_msg.identifier == request_id_from_other) {
                    twai_message_t response_msg = {};
                    response_msg.identifier = my_response_to_send;
                    response_msg.data_length_code = 8; // As per can_handler.cpp
                    for(int j=0; j<8; j++) { response_msg.data[j] = 0x00; } // Zero out payload
                    twai_transmit(&response_msg, pdMS_TO_TICKS(50));
                }
                // Check if this is a response TO MY request
                else if (rx_msg.identifier == expected_response_to_my_request) {
                    rx_count++; // It's a valid response
                    print_frame_details(&rx_msg);
                }
            }
        }

        // Now, it's our turn to send our next request
        twai_message_t tx_msg = {};
        tx_msg.identifier = my_request_to_send;
        tx_msg.data_length_code = 0; // Request has no data payload
        
        if (twai_transmit(&tx_msg, pdMS_TO_TICKS(50)) == ESP_OK) {
            tx_success++;
        } else {
            tx_fail++;
        }
        last_send = millis();
    }
    
    // After the loop, wait a bit longer to catch the final response to our last request
    unsigned long end_wait = millis() + 500;
    while(millis() < end_wait) {
        twai_message_t rx_msg;
        if (twai_receive(&rx_msg, pdMS_TO_TICKS(10)) == ESP_OK) {
            if (rx_msg.identifier == expected_response_to_my_request) {
                rx_count++;
                print_frame_details(&rx_msg);
            }
        }
    }
    
    if (currentRole == MASTER) {
        Serial.printf("\n--- Master Results ---\n");
        Serial.printf("TX success: %d, fail: %d\n", tx_success, tx_fail);
        Serial.printf("RX count (valid responses): %d\n", rx_count);
    }
}

// ################# MAIN SETUP #################
void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("\n--- Unified CAN Test Firmware ---");

    // --- Role Detection ---
    pinMode(MASTER_SLAVE_PIN, INPUT_PULLUP);
    delay(10); // Short delay for pin state to settle
    if (digitalRead(MASTER_SLAVE_PIN) == LOW) {
        currentRole = MASTER;
        pinMode(MASTER_SLAVE_PIN, INPUT_PULLDOWN); // Prevent floating
        Serial.println("ROLE: MASTER (Pin 34 is LOW)");
    } else {
        currentRole = SLAVE;
        Serial.println("ROLE: SLAVE (Pin 34 is HIGH)");
    }

    // --- Hardware Initialization ---
    initialize_vcan_power();
    twai_init();
    SerialPort.begin(115200, SERIAL_8N1, UART_RX_PIN, UART_TX_PIN);

    if (currentRole == MASTER) {
        Serial.println("\nReady. Send 'start <num_messages>' to begin. (e.g., 'start 10')");
    } else {
        Serial.println("Waiting for commands from Master...");
    }
}

// ################# MAIN LOOP #################
void loop() {
    if (currentRole == MASTER) {
        if (Serial.available()) {
            String cmd = Serial.readStringUntil('\n');
            int num_to_send;
            if (sscanf(cmd.c_str(), "start %d", &num_to_send) == 1) {
                SerialPort.printf("START_TEST:%d\n", num_to_send);
                run_can_test(num_to_send);
                
                // BUG FIX: Add delay and flush buffer for UART reliability
                delay(100); // Give slave time to finish its tasks
                while(SerialPort.available() > 0) { // Clear any junk from the buffer
                  SerialPort.read();
                }

                Serial.println("\nRequesting results from Slave...");
                SerialPort.println("GET_RESULTS");

                unsigned long start_time = millis();
                bool received = false;
                while (millis() - start_time < 3000 && !received) {
                    if (SerialPort.available()) {
                        String msg = SerialPort.readStringUntil('\n');
                        if (msg.startsWith("RESULTS:")) {
                            int s_tx, s_fail, s_rx;
                            sscanf(msg.c_str(), "RESULTS:%d:%d:%d", &s_tx, &s_fail, &s_rx);
                            Serial.println("\n--- Slave Results ---");
                            Serial.printf("TX success: %d, fail: %d\n", s_tx, s_fail);
                            Serial.printf("RX count (valid responses): %d\n", s_rx);

                            Serial.println("\n--- OVERALL ---");
                            if (tx_fail == 0 && s_fail == 0 && rx_count >= num_to_send && s_rx >= num_to_send) {
                                Serial.println("PASS ✅");
                            } else {
                                Serial.println("FAIL ❌");
                            }
                            received = true;
                        }
                    }
                }
                if (!received) { Serial.println("FAIL: Did not receive results from slave."); }
                Serial.println("\nReady for next command.");
            }
        }
    } 
    else if (currentRole == SLAVE) {
        if (SerialPort.available()) {
            String cmd = SerialPort.readStringUntil('\n');
            cmd.trim();
            if (cmd.startsWith("START_TEST:")) {
                int num_messages = cmd.substring(cmd.indexOf(':') + 1).toInt();
                run_can_test(num_messages);
            } 
            else if (cmd.equals("GET_RESULTS")) {
                SerialPort.printf("RESULTS:%d:%d:%d\n", tx_success, tx_fail, rx_count);
            }
        }
    }
}

