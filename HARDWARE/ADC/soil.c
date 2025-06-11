#include "soil.h"
#include "delay.h"
#include "adc.h"
#include "stm32f10x_adc.h"

#define SOIL_READ_TIMES    10             // 土壤湿度传感器采样次数
#define SOIL_ADC_CHX       ADC_Channel_1  // 土壤湿度传感器接在 ADC 通道 1 (PA1)

/**
 * @brief  初始化土壤湿度传感器 ADC 输入引脚 (PA1)
 * @note   包括 GPIOA 时钟开启与 ADC2 初始化
 */
void Adc_Init_Soil(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);  // 使能 GPIOA 时钟

    GPIO_InitStructure.GPIO_Pin  = GPIO_Pin_1;             // 配置 PA1
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;          // 设置为模拟输入模式
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    Adc2_Init();  // 初始化 ADC2
}

/**
 * @brief  获取指定通道 ADC 多次采样平均值
 * @param  ch: ADC 通道
 * @param  times: 采样次数
 * @retval 采样平均值
 */
u16 Get_Adc_Average_Soil(u8 ch, u8 times)
{
    u32 temp_val = 0;
    u8 t;

    for (t = 0; t < times; t++)
    {
        temp_val += Get_Adc2(ch);
        delay_ms(5);  // 采样间隔
    }

    return temp_val / times;
}

/**
 * @brief  获取土壤湿度值（0~100%）
 * @note   对 PA1 进行多次 ADC 采样并映射为湿度百分比
 * @retval 土壤湿度 (0~100)
 */
u8 Soil_Get_Val(void)
{
    u32 temp_val = 0;
    u8 t;

    for (t = 0; t < SOIL_READ_TIMES; t++)
    {
        temp_val += Get_Adc2(SOIL_ADC_CHX);  // 读取 ADC 值
        delay_ms(5);                        // 采样间隔
    }

    temp_val /= SOIL_READ_TIMES;             // 求平均

    if (temp_val > 4000)
        temp_val = 4000;                     // 限制最大值，避免溢出

    return (u8)(100 - (temp_val / 40));       // 映射为 0~100%
}
