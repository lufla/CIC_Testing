#include <Arduino.h>
#include <SPI.h>

// ################# CONSTANTS & PIN DEFINITIONS #################

// --- Shared Control Pin ---
// Corrected based on schematic: IO35 controls both U10 and U11 switches.
const int CH_SEL_PIN = 32;

// --- C_SPI (Power Control) System ---
const int C_CS_PIN = 5;      // Shared Chip Select for I/O Expanders (CCS)
// Note: C_SPI bus pins (SCK, MOSI, MISO) are assumed to be default VSPI pins
// or are handled by the library. The schematic shows IO25, IO27, IO26.
// For robustness, they could be explicitly defined in c_spi.begin().
SPIClass c_spi(VSPI); // Use VSPI for the control bus

// MCP23S08 I/O Expander Constants
const byte MCP23S08_IODIR = 0x00;        // I/O Direction Register
const byte MCP23S08_GPIO = 0x09;         // General Purpose I/O Register
const byte MCP23S08_WRITE_OPCODE = 0x40; // Opcode to write to the expander

// --- M_SPI (ADC Monitor) System ---
const int M_CS_PIN = 15;     // Chip Select for ADCs (MCS)
const int M_MOSI_PIN = 13;   // From schematic (MSDI)
const int M_MISO_PIN = 12;   // From schematic (MSDO)
const int M_SCK_PIN = 14;    // From schematic (MSCK)
SPIClass m_spi(HSPI); // Use HSPI for the monitor bus


// ################# HELPER FUNCTIONS FOR SELECTION #################

/**
 * @brief Selects the active Power Control I/O Expander and enables its bus.
 * @param channel The target channel, 'A' or 'B'.
 */
void selectControlDevice(char channel) {
    // CORRECTED LOGIC: Based on schematic, HIGH selects 'A', LOW selects 'B'.
    if (channel == 'A' || channel == 'a') {
        digitalWrite(CH_SEL_PIN, HIGH);
    } else {
        digitalWrite(CH_SEL_PIN, LOW);
    }
    delayMicroseconds(10); // Allow time for the analog switch to settle
    digitalWrite(C_CS_PIN, LOW); // Enable the selected device
    delayMicroseconds(5);
}

/**
 * @brief Deselects the active I/O Expander.
 */
void deselectControlDevice() {
    digitalWrite(C_CS_PIN, HIGH);
    delayMicroseconds(5);
}

/**
 * @brief Selects the active ADC and enables its bus.
 * @param channel The target channel, 'A' or 'B'.
 */
void selectADC(char channel) {
    // Logic is consistent with schematic: HIGH for 'A', LOW for 'B'.
    if (channel == 'A' || channel == 'a') {
        digitalWrite(CH_SEL_PIN, HIGH);
    } else {
        digitalWrite(CH_SEL_PIN, LOW);
    }
    digitalWrite(M_CS_PIN, LOW); // Activate the ADC bus
}

/**
 * @brief Deselects the active ADC.
 */
void deselectADC() {
    digitalWrite(M_CS_PIN, HIGH);
}


// ################# POWER CONTROL (C_SPI) FUNCTIONS #################

/**
 * @brief Configures a specific MCP23S08 I/O Expander.
 * @param channel The target channel ('A' or 'B').
 */
void setupIoExpander(char channel) {
    selectControlDevice(channel);
    c_spi.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0));
    c_spi.transfer(MCP23S08_WRITE_OPCODE); // Command to write
    c_spi.transfer(MCP23S08_IODIR);        // Target I/O Direction register
    c_spi.transfer(0x00);                  // Set all pins to be outputs
    c_spi.endTransaction();
    deselectControlDevice();
}

/**
 * @brief Writes a setting to an I/O expander to control VCAN power.
 * @param channel The target channel ('A' or 'B').
 * @param setting The 8-bit value to write. 0x03 for ON, 0x00 for OFF.
 */
void setVcanPower(char channel, byte setting) {
    selectControlDevice(channel);
    c_spi.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0));
    c_spi.transfer(MCP23S08_WRITE_OPCODE); // Command to write
    c_spi.transfer(MCP23S08_GPIO);         // Target the GPIO register
    c_spi.transfer(setting);               // Send the power on/off value
    c_spi.endTransaction();
    deselectControlDevice();
}


// ################# ADC MONITOR (M_SPI) FUNCTIONS #################

/**
 * @brief Performs the one-time initialization for an ADC.
 * @param channel The ADC to initialize ('A' or 'B').
 */
