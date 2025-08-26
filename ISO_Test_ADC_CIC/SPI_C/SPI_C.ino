#include <Arduino.h>
#include <SPI.h>

// --- Pin Definitions ---
// Based on the tester board schematic, we use a Channel Select pin
// and a single, shared Chip Select pin.
const int CH_SEL_PIN = 32;         // GPIO35 controls the analog switch to select Channel A or B.
const int SHARED_CS_PIN = 5;       // A single CS pin is routed to A or B by the switch.

// --- SPI Setup ---
SPIClass spi(VSPI);

// --- MCP23S08 I/O Expander Constants ---
const byte MCP23S08_IODIR = 0x00;        // I/O Direction Register
const byte MCP23S08_GPIO = 0x09;         // General Purpose I/O Register
const byte MCP23S08_WRITE_OPCODE = 0x40; // Opcode to write to the expander

/**
 * @brief Selects the active channel and enables the SPI device.
 * @param channel The target channel, 'A' or 'B'.
 */
void selectSpiChannel(char channel) {
    // Set the CH_SEL pin to route the SPI signals to the correct expander
    if (channel == 'A' || channel == 'a') {
        digitalWrite(CH_SEL_PIN, LOW);
    } else { // Default to channel B
        digitalWrite(CH_SEL_PIN, HIGH);
    }
    delayMicroseconds(10); // Allow time for the analog switch to settle
    digitalWrite(SHARED_CS_PIN, LOW); // Enable the selected device
    delayMicroseconds(5);
}

/**
 * @brief Deselects the active SPI device.
 */
void deselectSpiDevice() {
    digitalWrite(SHARED_CS_PIN, HIGH);
    delayMicroseconds(5);
}

/**
 * @brief Writes a setting to a specific I/O expander's GPIO register.
 * @param channel The target channel ('A' or 'B').
 * @param setting The 8-bit value to write. 0x03 for ON, 0x00 for OFF.
 */
void setVcanPower(char channel, byte setting) {
    selectSpiChannel(channel);
    spi.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0));
    spi.transfer(MCP23S08_WRITE_OPCODE); // Command to write
    spi.transfer(MCP23S08_GPIO);         // Target the GPIO register
    spi.transfer(setting);               // Send the power on/off value
    spi.endTransaction();
    deselectSpiDevice();
}

/**
 * @brief Configures a specific MCP23S08 I/O Expander to control power.
 * @param channel The target channel ('A' or 'B').
 */
void setupIoExpander(char channel) {
    selectSpiChannel(channel);
    spi.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0));
    spi.transfer(MCP23S08_WRITE_OPCODE);
    spi.transfer(MCP23S08_IODIR); // Target the I/O Direction register
    spi.transfer(0x00);           // Set all pins to be outputs
    spi.endTransaction();
    deselectSpiDevice();
}

// --- Main Setup Function ---
void setup() {
    Serial.begin(115200);
    while (!Serial);
    Serial.println("\n--- Dual VCAN Power Control (Channel Select) ---");

    // Configure hardware
    pinMode(CH_SEL_PIN, OUTPUT);
    pinMode(SHARED_CS_PIN, OUTPUT);
    deselectSpiDevice(); // Start with CS high
    spi.begin();

    // Initialize both I/O expanders
    Serial.println("Initializing Expander A...");
    setupIoExpander('A');
    setVcanPower('A', 0x00); // Start with power off

    Serial.println("Initializing Expander B...");
    setupIoExpander('B');
    setVcanPower('B', 0x00); // Start with power off

    Serial.println("Initialization complete. Both VCANs are OFF.");
}

// --- Main Loop ---
void loop() {
    // --- Turn Power ON for both channels ---
    Serial.println(">>> Turning VCAN Power ON (Both Channels) <<<");
    setVcanPower('A', 0x03);
    setVcanPower('B', 0x03);
    delay(2000); // Wait for 2 seconds

    // --- Turn Power OFF for both channels ---
    Serial.println("<<< Turning VCAN Power OFF (Both Channels) >>>");
    setVcanPower('A', 0x00);
    setVcanPower('B', 0x00);
    delay(2000); // Wait for 2 seconds
}
