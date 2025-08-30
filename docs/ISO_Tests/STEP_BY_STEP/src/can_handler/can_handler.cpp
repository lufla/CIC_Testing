#include "can_handler.h"

// Define the actual global variable for the test results.
// The 'extern' in the header file makes this single instance visible elsewhere.
CanTestResults testResults;

/**
 * @brief Runs a two-way communication integrity test. Both Master and Slave
 * will transmit requests and listen for responses simultaneously.
 * @param num_messages The number of request/response cycles to perform.
 */
void run_can_communication_test(int num_messages) {
    // Ensure a clean state for the TWAI driver
    twai_driver_uninstall(); 
    delay(50);

    // Standard configuration for the single CAN bus
    twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT((gpio_num_t)CAN_TX_PIN, (gpio_num_t)CAN_RX_PIN, TWAI_MODE_NORMAL);
    twai_timing_config_t t_config = TWAI_TIMING_CONFIG_125KBITS();
    twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();
    if (twai_driver_install(&g_config, &t_config, &f_config) != ESP_OK || twai_start() != ESP_OK) {
        if(currentRole == MASTER) Serial.println("CAN_TEST_FINAL:FAIL:Could not start TWAI driver");
        return;
    }

    // Reset results and define CAN IDs based on the device's role
    testResults = {0, 0, 0, 0};
    unsigned long last_send = 0;
    const unsigned long SEND_INTERVAL = 50; // ms

    uint32_t my_request_id = (currentRole == MASTER) ? 0x581 : 0x582;
    uint32_t expected_response_id = (currentRole == MASTER) ? 0x701 : 0x702;
    uint32_t request_from_other_id = (currentRole == MASTER) ? 0x582 : 0x581;
    uint32_t my_response_id = (currentRole == MASTER) ? 0x702 : 0x701;

    if (currentRole == MASTER) Serial.printf("CAN_TEST_PROGRESS: Starting two-way test for %d messages...\n", num_messages);

    // Main communication loop
    for (int i = 0; i < num_messages; i++) {
        while (millis() - last_send < SEND_INTERVAL) {
            twai_message_t rx_msg;
            if (twai_receive(&rx_msg, 0) == ESP_OK) {
                if (rx_msg.identifier == request_from_other_id) {
                    twai_message_t response_msg = { .identifier = my_response_id, .data_length_code = 8 };
                    memset(response_msg.data, 0, sizeof(response_msg.data));
                    twai_transmit(&response_msg, pdMS_TO_TICKS(50));
                } else if (rx_msg.identifier == expected_response_id) {
                    testResults.rx_ok++;
                } else {
                    testResults.crosstalk++; // Unexpected CAN ID
                }
            }
        }

        // Send our own request
        twai_message_t tx_msg = { .identifier = my_request_id, .data_length_code = 0 };
        if (twai_transmit(&tx_msg, pdMS_TO_TICKS(50)) == ESP_OK) {
            testResults.tx_ok++;
        } else {
            testResults.tx_fail++;
        }
        last_send = millis();
    }
    
    // Wait a moment to catch any final in-flight responses
    delay(200);
    twai_message_t final_rx;
    while(twai_receive(&final_rx, pdMS_TO_TICKS(10)) == ESP_OK) {
        if(final_rx.identifier == expected_response_id) {
            testResults.rx_ok++;
        } else {
            testResults.crosstalk++;
        }
    }

    // Clean up the driver
    twai_stop();
    twai_driver_uninstall();
}
