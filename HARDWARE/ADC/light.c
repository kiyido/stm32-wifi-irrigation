#include <stdio.h>
#include "light.h"
#include "delay.h"
#include "adc.h"
#include "stm32f10x_adc.h"

// �����������Խǿ������ԽС����ѹԽ��
#define LSENS_READ_TIMES    10             // ����������������������
#define LSENS_ADC_CHX       ADC_Channel_4  // �������������� ADC ͨ�� 4 (PA4)

/**
 * @brief  ��ʼ������������ ADC �������� (PA4)
 * @note   ���� GPIOA ʱ�ӿ����� ADC1 ��ʼ��
 */
void Adc_Init_Light(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);  // ʹ�� GPIOA ʱ��

    GPIO_InitStructure.GPIO_Pin  = GPIO_Pin_4;             // ���� PA4
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;          // ����Ϊģ������ģʽ
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    Adc1_Init();  // ��ʼ�� ADC1
}

/**
 * @brief  ��ȡָ��ͨ�� ADC ��β���ƽ��ֵ
 * @param  ch: ADC ͨ��
 * @param  times: ��������
 * @retval ����ƽ��ֵ
 */
u16 Get_Adc_Average_Light(u8 ch, u8 times)
{
    u32 temp_val = 0;
    u8 t;

    for (t = 0; t < times; t++)
    {
        temp_val += Get_Adc1(ch);
        delay_ms(5);  // �������
    }

    return temp_val / times;
}

/**
 * @brief  ��ȡ��������������ֵ��0~100%��
 * @note   �� PA4 ���ж�� ADC ������ӳ��Ϊ���Ȱٷֱ�
 * @retval ����ǿ�� (0~100)
 */
u8 Lsens_Get_Val(void)
{
    u32 temp_val = 0;

//    for (t = 0; t < LSENS_READ_TIMES; t++)
//    {
//        temp_val += Get_Adc1(LSENS_ADC_CHX);  // ��ȡ ADC ֵ
//        delay_ms(5);                         // �������
//    }

//    temp_val /= LSENS_READ_TIMES;             // ��ƽ��
	
	temp_val = Get_Adc_Average_Light(LSENS_ADC_CHX, LSENS_READ_TIMES);

	// ADC ���ֵ4095
    if (temp_val > 4095)
        temp_val = 4095;                      // �������ֵ���������

    return (u8)(100 - (temp_val / 40));        // ӳ��Ϊ 0~100%
}

/**
 * @brief  ���ɼ��Ĺ��մ�����ADCֵ��ת��ΪLuxֵ
 * @note   ����ADC�ֶ���ϼ��㣬���ƹ�������ֵ�����ش��¹���ֵ
 * @retval ���ն� (��λ��Lux)
 */
//typedef struct {
//    float r_min;
//    float r_max;
//    u16 lux_start;
//    u16 lux_end;
//} LuxSegment;

//// ��ֵ����նȶ�Ӧ��
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

//    if (adc_value >= 4095)  // ��ֹ��0
//        adc_value = 4094;
//    if (adc_value <= 5)     // ��ֹ��ĸ����R_photo �쳣
//        return 0;

//    // ����ֵ���㣨�����ѹ��ʽ��
//    R_photo = 10000.0f * adc_value / (4095.0f - adc_value);

//    // ������������
//    for (i = 0; i < sizeof(lux_table) / sizeof(lux_table[0]); i++) {
//        if (R_photo >= lux_table[i].r_min && R_photo < lux_table[i].r_max) {
//            float ratio = (R_photo - lux_table[i].r_min) / 
//                          (lux_table[i].r_max - lux_table[i].r_min);

//            // �ж��Ƿ��ǵ�������
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

//    return 0; // �����������䣬����0
//}

/**
 * @brief  ���ɼ��Ĺ��մ�����ADCֵ��ת��ΪLuxֵ
 * @note   ֱ�Ӷ�ADCֵ����ϣ������Ƿ��Ƽ������ֵ
 * @retval ���ն� (��λ��Lux)
 */
// ADC �� Lux ��Ӧ���ֶ����ԣ�
typedef struct {
    uint16_t adc;
    uint16_t lux;
} AdcLuxMap;

// �Ӹ߹⣨СADC�����͹⣨��ADC���Ķ�Ӧ��
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

// ��������ADC ת��Ϊ Lux���ֶ�������ϣ�
uint16_t Lsens_Get_Lux(void)
{
	int i = 0;
    uint16_t adc_val = Get_Adc_Average_Light(LSENS_ADC_CHX, LSENS_READ_TIMES);
    const int table_size = sizeof(adc_lux_table) / sizeof(adc_lux_table[0]);

    // �߽�������С����СADCֵ��������ADCֵ
    if (adc_val <= adc_lux_table[0].adc)
        return adc_lux_table[0].lux;
    if (adc_val >= adc_lux_table[table_size - 1].adc)
        return adc_lux_table[table_size - 1].lux;

    // �ڱ��в��Ҷ�Ӧ���䣬�������Բ�ֵ
    for (i = 0; i < table_size - 1; i++) {
        uint16_t adc1 = adc_lux_table[i].adc;
        uint16_t lux1 = adc_lux_table[i].lux;
        uint16_t adc2 = adc_lux_table[i + 1].adc;
        uint16_t lux2 = adc_lux_table[i + 1].lux;

        if (adc_val >= adc1 && adc_val <= adc2) {
            // ���Բ�ֵ
            uint32_t lux = lux1 + ((int32_t)(lux2 - lux1) * (adc_val - adc1)) / (adc2 - adc1);
            return (uint16_t)lux;
        }
    }

    // ��������²���ִ�е�����
    return 0;
}
