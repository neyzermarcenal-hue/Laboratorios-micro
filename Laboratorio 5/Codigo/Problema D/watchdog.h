#ifndef WATCHDOG_H
#define WATCHDOG_H

#include <stdint.h>

// Tiempos disponibles para las pruebas
typedef enum
{
    WDT_16MS,
    WDT_250MS,
    WDT_1S,
    WDT_8S
} TiempoWDT;

// Configurar WDT en modo interrupcion
void watchdog_iniciar(TiempoWDT tiempo);

// Consultar si ocurrio la interrupcion
uint8_t watchdog_evento(void);

// Desactivar WDT
void watchdog_detener(void);

#endif
