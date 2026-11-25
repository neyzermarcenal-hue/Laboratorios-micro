#ifndef SEGURIDAD_H_
#define SEGURIDAD_H_

#include <stdint.h>

// Tiempo maximo sin recibir comandos validos
#define SEGURIDAD_TIMEOUT_MS 300UL

// Verificar perdida de comunicacion
uint8_t seguridad_enlace_perdido(
    uint8_t enlace_visto,
    uint32_t ahora,
    uint32_t ultimo_paquete_ms
);

// Consultar estado de bateria
uint8_t seguridad_bateria_baja(void);

// Detencion de seguridad
void seguridad_detener_motores(void);

#endif

