//
// Created by fladlon on 22.08.25.
//

#ifndef QC_TESTER_FIRMWARE_SLAVE_FLASHER_H
#define QC_TESTER_FIRMWARE_SLAVE_FLASHER_H

#pragma once

class SlaveFlasher {
public:
    SlaveFlasher();
    void begin();
    void updateFirmware();
};

#endif //QC_TESTER_FIRMWARE_SLAVE_FLASHER_H