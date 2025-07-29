---
# 📊 Datalogger de Movimento com IMU (MPU6050)

Este projeto implementa um **datalogger portátil de movimento** utilizando o sensor inercial **MPU6050** e o microcontrolador **RP2040** (plataforma BitDogLab). O sistema é capaz de registrar aceleração e giroscópio nos eixos X, Y e Z em tempo real, armazenando os dados em um cartão **MicroSD** no formato `.csv` para análise posterior.

Além da coleta de dados, o projeto inclui um **script em Python** que gera gráficos de aceleração e velocidade angular a partir dos arquivos gravados.
---

## 🚀 Funcionalidades

- Leitura de **aceleração e giroscópio** via protocolo I2C
- Armazenamento dos dados em **MicroSD** no formato `.csv`
- **Display OLED SSD1306** para exibição em tempo real:

  - Roll e Pitch
  - Dados brutos de aceleração
  - Dados brutos de giroscópio
  - Status do cartão MicroSD

- **Feedback visual e sonoro**:

  - LED RGB indica status do sistema (montagem, gravação, erro etc.)
  - Buzzers emitem alertas para início e fim de gravação

- Controle via **botões físicos**:

  - Botão A: Monta/Desmonta cartão MicroSD
  - Botão B: Inicia/Encerra gravação
  - Joystick SW: Alterna modos de exibição no OLED

- Script Python para **visualização gráfica dos dados coletados**

---

## 🖥️ Hardware Utilizado

- **Plataforma BitDogLab** com microcontrolador **RP2040**
- **Sensor IMU MPU6050** (Acelerômetro + Giroscópio)
- **Display OLED SSD1306**
- **Cartão MicroSD** e módulo de leitura
- **LED RGB**
- **2 Buzzers**
- **Botões A, B e Joystick SW**

---

## 📂 Estrutura do Repositório

```bash
├── src/                # Código-fonte do firmware para RP2040
│   ├── main.c
│   ├── imu.c / imu.h
│   ├── sd_file.c / sd_file.h
│   ├── ssd1306.c / ssd1306.h
│   └── buzzer.c / buzzer.h
├── scripts/
│   └── grafico.py      # Script Python para gerar gráficos dos dados gravados
├── data/
│   └── exemplo.csv     # Arquivo de exemplo gerado pelo datalogger
└── README.md
```

---

## ⚙️ Como Usar

### 1️⃣ Gravar dados com o Datalogger

1. Monte o cartão MicroSD no dispositivo (Botão A).
2. Inicie a gravação (Botão B).
3. Após coletar os dados desejados, pare a gravação (Botão B) e desmonte o cartão (Botão A).
4. Os arquivos `.csv` estarão disponíveis no cartão.

### 2️⃣ Visualizar gráficos em Python

> **Pré-requisitos**: Python 3.8+ instalado

```bash
# Clonar repositório
git clone https://github.com/limarich/IMU-dataloger
cd IMU-dataloger/scripts

# Criar ambiente virtual (opcional)
python3 -m venv venv
source venv/bin/activate    # Linux/Mac
venv\Scripts\activate       # Windows

# Instalar dependências
pip install pandas matplotlib

# Executar script
python grafico.py
```

Serão exibidos **8 gráficos**:

- Giroscópio XYZ combinado
- Aceleração XYZ combinada
- Giroscópio X, Y, Z individuais
- Aceleração X, Y, Z individuais

---

## 🌈 Indicações do LED RGB

| Cor                | Estado do Sistema                        |
| ------------------ | ---------------------------------------- |
| 🟡 Amarelo         | Inicializando / Montando cartão MicroSD  |
| 🟢 Verde           | Pronto para iniciar captura / SD montado |
| 🔴 Vermelho        | Gravação de dados em andamento           |
| 🔵 Azul (piscando) | Acessando cartão SD (leitura/gravação)   |
| 🟣 Roxo (piscando) | Erro (falha ao montar/gravar no SD)      |

---

## 📹 Demonstração

- **Vídeo:** [Assista no YouTube](https://youtu.be/F-c5UtbQmEU)

---

## 👤 Autor

**Richard Lima Ribeiro**
📍 Polo Juazeiro – EMBARCATECH – Residência Tecnológica em Sistemas Embarcados

---
