#include "mpu6050.hpp"
#include "bsp_delay.h"
#include <cmath>
static float yaw = 0.0f;
float gyro_z_offset = 0.0f;

// I2C通信相关的静态函数定义
// 设置 SCL 引脚电平，并添加延时
static void IIC_SCL_(uint8_t state) {
    if (state)
        DL_GPIO_setPins(SOFT_IIC_SCL_PORT, SOFT_IIC_SCL_PIN);
    else
        DL_GPIO_clearPins(SOFT_IIC_SCL_PORT, SOFT_IIC_SCL_PIN);
    BspDelay_us(5);
}

// 设置 SDA 引脚电平，并添加延时
static void IIC_SDA_(uint8_t state) {
    DL_GPIO_initDigitalOutput(MPU6050_sda_IOMUX);
    if (state)
        DL_GPIO_setPins(SOFT_IIC_SDA_PORT, SOFT_IIC_SDA_PIN);
    else
        DL_GPIO_clearPins(SOFT_IIC_SDA_PORT, SOFT_IIC_SDA_PIN);
    BspDelay_us(5);
}

// 读取 SDA 引脚电平
static uint8_t IIC_SDA_Read(void) {
    DL_GPIO_initDigitalInputFeatures(MPU6050_SDA_input, DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_NONE,
                                     DL_GPIO_HYSTERESIS_DISABLE, DL_GPIO_WAKEUP_DISABLE);
    return DL_GPIO_readPins(SOFT_IIC_SDA_PORT, SOFT_IIC_SDA_PIN);
}

// I2C 初始化
static void IIC_Init(void) {
    // 初始状态：SCL 和 SDA 都为高电平
    IIC_SCL_(1);
    IIC_SDA_(1);
}

// 发送起始信号
static void IIC_Start(void) {
    IIC_SDA_(1);
    IIC_SCL_(1);
    IIC_SDA_(0);
    IIC_SCL_(0);
}

// 发送停止信号
static void IIC_Stop(void) {
    IIC_SCL_(0);
    IIC_SDA_(0);
    IIC_SCL_(1);
    IIC_SDA_(1);
}

// 发送一个字节
static void IIC_SendByte(uint8_t data) {
    uint8_t i;
    for (i = 0; i < 8; i++) {
        IIC_SDA_((data & 0x80) >> 7);
        data <<= 1;
        IIC_SCL_(1);
        IIC_SCL_(0);
    }
}

// 接收一个字节
static uint8_t IIC_ReceiveByte(uint8_t ack) {
    uint8_t i, data = 0;
    IIC_SDA_(1); // 释放 SDA 线
    for (i = 0; i < 8; i++) {
        IIC_SCL_(1);
        data <<= 1;
        if (IIC_SDA_Read())
            data |= 0x01;
        IIC_SCL_(0);
    }
    if (ack)
        IIC_SDA_(0); // 发送应答
    else
        IIC_SDA_(1); // 发送非应答
    IIC_SCL_(1);
    IIC_SCL_(0);
    return data;
}

// 等待应答信号
static uint8_t IIC_WaitAck(void) {
    uint8_t ack;
    IIC_SDA_(1);
    IIC_SCL_(1);
    ack = IIC_SDA_Read();
    IIC_SCL_(0);
    return ack;
}

// 向 MPU6050 写入一个字节数据
static void IIC_Write_REG(uint8_t addr, uint8_t reg, uint8_t data) {
    IIC_Start();
    IIC_SendByte((addr << 1) | 0); // 发送写地址
    IIC_WaitAck();
    IIC_SendByte(reg); // 发送寄存器地址
    IIC_WaitAck();
    IIC_SendByte(data); // 发送数据
    IIC_WaitAck();
    IIC_Stop();
}

// 从 MPU6050 读取一个字节数据
static uint8_t IIC_Read_REG(uint8_t Address, uint8_t regaddress) {
    uint8_t data;
    IIC_Start();                    // 发送起始信号
    IIC_SendByte(Address << 1 | 0); // 发送设备地址和写操作
    IIC_WaitAck();                  // 等待 ACK
    IIC_SendByte(regaddress);       // 发送寄存器地址
    IIC_WaitAck();                  // 等待 ACK
    IIC_Start();                    // 发送起始信号
    IIC_SendByte(Address << 1 | 1); // 发送设备地址和读操作
    IIC_WaitAck();                  // 等待 ACK
    data = IIC_ReceiveByte(0);      // 读取数据
    IIC_Stop();                     // 发送停止信号
    return data;                    // 返回读取的数据
}

