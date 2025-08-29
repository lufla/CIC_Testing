#include "src/spi_handler/spi_handler.h" // For Slave role
#include <SPI.h> // For Master role

// Enum to define the role of the ESP32.
enum Role { UNKNOWN, MASTER, SLAVE };
Role currentRole = UNKNOWN;

// --- SHARED PIN DEFINITIONS ---
#define MASTER_SLAVE_PIN 34
#define UART_SERIAL Serial1
#define UART_TX_PIN 17
#define UART_RX_PIN 16
#define UART_BAUD_RATE 115200


// ####################################################################
// #                         MASTER ONLY CODE                         #
// ####################################################################

// --- MASTER State & Pin Definitions ---
bool master_last_power_state = false; // Tracks if the last command was power ON or OFF
const int CH_SEL_PIN = 32;
const int C_CS_PIN = 5;
SPIClass c_spi(VSPI);
const byte MCP23S08_IODIR = 0x00;
const byte MCP23S08_GPIO = 0x09;
const byte MCP23S08_WRITE_OPCODE = 0x40;
const int M_CS_PIN = 15;
const int M_MOSI_PIN = 13;
const int M_MISO_PIN = 12;
const int M_SCK_PIN = 14;
SPIClass m_spi(HSPI);

// --- MASTER Helper Functions ---
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
void selectADC(char channel) {
    digitalWrite(CH_SEL_PIN, (channel == 'A' || channel == 'a') ? HIGH : LOW);
    digitalWrite(M_CS_PIN, LOW);
}
void deselectADC() {
    digitalWrite(M_CS_PIN, HIGH);
}

void setupIoExpander(char channel) {
    selectControlDevice(channel);
    c_spi.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0));
    c_spi.transfer(MCP23S08_WRITE_OPCODE);
    c_spi.transfer(MCP23S08_IODIR);
    c_spi.transfer(0x00);
    c_spi.endTransaction();
    deselectControlDevice();
}

void setVcanPower(char channel, byte setting) {
    selectControlDevice(channel);
    c_spi.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0));
    c_spi.transfer(MCP23S08_WRITE_OPCODE);
    c_spi.transfer(MCP23S08_GPIO);
    c_spi.transfer(setting);
    c_spi.endTransaction();
    deselectControlDevice();
}

void ADC_Init(char channel) {
    selectADC(channel); for (int i = 0; i < 15; i++) { m_spi.transfer(0xff); } m_spi.transfer(0xfe); delay(1); deselectADC();
    selectADC(channel); m_spi.transfer(0x03); m_spi.transfer(0x00); m_spi.transfer(0x00); m_spi.transfer(0x80); delay(1); deselectADC(); delay(1);
    selectADC(channel); m_spi.transfer(0x03); m_spi.transfer(0x00); m_spi.transfer(0x00); m_spi.transfer(0x00); delay(1); deselectADC(); delay(2);
    selectADC(channel); m_spi.transfer(0x03); m_spi.transfer(0x00); m_spi.transfer(0x30); m_spi.transfer(0x00); delay(1); deselectADC(); delay(2);
    selectADC(channel); m_spi.transfer(0x05); m_spi.transfer(0x02); m_spi.transfer(0xb0); m_spi.transfer(0xab); m_spi.transfer(0x12); m_spi.transfer(0xb1); m_spi.transfer(0xab); delay(1); deselectADC(); delay(2);
    for (int ch = 0; ch < 4; ch++) {
        int channelbyte = (ch * 8) | 0x81; selectADC(channel); delay(10); m_spi.transfer(channelbyte); deselectADC(); delay(10);
        channelbyte = (ch * 8) | 0x82; selectADC(channel); delay(10); m_spi.transfer(channelbyte); deselectADC(); delay(10);
    }
}

float readVcanVoltage(char channel) {
    int adc_ch = 0;
    selectADC(channel);
    int channelbyte = (adc_ch * 8) | 0x80;
    m_spi.transfer(channelbyte);
    m_spi.transfer(0x00); m_spi.transfer(0x00); m_spi.transfer(0x00); m_spi.transfer(0x00);
    deselectADC();
    delay(10);
    selectADC(channel);
    m_spi.transfer(channelbyte);
    m_spi.transfer(0x00);
    int byte2 = m_spi.transfer(0x00);
    int byte3 = m_spi.transfer(0x00);
    m_spi.transfer(0x00);
    deselectADC();
    long ADC_value = (256L * byte2) + byte3;
    float ADC_voltage = (ADC_value / 65535.0) * 2.5;
    ADC_voltage *= 2;
    return ADC_voltage;
}

