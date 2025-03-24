#ifndef MIC_MONITOR_H
#define MIC_MONITOR_H

#include <stdio.h>
#include <stdbool.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"

// Definição do botão
#define BUTTON_A_PIN 5

// Parâmetros para atualização da tela
#define DISPLAY_UPDATE_MS 50 // Tempo entre atualizações do display

// Declarações de funções públicas
void init_hardware(void);
int main(void);

#endif // MIC_MONITOR_H