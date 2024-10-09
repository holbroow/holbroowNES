#include <stdint.h>

// Registers and Important Vars
#define stackp uint8_t;
#define accum uint8_t;
#define x uint8_t;
#define y uint8_t;

#define status uint8_t;

enum STATUS_FLAGS {
    C = (1 << 0), 
    Z = (1 << 1), 
    I = (1 << 2), 
    D = (1 << 3), 
    B = (1 << 4), 
    NA = (1 << 5), 
    O = (1 << 6), 
    N = (1 << 7), 
};
