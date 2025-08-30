/*
  Fused_CIC_and_CAN.ino

  Merged firmware for VCAN power control, ADC monitoring, and CAN bus communication.

  This code integrates two main functionalities:
  1. Power Control & Monitoring:
     - Uses a VSPI bus (c_spi) to control MCP23S08 I/O expanders for VCAN power.
     - Uses an HSPI bus (m_spi) to read voltage data from ADCs.
     - VCAN power is enabled and voltages are read once during setup.
  2. CAN Bus Communication:
     - Uses the ESP32's built-in TWAI peripheral to send and receive CAN messages.
     - Periodically transmits a request frame and listens for responses in the main loop.
*/

// ################# INCLUDES #################
#include <Arduino.h>
#include <SPI.h>
#include "driver/twai.h" // Official ESP-IDF driver for TWAI (CAN)

// ################# CONSTANTS & PIN DEFINITIONS #################

// --- Shared Control Pin ---
const int CH_SEL_PIN = 32; // Controls U10/U11 analog switches for A/B channel selection

// --- C_SPI (Power Control) System ---
const int C_CS_PIN = 5; // Shared Chip Select for I/O Expanders (CCS)
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

// --- TWAI (CAN) System ---
const int CAN_TX_PIN = 0;
const int CAN_RX_PIN = 2;
const uint8_t NODE_ID_TO_REQUEST = 1; // The node ID we want to query

// ################# GLOBAL VARIABLES #################
unsigned long last_request_time = 0;
const long request_interval = 2000; // Perform tasks every 2 seconds

// ################# HELPER FUNCTIONS FOR SELECTION #################

// Selects the active Power Control I/O Expander ('A' or 'B').
void selectControlDevice(char channel) {
    if (channel == 'A' || channel == 'a') {
        digitalWrite(CH_SEL_PIN, HIGH);
    } else {
        digitalWrite(CH_SEL_PIN, LOW);
    }
    delayMicroseconds(10); // Allow time for the analog switch to settle
    digitalWrite(C_CS_PIN, LOW); // Enable the selected device
    delayMicroseconds(5);
}

// Deselects the active I/O Expander.
void deselectControlDevice() {
    digitalWrite(C_CS_PIN, HIGH);
    delayMicroseconds(5);
}

// Selects the active ADC ('A' or 'B').
void selectADC(char channel) {
    if (channel == 'A' || channel == 'a') {
        digitalWrite(CH_SEL_PIN, HIGH);
    } else {
        digitalWrite(CH_SEL_PIN, LOW);
    }
    digitalWrite(M_CS_PIN, LOW); // Activate the ADC bus
}

// Deselects the active ADC.
void deselectADC() {
    digitalWrite(M_CS_PIN, HIGH);
}

// ################# POWER CONTROL (C_SPI) FUNCTIONS #################

// Configures a specific MCP23S08 I/O Expander ('A' or 'B').
void setupIoExpander(char channel) {
    selectControlDevice(channel);
    c_spi.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0));
    c_spi.transfer(MCP23S08_WRITE_OPCODE); // Command to write
    c_spi.transfer(MCP23S08_IODIR);        // Target I/O Direction register
    c_spi.transfer(0x00);                  // Set all pins to be outputs
    c_spi.endTransaction();
    deselectControlDevice();
}

// Writes a setting to an I/O expander to control VCAN power.
void setVcanPower(char channel, byte setting) {
    selectControlDevice(channel);
    c_spi.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0));
    c_spi.transfer(MCP23S08_WRITE_OPCODE); // Command to write
    c_spi.transfer(MCP23S08_GPIO);         // Target the GPIO register
    c_spi.transfer(setting);               // Send the power on/off value (0x03=ON, 0x00=OFF)
    c_spi.endTransaction();
    deselectControlDevice();
}

// ################# ADC MONITOR (M_SPI) FUNCTIONS #################

