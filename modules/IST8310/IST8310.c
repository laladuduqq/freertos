#include "IST8310.h"

#include <string.h>

#include "i2c.h"
#include "stm32f4xx_hal_i2c.h"

#define IST8310_RESET_PIN GET_PIN(G,6)

ist8310_data_t ist8310_data;

#define MAG_SEN 0.3f //raw int16 data change to uT unit. 原始整型数据变成 单位ut
#define IST8310_I2C_ADDR          0x0E
#define IST8310_WHO_AM_I 0x00       //ist8310 "who am I " 
#define IST8310_WHO_AM_I_VALUE 0x10 //device ID
#define IST8310_WRITE_REG_NUM 4
#define IST8310_DATA_REG 0x03       // ist8310的数据寄存器
#define IST8310_WHO_AM_I 0x00       // ist8310 id 寄存器值


// the first column:the registers of IST8310. 第一列:IST8310的寄存器
// the second column: the value to be writed to the registers.第二列:需要写入的寄存器值
// the third column: return error value.第三列:返回的错误码
static uint8_t ist8310_write_reg_data_error[IST8310_WRITE_REG_NUM][3] = {
    {0x0B, 0x08, 0x01},  // enalbe interrupt  and low pin polarity.开启中断，并且设置低电平
    {0x41, 0x12, 0x02},  // average 4 times.平均采样四次
    {0x42, 0xC0, 0x03},  // must be 0xC0. 必须是0xC0
    {0x0A, 0x0B, 0x04}}; // 200Hz output rate.200Hz输出频率


/* 写传感器寄存器 */
static void rt_i2c_write_reg(uint16_t addr, uint16_t reg, uint8_t *data, uint16_t len)
{
    HAL_I2C_Mem_Write(&hi2c3,addr<< 1,reg,I2C_MEMADD_SIZE_8BIT,data,len,100);
}

static void rt_i2c_read_reg(uint16_t addr, uint16_t reg, uint8_t *data, uint16_t len)
{
    HAL_I2C_Mem_Read(&hi2c3,addr<< 1,reg,I2C_MEMADD_SIZE_8BIT,data,len,100);
}


void ReadIST8310Data() {
    uint8_t buf[6];
    int16_t temp_ist8310_data = 0;
    rt_i2c_read_reg(IST8310_I2C_ADDR,IST8310_DATA_REG,buf,6);
    temp_ist8310_data = (int16_t)((buf[1] << 8) | buf[0]);
    ist8310_data.raw_data.x = MAG_SEN * temp_ist8310_data;
    temp_ist8310_data = (int16_t)((buf[3] << 8) | buf[2]);
    ist8310_data.raw_data.y = MAG_SEN * temp_ist8310_data;
    temp_ist8310_data = (int16_t)((buf[5] << 8) | buf[4]);
    ist8310_data.raw_data.z= MAG_SEN * temp_ist8310_data;
}

ist8310_error_e VerifyMegId(uint8_t* id) {
    rt_i2c_read_reg(IST8310_I2C_ADDR,IST8310_WHO_AM_I,id,1);
    if (*id != IST8310_WHO_AM_I_VALUE) {
        return MEG_ID_ERROR;
    } else {
        return IST8310_NO_ERROR;
    }
}

void IST8310_INIT() {
    ist8310_data.ist8310_error = IST8310_NO_ERROR;
    // 把磁力计重启
    HAL_GPIO_WritePin(GPIOG,GPIO_PIN_6,GPIO_PIN_RESET);
    HAL_Delay(50);
    HAL_GPIO_WritePin(GPIOG,GPIO_PIN_6,GPIO_PIN_SET);
    HAL_Delay(50);

    // 基础配置
    uint8_t tmp=0;
    // 进行初始化配置写入并检查是否写入成功,这里用循环把最上面初始化数组的东西都写进去
    for (uint8_t i = 0; i < IST8310_WRITE_REG_NUM; i++)
    { 
        // 写入配置,写一句就读一下看看ist8310是否仍然在线
        rt_i2c_write_reg(IST8310_I2C_ADDR, ist8310_write_reg_data_error[i][0], &ist8310_write_reg_data_error[i][1], 1);
        HAL_Delay(1);
        rt_i2c_read_reg(IST8310_I2C_ADDR, ist8310_write_reg_data_error[i][0], &tmp, 1); // 读回自身id
        HAL_Delay(1);
        if (tmp != ist8310_write_reg_data_error[i][1])
        {
                ist8310_data.ist8310_error = ist8310_write_reg_data_error[i][2];
        }
    }
    ist8310_data.ist8310_error = VerifyMegId(&ist8310_data.chip_id);
}

void ist8310_init(void)
{
    ist8310_data.ist8310_error = ist8310_no_init;
    IST8310_INIT(&ist8310_data);

}


