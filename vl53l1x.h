#include <stdint.h>
#include <stdbool.h>

#define TARGET_RATE 0x0A00

bool vl53l1x_init();
bool vl53l1x_read_distance(uint16_t* distance_mm);