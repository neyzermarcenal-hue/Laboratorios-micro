#ifndef PERIFERICOS_H
#define PERIFERICOS_H

#include <stdint.h>

// Inicializar los perifericos
void perifericos_init(void);

// Desactivar un periferico
void periferico_desactivar(uint8_t mascara);

// Activar un periferico
void periferico_activar(uint8_t mascara);

// Restaurar todos los perifericos
void perifericos_restaurar(void);

#endif
