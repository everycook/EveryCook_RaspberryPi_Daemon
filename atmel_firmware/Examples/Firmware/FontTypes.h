#ifndef FontTypes_h
#define FontTypes_h

#include <inttypes.h>

#undef LOWERCASE
//#define LOWERCASE

#define L(x) letter_##x
#define C(x) PROGMEM const uint8_t L(x)[]
#define P(c,r) ((c << 4) | r)
#define	END 255


#ifndef NULL
#define NULL 0
#endif

#endif
