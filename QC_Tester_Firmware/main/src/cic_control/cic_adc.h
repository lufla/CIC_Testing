//
// Created by fladlon on 25.08.25.
//

#ifndef QC_TESTER_FIRMWARE_CIC_ADC_H
#define QC_TESTER_FIRMWARE_CIC_ADC_H

#pragma once
#include <ArduinoJson.h>

namespace CIC_ADC {
    // Initializes the ADC with a full reset and calibration sequence.
    void initialize(int cs_pin);

    // Reads a specific channel from the ADC.
    // channel_name can be "VCAN", "IMON", "UH", or "TEMP"
    float readChannel(int cs_pin, const char* channel_name);
}




#endif //QC_TESTER_FIRMWARE_CIC_ADC_H