#ifndef AUDIO_ANALYZER_H
#define AUDIO_ANALYZER_H
#include <stdbool.h>

/** 
 * @brief Limite inferior de volume 
 * Tensão abaixo da qual o sinal é considerado muito baixo 
 */
#define VOLUME_THRESHOLD_LOW 0.05

/** 
 * @brief Limite de volume aceitável 
 * Tensão considerada como volume adequado 
 */
#define VOLUME_THRESHOLD_OK 0.20

/** 
 * @brief Limite superior de volume 
 * Tensão acima da qual ocorre clipping de áudio 
 */
#define VOLUME_THRESHOLD_HIGH 0.60

/** 
 * @brief Limite inferior de ruído 
 * Nível de ruído considerado silencioso em decibéis 
 */
#define NOISE_THRESHOLD_LOW 30

/** 
 * @brief Limite médio de ruído 
 * Nível de ruído considerado moderado em decibéis 
 */
#define NOISE_THRESHOLD_MEDIUM 50

/** 
 * @brief Fator de ganho do microfone 
 * Multiplicador para aumentar a sensibilidade de captação 
 */
#define MIC_GAIN_FACTOR 5.0f

/** 
 * @brief Tamanho do histórico de volume 
 * Número de amostras mantidas para análise histórica 
 */
#define HISTORY_SIZE 64

/**
 * @brief Estrutura de análise de áudio
 * 
 * Armazena todos os parâmetros e métricas relevantes 
 * para análise do sinal de áudio captado
 */
typedef struct
{
    float voltage;      ///< Tensão instantânea do microfone
    float rms_value;    ///< Valor RMS do sinal
    float estimated_db; ///< Estimativa de nível em decibéis
    bool is_clipping;   ///< Flag de saturação do sinal
    bool is_low_volume; ///< Flag de volume baixo
    float noise_floor;  ///< Nível de ruído de fundo
} AudioAnalysis;

/**
 * @brief Inicializa o histórico de áudio
 * 
 * Prepara as estruturas de dados para armazenamento 
 * do histórico de análise de áudio
 */
void init_audio_history(void);

/**
 * @brief Calcula o nível de ruído de fundo
 * 
 * Determina o nível de ruído ambiente baseado no sinal atual
 * 
 * @param current_voltage Tensão atual do sinal de áudio
 * @return float Nível estimado de ruído de fundo
 */
float calculate_noise_floor(float current_voltage);

/**
 * @brief Analisa o sinal de áudio
 * 
 * Processa o sinal de áudio e extrai métricas relevantes
 * 
 * @return AudioAnalysis Resultado da análise de áudio
 */
AudioAnalysis analyze_audio(void);

// Históricos para visualização
extern float voltage_history[HISTORY_SIZE];      ///< Histórico de tensão
extern float noise_floor_history[HISTORY_SIZE];  ///< Histórico de nível de ruído
extern int history_index;                        ///< Índice atual no histórico

#endif // AUDIO_ANALYZER_H