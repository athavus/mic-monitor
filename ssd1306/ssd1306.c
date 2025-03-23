#include "ssd1306.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "math.h"
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "hardware/i2c.h"


#if defined(SSD1306_USE_I2C) //Verifica se o protocolo I2C está habilitado.

//Define os pinos SDA (dados) como GPIO14 e SCL (clock) como GPIO15.
const uint8_t I2C_SDA_PIN = 14;
const uint8_t I2C_SCL_PIN = 15;

// Enviar um byte para o registrador de comando
void ssd1306_WriteCommand(uint8_t byte) {
    uint8_t buffer[2];           // Buffer contendo o registrador e o dado (Cria um buffer de 2 bytes.)
    buffer[0] = 0x00;            // Endereço do registrador, define o byte de controle como 0x00 (indica que é um comando).
    buffer[1] = byte;            // Armazena o comando a ser enviado. Dado a ser enviado 

    i2c_write_blocking(SSD1306_I2C_PORT, SSD1306_I2C_ADDR, buffer, sizeof(buffer), false); // Envia o buffer via I2C para o endereço do display.
}

// Enviar dados
void ssd1306_WriteData(uint8_t* buffer, size_t buff_size) {
    uint8_t temp_buffer[buff_size + 1]; // Cria um buffer temporário com espaço para o byte de controle e os dados.
    temp_buffer[0] = 0x40;             // Define o byte de controle como 0x40 (indica que são dados).
    memcpy(&temp_buffer[1], buffer, buff_size); // Copia os dados para o buffer temporário

    i2c_write_blocking(SSD1306_I2C_PORT, SSD1306_I2C_ADDR, temp_buffer, sizeof(temp_buffer), false); // Envia o buffer via I2C.
}

#else
#error "You should define SSD1306_USE_SPI or SSD1306_USE_I2C macro"
#endif


static uint8_t SSD1306_Buffer[SSD1306_BUFFER_SIZE]; //Cria um buffer para armazenar o estado de cada pixel (1 bit por pixel). De tam.: 1024 bytes.

// Objeto display
static SSD1306_t SSD1306;

/* Preenche o SSD1306_Buffer com valores de um buffer fornecido de comprimento fixo */
SSD1306_Error_t ssd1306_FillBuffer(uint8_t* buf, uint32_t len) {
    SSD1306_Error_t ret = SSD1306_ERR;
    if (len <= SSD1306_BUFFER_SIZE) { //Verifica se o tamanho do buffer de entrada é válido.
        memcpy(SSD1306_Buffer,buf,len);
        ret = SSD1306_OK;
    }
    return ret;
}

