#include <stdio.h>
#include <string.h>
#include <math.h>
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "hardware/i2c.h"
#include "ssd1306.h"
#include "font.h"
#include "pico/bootrom.h"
#include "imu.h"
#include "lib/sd_file.h"
#include "lib/buzzer.h"

// LED RGB
#define LED_RED 13   // pino do led vermelho
#define LED_BLUE 12  // pino do led azul
#define LED_GREEN 11 // pino do led verde

// BOTÕES
#define BUTTON_A 5
#define BUTTON_B 6
#define BUTTON_JOYSTICK 22

// BUZZERS
#define BUZZER_A 10          // PORTA DO BUZZER A
#define BUZZER_B 21          // PORTA DO BUZZER B
#define BUZZER_FREQUENCY 200 // FREQUENCIA DO BUZZER

// I2C
#define I2C_PORT i2c0
#define I2C_SDA 0
#define I2C_SCL 1
#define I2C_PORT_DISP i2c1
#define I2C_SDA_DISP 14
#define I2C_SCL_DISP 15
#define DISPLAY_ADDRESS 0x3C
#define DISP_W 128
#define DISP_H 64

// GRAFICOS
#define BAR_CENTER_X (DISP_W / 2)
#define BAR_MAX_WIDTH ((DISP_W / 2) - 10)

#define GYRO_SCALE 131.0f    // LSB por °/s
#define ACCEL_SCALE 16384.0f // LSB por g (já usado no imu_read)
#define ACCEL_MAX_G 2.0f     // Valor máximo esperado (±2g)

#define DEBOUNCE_TIME 200 // Tempo de debounce em milissegundos
static int addr = 0x68;

int modo_exibicao = 0; // 0: menu_sd, 1: Roll/Pitch, 2: Gyro, 3: Acel
uint last_a_interrupt = 0;
uint last_b_interrupt = 0;
uint last_joystick_interrupt = 0;
int sd_card_state = 0; // 0: desmontado, 1: montado, 2: gravando
bool sd_montado = false;
bool gravando = false;
int contador_amostras = 0;
volatile bool flag_toggle_sd = false;
volatile bool flag_toggle_gravacao = false;
volatile bool flag_modo_exibicao = false;

void gpio_irq_handler(uint gpio, uint32_t events)
{
    uint current_time = to_ms_since_boot(get_absolute_time());

    if (gpio == BUTTON_A && (current_time - last_a_interrupt > DEBOUNCE_TIME))
    {
        last_a_interrupt = current_time;
        flag_toggle_sd = true;
    }
    else if (gpio == BUTTON_B && (current_time - last_b_interrupt > DEBOUNCE_TIME))
    {
        last_b_interrupt = current_time;
        flag_toggle_gravacao = true;
    }
    else if (gpio == BUTTON_JOYSTICK && (current_time - last_joystick_interrupt > DEBOUNCE_TIME))
    {
        last_joystick_interrupt = current_time;
        flag_modo_exibicao = true;
    }
}

void draw_gyro_graph(ssd1306_t *ssd, float gx, float gy, float gz);
void draw_accel_graph(ssd1306_t *ssd, float ax, float ay, float az);
void set_rgb(bool r, bool g, bool b);
void piscar_rgb(bool r, bool g, bool b, int dur_ms);

