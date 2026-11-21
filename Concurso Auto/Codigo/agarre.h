#ifndef AGARRE_H_
#define AGARRE_H_

#include <stdint.h>

// Inicializar el servomotor
void agarre_init(void);

// Control de la pinza
void agarre_abrir(void);
void agarre_cerrar(void);

void agarre_tarea(void);

#endif