/* Initialize the oled screen */
void ssd1306_Init(void) { 
    
    sleep_ms(100); // Espera o display inicializar
    i2c_init(i2c1, SSD1306_I2C_CLK * 1000); // Inicializa I2C
    // Configura pinos
    gpio_set_function(I2C_SDA_PIN, GPIO_FUNC_I2C); 
    gpio_set_function(I2C_SCL_PIN, GPIO_FUNC_I2C);
    // Habilita pull-ups
    gpio_pull_up(I2C_SDA_PIN);
    gpio_pull_up(I2C_SCL_PIN);

    // Inicializa o display 
    ssd1306_SetDisplayOn(0); // Desliga o display temporariamente
    // Envia comandos de configuração
    ssd1306_WriteCommand(0x2E); // 0x2E é o comando para desativar o scroll. Garante que o scroll esteja desativado ao iniciar
    ssd1306_WriteCommand(0x20); // Modo de endereçamento
    ssd1306_WriteCommand(0x00); // 00 - Modo de endereçamento horizontal; 
                                // 01 - Modo de endereçamento vertical;
                                // 10 - Modo de endereçamento de página (RESET); 
                                // 11 - Inválido

    ssd1306_WriteCommand(0xB0); //Definir endereço inicial da página para o modo de endereçamento de página, 0-7

#ifdef SSD1306_MIRROR_VERT
    ssd1306_WriteCommand(0xC0); // Mirror vertically
#else
    ssd1306_WriteCommand(0xC8); //Set COM Output Scan Direction
#endif

    ssd1306_WriteCommand(0x00); //---set low column address
    ssd1306_WriteCommand(0x10); //---set high column address

    ssd1306_WriteCommand(0x40); //--set start line address - CHECK

    ssd1306_SetContrast(0xFF);

#ifdef SSD1306_MIRROR_HORIZ
    ssd1306_WriteCommand(0xA0); // Mirror horizontally
#else
    ssd1306_WriteCommand(0xA1); //--set segment re-map 0 to 127 - CHECK
#endif

#ifdef SSD1306_INVERSE_COLOR
    ssd1306_WriteCommand(0xA7); //--set inverse color
#else
    ssd1306_WriteCommand(0xA6); //--set normal color
#endif

// Set multiplex ratio.
#if (SSD1306_HEIGHT == 128)
    // Found in the Luma Python lib for SH1106.
    ssd1306_WriteCommand(0xFF);
#else
    ssd1306_WriteCommand(0xA8); //--set multiplex ratio(1 to 64) - CHECK
#endif

#if (SSD1306_HEIGHT == 32)
    ssd1306_WriteCommand(0x1F); //
#elif (SSD1306_HEIGHT == 64)
    ssd1306_WriteCommand(0x3F); //
#elif (SSD1306_HEIGHT == 128)
    ssd1306_WriteCommand(0x3F); // Seems to work for 128px high displays too.
#else
#error "Only 32, 64, or 128 lines of height are supported!"
#endif

    ssd1306_WriteCommand(0xA4); //0xa4,Output follows RAM content;0xa5,Output ignores RAM content

    ssd1306_WriteCommand(0xD3); //-set display offset - CHECK
    ssd1306_WriteCommand(0x00); //-not offset

    ssd1306_WriteCommand(0xD5); //--set display clock divide ratio/oscillator frequency
    ssd1306_WriteCommand(0xF0); //--set divide ratio

    ssd1306_WriteCommand(0xD9); //--set pre-charge period
    ssd1306_WriteCommand(0x22); //

    ssd1306_WriteCommand(0xDA); //--set com pins hardware configuration - CHECK
#if (SSD1306_HEIGHT == 32)
    ssd1306_WriteCommand(0x02);
#elif (SSD1306_HEIGHT == 64)
    ssd1306_WriteCommand(0x12);
#elif (SSD1306_HEIGHT == 128)
    ssd1306_WriteCommand(0x12);
#else
#error "Only 32, 64, or 128 lines of height are supported!"
#endif

    ssd1306_WriteCommand(0xDB); //--set vcomh
    ssd1306_WriteCommand(0x20); //0x20,0.77xVcc

    ssd1306_WriteCommand(0x8D); //--set DC-DC enable
    ssd1306_WriteCommand(0x14); //
    ssd1306_SetDisplayOn(1); //--turn on SSD1306 panel

    // Clear screen
    ssd1306_Fill(Black);
    
    // Flush buffer to screen
    ssd1306_UpdateScreen();
    
    // Set default values for screen object
    SSD1306.CurrentX = 0;
    SSD1306.CurrentY = 0;
    
    SSD1306.Initialized = 1;
}

/* Fill the whole screen with the given color */
void ssd1306_Fill(SSD1306_COLOR color) {
    memset(SSD1306_Buffer, (color == Black) ? 0x00 : 0xFF, sizeof(SSD1306_Buffer)); //Preenche o buffer de tela com 0x00 (preto) ou 0xFF (branco), dependendo da cor especificada.
}

/* Write the screenbuffer with changed to the screen */
void ssd1306_UpdateScreen(void) {
    // Write data to each page of RAM. Number of pages
    // depends on the screen height:
    //
    //  * 32px   ==  4 pages
    //  * 64px   ==  8 pages
    //  * 128px  ==  16 pages
    for(uint8_t i = 0; i < SSD1306_HEIGHT/8; i++) {  //Itera sobre cada página (bloco de 8 pixels de altura).
        ssd1306_WriteCommand(0xB0 + i); // Define a página atual. (RAM page address).
        ssd1306_WriteCommand(0x00 + SSD1306_X_OFFSET_LOWER); // Define a coluna inicial.
        ssd1306_WriteCommand(0x10 + SSD1306_X_OFFSET_UPPER); // Define a coluna final.
        ssd1306_WriteData(&SSD1306_Buffer[SSD1306_WIDTH*i],SSD1306_WIDTH); //Envia os dados da página atual para o display.
    }
}

