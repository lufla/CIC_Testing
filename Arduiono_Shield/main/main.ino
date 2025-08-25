#include "src/can_handler/can_handler.h"

// Timing Variables
#define uartClock 115200

/**
 * @brief Standard Arduino setup function. Runs once at the beginning.
 */
void setup() {
  // Initialize Serial
  Serial.begin(uartClock);
  while (!Serial) {
    // Wait for serial port to connect. Needed for native USB port only.
  }
  Serial.println("--- CAN Bus Responder Initializing ---");
  // Initialize the CAN controller.
  setupCAN();
}

/**
 * @brief Standard Arduino loop function. Runs repeatedly.
 */
void loop() {
  // Continuously check for incoming CAN messages and handle them.
  checkCANReceive();
}