// 【新增】从 MPU6050 连续读取多个字节数据
static void IIC_Read_Multi_REG(uint8_t addr, uint8_t reg_start, uint8_t *buf, uint8_t len) {
    IIC_Start();                           // 1. 发送起始信号
    IIC_SendByte((addr << 1) | 0);        // 2. 发送设备地址 + 写操作
    IIC_WaitAck();                         // 3. 等待 ACK
    IIC_SendByte(reg_start);               // 4. 发送起始寄存器地址
    IIC_WaitAck();                         // 5. 等待 ACK
    
    IIC_Start();                           // 6. 重新发送起始信号（切换到读模式）
    IIC_SendByte((addr << 1) | 1);        // 7. 发送设备地址 + 读操作
    IIC_WaitAck();                         // 8. 等待 ACK
    
    // 9. 循环读取 len 个字节
    for (uint8_t i = 0; i < len; i++) {
        // 关键：前 len-1 个字节发送 ACK（让传感器继续发下一个）
        // 最后一个字节发送 NACK（告诉传感器发完了）
        if (i < len - 1) {
            buf[i] = IIC_ReceiveByte(1); // 发送 ACK
        } else {
            buf[i] = IIC_ReceiveByte(0); // 发送 NACK
        }
    }
    
    IIC_Stop();                            // 10. 发送停止信号
}

// MPU6050 初始化函数
void MPU6050_Init(void) {
    IIC_Init(); // 初始化 I2C 总线
    // 唤醒 MPU6050
    IIC_Write_REG(MPU6050_ADDR_AD0_LOW, PWR_MGMT_1, 0x00);
    BspDelay_ms(100); // 等待唤醒

    // 设置采样率分频
    IIC_Write_REG(MPU6050_ADDR_AD0_LOW, SMPLRT_DIV, 0X01);
    // 设置低通滤波器
    IIC_Write_REG(MPU6050_ADDR_AD0_LOW, CONFIG, 0x06);
    // 设置陀螺仪量程 ±2000°/s
    IIC_Write_REG(MPU6050_ADDR_AD0_LOW, GYRO_CONFIG, 0X00);
    // 设置加速度计量程 ±2g
    IIC_Write_REG(MPU6050_ADDR_AD0_LOW, ACCEL_CONFIG, 0x00);
    BspDelay_ms(100); // 等待唤醒
}

// 读取 MPU6050 的设备 ID
uint8_t MPU6050_GetDeviceID(void) {
    uint8_t data;
    data = IIC_Read_REG(MPU6050_ADDR_AD0_LOW, WHO_AM_I); // 读取设备 ID 寄存器
    return data;                                         // 返回设备 ID
}

float MPU6050_GET_Tempure(void) {
    int16_t temp;                                       // 用于存储温度传感器数据
    uint8_t H, L;                                       // 用于存储高字节和低字节的数据
    H = IIC_Read_REG(MPU6050_ADDR_AD0_LOW, TEMP_OUT_H); // 读取温度传感器高字节
    L = IIC_Read_REG(MPU6050_ADDR_AD0_LOW, TEMP_OUT_L); // 读取温度传感器低字节
    temp = (H << 8) | L;                                // 将高字节和低字节合并为一个 16 位数据
    return (float) temp;                                // 计算温度值并返回
}

float MPU6050_GetAccelX(void) {
    int16_t accel;                                        // 用于存储加速度传感器数据
    uint8_t H, L;                                         // 用于存储高字节和低字节的数据
    H = IIC_Read_REG(MPU6050_ADDR_AD0_LOW, ACCEL_XOUT_H); // 读取加速度传感器 X 轴高字节
    L = IIC_Read_REG(MPU6050_ADDR_AD0_LOW, ACCEL_XOUT_L); // 读取加速度传感器 X 轴低字节
    accel = (H << 8) | L;                                 // 将高字节和低字节合并为一个 16 位数据
    return (float) accel * 6.1035e-5f;                    // 直接返回角度
}

float MPU6050_GetAccelY(void) {
    int16_t accel;                                        // 用于存储加速度传感器数据
    uint8_t H, L;                                         // 用于存储高字节和低字节的数据
    H = IIC_Read_REG(MPU6050_ADDR_AD0_LOW, ACCEL_YOUT_H); // 读取加速度传感器 Y 轴高字节
    L = IIC_Read_REG(MPU6050_ADDR_AD0_LOW, ACCEL_YOUT_L); // 读取加速度传感器 Y 轴低字节
    accel = (H << 8) | L;                                 // 将高字节和低字节合并为一个 16 位数据
    return (float) accel * 6.1035e-5f;                    // 直接返回角度
}

