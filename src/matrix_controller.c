#include "matrix_controller.h"
#include "pico/stdlib.h"
#include "hardware/clocks.h"
#include "mic-monitor.pio.h"
#include "drivers/matrix-leds-rgb/led_functions.h"

/**
 * Inicializa a matriz de LEDs RGB com o PIO
 */
bool init_matrix(PIO *pio, uint *sm, uint *offset)
{
    if (!set_sys_clock_khz(SYS_CLOCK_KHZ, false))
    {
        return false;
    }
   
    *pio = pio0;
    *offset = pio_add_program(*pio, &main_program);
    *sm = pio_claim_unused_sm(*pio, true);
   
    if (*offset == -1 || *sm == -1)
    {
        return false;
    }
   
    main_program_init(*pio, *sm, *offset, OUT_PIN);
    return true;
}