#ifndef AUDIO_ANALYZER_H
#define AUDIO_ANALYZER_H

#include <stdbool.h>

// Parâmetros do áudio - valores EXTREMAMENTE sensíveis
#define VOLUME_THRESHOLD_LOW 0.05  // Limite para volume baixo (Volts)
#define VOLUME_THRESHOLD_OK 0.20   // Limite para volume bom (Volts)
#define VOLUME_THRESHOLD_HIGH 0.60 // Limite para volume alto/clipping (Volts)

// Limites de ruído em dB (estimados)
#define NOISE_THRESHOLD_LOW 30    // Abaixo deste valor é considerado silencioso
#define NOISE_THRESHOLD_MEDIUM 50 // Abaixo deste valor é considerado moderado, acima é alto

// Aumento do ganho do microfone para alta sensibilidade
#define MIC_GAIN_FACTOR 5.0f // Aumento substancial do ganho para detectar sons muito baixos

// Histórico para o gráfico de volume
#define HISTORY_SIZE 64

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

// Funções públicas
void init_audio_history(void);
float calculate_noise_floor(float current_voltage);
AudioAnalysis analyze_audio(void);

// Funções para acesso ao histórico (para display)
extern float voltage_history[HISTORY_SIZE];
extern float noise_floor_history[HISTORY_SIZE];
extern int history_index;

#endif // AUDIO_ANALYZER_H