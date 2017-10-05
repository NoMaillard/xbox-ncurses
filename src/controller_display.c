//
// Created by noe on 9/26/17.
//

#include <ncurses.h>
#include "controller_display.h"

int init_controller_window(struct controller *controller, int window_number, int nb_windows) {
    int width, height;
    getmaxyx(stdscr, height, width);
    controller->window = newwin(height, width / nb_windows, 0, width / nb_windows * (window_number - 1));
    wrefresh(controller->window);

    return 0;
}


void draw_borders(WINDOW *screen, int orig_x, int orig_y, int width, int height) {
    int i;
    mvwprintw(screen, orig_y, orig_x, "+");
    mvwprintw(screen, orig_y + height - 1, orig_x, "+");
    mvwprintw(screen, orig_y, orig_x + width - 1, "+");
    mvwprintw(screen, orig_y + height - 1, orig_x + width - 1, "+");
    // sides
    for (i = orig_y + 1; i < height; i++) {
        mvwprintw(screen, i, orig_x, "|");
        mvwprintw(screen, i, orig_x + width - 1, "|");
    }
    // top and bottom
    for (i = orig_x + 1; i < width; i++) {
        mvwprintw(screen, orig_y, i, "-");
        mvwprintw(screen, orig_y + height - 1, i, "-");
    }
}