int main()
{
    stdio_init_all();

    // inicializa os buzzers
    initialization_buzzers(BUZZER_A, BUZZER_B);

    // inicializa botões
    // BOTÃO A
    gpio_init(BUTTON_A);
    gpio_set_dir(BUTTON_A, GPIO_IN);
    gpio_pull_up(BUTTON_A);
    gpio_set_irq_enabled_with_callback(BUTTON_A, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);

    // BOTÃO B
    gpio_init(BUTTON_B);
    gpio_set_dir(BUTTON_B, GPIO_IN);
    gpio_pull_up(BUTTON_B);
    gpio_set_irq_enabled_with_callback(BUTTON_B, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);

    // BOTÃO JOYSTICK
    gpio_init(BUTTON_JOYSTICK);
    gpio_set_dir(BUTTON_JOYSTICK, GPIO_IN);
    gpio_pull_up(BUTTON_JOYSTICK);
    gpio_set_irq_enabled_with_callback(BUTTON_JOYSTICK, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);

    // Inicializa o display SSD1306
    i2c_init(I2C_PORT_DISP, 400 * 1000);
    gpio_set_function(I2C_SDA_DISP, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL_DISP, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA_DISP);
    gpio_pull_up(I2C_SCL_DISP);

    ssd1306_t ssd;
    ssd1306_init(&ssd, DISP_W, DISP_H, false, DISPLAY_ADDRESS, I2C_PORT_DISP);
    ssd1306_config(&ssd);
    ssd1306_send_data(&ssd);

    ssd1306_fill(&ssd, false);
    ssd1306_send_data(&ssd);

    // Inicializa o IMU
    i2c_init(I2C_PORT, 400 * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);

    bi_decl(bi_2pins_with_func(I2C_SDA, I2C_SCL, GPIO_FUNC_I2C));

    //  LED RGB
    gpio_init(LED_RED);
    gpio_set_dir(LED_RED, GPIO_OUT);

    gpio_init(LED_GREEN);
    gpio_set_dir(LED_GREEN, GPIO_OUT);

    gpio_init(LED_BLUE);
    gpio_set_dir(LED_BLUE, GPIO_OUT);

    set_rgb(1, 1, 0); // Amarelo: sistema inicializando
    sleep_ms(1000);

    imu_data_t imu;
    bool cor = true;

    char str_roll[20];
    char str_pitch[20];
    while (1)
    {

        if (flag_toggle_sd)
        {
            flag_toggle_sd = false;

            if (!sd_montado)
            {
                if (sd_mount())
                {
                    sd_montado = true;
                    printf("SD montado com sucesso.\n");
                    set_rgb(0, 1, 0); // Verde: pronto
                }
                else
                {
                    printf("Falha ao montar o SD.\n");
                    piscar_rgb(1, 0, 1, 200); // Roxo piscando
                }
            }
            else
            {
                if (gravando)
                {
                    gravando = false;
                    set_rgb(1, 0, 0);
                    printf("Parando gravacao antes de desmontar...\n");
                }
                sd_unmount();
                sd_montado = false;
                printf("SD desmontado.\n");
            }
        }

        if (flag_toggle_gravacao)
        {
            flag_toggle_gravacao = false;

            if (sd_montado)
            {
                gravando = !gravando;

                if (gravando)
                {
                    buzzer_pwm(BUZZER_A, BUZZER_FREQUENCY * 3, 100); // Ativa o Buzzer A por 100ms
                    contador_amostras = 0;
                    sd_append_line("log.csv", "numero_amostra,accel_x,accel_y,accel_z,giro_x,giro_y,giro_z");

                    printf("Gravacao iniciada.\n");
                    set_rgb(1, 0, 0); // Vermelho: gravando
                }
                else
                {
                    buzzer_pwm(BUZZER_A, BUZZER_FREQUENCY * 3, 100); // Ativa o Buzzer A por 100ms
                    sleep_ms(50);                                    // Aguarda 50ms
                    buzzer_pwm(BUZZER_A, BUZZER_FREQUENCY * 3, 100); // Ativa o Buzzer A por 100ms
                    sleep_ms(50);                                    // Aguarda 50ms
                    ssd1306_fill(&ssd, false);
                    printf("Gravacao finalizada.\n");
                    ssd1306_draw_string(&ssd, "Dados Salvos!", 0, 0);
                    ssd1306_send_data(&ssd);
                    set_rgb(0, 1, 0); // Verde: SD montado, pronto
                    sleep_ms(500);
                }
            }
            else
            {
                printf("SD nao montado! Use botao A para montar.\n");
            }
        }

        if (flag_modo_exibicao)
        {
            flag_modo_exibicao = false;
            modo_exibicao = (modo_exibicao + 1) % 4;
        }

        imu_read(&imu); // Lê os dados do IMU

        // float gx = imu.gyro[0];
        // float gy = imu.gyro[1];
        // float gz = imu.gyro[2];

        float gx = imu.gyro[0] / 131.0f;
        float gy = imu.gyro[1] / 131.0f;
        float gz = imu.gyro[2] / 131.0f;

        float ax = imu.accel[0];
        float ay = imu.accel[1];
        float az = imu.accel[2];

        float roll = atan2(ay, az) * 180.0f / M_PI;
        float pitch = atan2(-ax, sqrt(ay * ay + az * az)) * 180.0f / M_PI;

        // EXIBE os dados no display
        ssd1306_fill(&ssd, false);

        switch (modo_exibicao)
        {
        case 0: // Menu SD

            ssd1306_draw_string(&ssd, "MENU SD", 5, 0);
            if (sd_montado)
            {
                ssd1306_draw_string(&ssd, "[A] Desmontar", 5, 30);
            }
            else
            {
                ssd1306_draw_string(&ssd, "[A] Montar", 5, 30);
            }
            if (gravando)
            {
                ssd1306_draw_string(&ssd, "[B] Parar", 5, 40);
                char str_count[20];
                snprintf(str_count, sizeof(str_count), "Amostras: %d", contador_amostras);
                ssd1306_draw_string(&ssd, str_count, 0, 8);
            }
            else
            {
                ssd1306_draw_string(&ssd, "[B] Gravar", 5, 40);
            }
            if (sd_montado)
            {
                if (gravando)
                {
                    ssd1306_draw_string(&ssd, "Gravando", 32, 50);
                }
                else
                {
                    ssd1306_draw_string(&ssd, "Montado", 32, 50);
                }
            }
            else
            {
                ssd1306_draw_string(&ssd, "Desmontado", 32, 50);
                set_rgb(1, 0, 0); // Verde: SD desmontado
            }
            // if (modo_exibicao == 0)
            //     ssd1306_draw_string(&ssd, "Roll/Pitch", 60, 50);
            // else if (modo_exibicao == 1)
            //     ssd1306_draw_string(&ssd, "Gyro", 60, 50);
            // else if (modo_exibicao == 2)
            //     ssd1306_draw_string(&ssd, "Accel", 60, 50);
            break;
        case 1: // Roll/Pitch

            snprintf(str_roll, sizeof(str_roll), "%5.1f", roll);
            snprintf(str_pitch, sizeof(str_pitch), "%5.1f", pitch);
            ssd1306_draw_string(&ssd, "ROLL", 5, 10);
            ssd1306_draw_string(&ssd, str_roll, 60, 10);
            ssd1306_draw_string(&ssd, "PITCH", 5, 30);
            ssd1306_draw_string(&ssd, str_pitch, 60, 30);
            break;

        case 2: // Giroscopio

            draw_gyro_graph(&ssd, gx, gy, gz);
            break;

        case 3: // Acelerometro

            draw_accel_graph(&ssd, ax, ay, az);
            break;
        }

        ssd1306_send_data(&ssd);

        printf("Gyro: [%.2f, %.2f, %.2f] | Accel: [%.2f, %.2f, %.2f] | Roll: %.2f | Pitch: %.2f | Temp: %.2f\n",
               gx, gy, gz, ax, ay, az, roll, pitch, imu.temp);

        if (gravando)
        {
            char linha[128];
            snprintf(linha, sizeof(linha),
                     "%d,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f",
                     contador_amostras++,
                     ax, ay, az,
                     gx, gy, gz);

            piscar_rgb(0, 0, 1, 50); // Azul piscando: acesso SD

            if (!sd_append_line("log.csv", linha))
            {
                printf("Erro ao gravar linha no SD! Parando gravacao.\n");
                gravando = false;
                set_rgb(1, 0, 1); // Roxo: erro
            }
        }

        ssd1306_send_data(&ssd);
        sleep_ms(50);
    }
}

