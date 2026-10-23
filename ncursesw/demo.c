/* SPDX-License-Identifier: MPL-2.0 */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <process.h>
#include <handle.h>
#include <curses.h>

int main(int argc, char **argv)
{
  int result = 1;
  int key = 0;
  bool raw_mode = false;
  handle_t receiver = HANDLE_INVALID;
  if (argc > 2 || (argc == 2 && strcmp(argv[1], "--cooperate") != 0)) {
    fprintf(stderr, "usage: ncurses-demo [--cooperate]\n");
    return 1;
  }
  if (argc == 2 && process_interrupt_receiver_create(&receiver) != CALL_OK) {
    fprintf(stderr, "ncurses-demo: cannot create cooperative receiver\n");
    return 1;
  }
  SCREEN *screen = newterm("pyxis", stdout, stdin);
  if (!screen) {
    fprintf(stderr, "ncurses-demo: terminal setup failed: %s\n", strerror(errno));
    goto done;
  }
  if (noecho() == ERR || keypad(stdscr, TRUE) == ERR || start_color() == ERR ||
      use_default_colors() == ERR || init_pair(1, COLOR_GREEN, -1) == ERR) {
    fprintf(stderr, "ncurses-demo: cannot initialize input/colour\n");
    goto close_screen;
  }
  wtimeout(stdscr, 100);
  for (;;) {
    int rows, columns;
    getmaxyx(stdscr, rows, columns);
    if (erase() == ERR)
      break;
    if (rows >= 6 && columns >= 32) {
      box(stdscr, 0, 0);
      attron(COLOR_PAIR(1));
      mvprintw(1, 2, "Pyxis ncursesw: %dx%d", columns, rows);
      attroff(COLOR_PAIR(1));
      mvprintw(2, 2, "q quit | r raw/cbreak | e reenter");
      mvprintw(3, 2, "mode=%s key=%d", raw_mode ? "raw" : "cbreak", key);
      mvprintw(4, 2, "resize and arrow keys; default colours");
    } else {
      addnstr("small", columns > 0 ? columns - 1 : 0);
    }
    if (refresh() == ERR) {
      fprintf(stderr, "ncurses-demo: display failed: %s\n", strerror(errno));
      break;
    }
    errno = 0;
    int next = getch();
    if (next == ERR && errno && errno != EAGAIN && errno != EINTR) {
      fprintf(stderr, "ncurses-demo: input failed: %s\n", strerror(errno));
      break;
    }
    if (next == 'q') {
      result = 0;
      break;
    }
    if (next != ERR)
      key = next;
    if (next == 'r') {
      int status = raw_mode ? cbreak() : raw();
      if (status == ERR) {
        fprintf(stderr, "ncurses-demo: mode change failed: %s\n", strerror(errno));
        break;
      }
      raw_mode = !raw_mode;
    } else if (next == 'e') {
      if (endwin() == ERR) {
        fprintf(stderr, "ncurses-demo: endwin failed: %s\n", strerror(errno));
        break;
      }
      puts("ncurses-demo: outside curses; reentering");
      fflush(stdout);
      if (refresh() == ERR)
        break;
    }
    if (receiver != HANDLE_INVALID) {
      enum call_status status = process_interrupt_take(receiver);
      if (status == CALL_OK) {
        /* Stay opted in, so a second press exercises ordinary escalation. */
        key = -3;
      } else if (status != CALL_WOULD_BLOCK) {
        fprintf(stderr, "ncurses-demo: interrupt receiver failed\n");
        break;
      }
    }
  }
close_screen:
  if (endwin() == ERR)
    result = 1;
  delscreen(screen);
done:
  if (receiver != HANDLE_INVALID)
    handle_close(receiver);
  return result;
}
