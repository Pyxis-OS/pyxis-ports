/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.
*/

/* Pyxis system layer and native frame loop. The upstream generic entry
 * supplies engine initialization; file I/O follows sys_null.c. */

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "quakedef.h"
#include "quakegeneric.h"
#include "quake_pyxis.h"

#define QUAKE_BASE_DIRECTORY "pkg://quake/"
#define QUAKE_WRITE_DIRECTORY "home://quake"
#define MAX_HANDLES 10

qboolean isDedicated;

static FILE *sys_handles[MAX_HANDLES];

static int find_handle(void)
{
  for (int i = 1; i < MAX_HANDLES; ++i) {
    if (!sys_handles[i]) {
      return i;
    }
  }
  Sys_Error("out of handles");
  return -1;
}

static int file_length(FILE *file)
{
  long position = ftell(file);
  fseek(file, 0, SEEK_END);
  long end = ftell(file);
  fseek(file, position, SEEK_SET);
  return (int)end;
}

int Sys_FileOpenRead(char *path, int *handle)
{
  int i = find_handle();
  FILE *file = fopen(path, "rb");
  if (!file) {
    *handle = -1;
    return -1;
  }
  sys_handles[i] = file;
  *handle = i;
  return file_length(file);
}

int Sys_FileOpenWrite(char *path)
{
  int i = find_handle();
  FILE *file = fopen(path, "wb");
  if (!file) {
    Sys_Error("Error opening %s: %s", path, strerror(errno));
  }
  sys_handles[i] = file;
  return i;
}

void Sys_FileClose(int handle)
{
  fclose(sys_handles[handle]);
  sys_handles[handle] = NULL;
}

void Sys_FileSeek(int handle, int position)
{
  fseek(sys_handles[handle], position, SEEK_SET);
}

int Sys_FileRead(int handle, void *destination, int count)
{
  return (int)fread(destination, 1, count, sys_handles[handle]);
}

int Sys_FileWrite(int handle, void *data, int count)
{
  return (int)fwrite(data, 1, count, sys_handles[handle]);
}

int Sys_FileTime(char *path)
{
  FILE *file = fopen(path, "rb");
  if (file) {
    fclose(file);
    return 1;
  }
  return -1;
}

void Sys_mkdir(char *path)
{
  /* COM_CreatePath offers every prefix ending before a '/', including "home:"
   * and "home:/" of a URI. Only a full scheme:// prefix selects a root, so libc
   * would create those as relative names in the working directory. */
  size_t length = strlen(path);
  if (!length || path[length - 1] == ':' ||
      (length >= 2 && path[length - 2] == ':' && path[length - 1] == '/')) {
    return;
  }
  if (mkdir(path, 0777) != 0 && errno != EEXIST) {
    Con_Printf("Couldn't create %s: %s\n", path, strerror(errno));
  }
}

void Sys_MakeCodeWriteable(unsigned long startaddr, unsigned long length)
{
  (void)startaddr;
  (void)length;
}

void Sys_Error(char *error, ...)
{
  va_list arguments;

  pyxis_quake_stop();
  fputs("Sys_Error: ", stderr);
  va_start(arguments, error);
  vfprintf(stderr, error, arguments);
  va_end(arguments);
  fputc('\n', stderr);
  exit(1);
}

void Sys_Printf(char *format, ...)
{
  va_list arguments;

  va_start(arguments, format);
  vprintf(format, arguments);
  va_end(arguments);
}

void Sys_Quit(void)
{
  /* Writes config.cfg, as the original platform layers do before exiting. */
  Host_Shutdown();
  pyxis_quake_stop();
  exit(0);
}

double Sys_FloatTime(void)
{
  return pyxis_quake_time();
}

char *Sys_ConsoleInput(void)
{
  return NULL;
}

void Sys_Sleep(void)
{
}

void Sys_SendKeyEvents(void)
{
}

void Sys_HighFPPrecision(void)
{
}

void Sys_LowFPPrecision(void)
{
}

int main(int argc, char **argv)
{
  /* Our options go before the caller's, so trailing +commands stay intact. */
  char **arguments = calloc((size_t)argc + 5, sizeof(*arguments));
  if (!arguments) {
    fputs("quake: cannot allocate arguments\n", stderr);
    return EXIT_FAILURE;
  }
  int count = 0;
  arguments[count++] = argv[0];
  qboolean has_base_directory = false;
  qboolean has_write_directory = false;
  for (int i = 1; i < argc; ++i) {
    has_base_directory |= Q_strcmp(argv[i], "-basedir") == 0;
    has_write_directory |= Q_strcmp(argv[i], "-writedir") == 0;
  }
  if (!has_base_directory) {
    arguments[count++] = "-basedir";
    arguments[count++] = QUAKE_BASE_DIRECTORY;
  }
  if (!has_write_directory) {
    arguments[count++] = "-writedir";
    arguments[count++] = QUAKE_WRITE_DIRECTORY;
  }
  for (int i = 1; i < argc; ++i) {
    arguments[count++] = argv[i];
  }

  pyxis_quake_start();
  QG_Create(count, arguments);

  double previous = pyxis_quake_time();
  for (;;) {
    double now = pyxis_quake_time();
    QG_Tick(now - previous);
    previous = now;
  }
}