/*
 * Draw one pixel in the screenbuffer
 * X => X Coordinate
 * Y => Y Coordinate
 * color => Pixel color
 */
void ssd1306_DrawPixel(uint8_t x, uint8_t y, SSD1306_COLOR color) {
    if(x >= SSD1306_WIDTH || y >= SSD1306_HEIGHT) { // Verifica se as coordenadas estão dentro dos limites da tela.
        // Don't write outside the buffer
        return;
    }
   
    // Draw in the right color
    if(color == White) { // Se a cor for branca, liga o pixel.
        SSD1306_Buffer[x + (y / 8) * SSD1306_WIDTH] |= 1 << (y % 8);
    } else { //  Se a cor for preta, desliga o pixel.
        SSD1306_Buffer[x + (y / 8) * SSD1306_WIDTH] &= ~(1 << (y % 8));
    }
}

/*
 * Draw 1 char to the screen buffer
 * ch       => char om weg te schrijven
 * Font     => Font waarmee we gaan schrijven
 * color    => Black or White
 */
char ssd1306_WriteChar(char ch, SSD1306_Font_t Font, SSD1306_COLOR color) {
    uint32_t i, b, j;
    
    // Check if character is valid
    if (ch < 32 || ch > 126) 
        return 0;
    
    // Check remaining space on current line
    if (SSD1306_WIDTH < (SSD1306.CurrentX + Font.width) ||
        SSD1306_HEIGHT < (SSD1306.CurrentY + Font.height))
    {
        // Not enough space on current line
        return 0;
    }
    
    // Use the font to write
    for(i = 0; i < Font.height; i++) {
        b = Font.data[(ch - 32) * Font.height + i];
        for(j = 0; j < Font.width; j++) {
            if((b << j) & 0x8000)  {
                ssd1306_DrawPixel(SSD1306.CurrentX + j, (SSD1306.CurrentY + i), (SSD1306_COLOR) color);
            } else {
                ssd1306_DrawPixel(SSD1306.CurrentX + j, (SSD1306.CurrentY + i), (SSD1306_COLOR)!color);
            }
        }
    }
    
    // The current space is now taken
    SSD1306.CurrentX += Font.char_width ? Font.char_width[ch - 32] : Font.width;
    
    // Return written char for validation
    return ch;
}

/* Write full string to screenbuffer */
char ssd1306_WriteString(char* str, SSD1306_Font_t Font, SSD1306_COLOR color) {
    while (*str) {
        if (ssd1306_WriteChar(*str, Font, color) != *str) {
            // Char could not be written
            return *str;
        }
        str++;
    }
    
    // Everything ok
    return *str;
}

/* Position the cursor */
void ssd1306_SetCursor(uint8_t x, uint8_t y) {
    SSD1306.CurrentX = x;
    SSD1306.CurrentY = y;
}

/* Draw line by Bresenhem's algorithm */
void ssd1306_Line(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, SSD1306_COLOR color) {
    int32_t deltaX = abs(x2 - x1);
    int32_t deltaY = abs(y2 - y1);
    int32_t signX = ((x1 < x2) ? 1 : -1);
    int32_t signY = ((y1 < y2) ? 1 : -1);
    int32_t error = deltaX - deltaY;
    int32_t error2;
    
    ssd1306_DrawPixel(x2, y2, color);

    while((x1 != x2) || (y1 != y2)) {
        ssd1306_DrawPixel(x1, y1, color);
        error2 = error * 2;
        if(error2 > -deltaY) {
            error -= deltaY;
            x1 += signX;
        }
        
        if(error2 < deltaX) {
            error += deltaX;
            y1 += signY;
        }
    }
    return;
}

