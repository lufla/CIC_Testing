#ifndef CAN_HANDLER_H
#define CAN_HANDLER_H

#include <Arduino.h>
#include "driver/twai.h"

// --- CAN Pin Definitions ---
// Moved here from the main .ino file to be self-contained
#define CAN_TX_PIN 0
#define CAN_RX_PIN 2

// --- Role Definition ---
// Moved here to be accessible by both the main .ino and the can_handler.cpp
enum Role { UNKNOWN, MASTER, SLAVE };
// Make the global currentRole variable visible to the can_handler.cpp file
extern Role currentRole;

// Global struct to hold detailed test results for one device
struct CanTestResults {
    int tx_ok = 0;
    int tx_fail = 0;
    int rx_ok = 0;
    int crosstalk = 0; // Counts unexpected CAN IDs
};
// Make the global testResults variable visible across files
extern CanTestResults testResults;

/**
 * @brief Runs a two-way communication integrity test. Both Master and Slave
 * will transmit requests and listen for responses simultaneously.
 * @param num_messages The number of request/response cycles to perform.
 */
void run_can_communication_test(int num_messages);

#endif // CAN_HANDLER_H
