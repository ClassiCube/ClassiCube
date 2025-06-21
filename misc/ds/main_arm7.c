// SPDX-License-Identifier: Zlib
//
// Copyright (C) 2005 Michael Noland (joat)
// Copyright (C) 2005 Jason Rogers (Dovoto)
// Copyright (C) 2005-2015 Dave Murphy (WinterMute)
// Copyright (C) 2023 Antonio Niño Díaz

// Default ARM7 core
#include <dswifi7.h>
#include <nds.h>

volatile bool exit_loop = false;

static void power_button_callback(void) {
    exit_loop = true;
}

static void vblank_handler(void) {
    inputGetAndSend();
    Wifi_Update();
}

int main(int argc, char *argv[]) {
    enableSound();
    readUserSettings();
    ledBlink(0);
    touchInit();

    irqInit();
    irqSet(IRQ_VBLANK, vblank_handler);

    fifoInit();
    installWifiFIFO();
    installSoundFIFO();
    installSystemFIFO();

    setPowerButtonCB(power_button_callback);

    initClockIRQTimer(3);
    irqEnable(IRQ_VBLANK | IRQ_RTC);

    while (!exit_loop)
    {
        const uint16_t key_mask = KEY_SELECT | KEY_START | KEY_L | KEY_R;
        uint16_t keys_pressed = ~REG_KEYINPUT;

        if ((keys_pressed & key_mask) == key_mask)
            exit_loop = true;

        swiWaitForVBlank();
    }

    return 0;
}