/* Draw polyline */
void ssd1306_Polyline(const SSD1306_VERTEX *par_vertex, uint16_t par_size, SSD1306_COLOR color) {
    uint16_t i;
    if(par_vertex == NULL) {
        return;
    }

    for(i = 1; i < par_size; i++) {
        ssd1306_Line(par_vertex[i - 1].x, par_vertex[i - 1].y, par_vertex[i].x, par_vertex[i].y, color);
    }

    return;
}

/* Convert Degrees to Radians */
static float ssd1306_DegToRad(float par_deg) {
    return par_deg * (3.14f / 180.0f);
}

/* Normalize degree to [0;360] */
static uint16_t ssd1306_NormalizeTo0_360(uint16_t par_deg) {
    uint16_t loc_angle;
    if(par_deg <= 360) {
        loc_angle = par_deg;
    } else {
        loc_angle = par_deg % 360;
        loc_angle = (loc_angle ? loc_angle : 360);
    }
    return loc_angle;
}

/*
 * DrawArc. Draw angle is beginning from 4 quart of trigonometric circle (3pi/2)
 * start_angle in degree
 * sweep in degree
 */
void ssd1306_DrawArc(uint8_t x, uint8_t y, uint8_t radius, uint16_t start_angle, uint16_t sweep, SSD1306_COLOR color) {
    static const uint8_t CIRCLE_APPROXIMATION_SEGMENTS = 36;
    float approx_degree;
    uint32_t approx_segments;
    uint8_t xp1,xp2;
    uint8_t yp1,yp2;
    uint32_t count;
    uint32_t loc_sweep;
    float rad;
    
    loc_sweep = ssd1306_NormalizeTo0_360(sweep);
    
    count = (ssd1306_NormalizeTo0_360(start_angle) * CIRCLE_APPROXIMATION_SEGMENTS) / 360;
    approx_segments = (loc_sweep * CIRCLE_APPROXIMATION_SEGMENTS) / 360;
    approx_degree = loc_sweep / (float)approx_segments;
    while(count < approx_segments)
    {
        rad = ssd1306_DegToRad(count*approx_degree);
        xp1 = x + (int8_t)(sinf(rad)*radius);
        yp1 = y + (int8_t)(cosf(rad)*radius);    
        count++;
        if(count != approx_segments) {
            rad = ssd1306_DegToRad(count*approx_degree);
        } else {
            rad = ssd1306_DegToRad(loc_sweep);
        }
        xp2 = x + (int8_t)(sinf(rad)*radius);
        yp2 = y + (int8_t)(cosf(rad)*radius);    
        ssd1306_Line(xp1,yp1,xp2,yp2,color);
    }
    
    return;
}

/*
 * Draw arc with radius line
 * Angle is beginning from 4 quart of trigonometric circle (3pi/2)
 * start_angle: start angle in degree
 * sweep: finish angle in degree
 */
void ssd1306_DrawArcWithRadiusLine(uint8_t x, uint8_t y, uint8_t radius, uint16_t start_angle, uint16_t sweep, SSD1306_COLOR color) {
    const uint32_t CIRCLE_APPROXIMATION_SEGMENTS = 36;
    float approx_degree;
    uint32_t approx_segments;
    uint8_t xp1;
    uint8_t xp2 = 0;
    uint8_t yp1;
    uint8_t yp2 = 0;
    uint32_t count;
    uint32_t loc_sweep;
    float rad;
    
    loc_sweep = ssd1306_NormalizeTo0_360(sweep);
    
    count = (ssd1306_NormalizeTo0_360(start_angle) * CIRCLE_APPROXIMATION_SEGMENTS) / 360;
    approx_segments = (loc_sweep * CIRCLE_APPROXIMATION_SEGMENTS) / 360;
    approx_degree = loc_sweep / (float)approx_segments;

    rad = ssd1306_DegToRad(count*approx_degree);
    uint8_t first_point_x = x + (int8_t)(sinf(rad)*radius);
    uint8_t first_point_y = y + (int8_t)(cosf(rad)*radius);   
    while (count < approx_segments) {
        rad = ssd1306_DegToRad(count*approx_degree);
        xp1 = x + (int8_t)(sinf(rad)*radius);
        yp1 = y + (int8_t)(cosf(rad)*radius);    
        count++;
        if (count != approx_segments) {
            rad = ssd1306_DegToRad(count*approx_degree);
        } else {
            rad = ssd1306_DegToRad(loc_sweep);
        }
        xp2 = x + (int8_t)(sinf(rad)*radius);
        yp2 = y + (int8_t)(cosf(rad)*radius);    
        ssd1306_Line(xp1,yp1,xp2,yp2,color);
    }
    
    // Radius line
    ssd1306_Line(x,y,first_point_x,first_point_y,color);
    ssd1306_Line(x,y,xp2,yp2,color);
    return;
}

