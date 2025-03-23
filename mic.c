#include "mic.h"
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include <math.h>

static uint16_t adc_buffer[SAMPLES];

// Inicializa o ADC para leitura do microfone
void mic_init() {
    adc_gpio_init(MIC_PIN);
    adc_init();
    adc_select_input(MIC_CHANNEL);
    adc_set_clkdiv(ADC_CLOCK_DIV);
}

// Realiza leituras do ADC e armazena no buffer
void mic_sample() {
    for (int i = 0; i < SAMPLES; i++) {
        adc_buffer[i] = adc_read();
    }
}

// Calcula a potência média (RMS) das amostras
float mic_get_rms() {
    float avg = 0.f;
    for (uint i = 0; i < SAMPLES; i++) {
        avg += adc_buffer[i] * adc_buffer[i];
    }
    avg /= SAMPLES;
    return sqrtf(avg);
}

// Converte a leitura RMS em tensão
float mic_get_voltage() {
    float rms = mic_get_rms();
    return fabs(rms * 3.3f / (1 << 12u) - 1.65f);
}