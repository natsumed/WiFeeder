#ifndef RFID_H
#define RFID_H

#include <stdint.h>
#include <stdbool.h>

void rfid_init(void);
bool rfid_poll(uint32_t *tag_out);

#endif /* RFID_H */