/* Draw circle by Bresenhem's algorithm */
void ssd1306_DrawCircle(uint8_t par_x,uint8_t par_y,uint8_t par_r,SSD1306_COLOR par_color) {
    int32_t x = -par_r;
    int32_t y = 0;
    int32_t err = 2 - 2 * par_r;
    int32_t e2;

    if (par_x >= SSD1306_WIDTH || par_y >= SSD1306_HEIGHT) {
        return;
    }

    do {
        ssd1306_DrawPixel(par_x - x, par_y + y, par_color);
        ssd1306_DrawPixel(par_x + x, par_y + y, par_color);
        ssd1306_DrawPixel(par_x + x, par_y - y, par_color);
        ssd1306_DrawPixel(par_x - x, par_y - y, par_color);
        e2 = err;

        if (e2 <= y) {
            y++;
            err = err + (y * 2 + 1);
            if(-x == y && e2 <= x) {
                e2 = 0;
            }
        }

        if (e2 > x) {
            x++;
            err = err + (x * 2 + 1);
        }
    } while (x <= 0);

    return;
}

/* Draw filled circle. Pixel positions calculated using Bresenham's algorithm */
void ssd1306_FillCircle(uint8_t par_x,uint8_t par_y,uint8_t par_r,SSD1306_COLOR par_color) {
    int32_t x = -par_r;
    int32_t y = 0;
    int32_t err = 2 - 2 * par_r;
    int32_t e2;

    if (par_x >= SSD1306_WIDTH || par_y >= SSD1306_HEIGHT) {
        return;
    }

    do {
        for (uint8_t _y = (par_y + y); _y >= (par_y - y); _y--) {
            for (uint8_t _x = (par_x - x); _x >= (par_x + x); _x--) {
                ssd1306_DrawPixel(_x, _y, par_color);
            }
        }

        e2 = err;
        if (e2 <= y) {
            y++;
            err = err + (y * 2 + 1);
            if (-x == y && e2 <= x) {
                e2 = 0;
            }
        }

        if (e2 > x) {
            x++;
            err = err + (x * 2 + 1);
        }
    } while (x <= 0);

    return;
}

/* Draw a rectangle */
void ssd1306_DrawRectangle(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, SSD1306_COLOR color) {
    ssd1306_Line(x1,y1,x2,y1,color);
    ssd1306_Line(x2,y1,x2,y2,color);
    ssd1306_Line(x2,y2,x1,y2,color);
    ssd1306_Line(x1,y2,x1,y1,color);

    return;
}

/* Draw a filled rectangle */
void ssd1306_FillRectangle(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, SSD1306_COLOR color) {
    uint8_t x_start = ((x1<=x2) ? x1 : x2);
    uint8_t x_end   = ((x1<=x2) ? x2 : x1);
    uint8_t y_start = ((y1<=y2) ? y1 : y2);
    uint8_t y_end   = ((y1<=y2) ? y2 : y1);

    for (uint8_t y= y_start; (y<= y_end)&&(y<SSD1306_HEIGHT); y++) {
        for (uint8_t x= x_start; (x<= x_end)&&(x<SSD1306_WIDTH); x++) {
            ssd1306_DrawPixel(x, y, color);
        }
    }
    return;
}

