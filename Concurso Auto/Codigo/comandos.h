
#ifndef COMANDOS_H_
#define COMANDOS_H_

#include <stdint.h>

void comandos_init(void);

// Consultar y procesar datos Bluetooth
void comandos_procesar(uint32_t ahora);

#endif

