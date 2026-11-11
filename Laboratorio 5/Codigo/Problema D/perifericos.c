#include <avr/io.h>
#include "perifericos.h"

// Inicializar sin modificar los perifericos
void perifericos_init(void)
{
    PRR = 0;
}

// Desactivar los perifericos seleccionados
void periferico_desactivar(uint8_t mascara)
{
    PRR |= mascara;
}

// Activar los perifericos seleccionados
void periferico_activar(uint8_t mascara)
{
    PRR &= ~mascara;
}

// Restaurar los perifericos
void perifericos_restaurar(void)
{
    PRR = 0;
}
