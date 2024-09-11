/*
 * Copyright (c) 2021 Max Thomas
 * This file is part of DSiWifi and is distributed under the MIT license.
 * See dsiwifi_license.txt for terms of use.
 */

#ifndef _DSIWIFI_BMI_H
#define _DSIWIFI_BMI_H

#define BMI_NOP                 (0x0) // No-op
#define BMI_DONE                (0x1) // Boot firmware
#define BMI_READ_MEMORY         (0x2) // Xtensa memory read (u8)
#define BMI_WRITE_MEMORY        (0x3) // Xtensa memory write (u8)
#define BMI_EXECUTE             (0x4) // Jump to a boot stub (used to read I2C EEPROM)
#define BMI_SET_APP_START       (0x5) // Set BMI_DONE entry addr (optional)
#define BMI_READ_SOC_REGISTER   (0x6) // Xtensa memory read (u32, for IO)
#define BMI_WRITE_SOC_REGISTER  (0x7) // Xtensa memory write (u32, for IO)
#define BMI_GET_TARGET_ID       (0x8) // Get ROM version
#define BMI_ROMPATCH_INSTALL    (0x9) // Install ROM patch
#define BMI_ROMPATCH_UNINSTALL  (0xA) // Uninstall ROM patch
#define BMI_ROMPATCH_ACTIVATE   (0xB) // Activate ROM patch
#define BMI_ROMPATCH_DEACTIVATE (0xC) // Deactivate ROM patch
#define BMI_LZ_STREAM_START     (0xD) // Begin LZ77 decompression to address
#define BMI_LZ_DATA             (0xE) // Send LZ77 compressed data

#endif // _DSIWIFI_BMI_H
