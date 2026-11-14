
#ifndef BLUETOOTH_H_
#define BLUETOOTH_H_

#include <stdint.h>

// Inicializar comunicacion UART
void bluetooth_init(void);

// Verificar si hay datos recibidos
uint8_t bluetooth_disponible(void);

// Leer un caracter recibido
char bluetooth_leer(void);

// Enviar un caracter
void bluetooth_enviar(char dato);

// Enviar una cadena de texto
void bluetooth_enviar_texto(const char *texto);

#endif

