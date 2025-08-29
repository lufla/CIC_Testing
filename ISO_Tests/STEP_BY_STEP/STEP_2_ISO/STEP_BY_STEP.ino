#include "src/spi_handler/spi_handler.h"


// Enum to define the role of the ESP32.
enum Role { UNKNOWN, MASTER, SLAVE };
Role currentRole = UNKNOWN;

// --- Pin Definitions ---
#define MASTER_SLAVE_PIN 34 // INPUT_PULLDOWN for Master, INPUT_PULLUP for Slave

// --- UART Communication (Master-Slave) ---
#define UART_SERIAL Serial1 // Hardware Serial 1
#define UART_TX_PIN 17
#define UART_RX_PIN 16
#define UART_BAUD_RATE 115200

// --- Function Prototypes ---
void perform_adc_check();
void slave_loop();
void master_loop();

// =====================================================================
//                             SLAVE LOGIC
// =====================================================================
void perform_adc_check() {
  // Use the handler to get all sensor readings
  AdcReadings readings = read_all_adc_values();

  // Buffer to format the output string
  char buffer[100];

  // Format the data into a single, comma-separated string
  snprintf(buffer, sizeof(buffer), "DATA:%.4f,%.4f,%.4f,%.4f",
           readings.cic_v, readings.cic_i, readings.vcan_v, readings.vcan_i);

  // Send the formatted string to the master
  UART_SERIAL.println(buffer);
}

void slave_loop() {
  // The slave waits for a command from the master.
  if (UART_SERIAL.available() > 0) {
    String command = UART_SERIAL.readStringUntil('\n');
    command.trim();
    if (command == "CHECK_SPI_ADC") {
      perform_adc_check();
    }
  }
}

// =====================================================================
//                             MASTER LOGIC
// =====================================================================
void master_loop() {
  // Check for a command from the PC (Python script).
  if (Serial.available() > 0) {
    String command = Serial.readStringUntil('\n');
    command.trim();
    // Forward the command to the slave.
    UART_SERIAL.println(command);
  }

  // Check for a response from the slave.
  if (UART_SERIAL.available() > 0) {
    String response = UART_SERIAL.readStringUntil('\n');
    response.trim();
    // Forward the response to the PC.
    Serial.println(response);
  }
}

// =====================================================================
//                            SETUP AND MAIN LOOP
// =====================================================================
void setup() {
  // Start serial communication with the PC.
  Serial.begin(115200);
  delay(1000); // Wait for serial monitor to open.
  Serial.println("\nESP32 Unified Firmware Initializing...");

  // --- Role Detection ---
  pinMode(MASTER_SLAVE_PIN, INPUT_PULLUP);
  delay(10); // Small delay for pin state to settle
  if (digitalRead(MASTER_SLAVE_PIN) == LOW) {
    currentRole = MASTER;
    pinMode(MASTER_SLAVE_PIN, INPUT_PULLDOWN); // Re-configure for master
    Serial.println("Role: MASTER");
    // Master initializes the UART link to the slave.
    UART_SERIAL.begin(UART_BAUD_RATE, SERIAL_8N1, UART_RX_PIN, UART_TX_PIN);
  } else {
    currentRole = SLAVE;
    Serial.println("Role: SLAVE");
    // Slave initializes the UART link to the master.
    UART_SERIAL.begin(UART_BAUD_RATE, SERIAL_8N1, UART_RX_PIN, UART_TX_PIN);
    // Slave also initializes the SPI ADC.
    setup_spi_adc();
  }
}

void loop() {
  if (currentRole == MASTER) {
    master_loop();
  } else if (currentRole == SLAVE) {
    slave_loop();
  }
  // If role is UNKNOWN, do nothing.
}

