#ifndef MOTOR_H
#define MOTOR_H

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    MOTOR_1 = 0,
    MOTOR_2 = 1
} motor_id_t;

void motor_init(void);
void motor_run(motor_id_t motor_id, uint16_t revs_target);
void motor_set_duty(motor_id_t motor_id, uint8_t duty_pct);
void motor_stop(motor_id_t motor_id);
void motor_set_dir(motor_id_t motor_id, bool forward);
bool motor_pca_ok(void);

#endif /* MOTOR_H */
