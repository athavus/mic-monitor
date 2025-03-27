# Sistema de Monitoramento de Áudio Avançado - BitDogLab

![Logo do Projeto](/documentation/virtuscc-logo.png)

## 📌 Descrição do Projeto

Um sistema de monitoramento de áudio desenvolvido para Raspberry Pi Pico W, projetando uma solução completa de análise e visualização de sinais acústicos em tempo real.

## 🔍 Arquitetura do Sistema

### Módulos Principais

#### 1. Análise de Áudio (`audio_analyzer.h`)
**Funcionalidades Avançadas:**
- Análise dinâmica de sinais de áudio
- Múltiplos limiares de volume
- Detecção de saturação (clipping)
- Estimativa de nível de decibéis

**Parâmetros Configuráveis:**
- Limiar de Volume Baixo: 0.05V
- Limiar de Volume Adequado: 0.20V
- Limiar de Volume Alto (Clipping): 0.60V
- Fator de Ganho do Microfone: 5.0
- Tamanho do Histórico: 64 amostras

**Estrutura de Dados:**
```c
typedef struct {
    float voltage;      // Tensão instantânea
    float rms_value;    // Valor RMS
    float estimated_db; // Estimativa em decibéis 
    bool is_clipping;   // Detecção de saturação
    bool is_low_volume; // Verificação de volume baixo
    float noise_floor;  // Nível de ruído de fundo
} AudioAnalysis;
```

#### 2. Gerenciamento de Botão (`button-handler.h`)
**Características:**
- Inicialização de pino GPIO para botão
- Verificação de estado de pressionamento
- Suporte a configurações flexíveis de GPIO

#### 3. Controle de Display (`display-manager.h`)
**Recursos:**
- Renderização de monitor de áudio
- Geração de gráfico de volume histórico
- Visualização em matriz de LEDs

#### 4. Controlador de Matriz de LEDs (`matrix-controller.h`)
**Especificações:**
- Pino de Saída: GPIO 7
- Frequência do Clock: 128 MHz
- Inicialização via PIO (Programmable I/O)

## 🛠 Requisitos de Hardware
- Raspberry Pi Pico W
- Módulo de Microfone
- Matriz de LEDs RGB
- Botão de Controle
- Protoboard
- Cabos Jumper

## 🔧 Configurações de Sistema

### Pinos e Interfaces
- **Botão de Controle:** GPIO 5
- **Matriz de LEDs:** GPIO 7
- **Comunicação:** Suporte a interfaces PIO

### Parâmetros de Áudio
- **Taxa de Amostragem:** Configurável
- **Ganho do Microfone:** Ajustável (Fator 5.0)
- **Histórico de Amostras:** 64 pontos

## 💻 Configuração e Instalação

### Pré-requisitos
- Raspberry Pi Pico SDK
- Ambiente de Desenvolvimento C
- Ferramentas de Compilação

### Passos de Instalação
1. Clone o repositório
2. Configure o ambiente Pico SDK
3. Instale dependências
4. Compile o projeto
5. Grave o firmware

## 🔬 Personalização e Extensão

### Pontos Ajustáveis
- Limiares de volume em `audio_analyzer.h`
- Configurações de GPIO
- Ganho do microfone
- Algoritmos de processamento de sinal

## 📊 Recursos Avançados
- Análise de áudio em tempo real
- Detecção de ruído de fundo
- Visualização gráfica
- Controle interativo por botão

## 🖼️ Galeria do Projeto

| Imagem 1 | Imagem 2 |
|----------|----------|
| ![Imagem 1](/documentation/image-bitdog-lab-1.png) | ![Imagem 2](/documentation/image-bitdog-lab-2.png) |

| Imagem 3 | Imagem 4 |
|----------|----------|
| ![Imagem 3](/documentation/image-bitdog-lab-3.png) | ![Imagem 4](/documentation/image-bitdog-lab-4.png) |
