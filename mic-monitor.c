#include <stdio.h>
#include <math.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "pico/time.h"
#include "ssd1306/ssd1306.h"
#include "ssd1306/ssd1306_fonts.h"
#include "ssd1306/ssd1306_bitmaps.h"
#include "mic.h"

// Definição do botão
#define BUTTON_A_PIN 5

// Parâmetros do áudio - valores EXTREMAMENTE sensíveis
#define VOLUME_THRESHOLD_LOW 0.05  // Limite para volume baixo (Volts) - drasticamente reduzido
#define VOLUME_THRESHOLD_OK 0.20   // Limite para volume bom (Volts) - reduzido
#define VOLUME_THRESHOLD_HIGH 0.60 // Limite para volume alto/clipping (Volts) - reduzido

// Limites de ruído em dB (estimados) - muito mais sensíveis
#define NOISE_THRESHOLD_LOW 30    // Abaixo deste valor é considerado silencioso
#define NOISE_THRESHOLD_MEDIUM 50 // Abaixo deste valor é considerado moderado, acima é alto

// Definição para atualização da tela
#define DISPLAY_UPDATE_MS 50 // Tempo entre atualizações do display reduzido para maior fluidez

// Aumento do ganho do microfone para alta sensibilidade
#define MIC_GAIN_FACTOR 5.0f // Aumento substancial do ganho para detectar sons muito baixos

// Estrutura para armazenar resultados da análise de áudio
typedef struct
{
    float voltage;      // Tensão do microfone
    float rms_value;    // Valor RMS
    float estimated_db; // Estimativa em dB
    bool is_clipping;   // Indica se está ocorrendo clipping
    bool is_low_volume; // Indica se o volume está baixo
    float noise_floor;  // Valor do ruído de fundo
} AudioAnalysis;

// Histórico para o gráfico de volume - aumentado para maior resolução
#define HISTORY_SIZE 64
float voltage_history[HISTORY_SIZE] = {0};
float noise_floor_history[HISTORY_SIZE] = {0}; // Adiciona histórico de ruído de fundo
int history_index = 0;

// Variáveis para controle do botão
bool button_pressed = false;
bool last_button_state = true; // Pull-up, então estado inativo é HIGH
uint32_t last_debounce_time = 0;
#define DEBOUNCE_DELAY 200 // ms

// Estado global de visualização
bool show_graph = false;

// Sistema de cálculo de ruído de fundo
#define NOISE_FLOOR_ALPHA 0.1f // Fator de suavização para cálculo do ruído de fundo
float current_noise_floor = 0.0f;

/**
 * Inicializa o hardware e as bibliotecas
 */
