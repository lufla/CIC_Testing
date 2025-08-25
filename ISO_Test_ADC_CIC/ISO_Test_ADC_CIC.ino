/**
 * @file Fused_Power_and_ADC_Test.ino
 * @brief Definitive fused code for VCAN power control and ADC readout.
 *
 * This script combines:
 * 1. The EXACT working power configuration from "Backup_with_working_sent.txt".
 * 2. The robust, multi-step ADC initialization and calibration from "reding_ADC.txt".
 * 3. The correct 24-bit ADC reading logic.
 *
 * All logic is adapted for the final, multiplexed chip-select hardware.
 */

// ################# LIBRARIES #################
#include <Arduino.h>
#include <SPI.h>

// ################# PIN DEFINITIONS (Multiplexed Hardware) #################
const int SPI_MOSI_PIN = 13;
const int SPI_MISO_PIN = 12;
const int SPI_SCK_PIN = 14;

const int MCS_PIN = 15;    // Primary Chip Select for Monitor bus (ADCs)
const int CCS_PIN = 2;     // Primary Chip Select for Control bus (Power Enables)
const int CH_SEL_PIN = 4;  // Selects between device A (HIGH) and B (LOW)

// ################# CONSTANTS #################
// --- MCP23S08 Registers (from Backup_with_working_sent.txt) ---
const byte MCP23S08_IODIR = 0x00;
const byte MCP23S08_IOCON = 0x05;
const byte MCP23S08_GPIO = 0x09;
const byte MCP23S08_WRITE_OPCODE = 0x40;

// --- CS5523 ADC Commands (from Backup_with_working_sent.txt) ---
const byte CS5523_CMD_CONV_SINGLE = 0x80;

// SPI object for VSPI
SPIClass spi(VSPI);

// --- Function Prototypes ---
void selectChip(int side, char bus);
void deselectChips();
void setupIoExpanders();
void ADC_Init(int side);
void setPower(int side, byte setting);
float readAdcVoltage_24bit(int side, int channel);
void readAndPrintAllChannels(int side);

// #################### SETUP ##########################
void setup() {
    Serial.begin(115200);
    while (!Serial);
    delay(1000);
    Serial.println("\n--- Fused VCAN Power & ADC Test ---");

    // Configure chip select pins
    pinMode(MCS_PIN, OUTPUT);
    pinMode(CCS_PIN, OUTPUT);
    pinMode(CH_SEL_PIN, OUTPUT);
    deselectChips();

    // Initialize SPI bus
    spi.begin(SPI_SCK_PIN, SPI_MISO_PIN, SPI_MOSI_PIN);

    // Configure both I/O expanders using the logic that WORKED
    setupIoExpanders();

    // Initialize both ADCs using the comprehensive routine
    ADC_Init(1); // Initialize Side A ADC
    ADC_Init(2); // Initialize Side B ADC

    Serial.println("\n--- Initialization Complete. Starting Test Cycle. ---");
}

// #################### MAIN LOOP ##########################
void loop() {
    // Correct power settings for the new CIC hardware
    byte vcan_on_setting = 0x03;
    byte vcan_off_setting = 0x00;

    // --- Turn Power ON ---
    Serial.println("\n>>> Turning VCAN Power ON <<<");
    setPower(1, vcan_on_setting); // Set Side A power
    setPower(2, vcan_on_setting); // Set Side B power
    delay(1000); // Wait for voltage to stabilize

    // --- Measure Voltages (ON) ---
    Serial.println("  --- Reading Voltages (State: ON) ---");
    readAndPrintAllChannels(1); // Read Side A
    readAndPrintAllChannels(2); // Read Side B

    delay(2500);

    // --- Turn Power OFF ---
    Serial.println("\n<<< Turning VCAN Power OFF >>>");
    setPower(1, vcan_off_setting); // Set Side A power
    setPower(2, vcan_off_setting); // Set Side B power
    delay(500); // Wait for voltage to discharge

    // --- Measure Voltages (OFF) ---
    Serial.println("  --- Reading Voltages (State: OFF) ---");
    readAndPrintAllChannels(1); // Read Side A
    readAndPrintAllChannels(2); // Read Side B

    delay(2500);
}


// ################# HELPER FUNCTIONS #################

/**
 * @brief Selects an SPI chip using the multiplexed control logic.
 */
void selectChip(int side, char bus) {
    if (side == 1) {
        digitalWrite(CH_SEL_PIN, HIGH); // Select device 'A'
    } else {
        digitalWrite(CH_SEL_PIN, LOW);  // Select device 'B'
    }

    if (bus == 'M' || bus == 'm') {
        digitalWrite(CCS_PIN, HIGH);
        digitalWrite(MCS_PIN, LOW);     // Select Monitor bus (ADC)
    } else if (bus == 'C' || bus == 'c') {
        digitalWrite(MCS_PIN, HIGH);
        digitalWrite(CCS_PIN, LOW);     // Select Control bus (Power)
    }
}

/**
 * @brief Deselects all SPI chips.
 */
void deselectChips() {
    digitalWrite(MCS_PIN, HIGH);
    digitalWrite(CCS_PIN, HIGH);
    delayMicroseconds(5);
}

