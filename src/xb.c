#include "usb_handler.h"
#include "controller_display.h"
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <pthread.h>

static bool volatile exitProgram = false;

pthread_mutex_t controller_mutex;
pthread_cond_t usb_init_cond;

int product_id, vendor_id, class_id;

void exitHandler(int dummy) {
    exitProgram = true;
}

void write_callback(struct libusb_transfer *transfer) {
    libusb_free_transfer(transfer);
}

void print_button(WINDOW * win, int x, int y, const chtype c){
    mvwaddch(win, x, y, c);
    mvwaddch(win, x - 1, y, ' '|A_REVERSE);
    mvwaddch(win, x - 1, y - 1, ' '|A_REVERSE);
    mvwaddch(win, x - 1, y + 1, ' '|A_REVERSE);
    mvwaddch(win, x + 1, y, ' '|A_REVERSE);
    mvwaddch(win, x + 1, y - 1, ' '|A_REVERSE);
    mvwaddch(win, x + 1, y + 1, ' '|A_REVERSE);
    mvwaddch(win, x, y - 1, ' '|A_REVERSE);
    mvwaddch(win, x, y + 1, ' '|A_REVERSE);
}

void read_cb(struct libusb_transfer *transfer) {
    int rc;
    struct controller *controller = transfer->user_data;
    // report input
    if (transfer->buffer[1] == 0x14) {
        pthread_mutex_lock(&controller_mutex);
        input_report(transfer->buffer, controller);
        pthread_mutex_unlock(&controller_mutex);
    }
    // if a buuton is pressed -> leds
    unsigned char led_cmd = (unsigned char) controller->button_report[1];
    if (led_cmd) {
        unsigned char leds[] = {0x01, 0x03, led_cmd};
        rc = write_device(controller, leds, sizeof(leds), write_callback);
        if (LIBUSB_SUCCESS != rc) {
            fprintf(stderr, "Error setting leds : %s\n", libusb_error_name(rc));
        }
    } else {
        // rumble command from triggers
        pthread_mutex_lock(&controller_mutex);
        unsigned char data[] = {0x00, 0x08, 0x00, controller->analog8_report[0], controller->analog8_report[1],
                                0x00, 0x00, 0x00};
        pthread_mutex_unlock(&controller_mutex);
        rc = write_device(controller, data, sizeof(data), write_callback);
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

void *usb_thread(void *arg) {
    libusb_context *context = (libusb_context *) arg;
    int rc;
    rc = usb_init(vendor_id, product_id, class_id, context, &read_cb);
    if (LIBUSB_SUCCESS != rc) {
        fprintf(stderr, "Error init: %s\n", libusb_error_name(rc));
    }
    pthread_cond_broadcast(&usb_init_cond);

    while (!exitProgram) {
        usleep(40000);
        rc = libusb_handle_events_completed(context, NULL);
        if (LIBUSB_SUCCESS != rc) {
            fprintf(stderr, "Error handling event: %s\n", libusb_error_name(rc));
            break;
        }
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

    int ch;

    initscr();            /* Start curses mode 		*/
    cbreak();
    start_color();

    init_color(3, 700, 700, 700);


    // usb_thread
    pthread_mutex_init(&controller_mutex, NULL);
    pthread_cond_init(&usb_init_cond, NULL);
    pthread_t usb_pid = NULL;
    rc = pthread_create(&usb_pid, NULL, &usb_thread, context);
    if (EXIT_SUCCESS != rc) {
        fprintf(stderr, "Error creating thread\n");
    }
    pthread_cond_wait(&usb_init_cond, &controller_mutex);
    pthread_mutex_unlock(&controller_mutex);
    int controller_it;

    struct controller *controller = controllers;
    for (controller_it = 0; controller_it < NUMBER_OF_DEVICES; controller_it++) {
        if (!controller->device)  {
            controller++;
            continue;
        }
        init_controller_window(controller, controller_it + 1, connnectedControllers);
        controller++;
    }
    int windowHeight, windowWidth;
    getmaxyx(stdscr, windowWidth, windowHeight);
    while (!exitProgram) {
        int width, height;
        getmaxyx(stdscr, width, height);
        mvwprintw(stdscr, 0, 18, " %d x %d - %d x %d", width, height, windowWidth, windowHeight);
        if (windowHeight != height || windowWidth != width) {
            windowHeight = height;
            windowWidth = width;
            int i = 0;
            while (controller && controller->window) {
                wresize(controller->window, width / connnectedControllers, width / connnectedControllers * (++i));
                controller++;
            }
        }


        controller = controllers;
        for (controller_it = 0; controller_it < NUMBER_OF_DEVICES; controller_it++) {
            if (!controller->window) {
                controller++;
                continue;
            }
            wclear(controller->window);

            box(controller->window, 0, 0);

            getmaxyx(controller->window, height, width);
            draw_borders(controller->window, 1, 1, (width / 2) - 1, (height / 2) - 1);
            draw_borders(controller->window, 1 + width / 2, 1, (width / 2) - 1, (height / 2) - 1);
            mvwprintw(controller->window, 0, 2, controller->name);
            // sticks
            mvwaddch(controller->window,
                     (int) ((height / 4) - (controller->analog16_report[1] * (height / 4.5) / 0x7FFF)),
                     (int) ((width / 4) + (controller->analog16_report[0] * (width / 4.5) / 0x7FFF)), ' ' | A_REVERSE);
            mvwaddch(controller->window,
                     (int) ((height / 4) - (controller->analog16_report[3] * (height / 4.5) / 0x7FFF)),
                     (int) ((3 * width / 4) + (controller->analog16_report[2] * (width / 4.5) / 0x7FFF)),
                     ' ' | A_REVERSE);
            int i = 1;
            while (controller->analog8_report[0] / i > 1) {
                mvwaddch(controller->window, 1 + height / 2, 1 + (i++ * (width) / 0xFF), ' ' | A_REVERSE);
            }
            i = 1;
            while (controller->analog8_report[1] / i > 1) {
                mvwaddch(controller->window, 1 + height / 2, width - 1 - (i++ * (width) / 0xFF), ' ' | A_REVERSE);
            }

            print_button(controller->window, 1 + (3 * height / 4), 4, getButtonState(controller, "LEFT") ? 'L'|A_REVERSE : ' ');
            print_button(controller->window, 1 + (3 * height / 4), 10, getButtonState(controller, "RIGHT") ? 'R'|A_REVERSE : ' ');
            print_button(controller->window, (3 * height / 4) - 2, 7, getButtonState(controller, "UP") ? 'U'|A_REVERSE : ' ');
            print_button(controller->window, 4 + (3 * height / 4), 7, getButtonState(controller, "DOWN") ? 'D'|A_REVERSE : ' ');
            print_button(controller->window, 1 + (3 * height / 4), 7, getButtonState(controller, "L_PAD") ? 'P'|A_REVERSE : ' ');

            print_button(controller->window, 1 + (3 * height / 4), width - 4, getButtonState(controller, "B") ? 'B'|A_REVERSE : ' ');
            print_button(controller->window, 1 + (3 * height / 4), width - 10, getButtonState(controller, "X") ? 'X'|A_REVERSE : ' ');
            print_button(controller->window, (3 * height / 4) - 2, width - 7, getButtonState(controller, "Y") ? 'Y'|A_REVERSE : ' ');
            print_button(controller->window, 4 + (3 * height / 4), width - 7, getButtonState(controller, "A") ? 'A'|A_REVERSE : ' ');
            print_button(controller->window, 1 + (3 * height / 4), width - 7, getButtonState(controller, "R_PAD") ? 'P'|A_REVERSE : ' ');

            print_button(controller->window,  3 + (height / 2), width - 7, getButtonState(controller, "R_BTN") ? 'R'|A_REVERSE : ' ');
            print_button(controller->window,  3 + (height / 2), 7, getButtonState(controller, "L_BTN") ? 'L'|A_REVERSE : ' ');

            print_button(controller->window, 3 + (height / 2), width / 2 + 4, getButtonState(controller, "START") ? 'S'|A_REVERSE : ' ');
            print_button(controller->window, 3 + (height / 2), width / 2, getButtonState(controller, "XBOX") ? 'X'|A_REVERSE : ' ');
            print_button(controller->window, 3 + (height / 2), width / 2 - 4, getButtonState(controller, "BACK") ? 'B'|A_REVERSE : ' ');
            wrefresh(controller->window);
            controller++;
        }
        usleep(70000);
        refresh();
    }
    endwin();
    pthread_kill(usb_pid, SIGINT);

    // close everything
    controller = controllers;
    while (controller && controller->device) {
        close_device((controller++)->device);
    };

    printf("exiting usb...\n");
    libusb_exit(context);
    printf("exited usb...\n");

    return EXIT_SUCCESS;
}
