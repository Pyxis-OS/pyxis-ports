/* BusyBox vi feature selection for Pyxis, replacing Kconfig's autoconf.h.
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Disabled features need facilities Pyxis does not provide: regex.h, signals,
 * a cursor-position reply, a shell for :! and an 8-bit-capable renderer. */
#ifndef PYXIS_BUSYBOX_VI_CONFIG_H
#define PYXIS_BUSYBOX_VI_CONFIG_H

#define CONFIG_FEATURE_VI_MAX_LEN 4096
#define CONFIG_FEATURE_VI_UNDO_QUEUE_MAX 256

#define ENABLE_LOCALE_SUPPORT 0
#define ENABLE_FEATURE_ALLOW_EXEC 0

#define ENABLE_FEATURE_VI_8BIT 0
#define ENABLE_FEATURE_VI_ASK_TERMINAL 0
#define ENABLE_FEATURE_VI_COLON 1
#define ENABLE_FEATURE_VI_COLON_EXPAND 0
#define ENABLE_FEATURE_VI_DOT_CMD 1
#define ENABLE_FEATURE_VI_READONLY 1
#define ENABLE_FEATURE_VI_REGEX_SEARCH 0
#define ENABLE_FEATURE_VI_SEARCH 1
#define ENABLE_FEATURE_VI_SET 1
#define ENABLE_FEATURE_VI_SETOPTS 1
#define ENABLE_FEATURE_VI_UNDO 1
#define ENABLE_FEATURE_VI_UNDO_QUEUE 1
#define ENABLE_FEATURE_VI_USE_SIGNALS 0
#define ENABLE_FEATURE_VI_VERBOSE_STATUS 1
#define ENABLE_FEATURE_VI_WIN_RESIZE 1
#define ENABLE_FEATURE_VI_YANKMARK 1

#define IF_FEATURE_VI_ASK_TERMINAL(...)
#define IF_FEATURE_VI_COLON(...) __VA_ARGS__
#define IF_FEATURE_VI_COLON_EXPAND(...)
#define IF_FEATURE_VI_READONLY(...) __VA_ARGS__
#define IF_FEATURE_VI_SEARCH(...) __VA_ARGS__
#define IF_FEATURE_VI_SETOPTS(...) __VA_ARGS__

#endif