SSD1306_Error_t ssd1306_InvertRectangle(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2) {
  if ((x2 >= SSD1306_WIDTH) || (y2 >= SSD1306_HEIGHT)) {
    return SSD1306_ERR;
  }
  if ((x1 > x2) || (y1 > y2)) {
    return SSD1306_ERR;
  }
  uint32_t i;
  if ((y1 / 8) != (y2 / 8)) {
    /* if rectangle doesn't lie on one 8px row */
    for (uint32_t x = x1; x <= x2; x++) {
      i = x + (y1 / 8) * SSD1306_WIDTH;
      SSD1306_Buffer[i] ^= 0xFF << (y1 % 8);
      i += SSD1306_WIDTH;
      for (; i < x + (y2 / 8) * SSD1306_WIDTH; i += SSD1306_WIDTH) {
        SSD1306_Buffer[i] ^= 0xFF;
      }
      SSD1306_Buffer[i] ^= 0xFF >> (7 - (y2 % 8));
    }
  } else {
    /* if rectangle lies on one 8px row */
    const uint8_t mask = (0xFF << (y1 % 8)) & (0xFF >> (7 - (y2 % 8)));
    for (i = x1 + (y1 / 8) * SSD1306_WIDTH;
         i <= (uint32_t)x2 + (y2 / 8) * SSD1306_WIDTH; i++) {
      SSD1306_Buffer[i] ^= mask;
    }
  }
  return SSD1306_OK;
}

/* Draw a bitmap */
void ssd1306_DrawBitmap(uint8_t x, uint8_t y, const unsigned char* bitmap, uint8_t w, uint8_t h, SSD1306_COLOR color) {
    int16_t byteWidth = (w + 7) / 8; // Bitmap scanline pad = whole byte
    uint8_t byte = 0;

    if (x >= SSD1306_WIDTH || y >= SSD1306_HEIGHT) {
        return;
    }

    for (uint8_t j = 0; j < h; j++, y++) {
        for (uint8_t i = 0; i < w; i++) {
            if (i & 7) {
                byte <<= 1;
            } else {
                byte = (*(const unsigned char *)(&bitmap[j * byteWidth + i / 8]));
            }

            if (byte & 0x80) {
                ssd1306_DrawPixel(x + i, y, color);
            }
        }
    }
    return;
}

void ssd1306_SetContrast(const uint8_t value) {
    const uint8_t kSetContrastControlRegister = 0x81;
    ssd1306_WriteCommand(kSetContrastControlRegister);
    ssd1306_WriteCommand(value);
}

void ssd1306_SetDisplayOn(const uint8_t on) {
    uint8_t value;
    if (on) {
        value = 0xAF;   // Display on
        SSD1306.DisplayOn = 1;
    } else {
        value = 0xAE;   // Display off
        SSD1306.DisplayOn = 0;
    }
    ssd1306_WriteCommand(value);
}

uint8_t ssd1306_GetDisplayOn() {
    return SSD1306.DisplayOn;
}


// ============== JÁ COM COMENTÁRIOS DE DOCUMENTAÇÃO API (Estilo doxygen) em Português(BR) ==========================


/**
 * @brief Inicia a rolagem horizontal para a direita.
 * @param startPage Página inicial (0-7)
 * @param endPage Página final (0-7)
 * @param scrollSpeed Velocidade do scroll (0 a 7, onde 0 é o mais rápido)
 */
void ssd1306_StartScrollRight(uint8_t startPage, uint8_t endPage, uint8_t scrollSpeed) {
    ssd1306_WriteCommand(0x26);              // Comando para scroll horizontal à direita
    ssd1306_WriteCommand(0x00);              // Byte dummy
    ssd1306_WriteCommand(startPage & 0x07);    // Página de início
    ssd1306_WriteCommand(scrollSpeed & 0x07);  // Intervalo de tempo
    ssd1306_WriteCommand(endPage & 0x07);      // Página final
    ssd1306_WriteCommand(0x00);              // Byte dummy
    ssd1306_WriteCommand(0xFF);              // Byte dummy
    ssd1306_WriteCommand(0x2F);              // Ativa o scroll
}

