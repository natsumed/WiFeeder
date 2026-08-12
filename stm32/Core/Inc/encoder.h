#ifndef ENCODER_H
#define ENCODER_H

#include <stdint.h>
#include "motor.h"

void encoder_init(void);
int32_t encoder_get_count(motor_id_t motor);
void encoder_reset(motor_id_t motor);

#endif /* ENCODER_H */
