#include <bits/time.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

#define FRAME_TIME 45

char charSet[] = "$abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456"
                 "789:\"_^<> ,=*`#;\"+.-@?/\\][";
int charSetLength = sizeof(charSet) / sizeof(char) - 1;
char head = '&';
char color[3] = "96"; // Default color is: Aqua
char **arr = NULL;
int rows, columns;
struct winsize w;

int keyDown() {
  struct timeval tv = {0, 0};
  fd_set fds;
  FD_ZERO(&fds);
  FD_SET(STDIN_FILENO, &fds);
  return select(STDIN_FILENO + 1, &fds, NULL, NULL, &tv);
}

char getChar() {
  char ch;
  if (read(STDIN_FILENO, &ch, 1) < 0) {
    return '\0';
  }
  return ch;
}

void hide_cursor() {
  printf("\033[?25l"); // ANSI escape sequence to hide the cursor
  fflush(stdout);
}

void disable_input() {
  struct termios new_settings;
  tcgetattr(STDIN_FILENO, &new_settings);
  new_settings.c_lflag &= ~ICANON;
  new_settings.c_lflag &= ~ECHO;
  tcsetattr(STDIN_FILENO, TCSANOW, &new_settings);
}

void enable_alternate_buffer() {
  printf("\033[?1049h"); // Enable alternate buffer
  printf("\033[?25l");   // Hide cursor
}

void disable_alternate_buffer() {
  printf("\033[?25h");   // Show cursor
  printf("\033[?1049l"); // Disable alternate buffer
}

void moveCursorTo(int row, int col) {
  printf("\033[%d;%dH", row, col); // Move cursor to the (row, col) position
}

void sig_handler(int signal) {
  struct termios term;
  tcgetattr(STDIN_FILENO, &term); // Get current terminal settings

  // Restore terminal settings
  term.c_lflag |= (ICANON | ECHO);
  tcsetattr(STDIN_FILENO, TCSANOW, &term);
  disable_alternate_buffer(); // Restore the cursor and terminal
  exit(0);
}

void resizeArray(int newRows, int newColumns) {
  char **newArr = realloc(arr, newRows * sizeof(char *));

  for (int i = 0; i < newRows; i++) {
    if (i >= rows) {
      // Initialize new rows with spaces
      newArr[i] = malloc(newColumns * sizeof(char));
      memset(newArr[i], ' ', newColumns);
    } else if (newColumns > columns) {
      // Resize existing rows
      newArr[i] = realloc(newArr[i], newColumns * sizeof(char));
      memset(newArr[i] + columns, ' ', newColumns - columns);
    }
  }

  arr = newArr;

  rows = newRows;
  columns = newColumns;
}

void checkWindowDimensions() {
  ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
  if (rows != w.ws_row || columns != w.ws_col) {
    resizeArray(w.ws_row, w.ws_col);
    rows = w.ws_row, columns = w.ws_col;
  }
}

void init(int argc, char *argv[]) {
  signal(SIGINT, sig_handler);
  srand(time(NULL));

  disable_input();
  enable_alternate_buffer(); // enable alternative buffering for better screen
                             // clearing support

  // Check for arguments
  for (int i = 0; i < argc; i++) {
    if (strcmp(argv[i], "b") == 0) {
      memset(charSet, 0, sizeof(charSet));
      strcpy(charSet, "01");
    }
  }
  printf("\x1b[%sm", color); // set color
}

int main(int argc, char *argv[]) {
  init(argc, argv);

  while (1) {
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    checkWindowDimensions();

    if (keyDown()) {
      char c = getChar();
      switch (c) {
      case '0' ... '9':
        color[0] = '9';
        color[1] = c;
        break;
      case 'r':
        strcpy(color, "31");
        break;
      case 'g':
        strcpy(color, "32");
        break;
      case 'b':
        strcpy(color, "34");
        break;
      case 'o':
        strcpy(color, "33");
        break;
      case 'p':
        strcpy(color, "35");
        break;
      case 'a':
        strcpy(color, "36");
        break;
      case 'w':
        strcpy(color, "37");
        break;
      case 'm':
        strcpy(color, "00");
        break;
      }
    }

    // Shift each row down
    for (int i = rows - 1; i > 0; i--) {
      for (int j = 0; j < columns; j++)
        arr[i][j] = arr[i - 1][j];
    }

    // Pattern generation
    for (int c = 0; c < columns; c++) {
      char symbol = ' ', next = arr[1][c];
      if (next != ' ') {
        if (next == head || next == charSet[0] || rand() % 10 != 0) {
          symbol = charSet[rand() % charSetLength];
        }
      } else if (rand() % 60 == 0)
        symbol = rand() % 4 == 0 ? head : charSet[0];

      arr[0][c] = symbol;
    }

    printf("\033[H"); // Move cursor to the top-left
    // Print array
    for (int r = 0; r < rows; r++) {
      putchar('\n');
      for (int c = 0; c < columns; c++) {
        char symbol = arr[r][c];
        if (symbol == '&')
          printf("\x1b[0m%c\x1b[%sm", arr[r][c],
                 color); // Print '&' in white.
        else {
          // Handle rainbow mode
          if (strcmp(color, "00") == 0) {
            int rndColor = rand() % 6 + 2;
            printf("\x1b[9%dm", rndColor);
          }
          putchar(arr[r][c]);
        }
      }
    }
    // delay calculation
    clock_gettime(CLOCK_MONOTONIC, &end);
    long elapsed_time = (end.tv_sec - start.tv_sec) * 1000 +
                        (end.tv_nsec - start.tv_nsec) / 1000000;

    long sleep_time = FRAME_TIME - elapsed_time;
    if (sleep_time > 0)
      usleep(sleep_time * 1000); // Constant period of time between frames
  }
  return 0;
}