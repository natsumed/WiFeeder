#ifndef HX711_H
#define HX711_H

#include <stdint.h>

void hx711_init(void);
int32_t hx711_read_raw(void);
int32_t hx711_read_average(uint8_t n);

#endif /* HX711_H */