// ####################################################################
// #                          SLAVE ONLY CODE                         #
// ####################################################################
void perform_adc_check() {
  AdcReadings readings = read_all_adc_values();
  char buffer[100];
  snprintf(buffer, sizeof(buffer), "DATA:%.4f,%.4f,%.4f,%.4f", 
           readings.cic_v, readings.cic_i, readings.vcan_v, readings.vcan_i);
  UART_SERIAL.println(buffer);
}

// ####################################################################
// #                       MAIN LOGIC & LOOPS                         #
// ####################################################################
void slave_loop() {
  if (UART_SERIAL.available() > 0) {
    String command = UART_SERIAL.readStringUntil('\n');
    command.trim();
    if (command == "CHECK_SPI_ADC") {
      perform_adc_check();
    }
  }
}

void master_loop() {
  if (Serial.available() > 0) {
    String command = Serial.readStringUntil('\n');
    command.trim();
    
    if (command.startsWith("SET_VCAN_VOLTAGE")) {
        int setting = command.substring(19).toInt();
        
        bool current_power_state = (setting & 0x03) == 0x03;

        // Set the new power/voltage value for both channels first
        setVcanPower('A', (byte)setting);
        setVcanPower('B', (byte)setting);

        // **FIX:** Apply delay *after* setting the voltage to allow hardware to settle
        if (master_last_power_state && !current_power_state) {
            // If we just turned power OFF (ON -> OFF transition), wait longer for discharge
            delay(300); 
        } else {
            // Standard settling delay for all other cases
            delay(100);
        }
        
        float v_a = readVcanVoltage('A');
        float v_b = readVcanVoltage('B');

        char buffer[50];
        snprintf(buffer, sizeof(buffer), "VCAN_DATA:%.4f,%.4f", v_a, v_b);
        Serial.println(buffer);

        // Update the state for the next cycle
        master_last_power_state = current_power_state;

    } else {
      UART_SERIAL.println(command);
    }
  }

  if (UART_SERIAL.available() > 0) {
    String response = UART_SERIAL.readStringUntil('\n');
    response.trim();
    Serial.println(response);
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\nESP32 Unified Firmware Initializing...");
  pinMode(MASTER_SLAVE_PIN, INPUT_PULLUP);
  delay(10);
  if (digitalRead(MASTER_SLAVE_PIN) == LOW) {
    currentRole = MASTER;
    pinMode(MASTER_SLAVE_PIN, INPUT_PULLDOWN);
    Serial.println("Role: MASTER");
    pinMode(CH_SEL_PIN, OUTPUT);
    pinMode(C_CS_PIN, OUTPUT);
    pinMode(M_CS_PIN, OUTPUT);
    digitalWrite(C_CS_PIN, HIGH);
    digitalWrite(M_CS_PIN, HIGH);
    c_spi.begin();
    m_spi.begin(M_SCK_PIN, M_MISO_PIN, M_MOSI_PIN);
    Serial.println("Initializing I/O Expanders and ADCs...");
    setupIoExpander('A');
    setupIoExpander('B');
    setVcanPower('A', 0x00);
    setVcanPower('B', 0x00);
    ADC_Init('A');
    ADC_Init('B');
    Serial.println("Master hardware initialized.");
    UART_SERIAL.begin(UART_BAUD_RATE, SERIAL_8N1, UART_RX_PIN, UART_TX_PIN);
  } else {
    currentRole = SLAVE;
    Serial.println("Role: SLAVE");
    UART_SERIAL.begin(UART_BAUD_RATE, SERIAL_8N1, UART_RX_PIN, UART_TX_PIN);
    setup_spi_adc();
  }
}

void loop() {
  if (currentRole == MASTER) { master_loop(); } 
  else if (currentRole == SLAVE) { slave_loop(); }
}

