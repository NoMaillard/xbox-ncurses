
#include <libusb-1.0/libusb.h>
#include <stdbool.h>
#include <curses.h>

#ifndef CONTROLLER_H
#define CONTROLLER_H 1


struct controller {
    char name[32];
    libusb_device *device;
    libusb_device_handle *handle;
    struct libusb_transfer *transfer_in;
    unsigned char *buffer_in;
    char button_report[2];
    uint8_t analog8_report[2];
    int16_t analog16_report[4];
    WINDOW *window;
    unsigned char write_endpoint;
};

int getButtonState(struct controller * controller, char * button);
void input_report(unsigned char *buffer, struct controller *controller);

#endif
