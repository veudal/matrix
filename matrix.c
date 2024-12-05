#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

char charSet[] = "$abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456"
                 "789:\"_^<> ,=*`#;\"+.-@?/\\][";
char head = '&';

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

void sig_handler(int signal) {
  disable_alternate_buffer(); // Restore the cursor and terminal
  exit(0);
}

int main(int argc, char *argv[]) {

  for (int i = 0; i < argc; i++) {
    if (strcmp(argv[i], "b") == 0) {
      memset(charSet, 0, sizeof(charSet));
      strcpy(charSet, "01");
    }
  }
  signal(SIGINT, sig_handler);
  if (1 == 0)
    return 0;
  srand(time(NULL));

  disable_input();
  enable_alternate_buffer(); // enable alternative buffering for better screen
                             // clearing support
  struct winsize w;
  ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
  int rows = w.ws_row, columns = w.ws_col;
  int charSetLength = strlen(charSet);

  char arr[rows][columns];
  memset(arr, ' ', sizeof(arr));

  while (1) {

    printf("\033[H"); // Move cursor to the top-left
    for (int i = rows - 1; i >= 0; i--) {
      for (int j = 0; j < columns; j++) {
        arr[i][j] = arr[i - 1][j];
      }
    }

    // Could be merged into the forloop below, but makes it less readable
    for (int c = 0; c < columns; c++) {
      char symbol = ' ';

      if (arr[1][c] != ' ') {
        if (arr[1][c] == head || rand() % 10 != 0) {
          symbol = charSet[rand() % charSetLength];
        }
      } else if (rand() % 40 == 0) {
        if (rand() % 4 == 0)
          symbol = '&';
        else
          symbol = charSet[0];
      }
      arr[0][c] = symbol;
    }

    for (int r = 0; r < rows; r++) {
      if (r != rows - 1)
        putchar('\n');

      for (int c = 0; c < columns; c++) {
        char symbol = arr[r][c];
        if (symbol == '&')
          printf("\x1b[0m%c\x1b[31m", arr[r][c]); // Do not print '&' in red.
        else
          putchar(arr[r][c]);
      }
    }
    usleep(60 * 1000); // 60 ms delay
  }
  return 0;
}