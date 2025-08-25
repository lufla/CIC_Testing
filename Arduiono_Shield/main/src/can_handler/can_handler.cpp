#include "can_handler.h"

// Set the pins for the MCP2515
const int CS_PIN = 10;
unsigned char data[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
byte sndStat = 0;
long canId;
unsigned char len = 0;
unsigned char buf[8];

// Create an instance of the MCP2515 CAN controller object.
MCP_CAN CAN0(CS_PIN);

/**
 * @brief Initializes the MCP2515 CAN controller.
 */
void setupCAN() {
    // Initialize MCP2515 with 20MHz crystal frequency.
    if (CAN0.begin(MCP_ANY, CAN_125KBPS, MCP_20MHZ) == CAN_OK) {
        Serial.println("MCP2515 Initialized Successfully!");
    } else {
        Serial.println("Error Initializing MCP2515. Halting.");
        // If initialization fails, loop forever to halt the program.
        while (1) {
            Serial.println("FATAL ERROR: MCP2515 Initialization Failed!");
            delay(2000); // Print every 2 seconds
        }
    }

    CAN0.setMode(MCP_NORMAL);
    Serial.println("MCP2515 set to Normal Mode. Listening for requests...");
}

/**
 * @brief Constructs and sends the response CAN message.
 * @param receivedNodeID The node ID extracted from the incoming message.
 */
void sendResponse(byte receivedNodeID) {
    // Data payload is 8 bytes, all set to 0x00.
    unsigned char data[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    long responseId = 0x700 + receivedNodeID;

    // Send the message.
    // Arguments: CAN ID, isExtendedFrame (0 for standard), dataLength, dataPayload
    byte sndStat = CAN0.sendMsgBuf(responseId, 0, 8, data);

    if (sndStat == CAN_OK) {
        Serial.print("Response message sent with ID: 0x");
        Serial.println(responseId, HEX);
    } else {
        Serial.println("Error Sending Response Message...");
    }
}

/**
 * @brief Checks for received CAN messages and triggers a response if the ID matches.
 */
void checkCANReceive() {
    // Check if data is available
    if (CAN0.checkReceive() == CAN_MSGAVAIL) {
        long canId;
        unsigned char len = 0;
        unsigned char buf[8];

        // Read the data: CAN ID, length, and data buffer
        CAN0.readMsgBuf(&canId, &len, buf);

        // Check if the received message ID is in the request range (e.g., 0x580 - 0x582).
        // This prevents responding to unrelated CAN traffic.
        if (canId >= 0x580 && canId < 0x590) {
            // Dynamically determine the node ID from the received message.
            byte receivedNodeID = canId - 0x580;
            Serial.println("-----------------------------");
            Serial.print("Request received for Node ID: ");
            Serial.print(receivedNodeID);
            Serial.print(" (from CAN ID 0x");
            Serial.print(canId, HEX);
            Serial.println(")");
            Serial.println("-----------------------------");

            sendResponse(receivedNodeID);
        }
    }
}
