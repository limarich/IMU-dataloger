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

#define BUTTON_A 5
#define BUTTON_B 6
#define BUTTON_JOYSTICK 22

#define I2C_PORT i2c0
#define I2C_SDA 0
#define I2C_SCL 1
#define I2C_PORT_DISP i2c1
#define I2C_SDA_DISP 14
#define I2C_SCL_DISP 15
#define DISPLAY_ADDRESS 0x3C
#define DISP_W 128
#define DISP_H 64

#define BAR_CENTER_X (DISP_W / 2)
#define BAR_MAX_WIDTH ((DISP_W / 2) - 10)

#define GYRO_SCALE 131.0f    // LSB por °/s
#define ACCEL_SCALE 16384.0f // LSB por g (já usado no imu_read)
#define ACCEL_MAX_G 2.0f     // Valor máximo esperado (±2g)

#define DEBOUNCE_TIME 200 // Tempo de debounce em milissegundos
static int addr = 0x68;

int modo_exibicao = 2; // 0: Roll/Pitch, 1: Gyro, 2: Acel
uint last_a_interrupt = 0;
uint last_b_interrupt = 0;
uint last_joystick_interrupt = 0;

void gpio_irq_handler(uint gpio, uint32_t events)
{
    uint current_time = to_ms_since_boot(get_absolute_time());
    if (gpio == BUTTON_A && (current_time - last_a_interrupt > DEBOUNCE_TIME))
    {
        last_a_interrupt = current_time;
        modo_exibicao = (modo_exibicao + 1) % 3; // Cicla entre 0, 1 e 2
    }
    else if (gpio == BUTTON_B && (current_time - last_b_interrupt > DEBOUNCE_TIME))
    {
        last_b_interrupt = current_time;
        modo_exibicao = (modo_exibicao - 1 + 3) % 3; // Cicla para trás
    }
    else if (gpio == BUTTON_JOYSTICK && (current_time - last_joystick_interrupt > DEBOUNCE_TIME))
    {
        last_joystick_interrupt = current_time;
        modo_exibicao++;
        if (modo_exibicao > 2)
        {
            modo_exibicao = 0; // Reseta para 0 se passar do limite
        }
    }
}

void draw_gyro_graph(ssd1306_t *ssd, float gx, float gy, float gz);
void draw_accel_graph(ssd1306_t *ssd, float ax, float ay, float az);
int main()
{
    stdio_init_all();

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

    imu_data_t imu;
    bool cor = true;

    char str_roll[20];
    char str_pitch[20];
    while (1)
    {

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
        case 0: // Roll/Pitch

            snprintf(str_roll, sizeof(str_roll), "%5.1f", roll);
            snprintf(str_pitch, sizeof(str_pitch), "%5.1f", pitch);
            ssd1306_draw_string(&ssd, "ROLL", 5, 10);
            ssd1306_draw_string(&ssd, str_roll, 60, 10);
            ssd1306_draw_string(&ssd, "PITCH", 5, 30);
            ssd1306_draw_string(&ssd, str_pitch, 60, 30);
            break;

        case 1: // Giroscopio

            draw_gyro_graph(&ssd, gx, gy, gz);
            break;

        case 2: // Acelerometro

            draw_accel_graph(&ssd, ax, ay, az);
            break;
        }

        ssd1306_send_data(&ssd);

        printf("Gyro: [%.2f, %.2f, %.2f] | Accel: [%.2f, %.2f, %.2f] | Roll: %.2f | Pitch: %.2f | Temp: %.2f\n",
               gx, gy, gz, ax, ay, az, roll, pitch, imu.temp);
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
