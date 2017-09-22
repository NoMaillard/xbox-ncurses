
#include <libusb-1.0/libusb.h>
#include "controller.h"

#ifndef NUMBER_OF_DEVICES
#define NUMBER_OF_DEVICES 2
#endif
#ifndef USB_HELPER_H
#define USB_HELPER_H 1

struct controller *controllers;


int usb_init(int vendor_id, int product_id, int class_id, libusb_context *context, void (*read)(struct libusb_transfer *));

int close_device(libusb_device *dev);

struct controller *open_device(libusb_device *dev);

int start_read_device(struct libusb_transfer *transfer,
                      libusb_device_handle *handle,
                      unsigned char *data,
                      int length,
                      libusb_transfer_cb_fn callback);

int write_device(libusb_device_handle *handle,
                 unsigned char *data,
                 int length,
                 libusb_transfer_cb_fn callback);


#endif
