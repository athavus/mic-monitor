#ifndef LED_FUNCTIONS_H
#define LED_FUNCTIONS_H

#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pico/stdlib.h"
#include "mic-monitor.pio.h"
#include "pico/bootrom.h"
#include "hardware/pio.h"
#include "letters.h"

#define NUM_LEDS 25                      // Número total de LEDs na matriz (5x5)
#define MAX_TEXT_LENGTH 100              // Número máximo de caracteres na mensagem
#define MAX_ROWS (5 * MAX_TEXT_LENGTH)   // Número máximo de colunas no texto concatenado

typedef struct {
    double r; // Red (0.0 a 1.0)
    double g; // Green (0.0 a 1.0)
    double b; // Blue (0.0 a 1.0)
} RGBColor;

typedef enum {
    CHAR_A, CHAR_B, CHAR_C, CHAR_D, CHAR_E, CHAR_F, CHAR_G, CHAR_H, CHAR_I, CHAR_J,
    CHAR_K, CHAR_L, CHAR_M, CHAR_N, CHAR_O, CHAR_P, CHAR_Q, CHAR_R, CHAR_S, CHAR_T,
    CHAR_U, CHAR_V, CHAR_W, CHAR_X, CHAR_Y, CHAR_Z, CHAR_SPACE, CHAR_EXCLAMATION, CHAR_DOT
} Char;

/**
 * Converte valores RGB (0.0 a 1.0) para um valor codificado de 32 bits (formato G|R|B).
 * @param b Valor do canal azul
 * @param r Valor do canal vermelho
 * @param g Valor do canal verde
 * @return Valor codificado RGB para envio via PIO
 */
uint32_t rgb_matrix(double b, double r, double g);

/**
 * Normaliza valores de cor RGB de 0–255 para 0.0–1.0.
 * @param color Ponteiro para struct RGBColor a ser normalizada
 */
void normalize_color(RGBColor *color);

/**
 * Mapeia um índice lógico (0 a 24) para o índice físico correto do LED na matriz 5x5.
 * @param index Índice lógico do LED
 * @return Índice físico ajustado para mapeamento da matriz
 */
int map_index_to_position(int index);

/**
 * Acende um LED específico com uma cor.
 * @param index Índice do LED (0 a 24)
 * @param color Cor no formato RGBColor
 * @param pio Instância do PIO usada
 * @param sm State machine ativa
 */
void set_led(int index, RGBColor color, PIO pio, uint sm);

/**
 * Cria um vetor de frames a partir de uma string de texto.
 * @param text Texto a ser convertido em frames
 * @return Ponteiro para vetor de frames
 */
double **create_text(const char *text);

/**
 * Exibe um frame completo na matriz de LEDs.
 * @param frame Frame 5x5 contendo valores de brilho
 * @param color Cor base para os LEDs acesos
 * @param pio Instância do PIO usada
 * @param sm State machine ativa
 * @param intensity Intensidade (0.0 a 1.0)
 */
void display_frame(double *frame, RGBColor color, PIO pio, uint sm, double intensity);

/**
 * Concatena múltiplos caracteres (frames) em uma matriz horizontal.
 * @param text Array de ponteiros para caracteres (frames)
 * @param text_length Quantidade de caracteres
 * @param full_text Matriz de saída contendo o texto completo concatenado
 */
void concatenate_text(double *text[], int text_length, double full_text[5][MAX_ROWS]);

/**
 * Acende um LED específico sem apagar os demais.
 * @param index Índice do LED (0 a 24)
 * @param color Cor desejada
 * @param pio Instância do PIO usada
 * @param sm State machine ativa
 * @param intensity Intensidade da cor do led
 */
void add_led(int index, RGBColor color, PIO pio, uint sm, double intensity);

/**
 * Exibe uma mensagem rolando na matriz de LEDs.
 * @param text Texto a ser mostrado
 * @param color Cor da mensagem
 * @param pio Instância do PIO usada
 * @param sm State machine ativa
 * @param intensity Intensidade dos LEDs (0.0 a 1.0)
 * @param speed Velocidade da rolagem em milissegundos
 */
void show_message(const char *text, RGBColor color, PIO pio, uint sm, double intensity, int speed);

/**
 * Exibe um modo de demonstração com várias cores na matriz.
 * @param pio Instância do PIO usada
 * @param sm State machine ativa
 * @param speed Tempo de espera entre mudanças de cor
 */
void show_demo1(PIO pio, uint sm, int speed);

#endif
