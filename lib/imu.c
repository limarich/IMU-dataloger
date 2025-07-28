#include "imu.h"
#include "hardware/i2c.h"
#include "pico/stdlib.h"
#include <math.h>

static void mpu6050_reset()
{
    uint8_t buf[] = {0x6B, 0x80}; // Reset
    i2c_write_blocking(I2C_PORT, MPU6050_ADDR, buf, 2, false);
    sleep_ms(100);
    buf[1] = 0x00; // Wake up
    i2c_write_blocking(I2C_PORT, MPU6050_ADDR, buf, 2, false);
    sleep_ms(10);
}

void imu_init()
{
    i2c_init(I2C_PORT, 400 * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);
    mpu6050_reset();
}

void imu_read(imu_data_t *data)
{
    uint8_t buffer[14];
    uint8_t reg = 0x3B;

    i2c_write_blocking(I2C_PORT, MPU6050_ADDR, &reg, 1, true);
    i2c_read_blocking(I2C_PORT, MPU6050_ADDR, buffer, 14, false);

    // Acelerômetro
    for (int i = 0; i < 3; i++)
        data->accel[i] = ((int16_t)(buffer[i * 2] << 8 | buffer[i * 2 + 1])) / 16384.0f;

    // Temperatura
    int16_t raw_temp = (buffer[6] << 8) | buffer[7];
    data->temp = (raw_temp / 340.0f) + 15.0f;

    // Giroscópio
    for (int i = 0; i < 3; i++)
        data->gyro[i] = (float)((int16_t)(buffer[8 + i * 2] << 8 | buffer[9 + i * 2]));
}
