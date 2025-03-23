#ifndef MIC_H
#define MIC_H

#include <stdint.h>

// Configurações padrão do ADC
#define MIC_CHANNEL 2
#define MIC_PIN (26 + MIC_CHANNEL)
#define ADC_CLOCK_DIV 96.f
#define SAMPLES 200

// Funções públicas da biblioteca
void mic_init();
void mic_sample();
float mic_get_rms();
float mic_get_voltage();

#endif // MIC_H