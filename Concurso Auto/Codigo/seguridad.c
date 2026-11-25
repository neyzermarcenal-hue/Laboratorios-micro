
#include <avr/io.h>
#include <stdint.h>

#include "seguridad.h"
#include "motores.h"
#include "sensores.h"

uint8_t seguridad_enlace_perdido(
    uint8_t enlace_visto,
    uint32_t ahora,
    uint32_t ultimo_paquete_ms)
{
    // No considerar perdida hasta
    // haber recibido alguna trama valida
    if (!enlace_visto)
    {
        return 0;
    }

    // Verificar tiempo transcurrido
    if ((uint32_t)(ahora - ultimo_paquete_ms)
        >= SEGURIDAD_TIMEOUT_MS)
    {
        return 1;
    }

    return 0;
}

uint8_t seguridad_bateria_baja(void)
{
#if MONITOREAR_BATERIA

    // Consultar resultado de sensores.c
    if (sensores_bateria_baja())
    {
        return 1;
    }

#endif

    return 0;
}


void seguridad_detener_motores(void)
{
    motores_detener();
}

