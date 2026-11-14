
#ifndef MOTORES_H_
#define MOTORES_H_

#include <stdint.h>

// Inicializar motores y PWM
void motores_init(void);

// Control de movimiento
void motores_avanzar(void);
void motores_retroceder(void);
void motores_detener(void);

// Control de velocidad (0 a 255)
void motores_velocidad(uint8_t velocidad);

#endif