/**
 * @brief Configures the MCP23S08 I/O expanders using the working logic from Backup_with_working_sent.txt.
 */
void setupIoExpanders() {
    Serial.println("Configuring I/O Expanders (using verified method)...");
    for (int side = 1; side <= 2; side++) {
        // Transaction 1: Enable hardware address pin (HAEN)
        selectChip(side, 'C');
        spi.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0));
        spi.transfer(MCP23S08_WRITE_OPCODE);
        spi.transfer(MCP23S08_IOCON);    // IOCON register address (0x05)
        spi.transfer(0x08);             // Value to set HAEN bit
        spi.endTransaction();
        deselectChips();
        delay(1);

        // Transaction 2: Set all GPIO pins to be outputs
        selectChip(side, 'C');
        spi.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0));
        spi.transfer(MCP23S08_WRITE_OPCODE);
        spi.transfer(MCP23S08_IODIR);   // IODIR register address (0x00)
        spi.transfer(0x00);             // Value for all outputs
        spi.endTransaction();
        deselectChips();
        delay(1);

        // Transaction 3: Set initial output state to LOW
        setPower(side, 0x00);
    }
    Serial.println("I/O Expander configuration complete.");
}


/**
 * @brief Writes a setting to the GPIO register of the MCP23S08.
 */
void setPower(int side, byte setting) {
    selectChip(side, 'C');
    spi.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0));
    spi.transfer(MCP23S08_WRITE_OPCODE);
    spi.transfer(MCP23S08_GPIO);         // GPIO register address
    spi.transfer(setting);               // The power setting value
    spi.endTransaction();
    deselectChips();
}


/**
 * @brief Performs the one-time comprehensive initialization for an ADC.
 */
void ADC_Init(int side) {
    Serial.print("Initializing ADC on side ");
    Serial.println(side == 1 ? "A" : "B");
    
    // This detailed sequence (Sync, Reset, Config, Calibrate) is critical for ADC stability.
    selectChip(side, 'M');
    for (int i = 0; i < 15; i++) { spi.transfer(0xff); }
    spi.transfer(0xfe);
    delay(1);
    spi.transfer(0x03); spi.transfer(0x00); spi.transfer(0x00); spi.transfer(0x80);
    delay(1);
    deselectChips();
    delay(2);
    selectChip(side, 'M');
    spi.transfer(0x03); spi.transfer(0x00); spi.transfer(0x30); spi.transfer(0x00);
    delay(1);
    deselectChips();
    delay(2);
    selectChip(side, 'M');
    spi.transfer(0x05);
    spi.transfer(0x02); spi.transfer(0xb0); spi.transfer(0xab);
    spi.transfer(0x12); spi.transfer(0xb1); spi.transfer(0xab);
    delay(1);
    deselectChips();
    delay(2);
    for (int channel = 0; channel < 4; channel++) {
        byte channelbyte;
        selectChip(side, 'M'); delay(10);
        channelbyte = (channel << 3) | 0x81;
        spi.transfer(channelbyte);
        deselectChips(); delay(10);
        selectChip(side, 'M'); delay(10);
        channelbyte = (channel << 3) | 0x82;
        spi.transfer(channelbyte);
        deselectChips(); delay(10);
    }
    Serial.printf("  -> ADC %c Initialization complete.\n", side == 1 ? 'A' : 'B');
}


/**
 * @brief Reads voltage from a specific ADC channel using the correct 24-bit method.
 */
float readAdcVoltage_24bit(int side, int channel) {
    if (channel < 1 || channel > 4) return -1.0;

    // Create command byte for single conversion on the selected channel
    byte convCmd = CS5523_CMD_CONV_SINGLE | ((channel - 1) << 1);

    // Start conversion
    selectChip(side, 'M');
    spi.beginTransaction(SPISettings(2000000, MSBFIRST, SPI_MODE0));
    spi.transfer(convCmd);
    spi.endTransaction();
    deselectChips();

    delay(250); // Wait for conversion to complete

    // Read 24-bit conversion result
    long raw = 0;
    selectChip(side, 'M');
    spi.beginTransaction(SPISettings(2000000, MSBFIRST, SPI_MODE0));
    spi.transfer(0x0C); // "Read Data" command
    raw |= ((long)spi.transfer(0x00) << 16);
    raw |= ((long)spi.transfer(0x00) << 8);
    raw |= ((long)spi.transfer(0x00));
    spi.endTransaction();
    deselectChips();

    // Sign-extend 24-bit two's complement number to 32 bits
    if (raw & 0x800000) {
        raw |= 0xFF000000;
    }

    // Convert raw ADC value to voltage
    const float V_REF = 2.5;
    const long MAX_ADC_VALUE = 8388607;
    float voltage = ((float)raw / MAX_ADC_VALUE) * V_REF;

    return voltage;
}

/**
 * @brief Helper function to read all 4 channels for a given side and print them.
 */
void readAndPrintAllChannels(int side) {
    char side_char = (side == 1) ? 'A' : 'B';
    Serial.printf("  Side %c: ", side_char);
    for (int ch = 1; ch <= 4; ch++) {
        float voltage = readAdcVoltage_24bit(side, ch);
        Serial.printf("[Ch%d: %.3f V] ", ch, voltage);
    }
    Serial.println();
}