#ifndef IMU_H
#define IMU_H

#include <stdint.h>

#define I2C_PORT i2c0
#define I2C_SDA 0
#define I2C_SCL 1
#define MPU6050_ADDR 0x68

typedef struct
{
    float accel[3]; // [ax, ay, az] em 'g'
    float gyro[3];  // [gx, gy, gz] em LSB
    float temp;     // em graus Celsius
} imu_data_t;

void imu_init();
void imu_read(imu_data_t *data);

#endif // IMU_H
