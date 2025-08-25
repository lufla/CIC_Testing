//
// Created by fladlon on 22.08.25.
//

#ifndef QC_TESTER_FIRMWARE_CHANNEL_B_H
#define QC_TESTER_FIRMWARE_CHANNEL_B_H


#pragma once
#include <ArduinoJson.h>

namespace ChannelB {
    void initialize();
    void executeCommand(JsonDocument& doc);
}

#endif //QC_TESTER_FIRMWARE_CHANNEL_B_H