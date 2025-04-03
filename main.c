#include <stdio.h>
#include <stdlib.h>
#include <ncurses.h>
#include <time.h>
#include <math.h>
#include <stdbool.h>
#include <unistd.h>

typedef struct {
    int high_score;
} GameState;

// function to show game over screen
bool show_game_over(WINDOW* stdscr, int score, int high_score) {
    int height, width;
    getmaxyx(stdscr, height, width); // get terminal dimensions
    int game_height = height - 4;
    int game_width = width - 4;

    WINDOW* game_win = newwin(game_height, game_width, 2, 2);
    wclear(game_win);
    box(game_win, 0, 0);

    mvwprintw(game_win, game_height / 2 - 2, game_width / 2 - 10, "Game Over!");
    wattron(game_win, A_BOLD);
    mvwprintw(game_win, game_height / 2, game_width / 2 - 15, "Score: %d", score);
    mvwprintw(game_win, game_height / 2 + 1, game_width / 2 - 20, "High Score: %d", high_score);
    mvwprintw(game_win, game_height / 2 + 3, game_width / 2 - 25, "Press 'p' to play agaon or 'q' to quit");
    wattroff(game_win, A_BOLD);

    wrefresh(game_win);
    refresh();

    int key;
    while (true) {
        key = getch();
        if (key == 'p') {
            delwin(game_win);
            return true;
        } else if (key == 'q') {
            delwin(game_win);
            return false;
        }
    }
}

// function to generate a new dot position
void generate_new_dot(int game_height, int game_width, int cursor_y, int cursor_x, 
                      int* dot_y, int* dot_x) {
    int max_distance = 15;
    while (true) {
        int distance = rand() % max_distance + 1; // random distance between 1-15
        double angle = ((double)rand() / RAND_MAX) * 2 * M_PI; // random angle (0-359 degrees)
        // calc position using polar coordinates
        *dot_y = cursor_y + (int)(distance * sin(angle));
        *dot_x = cursor_x + (int)(distance * cos(angle));
        // ensure dot within game boundaries
        *dot_y = (*dot_y < 1) ? 1 : (*dot_y >= game_height - 1) ? game_height - 2 : *dot_y;
        *dot_x = (*dot_x < 1) ? 1 : (*dot_x >= game_width - 1) ? game_height - 2 : *dot_x;
        // ensure dot not on cursor
        if (*dot_y != cursor_y || *dot_x != cursor_x) {
            return;
        }
    }
}

// function to draw status bar
void draw_status_bar(WINDOW* stdscr, int score, int time_left) {
    mvwprintw(stdscr, 0, 0, "                                        "); // clear the line
    wattron(stdscr, A_BOLD);
    mvwprintw(stdscr, 0, 2, "Score: %d | Time: %ds", score, time_left);
    wattroff(stdscr, A_BOLD);
    wrefresh(stdscr);
}

// function to draw instructions
void draw_instructions(WINDOW* stdscr, int height) {
    mvwprintw(stdscr, height - 1, 2, "Use h,j,k,l to move | q to quit");
    wrefresh(stdscr);
}

