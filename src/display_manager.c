#include "display_manager.h"
#include "audio_analyzer.h"
#include "drivers/display-lcd/ssd1306.h"
#include "drivers/display-lcd/ssd1306_fonts.h"
#include <stdio.h>

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
void display_volume_graph(void) {
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