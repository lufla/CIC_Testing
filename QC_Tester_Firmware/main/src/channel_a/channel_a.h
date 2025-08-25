//
// Created by fladlon on 22.08.25.
//

#ifndef QC_TESTER_FIRMWARE_CHANNEL_A_H
#define QC_TESTER_FIRMWARE_CHANNEL_A_H

//TODO can cannel A and be be fused and use less code ?
#pragma once
#include <ArduinoJson.h>

namespace ChannelA {
    void initialize();
    void executeCommand(JsonDocument& doc);
}

#endif //QC_TESTER_FIRMWARE_CHANNEL_A_H