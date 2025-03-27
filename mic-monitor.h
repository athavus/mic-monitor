#ifndef MIC_MONITOR_H
#define MIC_MONITOR_H
#include <stdio.h>
#include <stdbool.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"

/** 
 * @brief Pino GPIO do botão de controle
 * Define o número do pino GPIO usado para o botão A 
 */
#define BUTTON_A_PIN 5

/** 
 * @brief Intervalo de atualização do display
 * Tempo em milissegundos entre cada atualização da tela 
 */
#define DISPLAY_UPDATE_MS 50

/**
 * @brief Inicializa o hardware do sistema
 * 
 * Configura todos os periféricos e componentes de hardware
 * necessários para o funcionamento do monitor de microfone.
 * Isso pode incluir inicialização de GPIO, PIO, botões, etc.
 */
void init_hardware(void);

/**
 * @brief Função principal do programa
 * 
 * Ponto de entrada do aplicativo de monitoramento de microfone.
 * Gerencia o loop principal, processamento de áudio e 
 * interação com o usuário.
 * 
 * @return int Código de status de saída do programa
 */
int main(void);

#endif // MIC_MONITOR_H