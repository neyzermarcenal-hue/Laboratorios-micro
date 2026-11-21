
#ifndef SENSORES_H_
#define SENSORES_H_

#include <stdint.h>

// Habilitar solo cuando el circuito ADC
// y el umbral de bateria esten verificados.
#define MONITOREAR_BATERIA 0

void sensores_init(void);
void sensores_tarea(void);

uint8_t sensores_bateria_dv(void);
uint8_t sensores_bateria_baja(void);
uint8_t sensores_objeto_agarrado(void);

#endif

