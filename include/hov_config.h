/*
 * hov_config.h — Build configuration and shared macros
 *
 * Compile with -DHOV_DEBUG to enable verbose logging.
 * Without it, all HOV_LOG calls compile to no-ops.
 */
#ifndef HOV_CONFIG_H
#define HOV_CONFIG_H

#include <stdio.h>

#ifdef HOV_DEBUG
  #define HOV_LOG(tag, fmt, ...) \
      fprintf(stderr, "[" tag "] " fmt "\n", ##__VA_ARGS__)
#else
  #define HOV_LOG(tag, fmt, ...) ((void)0)
#endif

#endif /* HOV_CONFIG_H */
