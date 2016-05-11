#ifndef _HELPERS_H_
#define _HELPERS_H_

#include <stdint.h>

// http://stackoverflow.com/questions/3599160/unused-parameter-warnings-in-c-code
// Needs to be named NOTUSED to void conflict with UNUSED from
// libavr32/asf/avr32/utils/compiler.h
// Probably should put this macro somewhere else
#ifdef __GNUC__
#define NOTUSED(x) UNUSED_##x __attribute__((__unused__))
#else
#define NOTUSED(x) UNUSED_##x
#endif


int16_t normalise_value(int16_t min, int16_t max, int16_t wrap, int16_t value);

#endif
