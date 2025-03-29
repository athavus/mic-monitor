#include "inc/audio_analyzer.h"
#include "drivers/mic/mic.h"
#include <math.h>

// Histórico para o gráfico de volume
float voltage_history[HISTORY_SIZE] = {0};
float noise_floor_history[HISTORY_SIZE] = {0}; // Histórico de ruído de fundo
int history_index = 0;

// Sistema de cálculo de ruído de fundo
#define NOISE_FLOOR_ALPHA 0.1f // Fator de suavização para cálculo do ruído de fundo
float current_noise_floor = 0.0f;

/**
 * Inicializa o histórico de áudio com valores básicos
 */
void init_audio_history(void)
{
    for (int i = 0; i < HISTORY_SIZE; i++)
    {
        mic_sample();
        float voltage = mic_get_voltage() * MIC_GAIN_FACTOR; // Aplicando o mesmo ganho de sensibilidade
        voltage_history[i] = voltage > 3.3f ? 3.3f : voltage;
        noise_floor_history[i] = voltage * 0.3f; // Estimativa inicial de ruído
    }
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
AudioAnalysis analyze_audio(void)
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