#include "stm32f10x.h"
#include "dht11.h"
#include "delay.h"

/**
 * @brief  ��ʼ�� DHT11 ��������Ϊ�������ģʽ
 * @param  ��
 * @retval ��
 */
void DHT11_GPIO_Config(void)
{		
    GPIO_InitTypeDef GPIO_InitStructure;

    RCC_APB2PeriphClockCmd(DHT11_CLK, ENABLE);        // ʹ�� DHT11 ���ڶ˿�ʱ��

    GPIO_InitStructure.GPIO_Pin = DHT11_PIN;	
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;  // �������ģʽ
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz; 
    GPIO_Init(DHT11_PORT, &GPIO_InitStructure);		  

    GPIO_SetBits(DHT11_PORT, DHT11_PIN);              // Ĭ������������
}

/**
 * @brief  �� DHT11 ������������Ϊ��������ģʽ
 * @param  ��
 * @retval ��
 */
static void DHT11_Mode_IPU(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    GPIO_InitStructure.GPIO_Pin = DHT11_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;    // ��������
    GPIO_Init(DHT11_PORT, &GPIO_InitStructure);	 
}

/**
 * @brief  �� DHT11 ������������Ϊ�������ģʽ
 * @param  ��
 * @retval ��
 */
static void DHT11_Mode_Out_PP(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    GPIO_InitStructure.GPIO_Pin = DHT11_PIN;	
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;  // �������
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(DHT11_PORT, &GPIO_InitStructure);	 
}

/**
 * @brief  �� DHT11 ��ȡ 1 �ֽ����ݣ�MSB ���У�
 * @param  ��
 * @retval ��ȡ���� 1 �ֽ�����
 */
static uint8_t Read_Byte(void) 
{
    uint8_t i, temp = 0;

    for (i = 0; i < 8; i++)    
    {	   
        while (DHT11_DATA_IN() == Bit_RESET);       // �ȴ��͵�ƽ��������ʼ�źţ�
        delay_us(40);                               // ��ʱ 40us��������0���źŸߵ�ƽ���

        if (DHT11_DATA_IN() == Bit_SET)             // �����Ϊ�ߵ�ƽ����Ϊ����λ 1
        {
            while (DHT11_DATA_IN() == Bit_SET);     // �ȴ�����λ�ߵ�ƽ����
            temp |= (uint8_t)(0x01 << (7 - i));     // д�뵱ǰλ����λ���У�
        }
        else                                        // �����Ϊ�͵�ƽ����Ϊ����λ 0
        {
            temp &= (uint8_t)~(0x01 << (7 - i));    // ����Ϊ 0
        }
    }

    return temp;
}

/**
 * @brief  ��ȡһ�� DHT11 ���ݣ��ܹ� 40 λ��5 ���ֽڣ�
 * @param  DHT11_Data : ָ�� DHT11_Data_TypeDef �ṹ���ָ��
 * @retval SUCCESS��0������ȡ�ɹ�
 *         ERROR  ��1������ȡʧ��
 */
uint8_t Read_DHT11(DHT11_Data_TypeDef *DHT11_Data)
{
    DHT11_Mode_Out_PP();     // ����Ϊ�������ģʽ

    DHT11_DATA_OUT(LOW);     // ���������ź�
    delay_ms(18);            // ��ʱ���� 18ms��������ʼ�ź�

    DHT11_DATA_OUT(HIGH);    // ��������
    delay_us(30);            // ��ʱ 20~40us���ȴ� DHT11 ��Ӧ

    DHT11_Mode_IPU();        // ����Ϊ�������룬׼��������Ӧ

    if (DHT11_DATA_IN() == Bit_RESET)               // ��⵽ DHT11 ����Ӧ�͵�ƽ
    {
        while (DHT11_DATA_IN() == Bit_RESET);        // �ȴ���Ӧ�͵�ƽ����
        while (DHT11_DATA_IN() == Bit_SET);          // �ȴ�׼���źŸߵ�ƽ����

        /* ����40λ���� */
        DHT11_Data->humi_int  = Read_Byte();         // ʪ����������
        DHT11_Data->humi_deci = Read_Byte();         // ʪ��С������
        DHT11_Data->temp_int  = Read_Byte();         // �¶���������
        DHT11_Data->temp_deci = Read_Byte();         // �¶�С������
        DHT11_Data->check_sum = Read_Byte();         // У���

        DHT11_Mode_Out_PP();                         // ��ȡ��ϣ��л����ģʽ
        DHT11_DATA_OUT(HIGH);                        // �������ߣ�׼����һ��ͨ��

        /* У��������ȷ�� */
        if (DHT11_Data->check_sum == (DHT11_Data->humi_int + DHT11_Data->humi_deci + 
                                      DHT11_Data->temp_int + DHT11_Data->temp_deci))
        {
            return SUCCESS;   // У��ͨ��
        }
        else
        {
            return ERROR;     // У��ʧ��
        }
    }
    else
    {
        return ERROR;         // ����Ӧ
    }
}
