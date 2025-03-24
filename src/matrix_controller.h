#ifndef MATRIX_CONTROLLER_H
#define MATRIX_CONTROLLER_H

#include <stdbool.h>
#include "hardware/pio.h"

// Defines da matriz de leds RGB
#define OUT_PIN 7
#define SYS_CLOCK_KHZ 128000 // Clock do sistema em kHz (128 MHz)

// Funções públicas
bool init_matrix(PIO *pio, uint *sm, uint *offset);

#endif // MATRIX_CONTROLLER_H