void draw_gyro_graph(ssd1306_t *ssd, float gx, float gy, float gz)
{
    const float scale = (float)BAR_MAX_WIDTH / 250.0f; // Max ±250°/s
    const int y_gx = 10;
    const int y_gy = 25;
    const int y_gz = 40;

    // Títulos
    ssd1306_draw_string(ssd, "GYRO X", 0, y_gx - 8);
    ssd1306_draw_string(ssd, "GYRO Y", 0, y_gy - 8);
    ssd1306_draw_string(ssd, "GYRO Z", 0, y_gz - 8);

    // Barra GX
    int length_x = (int)(gx * scale);
    ssd1306_line(ssd, BAR_CENTER_X, y_gx, BAR_CENTER_X + length_x, y_gx, true);

    // Barra GY
    int length_y = (int)(gy * scale);
    ssd1306_line(ssd, BAR_CENTER_X, y_gy, BAR_CENTER_X + length_y, y_gy, true);

    // Barra GZ
    int length_z = (int)(gz * scale);
    ssd1306_line(ssd, BAR_CENTER_X, y_gz, BAR_CENTER_X + length_z, y_gz, true);
}

void draw_accel_graph(ssd1306_t *ssd, float ax, float ay, float az)
{
    const float scale = (float)BAR_MAX_WIDTH / ACCEL_MAX_G;
    const int y_ax = 10;
    const int y_ay = 25;
    const int y_az = 40;

    // Títulos
    ssd1306_draw_string(ssd, "ACC X", 0, y_ax - 8);
    ssd1306_draw_string(ssd, "ACC Y", 0, y_ay - 8);
    ssd1306_draw_string(ssd, "ACC Z", 0, y_az - 8);

    // Barra AX
    int length_x = (int)(ax * scale);
    ssd1306_line(ssd, BAR_CENTER_X, y_ax, BAR_CENTER_X + length_x, y_ax, true);

    // Barra AY
    int length_y = (int)(ay * scale);
    ssd1306_line(ssd, BAR_CENTER_X, y_ay, BAR_CENTER_X + length_y, y_ay, true);

    // Barra AZ
    int length_z = (int)(az * scale);
    ssd1306_line(ssd, BAR_CENTER_X, y_az, BAR_CENTER_X + length_z, y_az, true);
}

void set_rgb(bool r, bool g, bool b)
{
    gpio_put(LED_RED, r);
    gpio_put(LED_GREEN, g);
    gpio_put(LED_BLUE, b);
}

void piscar_rgb(bool r, bool g, bool b, int dur_ms)
{
    set_rgb(r, g, b);
    sleep_ms(dur_ms);
    set_rgb(0, 0, 0);
    sleep_ms(dur_ms);
}