// main game function
void play_game(WINDOW* stdscr, GameState* game_state) {
    // setup colors
    start_color();
    init_pair(1, COLOR_GREEN, COLOR_BLACK); // dot
    init_pair(2, COLOR_WHITE, COLOR_BLACK); // cursor
    init_pair(3, COLOR_WHITE, COLOR_BLACK); // border

    curs_set(0); // hide cursor
    noecho();
    nodelay(stdscr, TRUE); // enable non-blocking input
    
    int height, width;
    getmaxyx(stdscr, height, width); // get terminal dimensions
    // game arena dimensions
    int game_height = height - 4;
    int game_width = width - 4;
    // create window for game arena
    WINDOW* game_win = newwin(game_height, game_width, 2, 2);

    while (true) {
        // init game state
        wclear(stdscr);
        wclear(game_win);
        // draw border
        wattron(game_win, COLOR_PAIR(3));
        box(game_win, 0, 0);
        wattroff(game_win, COLOR_PAIR(3));
        // init cursor at center
        int cursor_y = game_height / 2;
        int cursor_x = game_width / 2;
        // init dot
        int dot_y, dot_x;
        generate_new_dot(game_height, game_width, cursor_y, cursor_x, &dot_y, &dot_x);
        // game variables
        int score = 0;
        time_t start_time = time(NULL);
        int game_duration = 60; // 60 secs
        int time_left = game_duration;
        // draw initial state
        wattron(game_win, COLOR_PAIR(1));
        mvwaddch(game_win, dot_y, dot_x, ACS_BULLET);
        wattroff(game_win, COLOR_PAIR(1));

        wattron(game_win, COLOR_PAIR(2));
        mvwaddch(game_win, cursor_y, cursor_x, ACS_BLOCK);
        wattroff(game_win, COLOR_PAIR(2));
        // draw status bar and instructions
        draw_status_bar(stdscr, score, time_left);
        draw_instructions(stdscr, height);
        // refresh windows
        wrefresh(stdscr);
        wrefresh(game_win);
        // main game loop
        while (time_left > 0) {
            // update time
            time_t current_time = time(NULL);
            time_left = game_duration - (current_time - start_time);

            if (time_left <= 0) {
                break;
            }
            draw_status_bar(stdscr, score, time_left); // update status bar
            int key = getch(); // get user input
            // process input
            if (key == 'q') {
                delwin(game_win);
                return;
            }
            // move cursor based on vim keys
            int new_y = cursor_y, new_x = cursor_x;
            if (key == 'h') { // left
                new_x = (cursor_x > 1) ? cursor_x - 1 : 1;
            } else if (key == 'j') { // down
                new_y = (cursor_y < game_height - 2) ? cursor_y + 1 : game_height - 2;
            } else if (key == 'k') { // up
                new_y = (cursor_y > 1) ? cursor_y - 1 : 1;
            } else if (key == 'l') { // right
                new_x = (cursor_x < game_width - 2) ? cursor_x + 1 : game_width - 2;
            }
            // erase old cursor position if moved
            if (new_y != cursor_y || new_x != cursor_x) {
                mvwaddch(game_win, cursor_y, cursor_x, ' ');
                cursor_y = new_y;
                cursor_x = new_x;
            }
            // check if cursor hit the dot
            if (cursor_y == dot_y && cursor_x == dot_x) {
                score += 1;
                generate_new_dot(game_height, game_width, cursor_y, cursor_x,
                                 &dot_y, &dot_x);
                wattron(game_win, COLOR_PAIR(1));
                mvwaddch(game_win, dot_y, dot_x, ACS_BULLET);
                wattroff(game_win, COLOR_PAIR(1));
            }
            // draw cursor at new position
            wattron(game_win, COLOR_PAIR(2));
            mvwaddch(game_win, cursor_y, cursor_x, ACS_BLOCK);
            wattroff(game_win, COLOR_PAIR(2));
            wrefresh(game_win); // refresh game window
            usleep(10000); // 0.01s delay to reduce cpu usage
        }
        // update high score if necessary
        if (score > game_state->high_score) {
            game_state->high_score = score;
        }
        // show game over screen
        bool play_again = show_game_over(stdscr, score, game_state->high_score);
        if (!play_again) {
            delwin(game_win);
            break;
        }
    }
}

int main() {
    GameState game_state = {0}; // init game state
    // init ncurses
    WINDOW* stdscr = initscr();
    if (stdscr == NULL) {
        fprintf(stderr, "Error initializing ncurses.\n");
        return EXIT_FAILURE;
    }
    srand(time(NULL)); // seed random number generator
    play_game(stdscr, &game_state); // run game
    endwin(); // clean up
    return EXIT_SUCCESS;
}