/**
 * @brief Inicia a rolagem horizontal para a esquerda.
 * @param startPage Página inicial (0-7)
 * @param endPage Página final (0-7)
 * @param scrollSpeed Velocidade do scroll (0 a 7, onde 0 é o mais rápido)
 */
void ssd1306_StartScrollLeft(uint8_t startPage, uint8_t endPage, uint8_t scrollSpeed) {
    ssd1306_WriteCommand(0x27);              // Comando para scroll horizontal à esquerda
    ssd1306_WriteCommand(0x00);              // Byte dummy
    ssd1306_WriteCommand(startPage & 0x07);    // Página de início
    ssd1306_WriteCommand(scrollSpeed & 0x07);  // Intervalo de tempo
    ssd1306_WriteCommand(endPage & 0x07);      // Página final
    ssd1306_WriteCommand(0x00);              // Byte dummy
    ssd1306_WriteCommand(0xFF);              // Byte dummy
    ssd1306_WriteCommand(0x2F);              // Ativa o scroll
}

/**
 * @brief Para a rolagem horizontal.
 */
void ssd1306_StopScroll(void) {
    ssd1306_WriteCommand(0x2E); // Desativa o scroll
}

/**
 * @brief Escreve uma string com quebra automática de linha.
 * @param str String a ser escrita.
 * @param Font Fonte a ser utilizada.
 * @param color Cor do texto.
 */
void ssd1306_WriteStringWrapped(const char* str, SSD1306_Font_t Font, SSD1306_COLOR color) {
    while (*str) {
        if (*str == '\n') {
            SSD1306.CurrentX = 0;
            SSD1306.CurrentY += Font.height;
            str++;
            continue;
        }
        // Obtém a largura do caractere (usa fonte proporcional se disponível)
        uint8_t charWidth = Font.char_width ? Font.char_width[*str - 32] : Font.width;
        if (SSD1306.CurrentX + charWidth > SSD1306_WIDTH) {
            // Se não houver espaço suficiente na linha, passa para a próxima
            SSD1306.CurrentX = 0;
            SSD1306.CurrentY += Font.height;
        }
        // Verifica se há espaço vertical suficiente
        if (SSD1306.CurrentY + Font.height > SSD1306_HEIGHT) {
            break; // Não há mais espaço para escrever
        }
        ssd1306_WriteChar(*str, Font, color);
        str++;
    }
}

/**
 * @brief Desenha o contorno de um triângulo conectando três pontos.
 * @param x0 Coordenada X do primeiro ponto.
 * @param y0 Coordenada Y do primeiro ponto.
 * @param x1 Coordenada X do segundo ponto.
 * @param y1 Coordenada Y do segundo ponto.
 * @param x2 Coordenada X do terceiro ponto.
 * @param y2 Coordenada Y do terceiro ponto.
 * @param color Cor do triângulo.
 */
void ssd1306_DrawTriangle(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, SSD1306_COLOR color) {
    ssd1306_Line(x0, y0, x1, y1, color);
    ssd1306_Line(x1, y1, x2, y2, color);
    ssd1306_Line(x2, y2, x0, y0, color);
}

/**
 * @brief Função auxiliar para trocar dois valores inteiros de 16 bits.
 */
static inline void swap_int16(int16_t *a, int16_t *b) {
    int16_t t = *a;
    *a = *b;
    *b = t;
}

/**
 * @brief Desenha um triângulo preenchido usando o algoritmo de scanline.
 * @param x0 Coordenada X do primeiro ponto.
 * @param y0 Coordenada Y do primeiro ponto.
 * @param x1 Coordenada X do segundo ponto.
 * @param y1 Coordenada Y do segundo ponto.
 * @param x2 Coordenada X do terceiro ponto.
 * @param y2 Coordenada Y do terceiro ponto.
 * @param color Cor do triângulo.
 */
