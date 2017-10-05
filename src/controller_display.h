//
// Created by noe on 9/26/17.
//
#include "controller.h"
#ifndef D_CONTROLLER_DISPLAY_H
#define D_CONTROLLER_DISPLAY_H


int init_controller_window(struct controller *controller, int window_number, int nb_windows);
void draw_borders(WINDOW *screen, int orig_x, int orig_w, int width, int height);
#endif //D_CONTROLLER_DISPLAY_H
