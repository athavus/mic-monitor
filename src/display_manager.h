#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H
#include "audio_analyzer.h"

/**
 * @brief Exibe o monitor de áudio com os resultados da análise.
 * 
 * Esta função processa e mostra os resultados da análise de áudio 
 * na matriz de LEDs ou em outro meio de exibição.
 * 
 * @param analysis Estrutura contendo os resultados da análise de áudio
 */
void display_audio_monitor(AudioAnalysis analysis);

/**
 * @brief Desenha um gráfico de volume na matriz de LEDs.
 * 
 * Renderiza um gráfico histórico do volume de áudio, 
 * mostrando a variação dos níveis de áudio ao longo do tempo.
 */
void display_volume_graph(void);

#endif // DISPLAY_MANAGER_H