#include <stdio.h>
#include "light.h"
#include "delay.h"
#include "adc.h"
#include "stm32f10x_adc.h"

// 光敏电阻光照越强，电阻越小，电压越大
#define LSENS_READ_TIMES    10             // 光敏传感器连续采样次数
#define LSENS_ADC_CHX       ADC_Channel_4  // 光敏传感器接在 ADC 通道 4 (PA4)

/**
 * @brief  初始化光敏传感器 ADC 输入引脚 (PA4)
 * @note   包括 GPIOA 时钟开启与 ADC1 初始化
 */
void Adc_Init_Light(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);  // 使能 GPIOA 时钟

    GPIO_InitStructure.GPIO_Pin  = GPIO_Pin_4;             // 配置 PA4
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;          // 设置为模拟输入模式
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    Adc1_Init();  // 初始化 ADC1
}

/**
 * @brief  获取指定通道 ADC 多次采样平均值
 * @param  ch: ADC 通道
 * @param  times: 采样次数
 * @retval 采样平均值
 */
u16 Get_Adc_Average_Light(u8 ch, u8 times)
{
    u32 temp_val = 0;
    u8 t;

    for (t = 0; t < times; t++)
    {
        temp_val += Get_Adc1(ch);
        delay_ms(5);  // 采样间隔
    }

    return temp_val / times;
}

/**
 * @brief  获取光敏传感器亮度值（0~100%）
 * @note   对 PA4 进行多次 ADC 采样并映射为亮度百分比
 * @retval 光照强度 (0~100)
 */
u8 Lsens_Get_Val(void)
{
    u32 temp_val = 0;

//    for (t = 0; t < LSENS_READ_TIMES; t++)
//    {
//        temp_val += Get_Adc1(LSENS_ADC_CHX);  // 读取 ADC 值
//        delay_ms(5);                         // 采样间隔
//    }

//    temp_val /= LSENS_READ_TIMES;             // 求平均
	
	temp_val = Get_Adc_Average_Light(LSENS_ADC_CHX, LSENS_READ_TIMES);

	// ADC 最大值4095
    if (temp_val > 4095)
        temp_val = 4095;                      // 限制最大值，避免溢出

    return (u8)(100 - (temp_val / 40));        // 映射为 0~100%
}

/**
 * @brief  将采集的光照传感器ADC值，转换为Lux值
 * @note   根据ADC分段拟合计算，反推光敏电阻值，返回大致光照值
 * @retval 光照度 (单位：Lux)
 */
//typedef struct {
//    float r_min;
//    float r_max;
//    u16 lux_start;
//    u16 lux_end;
//} LuxSegment;

//// 阻值与光照度对应表
//static const LuxSegment lux_table[] = {
//    {0,    130, 30000, 17000},
//    {130,  160, 17000, 14000},
//    {160,  200, 14000, 8800},
//    {200,  260, 8800,  3500},
//    {260,  290, 3500,  2900},
//    {290,  560, 2900,  680},
//    {560,  600, 680,   520},
//    {600,  800, 520,   310},
//    {800,  900, 310,   240},
//    {900, 1250, 240,   125},
//    {1250,1300, 125,   10}
//};

//u16 Lsens_Get_Lux(void)
//{
//    float R_photo;
//	u8 i = 0;
//    u16 lux = 0;
//    u16 adc_value = Get_Adc_Average_Light(LSENS_ADC_CHX, LSENS_READ_TIMES);

//    if (adc_value >= 4095)  // 防止除0
//        adc_value = 4094;
//    if (adc_value <= 5)     // 防止分母过大，R_photo 异常
//        return 0;

//    // 电阻值计算（电阻分压公式）
//    R_photo = 10000.0f * adc_value / (4095.0f - adc_value);

//    // 遍历查找区间
//    for (i = 0; i < sizeof(lux_table) / sizeof(lux_table[0]); i++) {
//        if (R_photo >= lux_table[i].r_min && R_photo < lux_table[i].r_max) {
//            float ratio = (R_photo - lux_table[i].r_min) / 
//                          (lux_table[i].r_max - lux_table[i].r_min);

//            // 判断是否是递增区间
//            if (lux_table[i].lux_end > lux_table[i].lux_start) {
//                lux = lux_table[i].lux_start + 
//                      (u16)((lux_table[i].lux_end - lux_table[i].lux_start) * ratio);
//            } else {
//                lux = lux_table[i].lux_start - 
//                      (u16)((lux_table[i].lux_start - lux_table[i].lux_end) * ratio);
//            }
//            return lux;
//        }
//    }

//    return 0; // 超出所有区间，返回0
//}

/**
 * @brief  将采集的光照传感器ADC值，转换为Lux值
 * @note   直接对ADC值做拟合，而不是反推计算电阻值
 * @retval 光照度 (单位：Lux)
 */
// ADC 和 Lux 对应表（分段线性）
typedef struct {
    uint16_t adc;
    uint16_t lux;
} AdcLuxMap;

// 从高光（小ADC）到低光（大ADC）的对应表
static const AdcLuxMap adc_lux_table[] = {
	{0,    30000},    // 100
	{10,   28000},    // 99
	{20,   25000},    // 99
	{40,   20000},    // 99
	{60,   12000},    // 98
	{80,   7000},     // 98
	{100,  4000},     // 97
    {130,  2500},     // 96
    {160,  1500},     // 95
    {200,  600},      // 94
    {300,  520},      // 91
    {500,  450},      // 87
    {1000, 100},      // 75
    {1500, 70},
    {2000, 40},       // 50
    {2500, 30},
    {3000, 25},       // 25
    {3500, 20},
    {3800, 10},
    {4000, 5},
    {4095, 0}         // 0
};

// 主函数：ADC 转换为 Lux（分段线性拟合）
uint16_t Lsens_Get_Lux(void)
{
	int i = 0;
    uint16_t adc_val = Get_Adc_Average_Light(LSENS_ADC_CHX, LSENS_READ_TIMES);
    const int table_size = sizeof(adc_lux_table) / sizeof(adc_lux_table[0]);

    // 边界条件：小于最小ADC值或大于最大ADC值
    if (adc_val <= adc_lux_table[0].adc)
        return adc_lux_table[0].lux;
    if (adc_val >= adc_lux_table[table_size - 1].adc)
        return adc_lux_table[table_size - 1].lux;

    // 在表中查找对应区间，进行线性插值
    for (i = 0; i < table_size - 1; i++) {
        uint16_t adc1 = adc_lux_table[i].adc;
        uint16_t lux1 = adc_lux_table[i].lux;
        uint16_t adc2 = adc_lux_table[i + 1].adc;
        uint16_t lux2 = adc_lux_table[i + 1].lux;

        if (adc_val >= adc1 && adc_val <= adc2) {
            // 线性插值
            uint32_t lux = lux1 + ((int32_t)(lux2 - lux1) * (adc_val - adc1)) / (adc2 - adc1);
            return (uint16_t)lux;
        }
    }

    // 正常情况下不会执行到这里
    return 0;
}