// Performs the one-time initialization for an ADC ('A' or 'B').
void ADC_Init(char channel) {
    Serial.print("Initializing ADC on side ");
    Serial.println(channel);

    // Sync, Reset, Configuration, and Calibration sequence
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

// Reads and prints all 4 channels from a pre-initialized ADC ('A' or 'B').
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

// ################# TWAI (CAN) FUNCTIONS #################

// Initializes the TWAI driver.
void twai_init() {
  twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(
      (gpio_num_t)CAN_TX_PIN, 
      (gpio_num_t)CAN_RX_PIN, 
      TWAI_MODE_NORMAL
  );
  g_config.alerts_enabled = TWAI_ALERT_NONE;
  twai_timing_config_t t_config = TWAI_TIMING_CONFIG_125KBITS();
  twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

  if (twai_driver_install(&g_config, &t_config, &f_config) == ESP_OK) {
    Serial.println("TWAI driver installed successfully.");
  } else {
    Serial.println("Failed to install TWAI driver.");
    return;
  }

  if (twai_start() == ESP_OK) {
    Serial.println("TWAI driver started.");
  } else {
    Serial.println("Failed to start TWAI driver.");
  }
}

// Prints a TWAI/CAN frame to the Serial monitor.
void print_frame(const twai_message_t* message, const char* direction) {
  Serial.printf("%s Frame | ID: 0x%03lX, ", direction, message->identifier);
  Serial.printf("Ext: %d, RTR: %d, DLC: %d, Data: ", message->extd, message->rtr, message->data_length_code);
  for (int i = 0; i < message->data_length_code; i++) {
    Serial.printf("%02X ", message->data[i]);
  }
  Serial.println();
}

// #################### MAIN SETUP & LOOP ##########################

void setup() {
    Serial.begin(115200);
    while (!Serial);
    Serial.println("\n--- Fused VCAN Power, ADC, and CAN Test ---");

    // --- Configure Hardware Pins ---
    pinMode(CH_SEL_PIN, OUTPUT);
    pinMode(C_CS_PIN, OUTPUT);
    pinMode(M_CS_PIN, OUTPUT);
    digitalWrite(C_CS_PIN, HIGH); // Deselect I/O Expander bus
    digitalWrite(M_CS_PIN, HIGH); // Deselect ADC bus

    // --- Initialize SPI Busses ---
    c_spi.begin(); // Uses default VSPI pins
    m_spi.begin(M_SCK_PIN, M_MISO_PIN, M_MOSI_PIN); // Uses specific HSPI pins

    // --- Initialize and Enable Power Control ---
    Serial.println("\nInitializing I/O Expanders...");
    setupIoExpander('A');
    setupIoExpander('B');
    Serial.println("Turning VCAN Power ON for both channels...");
    setVcanPower('A', 0x03); // Turn VCAN A ON
    setVcanPower('B', 0x03); // Turn VCAN B ON
    delay(500); // Wait for power to stabilize

    // --- Initialize ADCs ---
    Serial.println("\nInitializing ADCs...");
    ADC_Init('A');
    ADC_Init('B');

    // --- Initialize TWAI (CAN) Driver ---
    Serial.println("\nInitializing TWAI (CAN) Driver...");
    twai_init();
    
    // --- Perform Initial Voltage Reading ---
    Serial.println("\n--- Performing initial voltage reading ---");
    ADC_ReadAll('A');
    ADC_ReadAll('B');

    Serial.println("\n--- Initialization Complete ---");
}

void loop() {
  // Periodically send a CAN request
  if (millis() - last_request_time >= request_interval) {
    last_request_time = millis();
    Serial.println("\n---------------------------------");

    // Create and transmit a CAN request message
    twai_message_t tx_message;
    tx_message.identifier = 0x580 + NODE_ID_TO_REQUEST;
    tx_message.extd = 0;             // Standard frame
    tx_message.rtr = 0;              // Not a remote frame
    tx_message.data_length_code = 0; // No data needed for this request

    if (twai_transmit(&tx_message, pdMS_TO_TICKS(1000)) == ESP_OK) {
      print_frame(&tx_message, "TX");
    } else {
      Serial.println("Failed to send CAN request.");
    }
  }

  // Continuously check for received CAN messages
  twai_message_t rx_message;
  esp_err_t rx_status = twai_receive(&rx_message, pdMS_TO_TICKS(10));

  if (rx_status == ESP_OK) {
    print_frame(&rx_message, "RX");
  } else if (rx_status != ESP_ERR_TIMEOUT) {
    // An error occurred during receive, other than a timeout (which is normal)
    Serial.printf("Error receiving CAN frame: %s\n", esp_err_to_name(rx_status));
  }
}

