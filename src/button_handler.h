#ifndef BUTTON_HANDLER_H
#define BUTTON_HANDLER_H
#include <stdbool.h>
#include "pico/stdlib.h"

/**
 * @brief Inicializa um pino de botão
 * 
 * Configura o pino GPIO especificado como entrada para um botão,
 * aplicando as configurações necessárias como pull-up/pull-down.
 * 
 * @param button_pin Número do pino GPIO do botão
 */
void init_button(uint button_pin);

/**
 * @brief Verifica o estado atual do botão
 * 
 * Lê o estado do botão e retorna se foi pressionado.
 * 
 * @return bool Verdadeiro se o botão foi pressionado, falso caso contrário
 */
bool check_button(void);

#endif // BUTTON_HANDLER_H