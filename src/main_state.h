#ifndef MAIN_STATE_H_
#define MAIN_STATE_H_

#include <stdbool.h>

// This enum defines the two operating modes of our device.
typedef enum {
  MODE_MSC_WITH_BASIC_REPL,
  MODE_FULL_REPL,
} operating_mode_t;

// By declaring this as 'extern', we make the global 'current_mode'
// variable in main.c visible to any other file that includes this header.
extern operating_mode_t current_mode;

#endif /* MAIN_STATE_H_ */
