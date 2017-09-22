#include "usb_handler.h"
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <pthread.h>
#include <unistd.h>

static bool volatile exitProgram = false;

int product_id, vendor_id, class_id;

void exitHandler(int dummy) {
    exitProgram = true;
}

void *usb_thread(void *arg) {
    libusb_context *context = (libusb_context *) arg;
    int rc;
    while (!exitProgram) {
        rc = libusb_handle_events_completed(context, NULL);
        if (LIBUSB_SUCCESS != rc) {
            fprintf(stderr, "Error handling event: %s\n", libusb_error_name(rc));
            break;
        }
    }
}

void write_callback(struct libusb_transfer *transfer) {
    libusb_free_transfer(transfer);
}

void read_cb(struct libusb_transfer *transfer) {
    int rc;
    // report input
    input_report(transfer->buffer);
    printf("\n");
    // if a buuton is pressed -> leds
    if (button_report[1]) {
        unsigned char leds[] = {0x01, 0x03, (unsigned char) button_report[1]};
        rc = write_device(transfer->dev_handle, leds, sizeof(leds), write_callback);
        if (LIBUSB_SUCCESS != rc) {
            fprintf(stderr, "Error setting leds : %s\n", libusb_error_name(rc));
        }
    } else {
        // rumble command from triggers
        unsigned char data[] = {0x00, 0x08, 0x00, analog8_report[0], analog8_report[1], 0x00, 0x00, 0x00};
        rc = write_device(transfer->dev_handle, data, sizeof(data), write_callback);
        if (LIBUSB_SUCCESS != rc) {
            fprintf(stderr, "Error setting rumble : %s\n", libusb_error_name(rc));
        }
    }

    //read again
    rc = libusb_submit_transfer(transfer);
    if (LIBUSB_SUCCESS != rc) {
        fprintf(stderr, "Error retransferring : %s\n", libusb_error_name(rc));
    }
}

int main(int argc, char *argv[]) {

    libusb_context *context = NULL;
    controllers = (struct controller *) calloc(sizeof(struct controller), NUMBER_OF_DEVICES);
    signal(SIGINT, exitHandler);
    int rc = libusb_init(&context);
    if (LIBUSB_SUCCESS != rc) {
        fprintf(stderr, "Error initializing : %s\n", libusb_error_name(rc));
        return EXIT_FAILURE;
    }


    vendor_id = (argc > 1) ? (int) strtol(argv[1], NULL, 0) : 0x045e;
    product_id = (argc > 2) ? (int) strtol(argv[2], NULL, 0) : 0x028e;
    class_id = (argc > 3) ? (int) strtol(argv[3], NULL, 0) : LIBUSB_HOTPLUG_MATCH_ANY;
    rc = usb_init(vendor_id, product_id, class_id, context, &read_cb);
    if (LIBUSB_SUCCESS != rc) {
        fprintf(stderr, "Error init: %s\n", libusb_error_name(rc));
    }

    pthread_t usb_pid = NULL;
    rc = pthread_create(&usb_pid, NULL, &usb_thread, context);

    while (!exitProgram) {
        printf("THREADED\n");
        sleep(1);
    }

    pthread_join(usb_pid, NULL);

    // close everything
    int controller_it;
    struct controller *controller = controllers;
    for (controller_it = 0; controller_it < NUMBER_OF_DEVICES; controller_it++) {
        if (!controller->device) {
            controller++;
            continue;
        }
        close_device(controller->device);
        controller++;
    }

    printf("exiting usb...\n");
    libusb_exit(context);
    printf("exited usb...\n");


    return EXIT_SUCCESS;
}
