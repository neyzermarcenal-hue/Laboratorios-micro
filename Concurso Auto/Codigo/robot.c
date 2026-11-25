
#include <avr/io.h>
#include <stdint.h>

#include "robot.h"
#include "motores.h"
#include "agarre.h"
#include "sensores.h"
#include "bluetooth.h"
#include "seguridad.h"

// =====================================
// CONFIGURACION DEL ROBOT
// =====================================

// Tiempo del impulso inicial de motores
#define IMPULSO_ARRANQUE_MS 120UL

// =====================================
// VARIABLES DE ESTADO
// =====================================

static estado_robot_t estado = INICIO;

static uint8_t orden_mov = 0;
static uint8_t porcentaje = 70;

// Control de comunicacion Bluetooth
static uint8_t enlace_visto = 0;
static uint32_t ultimo_paquete_ms = 0;

// Indicadores de seguridad
static uint8_t hubo_failsafe = 0;
static uint8_t brownout_inicial = 0;

// Control del impulso de arranque
static uint8_t impulso_activo = 0;
static uint32_t inicio_impulso_ms = 0;

// =====================================
// DETENCION DE MOTORES
// =====================================

static void apagar_motores(void)
{
    seguridad_detener_motores();
}

// =====================================
// INICIALIZAR ROBOT
// =====================================

void robot_init(uint8_t reinicio_brownout)
{
    // Configurar LED integrado D13
    DDRB |= (1 << PB5);
    PORTB &= ~(1 << PB5);

    // Registrar causa del reinicio
    brownout_inicial = (reinicio_brownout != 0);

    // Valores iniciales
    orden_mov = 0;
    porcentaje = 70;

    // Comunicacion Bluetooth
    enlace_visto = 0;
    ultimo_paquete_ms = 0;

    // Seguridad
    hubo_failsafe = 0;

    // Impulso de arranque
    impulso_activo = 0;
    inicio_impulso_ms = 0;

    // Mantener motores detenidos
    apagar_motores();

    // Estado inicial
    estado = ESPERA_ENLACE;
}

// =====================================
// RECIBIR TRAMA BLUETOOTH
// =====================================

