
#include "usb_handler.h"
#include "controller_display.h"
#include <stdlib.h>

libusb_hotplug_callback_handle hp[2];

void (*read_callback)(struct libusb_transfer *);

int LIBUSB_CALL hotplug_callback(libusb_context *ctx, libusb_device *dev, libusb_hotplug_event event, void *user_data) {
    struct controller *controller = open_device(dev);
    struct controller * controller_p = controller;
    while (controllers && controllers->device != dev) controller_p++;
    init_controller_window(controller, (int) ((controller_p - controller) / sizeof(struct controller)), connnectedControllers);

    controller->transfer_in = libusb_alloc_transfer(0);

    controller->buffer_in = (unsigned char *) malloc(20 * sizeof(char));
    int rc = start_read_device(controller->transfer_in, controller->handle, controller->buffer_in, 20 * sizeof(char),
                               read_callback, controller);
    if (LIBUSB_SUCCESS != rc) {
        fprintf(stderr, "Error starting device: %s\n", libusb_error_name(rc));
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

int LIBUSB_CALL
hotplug_callback_detach(libusb_context *ctx, libusb_device *dev, libusb_hotplug_event event, void *user_data) {
    struct  controller * controller = user_data;
    while (controller && controller->device != dev) controller++;

    if (controller) {
        delwin(controller->window);
    }
    return close_device(dev);
}

int
usb_init(int vendor_id, int product_id, int class_id, libusb_context *context, void (*read)(struct libusb_transfer *)) {

    libusb_set_debug(context, LIBUSB_LOG_LEVEL_NONE);
    read_callback = read;

    libusb_device **list;
    ssize_t num_devices = libusb_get_device_list(NULL, &list);

    for (int controller_it = 0; controller_it < num_devices; ++controller_it) {
        libusb_device *dev = list[controller_it];
        struct libusb_device_descriptor desc;

        if (libusb_get_device_descriptor(dev, &desc) == LIBUSB_SUCCESS) {
            if (desc.idProduct == product_id && desc.idVendor == vendor_id) {
                open_device(dev);
            }
        }
    }

    libusb_free_device_list(list, 1);

    if (!libusb_has_capability(LIBUSB_CAP_HAS_HOTPLUG)) {
        printf("Hotplug capabilites are not supported on this platform\n");
        libusb_exit(context);
        return EXIT_FAILURE;
    }

    int rc = libusb_hotplug_register_callback(NULL, LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED, 0,
                                              vendor_id, product_id, class_id, hotplug_callback, controllers, &hp[0]);
    if (LIBUSB_SUCCESS != rc) {
        fprintf(stderr, "Error registering callback 0\n");
        return EXIT_FAILURE;
    }

    rc = libusb_hotplug_register_callback(NULL, LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT, 0, vendor_id,
                                          product_id, class_id, hotplug_callback_detach, controllers, &hp[1]);
    if (LIBUSB_SUCCESS != rc) {
        fprintf(stderr, "Error registering callback 1\n");
        return EXIT_FAILURE;
    }
    int controller_it;
    struct controller *controller = controllers;
    for (controller_it = 0; controller_it < NUMBER_OF_DEVICES; controller_it++) {
        if (controller->device == NULL) {
            controller++;
            continue;
        }
        controller->transfer_in = libusb_alloc_transfer(0);

        controller->buffer_in = (unsigned char *) malloc(20 * sizeof(char));
        controller->write_endpoint = 0x1;

        rc = start_read_device(controller->transfer_in, controller->handle, controller->buffer_in, 20 * sizeof(char),
                               read_callback, controller);
        if (LIBUSB_SUCCESS != rc) {
            fprintf(stderr, "Error starting device %d: %s\n", controller_it, libusb_error_name(rc));
            return EXIT_FAILURE;
        }
        controller++;
    }

    return EXIT_SUCCESS;
}

struct controller *open_device(libusb_device *dev) {
    struct libusb_device_descriptor desc;
    int rc;
    int controller_it;

    struct controller *controller = controllers;
    for (controller_it = 0; controller_it < NUMBER_OF_DEVICES; controller_it++) {
        if (!controller->device) break;
        controller++;
    }

    rc = libusb_get_device_descriptor(dev, &desc);
    if (LIBUSB_SUCCESS != rc) {
        fprintf(stderr, "Error getting device descriptor\n");
    }


    libusb_device_handle *handle;
    rc = libusb_open(dev, &handle);

    snprintf(controller->name, sizeof controller->name, "%s%d", "Controller ", ++connnectedControllers);
    controller->device = dev;
    controller->handle = handle;


    if (LIBUSB_SUCCESS != rc) {
        fprintf(stderr, "Error opening device : %s\n", libusb_error_name(rc));
        return NULL;
    }
    libusb_detach_kernel_driver(handle, 0);
    rc = libusb_set_configuration(handle, 1);
    if (LIBUSB_SUCCESS != rc) {
        fprintf(stderr, "Error setting config : %s\n", libusb_error_name(rc));
        return NULL;
    }
    rc = libusb_claim_interface(handle, 0);
    if (LIBUSB_SUCCESS != rc) {
        fprintf(stderr, "Error claiming device : %s\n", libusb_error_name(rc));
        return NULL;
    }

    printf("Device %d attached: %04x:%04x\n", controller_it, desc.idVendor, desc.idProduct);
    return controller;
}

int close_device(libusb_device *dev) {

    int controller_it = 0;
    struct controller *controller = controllers;
    for (controller_it = 0; controller_it < NUMBER_OF_DEVICES; controller_it++) {
        if (controller->device == dev) break;
        controller++;
    }

    printf("Device %s free\n", controller->name);
    free(controller->buffer_in);

    printf("Device %d detached\n", controller_it);
    if (controller->handle) {
        libusb_attach_kernel_driver(controller->handle, 0);
        libusb_reset_device(controller->handle);
        int rc = libusb_release_interface(controller->handle, 0);
        libusb_close(controller->handle);
    }
    connnectedControllers--;

    printf("Device %d freed\n", controller_it);

    return 0;
}

int start_read_device(
        struct libusb_transfer *transfer,
        libusb_device_handle *handle,
        unsigned char *data,
        int length,
        libusb_transfer_cb_fn callback,
        struct controller *controller
) {
    libusb_fill_interrupt_transfer(transfer, handle, 0x81, data, length, callback, controller, 0);
    return libusb_submit_transfer(transfer);
}

int write_device(
        struct controller *controller,
        unsigned char *data,
        int length,
        libusb_transfer_cb_fn callback
) {
    struct libusb_transfer *transfer = libusb_alloc_transfer(0);
    libusb_fill_interrupt_transfer(transfer, controller->handle, controller->write_endpoint, data, length, callback,
                                   NULL, 0);
    return libusb_submit_transfer(transfer);
}