void ADC_Init(char channel) {
    Serial.print("Initializing ADC on side ");
    Serial.println(channel);
    
    // Sync, Reset, Configuration, and Calibration sequence...
    // (This code is ported directly from SPI_M.ino, now using m_spi object)
    
    selectADC(channel);
    for (int i = 0; i < 15; i++) { m_spi.transfer(0xff); }
    m_spi.transfer(0xfe);
    delay(1);
    deselectADC();

    selectADC(channel);
    m_spi.transfer(0x03); m_spi.transfer(0x00); m_spi.transfer(0x00); m_spi.transfer(0x80);
    delay(1);
    deselectADC();
    delay(1);
    selectADC(channel);
    m_spi.transfer(0x03); m_spi.transfer(0x00); m_spi.transfer(0x00); m_spi.transfer(0x00);
    delay(1);
    deselectADC();
    delay(2);

    selectADC(channel);
    m_spi.transfer(0x03); m_spi.transfer(0x00); m_spi.transfer(0x30); m_spi.transfer(0x00);
    delay(1);
    deselectADC();
    delay(2);
    
    selectADC(channel);
    m_spi.transfer(0x05);
    m_spi.transfer(0x02); m_spi.transfer(0xb0); m_spi.transfer(0xab);
    m_spi.transfer(0x12); m_spi.transfer(0xb1); m_spi.transfer(0xab);
    delay(1);
    deselectADC();
    delay(2);

    for (int ch = 0; ch < 4; ch++) {
        int channelbyte = (ch * 8) | 0x81; // Offset cal
        selectADC(channel);
        delay(10);
        m_spi.transfer(channelbyte);
        deselectADC();
        delay(10);
        
        channelbyte = (ch * 8) | 0x82; // Gain cal
        selectADC(channel);
        delay(10);
        m_spi.transfer(channelbyte);
        deselectADC();
        delay(10);
    }
    Serial.println("ADC Initialization complete.");
}

/**
 * @brief Reads and prints all 4 channels from a pre-initialized ADC.
 * @param channel The ADC to read from ('A' or 'B').
 */
void ADC_ReadAll(char channel) {
    Serial.print("Reading ADC [");
    Serial.print(channel);
    Serial.print("]: ");

    for (int ch = 0; ch < 4; ch++) {
        // Start conversion
        selectADC(channel);
        int channelbyte = (ch * 8) | 0x80;
        m_spi.transfer(channelbyte);
        m_spi.transfer(0x00); m_spi.transfer(0x00); m_spi.transfer(0x00); m_spi.transfer(0x00);
        deselectADC();
        delay(100); // Wait for conversion

        // Read result
        selectADC(channel);
        m_spi.transfer(channelbyte);
        m_spi.transfer(0x00); // Dummy byte
        int byte2 = m_spi.transfer(0x00);
        int byte3 = m_spi.transfer(0x00);
        m_spi.transfer(0x00); // Dummy byte
        deselectADC();

        long ADC_value = (256L * byte2) + byte3;
        float ADC_voltage = (ADC_value / 65535.0) * 2.5;
        if (ch < 3) { ADC_voltage *= 2; }
        
        Serial.print("ch");
        Serial.print(ch);
        Serial.print("=");
        Serial.print((int)(ADC_voltage * 1000));
        Serial.print("mV");
        if (ch < 3) { Serial.print(" | "); }
    }
    Serial.println();
}


// #################### MAIN SETUP & LOOP ##########################

void setup() {
    Serial.begin(115200);
    while (!Serial);
    Serial.println("\n--- Fused VCAN Power and ADC Measurement Test ---");

    // Configure all control pins
    pinMode(CH_SEL_PIN, OUTPUT);
    pinMode(C_CS_PIN, OUTPUT);
    pinMode(M_CS_PIN, OUTPUT);

    // Set initial states (all devices deselected)
    digitalWrite(C_CS_PIN, HIGH);
    digitalWrite(M_CS_PIN, HIGH);

    // Initialize both SPI busses
    c_spi.begin(); // Uses default VSPI pins
    m_spi.begin(M_SCK_PIN, M_MISO_PIN, M_MOSI_PIN); // Uses specific HSPI pins

    // --- Initialize Power Control Expanders ---
    Serial.println("\nInitializing I/O Expanders...");
    setupIoExpander('A');
    setupIoExpander('B');
    setVcanPower('A', 0x00); // Start with power OFF
    setVcanPower('B', 0x00); // Start with power OFF
    Serial.println("I/O Expanders Initialized. VCAN Power is OFF.");

    // --- Initialize ADCs ---
    Serial.println("\nInitializing ADCs...");
    ADC_Init('A');
    ADC_Init('B');
    Serial.println("\n--- Initialization Complete ---");
}

void loop() {
    Serial.println("\n---------------------------------");
    
    // --- Test Channel A ---
    Serial.println(">>> Testing Channel A <<<");
    setVcanPower('A', 0x03); // Turn VCAN A ON
    Serial.println("VCAN A Power: ON");
    delay(500); // Wait for power to stabilize
    ADC_ReadAll('A'); // Read ADC A values
    setVcanPower('A', 0x00); // Turn VCAN A OFF
    Serial.println("VCAN A Power: OFF");
    delay(500);
    ADC_ReadAll('A'); // Read ADC A values
    
    delay(2000); // Wait 2 seconds

    // --- Test Channel B ---
    Serial.println("\n>>> Testing Channel B <<<");
    setVcanPower('B', 0x03); // Turn VCAN B ON
    Serial.println("VCAN B Power: ON");
    delay(500); // Wait for power to stabilize
    ADC_ReadAll('B'); // Read ADC B values
    setVcanPower('B', 0x00); // Turn VCAN B OFF
    Serial.println("VCAN B Power: OFF");
    delay(500);
    ADC_ReadAll('B'); // Read ADC A values
    
    delay(3000); // Wait 3 seconds before repeating the cycle
}