void robot_recibir_trama(
    uint8_t cmd,
    uint8_t valor,
    uint32_t ahora)
{
    // Registrar recepcion valida
    ultimo_paquete_ms = ahora;
    enlace_visto = 1;

    // =================================
    // COMANDO STOP
    // =================================

    if (cmd == 0)
    {
        orden_mov = 0;
        impulso_activo = 0;

        // Detener inmediatamente
        apagar_motores();

        // Establecer o recuperar enlace
        if (estado == ESPERA_ENLACE ||
            estado == FAILSAFE)
        {
            if (seguridad_bateria_baja())
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

    // =================================
    // VERIFICAR ESTADO
    // =================================

    // No aceptar movimientos si
    // el robot no esta en MANUAL
    if (estado != MANUAL)
    {
        return;
    }

    // =================================
    // COMANDOS DE MOVIMIENTO
    // =================================

    if (cmd >= 1 && cmd <= 4)
    {
        // Impulso de arranque solamente
        // al iniciar movimiento desde STOP
        if (orden_mov == 0 &&
            valor > 0 &&
            valor < 100)
        {
            inicio_impulso_ms = ahora;
            impulso_activo = 1;
        }

        // Guardar comando y velocidad
        orden_mov = cmd;
        porcentaje = valor;
    }

    // =================================
    // CAMBIAR VELOCIDAD
    // =================================

    else if (cmd == 7)
    {
        porcentaje = valor;

        if (porcentaje == 0)
        {
            impulso_activo = 0;
        }
    }

    // =================================
    // ABRIR PINZA
    // =================================

    else if (cmd == 5)
    {
        agarre_abrir();
    }

    // =================================
    // CERRAR PINZA
    // =================================

    else if (cmd == 6)
    {
        agarre_cerrar();
    }
}

// =====================================
// ACTUALIZAR LED DE ESTADO
// =====================================

static void actualizar_led(uint32_t ahora)
{
    uint8_t led = 0;

    uint16_t fase =
        (uint16_t)(ahora % 1000UL);

    // Esperando enlace Bluetooth
    if (estado == ESPERA_ENLACE)
    {
        led = (fase < 500U);
    }

    // FAILSAFE: parpadeo rapido
    else if (estado == FAILSAFE)
    {
        led = ((ahora % 200UL) < 100UL);
    }

    // Bateria baja: doble destello
    else if (estado == BATERIA_BAJA)
    {
        led = (fase < 100U ||
               (fase >= 200U && fase < 300U));
    }

    // Actualizar LED D13
    if (led)
    {
        PORTB |= (1 << PB5);
    }
    else
    {
        PORTB &= ~(1 << PB5);
    }
}

// =====================================
// MAQUINA DE ESTADOS PRINCIPAL
// =====================================

void robot_tarea(uint32_t ahora)
{
    // =================================
    // SEGURIDAD: PERDIDA DE ENLACE
    // =================================

    if (seguridad_enlace_perdido(
            enlace_visto,
            ahora,
            ultimo_paquete_ms))
    {
        if (estado == MANUAL ||
            estado == BATERIA_BAJA)
        {
            // Entrar en FAILSAFE
            estado = FAILSAFE;

            // Registrar evento
            hubo_failsafe = 1;

            // Cancelar movimiento
            orden_mov = 0;
            impulso_activo = 0;

            // Detener motores
            apagar_motores();
        }
    }

    // =================================
    // SEGURIDAD: BATERIA BAJA
    // =================================

    if (estado == MANUAL &&
        seguridad_bateria_baja())
    {
        // Cambiar estado
        estado = BATERIA_BAJA;

        // Cancelar movimiento
        orden_mov = 0;
        impulso_activo = 0;

        apagar_motores();
    }

    // =================================
    // RECUPERACION DE BATERIA
    // =================================

    else if (estado == BATERIA_BAJA &&
             !seguridad_bateria_baja())
    {
        // Recuperar control manual
        estado = MANUAL;

        // No recuperar movimiento anterior
        orden_mov = 0;
        impulso_activo = 0;

        apagar_motores();
    }

    // =================================
    // CONTROL DE MOTORES
    // =================================

    if (estado == MANUAL)
    {
        // Convertir porcentaje a PWM
        // Rango: 0 a 255
        uint8_t pwm =
            (uint8_t)(
                ((uint16_t)porcentaje * 255U)
                / 100U
            );

        // =================================
        // IMPULSO DE ARRANQUE
        // =================================

        if (impulso_activo)
        {
            if (orden_mov != 0 &&
                porcentaje > 0 &&
                (uint32_t)(
                    ahora - inicio_impulso_ms
                ) < IMPULSO_ARRANQUE_MS)
            {
                // Potencia maxima temporal
                pwm = 255;
            }
            else
            {
                // Finalizar impulso
                impulso_activo = 0;
            }
        }

        // Aplicar velocidad
        motores_velocidad(pwm);

        // =================================
        // ADELANTE
        // =================================

        if (orden_mov == 1)
        {
            motores_avanzar();
        }

        // =================================
        // ATRAS
        // =================================

        else if (orden_mov == 2)
        {
            motores_retroceder();
        }

        // =================================
        // IZQUIERDA
        // =================================

        else if (orden_mov == 3)
        {
            motores_izquierda();
        }

        // =================================
        // DERECHA
        // =================================

        else if (orden_mov == 4)
        {
            motores_derecha();
        }

        // =================================
        // STOP
        // =================================

        else
        {
            apagar_motores();
        }
    }

    // =================================
    // ESTADOS SIN MOVIMIENTO
    // =================================

    else
    {
        // En cualquier estado distinto
        // de MANUAL, detener motores
        apagar_motores();
    }

    // Actualizar LED del Arduino
    actualizar_led(ahora);
}

// =====================================
// TELEMETRIA BLUETOOTH
// =====================================

void robot_telemetria(void)
{
    uint8_t flags = 0;

    // =================================
    // SENSOR DE OBJETO AGARRADO
    // =================================

    // Bit 0: objeto detectado
    if (sensores_objeto_agarrado())
    {
        flags |= (1 << 0);
    }

    // =================================
    // ESTADO DE BATERIA
    // =================================

    // Bit 1: bateria baja
    if (seguridad_bateria_baja())
    {
        flags |= (1 << 1);
    }

    // =================================
    // EVENTO FAILSAFE
    // =================================

    // Bit 2: ocurrio FAILSAFE
    if (hubo_failsafe)
    {
        flags |= (1 << 2);
    }

    // =================================
    // REINICIO POR BROWNOUT
    // =================================

    // Bit 3: reinicio por baja tension
    if (brownout_inicial)
    {
        flags |= (1 << 3);
    }

    // =================================
    // PREPARAR DATOS
    // =================================

    // Bateria en decivoltios
    uint8_t vbat = sensores_bateria_dv();

    // Estado de la maquina
    uint8_t est = (uint8_t)estado;

    // =================================
    // ENVIAR TRAMA
    // =================================

    // Byte 1: inicio
    bluetooth_enviar(0xBB);

    // Byte 2: voltaje bateria
    bluetooth_enviar(vbat);

    // Byte 3: estado
    bluetooth_enviar(est);

    // Byte 4: indicadores
    bluetooth_enviar(flags);

    // Byte 5: checksum XOR
    bluetooth_enviar(
        (uint8_t)(vbat ^ est ^ flags)
    );

    // Reiniciar indicador del evento
    hubo_failsafe = 0;
}
