
#ifndef ROBOT_H_
#define ROBOT_H_

#include <stdint.h>

typedef enum
{
    INICIO = 0,
    ESPERA_ENLACE = 1,
    MANUAL = 2,
    FAILSAFE = 3,
    BATERIA_BAJA = 4

} estado_robot_t;

void robot_init(uint8_t reinicio_brownout);

void robot_recibir_trama(
    uint8_t cmd,
    uint8_t valor,
    uint32_t ahora);

void robot_tarea(uint32_t ahora);

void robot_telemetria(void);

#endif
