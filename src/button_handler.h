#ifndef BUTTON_HANDLER_H
#define BUTTON_HANDLER_H

#include <stdbool.h>
#include "pico/stdlib.h"

// Funções públicas
void init_button(uint button_pin);
bool check_button(void);

#endif // BUTTON_HANDLER_H