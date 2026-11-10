#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>

#include "sleep.h"

// Modo Idle
void entrar_idle(void)
{
    set_sleep_mode(SLEEP_MODE_IDLE);
    sleep_enable();
    sleep_cpu();
    sleep_disable();
}

// Modo ADC Noise Reduction
void entrar_adc_noise(void)
{
    set_sleep_mode(SLEEP_MODE_ADC);
    sleep_enable();
    sleep_cpu();
    sleep_disable();
}

// Modo Power-Down
void entrar_power_down(void)
{
    set_sleep_mode(SLEEP_MODE_PWR_DOWN);
    sleep_enable();
    sleep_cpu();
    sleep_disable();
}

