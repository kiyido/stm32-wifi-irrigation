#include "soil.h"
#include "delay.h"
#include "adc.h"
#include "stm32f10x_adc.h"

#define SOIL_READ_TIMES    10             // ����ʪ�ȴ�������������
#define SOIL_ADC_CHX       ADC_Channel_1  // ����ʪ�ȴ��������� ADC ͨ�� 1 (PA1)

/**
 * @brief  ��ʼ������ʪ�ȴ����� ADC �������� (PA1)
 * @note   ���� GPIOA ʱ�ӿ����� ADC2 ��ʼ��
 */
void Adc_Init_Soil(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);  // ʹ�� GPIOA ʱ��

    GPIO_InitStructure.GPIO_Pin  = GPIO_Pin_1;             // ���� PA1
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;          // ����Ϊģ������ģʽ
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    Adc2_Init();  // ��ʼ�� ADC2
}

/**
 * @brief  ��ȡָ��ͨ�� ADC ��β���ƽ��ֵ
 * @param  ch: ADC ͨ��
 * @param  times: ��������
 * @retval ����ƽ��ֵ
 */
u16 Get_Adc_Average_Soil(u8 ch, u8 times)
{
    u32 temp_val = 0;
    u8 t;

    for (t = 0; t < times; t++)
    {
        temp_val += Get_Adc2(ch);
        delay_ms(5);  // �������
    }

    return temp_val / times;
}

/**
 * @brief  ��ȡ����ʪ��ֵ��0~100%��
 * @note   �� PA1 ���ж�� ADC ������ӳ��Ϊʪ�Ȱٷֱ�
 * @retval ����ʪ�� (0~100)
 */
u8 Soil_Get_Val(void)
{
    u32 temp_val = 0;
    u8 t;

    for (t = 0; t < SOIL_READ_TIMES; t++)
    {
        temp_val += Get_Adc2(SOIL_ADC_CHX);  // ��ȡ ADC ֵ
        delay_ms(5);                        // �������
    }

    temp_val /= SOIL_READ_TIMES;             // ��ƽ��

    if (temp_val > 4000)
        temp_val = 4000;                     // �������ֵ���������

    return (u8)(100 - (temp_val / 40));       // ӳ��Ϊ 0~100%
}
