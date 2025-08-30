// include the SPI library:
#include <SPI.h>
#include <math.h>

// ################# CONSTANTS #################

// CAN Test Signal Pins (Originally CANTXA/B)
const int CANTX_Pin = 17; // ESP32's TX2, used for PWM signal
const int CANRX_Pin = 16; // ESP32's RX2

// SPI Bus Pins (VSPI default)
const int SPI_MOSI_PIN = 13;
const int SPI_MISO_PIN = 12;
const int SPI_SCK_PIN = 14;

// Chip Select Control Pins
const int MCS_PIN = 15;    // Chip Select for Monitor bus (ADCs)
const int CH_SEL_PIN = 4;  // Selects between device A (HIGH) and B (LOW)

// PWM Configuration for CAN Test Signal
const int PWM_FREQ = 980;       // Match original Arduino's ~980 Hz
const int PWM_RESOLUTION = 8;   // 8-bit resolution (0-255)

// ################# HELPER FUNCTIONS FOR CHIP SELECT #################

/**
 * @brief Selects an ADC chip.
 * @param side Selects device A or B. 1 for A (CH_SEL=HIGH), 2 for B (CH_SEL=LOW).
 */
void selectADC(int side) {
  if (side == 1) {
    digitalWrite(CH_SEL_PIN, HIGH); // Select device 'A'
  } else {
    digitalWrite(CH_SEL_PIN, LOW); // Select device 'B'
  }
  digitalWrite(MCS_PIN, LOW); // Activate the ADC bus
}

/**
 * @brief Deselects the ADC chip.
 */
void deselectADC() {
  digitalWrite(MCS_PIN, HIGH);
}

// ################# ADC FUNCTIONS #################

/**
 * @brief Performs the one-time initialization for an ADC.
 * Includes sync, reset, configuration, and calibration of all channels.
 * @param side The ADC to initialize (1 for A, 2 for B).
 */
void ADC_Init(int side) {
  Serial.print("Initializing ADC on side ");
  Serial.println(side == 1 ? "A" : "B");

  // Sync sequence
  selectADC(side);
  for (int i = 0; i < 15; i++) {
    SPI.transfer(0xff);
  }
  SPI.transfer(0xfe);
  delay(1);

  // ADC-Reset
  SPI.transfer(0x03); SPI.transfer(0x00); SPI.transfer(0x00); SPI.transfer(0x80);
  delay(1);
  deselectADC();
  delay(1);
  selectADC(side);
  delay(1);
  SPI.transfer(0x03); SPI.transfer(0x00); SPI.transfer(0x00); SPI.transfer(0x00);
  delay(1);
  deselectADC();
  delay(2);

  // ADC-configuration
  selectADC(side);
  SPI.transfer(0x03); SPI.transfer(0x00); SPI.transfer(0x30); SPI.transfer(0x00);
  delay(1);
  deselectADC();
  delay(2);

  // Write configuration registers
  selectADC(side);
  SPI.transfer(0x05);
  SPI.transfer(0x02); SPI.transfer(0xb0); SPI.transfer(0xab);
  SPI.transfer(0x12); SPI.transfer(0xb1); SPI.transfer(0xab);
  delay(1);
  deselectADC();
  delay(2);

  // Calibrate all channels
  for (int channel = 0; channel < 4; channel++) {
    int channelbyte;
    // Offset cal
    selectADC(side);
    delay(10);
    channelbyte = (channel * 8) | 0x81;
    SPI.transfer(channelbyte);
    deselectADC();
    delay(10);

    // Gain cal
    selectADC(side);
    delay(10);
    channelbyte = (channel * 8) | 0x82;
    SPI.transfer(channelbyte);
    deselectADC();
    delay(10);
  }
  Serial.println("Initialization complete.");
}

/**
 * @brief Reads a single value from a pre-initialized ADC channel and prints it.
 * @param side The ADC to read from (1 for A, 2 for B).
 * @param channel The channel to read (0-3).
 */
void ADC_Read(int side, int channel) {
  int channelbyte;
  long ADC_value;
  float ADC_voltage;

  // Start conversion
  selectADC(side);
  delay(10);
  channelbyte = (channel * 8) | 0x80;
  SPI.transfer(channelbyte);
  SPI.transfer(0x00); SPI.transfer(0x00); SPI.transfer(0x00); SPI.transfer(0x00);
  delay(1);
  deselectADC();
  delay(100); // Wait for conversion

  // Read the result
  selectADC(side);
  SPI.transfer(channelbyte);
  SPI.transfer(0x00); // Dummy byte
  int byte2 = SPI.transfer(0x00);
  int byte3 = SPI.transfer(0x00);
  SPI.transfer(0x00); // Dummy byte

  // Print data in a compact format. Prefix ("A:" or "B:") is handled in the loop.
  Serial.print("ch");
  Serial.print(channel);
  Serial.print("=");
  ADC_value = (256L * byte2) + byte3;
  Serial.print(ADC_value);

  // Also print the calculated millivolts for quick reference
  ADC_voltage = ADC_value;
  ADC_voltage = ADC_voltage / 65535;
  ADC_voltage = ADC_voltage * 2.5;
  if (channel < 3) {
    ADC_voltage = ADC_voltage * 2;
  }
  Serial.print(" (");
  Serial.print(ADC_voltage * 1000, 0); // Print mV with no decimal places
  Serial.print("mV)");

  delay(1);
  deselectADC();
  delay(1);
}

// #################### SETUP ##########################
void setup() {
  pinMode(MCS_PIN, OUTPUT);
  pinMode(CH_SEL_PIN, OUTPUT);
  deselectADC(); // Deselect the ADC bus initially

  ledcAttach(CANTX_Pin, PWM_FREQ, PWM_RESOLUTION);

  SPI.begin(SPI_SCK_PIN, SPI_MISO_PIN, SPI_MOSI_PIN);
  SPI.beginTransaction(SPISettings(10000, MSBFIRST, SPI_MODE0));

  pinMode(CANTX_Pin, OUTPUT);
  pinMode(CANRX_Pin, INPUT);

  Serial.begin(115200);
  delay(2000); // Wait for serial monitor to connect

  // Initialize both ADCs once
  ADC_Init(1); // Initialize Side A
  ADC_Init(2); // Initialize Side B
}

// #################### MAIN LOOP ##########################
void loop() {
  // -------- Read and print ALL ADC channel values in a compact format ------
  Serial.println("\n---------------------------------");
  Serial.println("Reading ADC Values:");
  Serial.print("A: ");
  for (int i = 0; i < 4; i++) {
    ADC_Read(1, i);
    if (i < 3) {
      Serial.print(" | "); // Separator between channel readings
    }
  }
  Serial.println(); // End of line for Side A

  Serial.print("B: ");
  for (int i = 0; i < 4; i++) {
    ADC_Read(2, i);
    if (i < 3) {
      Serial.print(" | "); // Separator between channel readings
    }
  }
  Serial.println(); // End of line for Side B
  Serial.println("---------------------------------");

  delay(500);

  // Send "CAN-Signal" using ESP32's LEDC PWM
  ledcWrite(CANTX_Pin, 64);
  ledcWrite(CANTX_Pin, 255);
}