// === GLOBAL DATA ===
#define ADDR_MVP_ROW1 0
#define ADDR_MVP_ROW2 1
#define ADDR_MVP_ROW3 2
#define ADDR_MVP_ROW4 3

#define ADDR_GB_SCALE   4
#define ADDR_VP_ORIGIN  5 // viewport origin
#define ADDR_VP_SCALE   6 // viewport scale

// === DRAW DATA ===
// ADDR_VERTS_COUNT              0

// === GLOBAL REGISTERS ===
#define ONE_W  vf00[w] // (vf0.w is 1)

#define MVP_R1 vf01
#define MVP_R2 vf02
#define MVP_R3 vf03
#define MVP_R4 vf04

#define VP_ORIGIN vf05
#define VP_SCALE  vf06
#define GB_SCALE  vf07 // guardband scale

