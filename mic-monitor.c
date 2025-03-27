#include "mic-monitor.h"
#include "src/audio_analyzer.h"
#include "src/display_manager.h"
#include "src/button_handler.h"
#include "drivers/display-lcd/ssd1306.h"
#include "drivers/display-lcd/ssd1306_bitmaps.h"
#include "drivers/mic/mic.h"
#include "pico/time.h"

// Estado global de visualização
bool show_graph = false;

// PIO e sm
static PIO pio;
static uint sm, offset;

/**
 * Inicializa o hardware e as bibliotecas
 */
void init_hardware(void)
{
    // Inicializa o stdio
    stdio_init_all();

    // Inicializa o botão
    init_button(BUTTON_A_PIN);

    // Inicializa o microfone
    mic_init();

    // Inicializa o display SSD1306
    ssd1306_Init();
    ssd1306_Fill(Black);

    // Exibe mensagem de inicialização
    ssd1306_DrawBitmap(0, 0, virtuscc_bitmap, SSD1306_WIDTH, SSD1306_HEIGHT, White);
    ssd1306_UpdateScreen();
    sleep_ms(1500);
}

/**
 * Função principal que executa em loop
 */
int main(void)
{
    // Inicializa o hardware
    init_hardware();

    // Variável para controle do tempo de atualização
    uint32_t last_update_time = 0;

    // Preenche o buffer de histórico inicialmente com alguns valores
    init_audio_history();

    // Inicializa gráfico como visualização padrão
    show_graph = true;

    while (1)
    {
        uint32_t current_time = to_ms_since_boot(get_absolute_time());

        // Verifica se o botão foi pressionado
        if (check_button())
        {
            // Alterna entre os modos de visualização
            show_graph = !show_graph;
            // Atualiza imediatamente o display após a troca
            last_update_time = 0;
        }

        // Verifica se é hora de atualizar o display
        if (current_time - last_update_time >= DISPLAY_UPDATE_MS)
        {
            // Analisa o áudio
            AudioAnalysis analysis = analyze_audio();

            // Exibe resultados no display conforme o modo selecionado
            if (show_graph)
            {
                display_volume_graph();
            }
            else
            {
                display_audio_monitor(analysis);
            }

            last_update_time = current_time;
        }

        // Pequena pausa para reduzir uso da CPU
        sleep_ms(5); // Reduzido para maior taxa de amostragem
    }

    return 0;
}