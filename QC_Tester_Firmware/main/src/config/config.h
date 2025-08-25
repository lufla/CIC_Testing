//
// Created by fladlon on 22.08.25.
//

#ifndef QC_TESTER_FIRMWARE_CONFIG_H
#define QC_TESTER_FIRMWARE_CONFIG_H


// config.h

#pragma once
#define STATION_ID "QC-Station-01"

// --- ROLE SELECTION ---
// Pin to determine if the ESP32 is Master or Slave
// Connect to GND for SLAVE, leave floating for MASTER
// GPIO 2 is next to a GND pin on most ESP32 DevKit boards.
#define ROLE_SELECT_PIN 2

// --- SERIAL COMMUNICATION ---
#define PC_SERIAL_BAUD 115200
#define SLAVE_SERIAL Serial2 // UART for Master-Slave communication
#define SLAVE_SERIAL_BAUD 115200
#define SLAVE_RX_PIN 16
#define SLAVE_TX_PIN 17

// --- I2C PINS ---
#define I2C_SDA_PIN 21
#define I2C_SCL_PIN 22

// --- SPI PINS (for IO Expander MCP23S08) ---
// The ESP32 has two main SPI buses: VSPI and HSPI
// VSPI is commonly used and has default pins.
#define IO_EXPANDER_SPI_HOST VSPI_HOST
#define IO_EXPANDER_MISO_PIN 19
#define IO_EXPANDER_MOSI_PIN 23
#define IO_EXPANDER_SCLK_PIN 18

// --- CHANNEL A (MASTER) PINS ---
#define CH_A_IO_EXPANDER_CS_PIN 15 // MCP23S08 Chip Select
#define CH_A_CAN_TX_PIN 4
#define CH_A_CAN_RX_PIN 5 // Changed from 2 to avoid conflict with ROLE_SELECT_PIN

// --- CHANNEL B (SLAVE) PINS ---
#define CH_B_IO_EXPANDER_CS_PIN 15 // MCP23S08 CS (on slave)
#define CH_B_CAN_TX_PIN 4          // (on slave)
#define CH_B_CAN_RX_PIN 5          // (on slave)

// --- I2C ADDRESSES ---
// Note: Verify these with an I2C scanner if they don't work.
#define CH_A_VOLTAGE_ADC_ADDR 0x48 // TLA2022IRUGR Address
#define CH_A_CURRENT_DAC_ADDR 0x60 // MCP4726A0T-E/CH Address

#define CH_B_VOLTAGE_ADC_ADDR 0x49 // TLA2022IRUGR Address (assuming different)
#define CH_B_CURRENT_DAC_ADDR 0x61 // MCP4726A0T-E/CH Address (assuming different)


#endif //QC_TESTER_FIRMWARE_CONFIG_H