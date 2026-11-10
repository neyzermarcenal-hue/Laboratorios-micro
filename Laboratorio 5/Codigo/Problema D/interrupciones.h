#ifndef INTERRUPCIONES_H
#define INTERRUPCIONES_H

#include <stdint.h>

// Configurar el boton y su interrupcion
void interrupciones_init(void);

// Consultar si se presiono el boton
uint8_t boton_presionado(void);

#endif
