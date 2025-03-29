#include "inc/button_handler.h"
#include "pico/stdlib.h"
#include "pico/time.h"

// Variáveis para controle do botão
static uint button_pin;
static bool button_pressed = false;
static bool last_button_state = true; // Pull-up, então estado inativo é HIGH
static uint32_t last_debounce_time = 0;
#define DEBOUNCE_DELAY 200 // ms

/**
 * Inicializa o botão especificado
 * 
 * @param pin Número do pino do botão
 */
void init_button(uint pin)
{
    button_pin = pin;
    
    // Configura o pino como entrada com pull-up
    gpio_init(button_pin);
    gpio_set_dir(button_pin, GPIO_IN);
    gpio_pull_up(button_pin);
}

/**
 * Verifica o estado do botão com debouncing
 * 
 * @return true se o botão foi pressionado (evento único)
 */
bool check_button(void)
{
    bool button_state = !gpio_get(button_pin); // Invertido porque é pull-up
    uint32_t current_time = to_ms_since_boot(get_absolute_time());

    // Se o estado mudou, resetar o timer de debounce
    if (button_state != last_button_state)
    {
        last_debounce_time = current_time;
    }

    // Se passado tempo suficiente desde a última mudança de estado
    if ((current_time - last_debounce_time) > DEBOUNCE_DELAY)
    {
        // Se o botão está pressionado e isso ainda não foi registrado
        if (button_state && !button_pressed)
        {
            button_pressed = true;
            last_button_state = button_state;
            return true;
        }
        // Se o botão foi solto
        else if (!button_state && button_pressed)
        {
            button_pressed = false;
        }
    }

    last_button_state = button_state;
    return false;
}