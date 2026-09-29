# IHM de Monitoramento de Temperatura Infravermelha com ESP32-S3-Touch-LCD-7B e MLX90614

Projeto de interface homem-máquina (IHM) para monitoramento térmico sem contato utilizando o módulo **ESP32-S3-Touch-LCD-7B** (Waveshare), sensor infravermelho **MLX90614 (GY-906)** e interface gráfica desenvolvida no **EEZ Studio** (LVGL).

---

## 📌 Descrição do Projeto
Este sistema realiza a leitura contínua de temperatura de objetos/superfícies e ambiente via barramento I2C através do sensor infravermelho MLX90614. Os dados são processados pelo ESP32-S3 e exibidos em tempo real na tela touch de 7 polegadas com uma interface gráfica fluida e intuitiva.

A aplicação foi projetada com foco em inspeção e monitoramento térmico preventivo para ambientes e equipamentos industriais.

---

## 🛠️ Hardware & Tecnologias Utilizadas
- **Display & Microcontrolador:** ESP32-S3-Touch-LCD-7B (Waveshare 7" Capacitive Touch Display)
- **Sensor:** MLX90614 (GY-906) - Sensor de Temperatura Infravermelho Sem Contato (I2C)
- **Design de Interface (IHM):** EEZ Studio / LVGL
- **Ambiente de Desenvolvimento:** VS Code + PlatformIO
- **Linguagem:** C / C++

---

## 📂 Estrutura do Repositório

```
├── src/                # Código-fonte principal do firmware (main.cpp e rotinas de leitura)
├── include/            # Arquivos de cabeçalho (.h) e configurações da IHM
├── eez-studio/         # Projeto original do EEZ Studio (.eez)
├── platformio.ini      # Arquivo de configuração de dependências e bibliotecas do PlatformIO
└── README.md           # Documentação do projeto
```
---

## 🚀 Como Executar o Projeto
1. Clone este repositório:
   ```bash
   git clone https://github.com/samuelrsil/esp32-s3-touch-lcd-7b-ihm-mlx90614.git
   ```
