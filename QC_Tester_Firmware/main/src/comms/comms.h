//
// Created by fladlon on 22.08.25.
//

#ifndef QC_TESTER_FIRMWARE_COMMS_H
#define QC_TESTER_FIRMWARE_COMMS_H

#pragma once
#include <Arduino.h>

void handlePCCommand(String cmd);
void handleMasterCommand(String cmd);

#endif //QC_TESTER_FIRMWARE_COMMS_H
