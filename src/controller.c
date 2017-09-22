#include "controller.h"
#include <stdio.h>
#include <string.h>

char *buttons[] = {
        // 0
        "L_PAD",
        "R_PAD",
        "BACK",
        "START",
        "LEFT",
        "RIGHT",
        "DOWN",
        "UP",
        // 1
        "Y",
        "X",
        "B",
        "A",
        "NOTHING",
        "XBOX",
        "L_BTN",
        "R_BTN"
};

char *analog8[] = {
        "L_TRIG",
        "R_TRIG"
};


char *analog16[] = {
        "LEFT_X",
        "LEFT_Y",
        "RIGHT_X",
        "RIGHT_Y"
};


void input_report(unsigned char *buffer) {
    // buttons
    for (int i = 0; i < 16; i++) {
        if ((buffer[2 + (i / 8)] >> (7 - (i % 8))) & 0x1) {
            printf("%s ", buttons[i]);
            button_report[i / 8] = (char) ((0x1 << i % 8) | button_report[i / 8]);
        } else {
            printf("%*c ", (int) strlen(buttons[i]), ' ');
            button_report[i / 8] = (char) (~(0x1 << i % 8) & button_report[i / 8]);
        }
    }

    // triggers
    for (size_t i = 0; i < 2; i++) {
        printf("%s : %3d ", analog8[i], buffer[i + 4]);
        analog8_report[i] = buffer[i + 4];
    }

    // pads
    for (size_t i = 0; i < 8; i += 2) {
        int16_t analog = (int16_t) ((buffer[i + 7] << 8) | (buffer[i + 6] & 0xFF));
        analog16_report[i / 2] = analog;
        printf("%s : %7d ", analog16[i / 2], analog);
    }
}
