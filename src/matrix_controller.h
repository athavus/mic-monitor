#ifndef MATRIX_CONTROLLER_H
#define MATRIX_CONTROLLER_H
#include <stdbool.h>
#include "hardware/pio.h"

/** 
 * @brief Pino de saída para controle da matriz de LEDs 
 * Define o pino GPIO usado para transmissão de dados para a matriz
 */
#define OUT_PIN 7

/** 
 * @brief Frequência do clock do sistema 
 * Configura a frequência do clock do sistema em kilohertz (128 MHz)
 */
#define SYS_CLOCK_KHZ 128000

/**
 * @brief Inicializa a matriz de LEDs RGB
 * 
 * Configura os recursos de PIO necessários para controlar a matriz de LEDs.
 * 
 * @param pio Ponteiro para a instância de PIO a ser configurada
 * @param sm Ponteiro para o state machine a ser inicializado
 * @param offset Ponteiro para o offset do programa PIO
 * 
 * @return bool Indica se a inicialização foi bem-sucedida
 */
bool init_matrix(PIO *pio, uint *sm, uint *offset);

#endif // MATRIX_CONTROLLER_H