void ssd1306_FillTriangle(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, SSD1306_COLOR color) {
    int16_t ax = x0, ay = y0;
    int16_t bx = x1, by = y1;
    int16_t cx = x2, cy = y2;

    // Ordena os vértices pela coordenada Y (ax,ay) <= (bx,by) <= (cx,cy)
    if (ay > by) { swap_int16(&ax, &bx); swap_int16((int16_t *)&ay, &by); }
    if (by > cy) { swap_int16(&bx, &cx); swap_int16((int16_t *)&by, &cy); }
    if (ay > by) { swap_int16(&ax, &bx); swap_int16((int16_t *)&ay, &by); }

    int16_t dx01 = bx - ax, dy01 = by - ay;
    int16_t dx02 = cx - ax, dy02 = cy - ay;
    int16_t dx12 = cx - bx, dy12 = cy - by;
    int16_t a, b;
    int16_t y, last;

    if (by == cy) last = by; // Caso especial: triângulo plano na parte inferior
    else         last = by - 1;

    // Parte superior do triângulo
    for (y = ay; y <= last; y++) {
        a = ax + (dx01 * (y - ay)) / (dy01 ? dy01 : 1);
        b = ax + (dx02 * (y - ay)) / (dy02 ? dy02 : 1);
        if (a > b) { int16_t t = a; a = b; b = t; }
        for (int16_t x = a; x <= b; x++) {
            ssd1306_DrawPixel(x, y, color);
        }
    }
    // Parte inferior do triângulo
    for (y = by; y <= cy; y++) {
        a = bx + (dx12 * (y - by)) / (dy12 ? dy12 : 1);
        b = ax + (dx02 * (y - ay)) / (dy02 ? dy02 : 1);
        if (a > b) { int16_t t = a; a = b; b = t; }
        for (int16_t x = a; x <= b; x++) {
            ssd1306_DrawPixel(x, y, color);
        }
    }
}

/**
 * @brief Exibe um texto que não cabe totalmente na tela fazendo scroll horizontal (sem quebra de linha).
 * @param text Texto a ser exibido.
 * @param Font Fonte a ser utilizada.
 * @param color Cor do texto.
 * @param y Linha vertical onde o texto será exibido.
 * @param delay_ms Tempo de atraso (em milissegundos) entre as atualizações do scroll.
 */
void ssd1306_ScrollTextHorizontal(const char* text, SSD1306_Font_t Font, SSD1306_COLOR color, uint8_t y, uint16_t delay_ms) {
    // Calcula a largura total do texto
    uint16_t text_width = 0;
    const char* ptr = text;
    while (*ptr) {
        uint8_t charWidth = Font.char_width ? Font.char_width[*ptr - 32] : Font.width;
        text_width += charWidth;
        ptr++;
    }
    
    // O scroll ocorrerá do texto completamente à direita até ele ter rolado para fora pela esquerda.
    // Ou seja, o deslocamento varia de 0 até (text_width + SSD1306_WIDTH)
    for (int offset = 0; offset < text_width + SSD1306_WIDTH; offset++) {
        // Limpa a área onde o texto será desenhado
        ssd1306_FillRectangle(0, y, SSD1306_WIDTH - 1, y + Font.height - 1, Black);
        
        // Desenha o texto com deslocamento: x inicial negativo faz com que o início do texto fique fora da tela e entre gradualmente
        int currentX = SSD1306_WIDTH - offset;
        ptr = text;
        while (*ptr) {
            uint8_t charWidth = Font.char_width ? Font.char_width[*ptr - 32] : Font.width;
            // Desenha o caractere somente se ele estiver dentro da área visível
            if ( (currentX + charWidth > 0) && (currentX < SSD1306_WIDTH) ) {
                // Salva a posição atual para não interferir com o resto do desenho
                uint16_t savedX = SSD1306.CurrentX;
                uint16_t savedY = SSD1306.CurrentY;
                ssd1306_SetCursor(currentX, y);
                ssd1306_WriteChar(*ptr, Font, color);
                ssd1306_SetCursor(savedX, savedY);
            }
            currentX += charWidth;
            ptr++;
        }
        ssd1306_UpdateScreen();
        sleep_ms(delay_ms);
    }
}


