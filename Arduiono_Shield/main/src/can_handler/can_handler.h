#ifndef CAN_HANDLER_H
#define CAN_HANDLER_H

#include <Arduino.h>
#include <mcp_can.h>
#include <SPI.h>

#define sentNodeID1 1
#define sentNodeID2 2

const int SPI_CS_PIN = 10;

/**
 * @brief Initializes the MCP2515 CAN controller.
 */
void setupCAN();

/**
 * @brief Sends the specific response message.
 * @param localNodeID The node ID to use for constructing the CAN message ID.
 */
void sendResponse(byte localNodeID);

/**
 * @brief Checks for received CAN messages and prints them to the terminal.
 */
void checkCANReceive();


#endif //CAN_HANDLER_H