void init_hardware()
{
    // Inicializa o stdio
    stdio_init_all();

    // Inicializa o botão
    gpio_init(BUTTON_A_PIN);
    gpio_set_dir(BUTTON_A_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_A_PIN);

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
 * Verifica o estado do botão A com debouncing
 * @return true se o botão foi pressionado (evento único)
 */
bool check_button()
{
    bool button_state = !gpio_get(BUTTON_A_PIN); // Invertido porque é pull-up
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

/**
 * Calcula o ruído de fundo usando um filtro de média móvel exponencial
 *
 * @param current_voltage Tensão atual lida do microfone
 * @return Valor estimado do ruído de fundo
 */
float calculate_noise_floor(float current_voltage)
{
    // Se o ruído de fundo ainda não foi inicializado
    if (current_noise_floor < 0.001f)
    {
        current_noise_floor = current_voltage * 0.5f; // Inicializa com metade do valor atual
    }

    // Se o valor atual está abaixo do ruído atual + margem, considera parte do ruído
    if (current_voltage < (current_noise_floor * 1.5f))
    {
        // Atualiza o ruído de fundo com um filtro EMA (Exponential Moving Average)
        current_noise_floor = (NOISE_FLOOR_ALPHA * current_voltage) +
                              ((1.0f - NOISE_FLOOR_ALPHA) * current_noise_floor);
    }

    return current_noise_floor;
}

/**
 * Analisa os dados de áudio do microfone
 *
 * @return Estrutura AudioAnalysis com os resultados da análise
 */
AudioAnalysis analyze_audio()
{
    AudioAnalysis analysis = {0};

    // Coleta amostras do microfone
    mic_sample();

    // Obtém valores da biblioteca do microfone
    analysis.rms_value = mic_get_rms();
    analysis.voltage = mic_get_voltage();

    // Aplica um ganho para AUMENTAR DRASTICAMENTE a sensibilidade
    analysis.voltage *= MIC_GAIN_FACTOR;
    analysis.rms_value *= MIC_GAIN_FACTOR;

    // Calcula o ruído de fundo
    analysis.noise_floor = calculate_noise_floor(analysis.voltage * 0.3f);

    // Armazena no histórico para o gráfico (com limitação para evitar valores extremos)
    voltage_history[history_index] = analysis.voltage > 3.3f ? 3.3f : analysis.voltage;
    noise_floor_history[history_index] = analysis.noise_floor;
    history_index = (history_index + 1) % HISTORY_SIZE;

    // Verifica se o volume está baixo (em relação ao ruído de fundo)
    analysis.is_low_volume = (analysis.voltage < (analysis.noise_floor + VOLUME_THRESHOLD_LOW));

    // Verifica se está ocorrendo clipping
    analysis.is_clipping = (analysis.voltage > VOLUME_THRESHOLD_HIGH);

    // Estima o valor em dB (aproximação simplificada e muito mais sensível)
    if (analysis.voltage > 0)
    {
        // Conversão para dB - referência ajustada para faixas típicas de ambientes silenciosos
        // dB = 20 * log10(voltage/reference) + offset
        analysis.estimated_db = 20.0f * log10f(analysis.voltage) + 100.0f; // Offset aumentado para maior sensibilidade

        // Garantir um valor mínimo razoável para o dB
        if (analysis.estimated_db < 25.0f)
        {
            analysis.estimated_db = 25.0f;
        }
    }
    else
    {
        analysis.estimated_db = 25.0f; // Valor mínimo de dB ajustado para maior sensibilidade
    }

    return analysis;
}

/**
 * Exibe os resultados da análise de áudio no display OLED
 *
 * @param analysis Estrutura com os resultados da análise
 */
void display_audio_monitor(AudioAnalysis analysis)
{
    // Limpa o display
    ssd1306_Fill(Black);

    // === Seção 1: Monitor ===
    ssd1306_SetCursor(0, 0);
    ssd1306_WriteString("Monitor", Font_7x10, White);

    // Desenha uma linha divisória
    ssd1306_Line(0, 12, 127, 12, White);

    // Exibe status do áudio
    ssd1306_SetCursor(0, 15);
    if (analysis.is_clipping)
    {
        ssd1306_WriteString("VOLUME ALTO!", Font_7x10, White);
        // Desenha indicador visual de alerta
        ssd1306_FillRectangle(110, 15, 127, 33, White);
    }
    else if (analysis.is_low_volume)
    {
        ssd1306_WriteString("Volume Baixo", Font_7x10, White);
    }
    else
    {
        ssd1306_WriteString("Audio OK", Font_7x10, White);
    }

    // === Seção 2: Medidor de Ruído ===
    ssd1306_SetCursor(0, 36);
    ssd1306_WriteString("Nivel de Ruido:", Font_7x10, White);

    // Mostra o valor estimado em dB, a tensão e o ruído de fundo para depuração
    char info_str[32];
    sprintf(info_str, "%.1fdB(%.2fV)", analysis.estimated_db, analysis.voltage);
    ssd1306_SetCursor(0, 48);
    ssd1306_WriteString(info_str, Font_6x8, White);

    // Desenha barra de progresso para nível de ruído
    uint8_t bar_width = (uint8_t)((analysis.estimated_db / 100.0f) * 128.0f);
    if (bar_width > 128)
        bar_width = 128;

    // Agora sempre desenha a barra preenchida, apenas com diferentes estilos
    ssd1306_FillRectangle(0, 57, bar_width, 63, White);

    // Escolhe os indicadores visuais baseados no nível de ruído
    if (analysis.estimated_db < NOISE_THRESHOLD_LOW)
    {
        // Verde (silencioso) - barra normal
        ssd1306_SetCursor(100, 48);
        ssd1306_WriteString("SILC", Font_7x10, White);
    }
    else if (analysis.estimated_db < NOISE_THRESHOLD_MEDIUM)
    {
        // Amarelo (moderado) - barra normal
        ssd1306_SetCursor(100, 48);
        ssd1306_WriteString("MOD", Font_7x10, White);
    }
    else
    {
        // Vermelho (alto) - barra invertida para destacar
        ssd1306_InvertRectangle(0, 57, bar_width, 63); // Inverte para destacar
        ssd1306_SetCursor(100, 48);
        ssd1306_WriteString("ALTO", Font_7x10, White);
    }

    // Atualiza o display
    ssd1306_UpdateScreen();
}

/**
 * Desenha um gráfico de volume e ruído no display
 * Redesenhado para mostrar tanto o sinal quanto o ruído
 * Com zoom aprimorado para melhor visualização
 */
void display_volume_graph() {
    // Limpa a tela
    ssd1306_Fill(Black);
    
    // Título
    ssd1306_SetCursor(0, 0);
    ssd1306_WriteString("Grafico Vol+Ruido", Font_7x10, White);
    
    // Área útil do gráfico: y de 15 a 62 (47 pixels de altura)
    const uint8_t graph_top = 15;
    const uint8_t graph_bottom = 62;
    const uint8_t graph_height = graph_bottom - graph_top;
    
    // Desenhar eixos
    ssd1306_Line(0, graph_bottom, 127, graph_bottom, White); // Eixo X
    ssd1306_Line(0, graph_top, 0, graph_bottom, White);      // Eixo Y
    
    // Determinar valores mínimos e máximos para ajuste de escala
    float min_value = 3.3f; // Inicializar com valor máximo possível
    float max_value = 0.0f; // Inicializar com valor mínimo possível
    
    // Encontrar min/max para ajustar a escala automaticamente
    for (int i = 0; i < HISTORY_SIZE; i++) {
        int idx = (history_index + i) % HISTORY_SIZE;
        
        // Verificar valores de volume
        if (voltage_history[idx] < min_value) min_value = voltage_history[idx];
        if (voltage_history[idx] > max_value) max_value = voltage_history[idx];
        
        // Verificar valores de ruído
        if (noise_floor_history[idx] < min_value) min_value = noise_floor_history[idx];
        if (noise_floor_history[idx] > max_value) max_value = noise_floor_history[idx];
    }
    
    // Adicionar margem aos limites para melhor visualização
    min_value = (min_value > 0.1f) ? min_value * 0.8f : 0.0f;
    max_value = max_value * 1.1f;
    
    // Garantir que a escala tenha um intervalo mínimo
    if (max_value - min_value < 0.5f) {
        float center = (max_value + min_value) / 2.0f;
        max_value = center + 0.25f;
        min_value = center - 0.25f;
        // Certificar que não ficamos abaixo de zero
        if (min_value < 0.0f) min_value = 0.0f;
    }
    
    // Calcular novas posições Y dos limiares com base na nova escala
    uint8_t high_y = graph_top + (uint8_t)((1.0f - (VOLUME_THRESHOLD_HIGH - min_value) / (max_value - min_value)) * graph_height);
    uint8_t low_y = graph_top + (uint8_t)((1.0f - (VOLUME_THRESHOLD_LOW - min_value) / (max_value - min_value)) * graph_height);
    
    // Garantir que os limiares estejam dentro dos limites visíveis
    if (high_y < graph_top) high_y = graph_top;
    if (high_y > graph_bottom) high_y = graph_bottom;
    if (low_y < graph_top) low_y = graph_top;
    if (low_y > graph_bottom) low_y = graph_bottom;
    
    // Desenhar linhas de referência
    ssd1306_Line(0, high_y, 127, high_y, White); // Linha de clipping
    ssd1306_Line(0, low_y, 127, low_y, White);   // Linha de volume baixo
    
    // Desenhar o gráfico de ruído de fundo (área preenchida inferior)
    for (int i = 0; i < HISTORY_SIZE - 1; i++) {
        int current_idx = (history_index + i) % HISTORY_SIZE;
        int next_idx = (history_index + i + 1) % HISTORY_SIZE;
        
        // Normalizar para a nova escala ajustada
        float current_noise = noise_floor_history[current_idx];
        float next_noise = noise_floor_history[next_idx];
        
        // Converter para coordenadas Y com a nova escala (invertido, pois 0,0 é no canto superior esquerdo)
        uint8_t current_y = graph_bottom - (uint8_t)(((current_noise - min_value) / (max_value - min_value)) * graph_height);
        uint8_t next_y = graph_bottom - (uint8_t)(((next_noise - min_value) / (max_value - min_value)) * graph_height);
        
        // Garantir que está dentro dos limites
        if (current_y < graph_top) current_y = graph_top;
        if (next_y < graph_top) next_y = graph_top;
        
        // Desenhar área preenchida para o ruído de fundo
        int x1 = i * 2; // Usando 2 pixels por ponto para aumentar a resolução
        int x2 = (i + 1) * 2;
        
        // Desenhar linha vertical do ruído até o fundo
        ssd1306_Line(x1, current_y, x1, graph_bottom, White);
        ssd1306_Line(x2, next_y, x2, graph_bottom, White);
        
        // Desenhar linha horizontal para conectar os pontos de ruído
        ssd1306_Line(x1, current_y, x2, next_y, White);
    }
    
    // Desenhar o gráfico de volume principal (linha superior mais brilhante)
    for (int i = 0; i < HISTORY_SIZE - 1; i++) {
        int current_idx = (history_index + i) % HISTORY_SIZE;
        int next_idx = (history_index + i + 1) % HISTORY_SIZE;
        
        // Normalizar para a nova escala ajustada
        float current_val = voltage_history[current_idx];
        float next_val = voltage_history[next_idx];
        
        // Converter para coordenadas Y com a nova escala
        uint8_t current_y = graph_bottom - (uint8_t)(((current_val - min_value) / (max_value - min_value)) * graph_height);
        uint8_t next_y = graph_bottom - (uint8_t)(((next_val - min_value) / (max_value - min_value)) * graph_height);
        
        // Garantir que está dentro dos limites
        if (current_y < graph_top) current_y = graph_top;
        if (next_y < graph_top) next_y = graph_top;
        
        // Desenhar linha entre pontos (linha mais espessa para o sinal principal)
        int x1 = i * 2;
        int x2 = (i + 1) * 2;
        ssd1306_Line(x1, current_y, x2, next_y, White);
        
        // Desenhar pontos em cada amostra para melhor visualização
        ssd1306_DrawPixel(x1, current_y - 1, White);
        ssd1306_DrawPixel(x2, next_y - 1, White);
    }
    
    // Adicionar indicadores de escala
    char scale_buf[8];
    sprintf(scale_buf, "%.1fV", max_value);
    ssd1306_SetCursor(110, graph_top);
    ssd1306_WriteString(scale_buf, Font_6x8, White);
    
    sprintf(scale_buf, "%.1fV", min_value);
    ssd1306_SetCursor(110, graph_bottom - 8);
    ssd1306_WriteString(scale_buf, Font_6x8, White);
    
    ssd1306_UpdateScreen();
}

/**
 * Função principal que executa em loop
 */
int main()
{
    // Inicializa o hardware
    init_hardware();

    // Variável para controle do tempo de atualização
    uint32_t last_update_time = 0;

    // Preenche o buffer de histórico inicialmente com alguns valores
    for (int i = 0; i < HISTORY_SIZE; i++)
    {
        mic_sample();
        float voltage = mic_get_voltage() * MIC_GAIN_FACTOR; // Aplicando o mesmo ganho de sensibilidade
        voltage_history[i] = voltage > 3.3f ? 3.3f : voltage;
        noise_floor_history[i] = voltage * 0.3f; // Estimativa inicial de ruído
    }

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