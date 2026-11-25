#include <avr/io.h>
#include <stdint.h>

#include "robot.h"
#include "motores.h"
#include "agarre.h"
#include "sensores.h"
#include "bluetooth.h"
#include "seguridad.h"

// Configuracion de seguridad
#define TIMEOUT_ENLACE_MS 300UL

// Variables de estado
static estado_robot_t estado = INICIO;

static uint8_t orden_mov = 0;
static uint8_t porcentaje = 70;

static uint8_t enlace_visto = 0;
static uint32_t ultimo_paquete_ms = 0;

static uint8_t hubo_failsafe = 0;
static uint8_t brownout_inicial = 0;

// Detener motores
static void apagar_motores(void)
{
    motores_detener();
}

// Inicializar la maquina de estados
void robot_init(uint8_t reinicio_brownout)
{
    // LED integrado D13
    DDRB |= (1 << PB5);
    PORTB &= ~(1 << PB5);

    brownout_inicial = (reinicio_brownout != 0);

    orden_mov = 0;
    porcentaje = 70;

    enlace_visto = 0;
	ultimo_paquete_ms = 0;
	hubo_failsafe = 0;

    apagar_motores();

    estado = ESPERA_ENLACE;
}

// Recibir una trama valida
void robot_recibir_trama(
    uint8_t cmd,
    uint8_t valor,
    uint32_t ahora)
{
    ultimo_paquete_ms = ahora;
    enlace_visto = 1;

    // STOP
    if (cmd == 0)
    {
        orden_mov = 0;
        apagar_motores();

        // Establecer o recuperar el enlace
        if (estado == ESPERA_ENLACE ||
            estado == FAILSAFE)
        {
             if (sensores_bateria_baja())
			{
				estado = BATERIA_BAJA;
			}
			else
			{
				estado = MANUAL;
			}
		}

		return;
	}
    
    // Solo aceptar comandos en modo MANUAL
	if (estado != MANUAL)
	{
		return;
	}

    // Comandos de movimiento
	if (cmd >= 1 && cmd <= 4)
	{
		orden_mov = cmd;
		porcentaje = valor;
	}
	else if (cmd == 7)
	{
		// Cambiar velocidad
		porcentaje = valor;
	}
	else if (cmd == 5)
	{
		// Abrir pinza
		agarre_abrir();
	}
	else if (cmd == 6)
	{
		// Cerrar pinza
		agarre_cerrar();
	}
}

// Actualizar LED de estado
static void actualizar_led(uint32_t ahora)
{
    uint8_t led = 0;
    uint16_t fase = (uint16_t)(ahora % 1000UL);

    if (estado == ESPERA_ENLACE)
    {
        led = (fase < 500U);
    }
    else if (estado == FAILSAFE)
    {
        led = ((ahora % 200UL) < 100UL);
    }
    else if (estado == BATERIA_BAJA)
    {
        led = (fase < 100U ||
               (fase >= 200U && fase < 300U));
    }

    if (led)
        PORTB |= (1 << PB5);
    else
        PORTB &= ~(1 << PB5);
}

// Ejecutar maquina de estados
void robot_tarea(uint32_t ahora)
{
    // Detectar perdida de comunicacion
    if (enlace_visto && (uint32_t)(ahora - ultimo_paquete_ms)
            >= TIMEOUT_ENLACE_MS)
    {
        if (estado == MANUAL ||
            estado == BATERIA_BAJA)
        {
            estado = FAILSAFE;
            hubo_failsafe = 1;
            orden_mov = 0;
        }
    }

    // Verificar bateria
    if (estado == MANUAL && sensores_bateria_baja())
    {
        estado = BATERIA_BAJA;

// Cancelar cualquier movimiento
		orden_mov = 0;
		apagar_motores();
    }
    else if (estado == BATERIA_BAJA && !sensores_bateria_baja())
    {
        estado = MANUAL;
        
        // No reanudar movimientos anteriores
		orden_mov = 0;
		apagar_motores();
    }

    // Control de motores segun estado
    if (estado == MANUAL)
    {
       // Convertir porcentaje a PWM
		motores_velocidad((uint8_t)(((uint16_t)porcentaje * 255U) / 100U));
    if (orden_mov == 1)
		{
			motores_avanzar();
		}
		else if (orden_mov == 2)
		{
			motores_retroceder();
		}
		else if (orden_mov == 3)
		{
			motores_izquierda();
		}
		else if (orden_mov == 4)
		{
			motores_derecha();
		}
		else
		{
			apagar_motores();
		}
	}
	else
	{
		// En cualquier otro estado,
		// mantener los motores detenidos
		apagar_motores();
	}

	actualizar_led(ahora);
}
       

// Enviar telemetria de 5 bytes
void robot_telemetria(void)
{
    uint8_t flags = 0;

    if (sensores_objeto_agarrado())
        flags |= (1 << 0);

    if (sensores_bateria_baja())
        flags |= (1 << 1);

    if (hubo_failsafe)
        flags |= (1 << 2);

    if (brownout_inicial)
        flags |= (1 << 3);

    uint8_t vbat = sensores_bateria_dv();
    uint8_t est = (uint8_t)estado;

    // Enviar inicio 0xBB
    bluetooth_enviar(0xBB);

    // Enviar bateria, estado y flags
    bluetooth_enviar(vbat);
    bluetooth_enviar(est);
    bluetooth_enviar(flags);

    // Enviar checksum XOR
    bluetooth_enviar(
        (uint8_t)(vbat ^ est ^ flags)
    );

    hubo_failsafe = 0;
}