float MPU6050_GetAccelZ(void) {
    int16_t accel;                                        // 用于存储加速度传感器数据
    uint8_t H, L;                                         // 用于存储高字节和低字节的数据
    H = IIC_Read_REG(MPU6050_ADDR_AD0_LOW, ACCEL_ZOUT_H); // 读取加速度传感器 Z 轴高字节
    L = IIC_Read_REG(MPU6050_ADDR_AD0_LOW, ACCEL_ZOUT_L); // 读取加速度传感器 Z 轴低字节
    accel = (H << 8) | L;                                 // 将高字节和低字节合并为一个 16 位数据
    return (float) accel * 6.1035e-5f;                    // 直接返回角度
}

float MPU6050_GetGroX(void) {
    int16_t gyro;                                        // 用于存储陀螺仪数据
    uint8_t H, L;                                        // 用于存储高字节和低字节的数据
    H = IIC_Read_REG(MPU6050_ADDR_AD0_LOW, GYRO_XOUT_H); // 读取陀螺仪 X 轴高字节
    L = IIC_Read_REG(MPU6050_ADDR_AD0_LOW, GYRO_XOUT_L); // 读取陀螺仪 X 轴低字节
    gyro = (H << 8) | L;                                 // 将高字节和低字节合并为一个 16 位数据
    return (float) gyro * 6.1035e-2f;                    // 计算角速度值并返回
}

float MPU6050_GetGroY(void) {
    int16_t gyro;                                        // 用于存储陀螺仪数据
    uint8_t H, L;                                        // 用于存储高字节和低字节的数据
    H = IIC_Read_REG(MPU6050_ADDR_AD0_LOW, GYRO_YOUT_H); // 读取陀螺仪 Y 轴高字节
    L = IIC_Read_REG(MPU6050_ADDR_AD0_LOW, GYRO_YOUT_L); // 读取陀螺仪 Y 轴低字节
    gyro = (H << 8) | L;                                 // 将高字节和低字节合并为一个 16 位数据
    return (float) gyro * 6.1035e-2f;                    // 计算角速度值并返回
}

// float MPU6050_GetGroZ(void) {
//     int16_t gyro;                                        // 用于存储陀螺仪数据
//     uint8_t H, L;                                        // 用于存储高字节和低字节的数据
//     H = IIC_Read_REG(MPU6050_ADDR_AD0_LOW, GYRO_ZOUT_H); // 读取陀螺仪 Z 轴高字节
//     L = IIC_Read_REG(MPU6050_ADDR_AD0_LOW, GYRO_ZOUT_L); // 读取陀螺仪 Z 轴低字节
//     gyro = (H << 8) | L;                                 // 将高字节和低字节合并为一个 16 位数据
//     return (float) (gyro-63)/131.0f * 12.0f ;                    // 计算角速度值并返回
// }

float MPU6050Gyro_calibrate(void) {
    int32_t sum = 0;

    for(int i=0; i<1000; i++) {
        sum += MPU6050_GetGroZ();
    }
    return gyro_z_offset = sum / 1000.0f; // 零偏值
}

float MPU6050_GetGroZ(void) {
    int16_t gyro;
    uint8_t buf[2]; // 用于存储连续读取的 H 和 L
    
    // 【关键修改】一次性连续读取 GYRO_ZOUT_H 和 GYRO_ZOUT_L
    // 注意：GYRO_ZOUT_H 是高字节地址，连续读会自动读 GYRO_ZOUT_H -> GYRO_ZOUT_L
    IIC_Read_Multi_REG(MPU6050_ADDR_AD0_LOW, GYRO_ZOUT_H, buf, 2);
    
    // 合并数据（buf[0] 是 H，buf[1] 是 L）
    gyro = (int16_t)((buf[0] << 8) | buf[1]);
 
    return (float)gyro;
}

float MPU6050_Getyaw(void) {
    // 1. 计算原始陀螺仪Z轴值
    float raw_gyro_z = MPU6050_GetGroZ() + gyro_z_offset;
    
    // 2. 四舍五入保留一位小数（你可以换成上面任意一种方法）
    float processed_gyro_z = (float)(int)std::round(raw_gyro_z * 10.0f) / 10.0f - 0.00001;
    
    // 3. 用处理后的值更新yaw
    yaw += 0.0002 * processed_gyro_z / 100000000.0f; 
    
    // （原函数的返回逻辑有问题，正负都返回-yaw，这里先保留原样）
    if(yaw < 0)
        return -30000000.0 * yaw; 
    else 
        return -1.0 * yaw;  
}
