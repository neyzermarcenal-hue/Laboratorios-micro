#ifndef RELOJ_H
#define RELOJ_H

#include <stdint.h>

// Frecuencias para las pruebas
typedef enum
{
    RELOJ_16MHZ,
    RELOJ_8MHZ,
    RELOJ_2MHZ,
    RELOJ_250KHZ,
    RELOJ_125KHZ
} FrecuenciaReloj;

// Cambiar la frecuencia del procesador
void reloj_configurar(FrecuenciaReloj frecuencia);

// Consultar la frecuencia configurada
uint32_t reloj_obtener_hz(void);

#endif

