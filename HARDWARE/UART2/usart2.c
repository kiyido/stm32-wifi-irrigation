#include "delay.h"
#include "usart2.h"
#include "stdarg.h"
#include "stdio.h"
#include "string.h"

/* ���ͻ����� - 8�ֽڶ��� */
__align(8) u8 USART2_TX_BUF[USART2_MAX_SEND_LEN];

#ifdef USART2_RX_EN
/* ���ջ����� */
u8 USART2_RX_BUF[USART2_MAX_RECV_LEN];

/* ����״̬�Ĵ���
 * [15]: 0-δ���յ�����, 1-���յ�һ������
 * [14:0]: ���յ������ݳ���
 */
u16 USART2_RX_STA = 0;

/*
 * @brief USART2�жϷ�����
 * @note ��������жϣ�ʹ�ö�ʱ��4�ж�����֡����(10ms��ʱ)
 */
void USART2_IRQHandler(void)
{
    u8 res;
    
    // ����Ƿ���յ�����
    if(USART_GetITStatus(USART2, USART_IT_RXNE) != RESET)
    {
        res = USART_ReceiveData(USART2);  // ��ȡ���յ�������
        
        if(USART2_RX_STA < USART2_MAX_RECV_LEN)  // ������δ��
        {
            TIM_SetCounter(TIM4, 0);              // ���ö�ʱ��������
            if(USART2_RX_STA == 0)               // ����ǵ�һ���ֽ�
            {
                TIM4_Set(1);                     // ������ʱ��4
            }
            USART2_RX_BUF[USART2_RX_STA++] = res; // �洢���ݲ�����������
        }
        else  // ����������
        {
            USART2_RX_STA |= 1 << 15;            // ǿ�Ʊ�ǽ������
        }
    }
}
#endif

/*
 * @brief ��ʼ��USART2����
 * @param bound ������
 */
void USART2_Init(u32 bound)
{
    NVIC_InitTypeDef NVIC_InitStructure;
    GPIO_InitTypeDef GPIO_InitStructure;
    USART_InitTypeDef USART_InitStructure;

    // 1. ʱ��ʹ��
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);  // GPIOAʱ��
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE); // USART2ʱ��

    // 2. GPIO����
    USART_DeInit(USART2);  // ��λ����2
    
    // TX����(PA2)���ã������������
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    
    // RX����(PA3)���ã���������
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    // 3. USART��������
    USART_InitStructure.USART_BaudRate = bound;           // ������
    USART_InitStructure.USART_WordLength = USART_WordLength_8b; // 8λ����
    USART_InitStructure.USART_StopBits = USART_StopBits_1;     // 1λֹͣλ
    USART_InitStructure.USART_Parity = USART_Parity_No;        // ��У��
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None; // ������
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx; // �շ�ģʽ
    
    USART_Init(USART2, &USART_InitStructure);  // ��ʼ������2

    // 4. DMA��������
    USART_DMACmd(USART2, USART_DMAReq_Tx, ENABLE);  // ʹ�ܴ���2��DMA����
    UART_DMA_Config(DMA1_Channel7, (u32)&USART2->DR, (u32)USART2_TX_BUF); // ����DMA
    USART_Cmd(USART2, ENABLE);  // ʹ�ܴ���

#ifdef USART2_RX_EN
    // 5. �����ж�����
    USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);  // ʹ�ܽ����ж�
    
    NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2; // ��ռ���ȼ�2
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;       // �����ȼ�3
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
    
    // 6. ��ʱ��4��ʼ��(�����ж�����֡����)
    TIM4_Init(999, 7199);  // 10ms�ж�
    USART2_RX_STA = 0;     // �������״̬
    TIM4_Set(0);            // ��ʼ�رն�ʱ��4
#endif
}

/**
 * @brief ����2��ʽ���������
 * @param fmt ��ʽ���ַ���
 * @note ȷ��һ�η������ݲ�����USART2_MAX_SEND_LEN�ֽ�
 */
void u2_printf(char* fmt, ...)
{
    va_list ap;
    
    va_start(ap, fmt);
    vsprintf((char*)USART2_TX_BUF, fmt, ap);  // ��ʽ���ַ��������ͻ�����
    va_end(ap);
    
    // �ȴ���һ��DMA�������
    while(DMA_GetCurrDataCounter(DMA1_Channel7) != 0);
    
    // ����DMA����
    UART_DMA_Enable(DMA1_Channel7, strlen((const char*)USART2_TX_BUF));
}

// ���ַ�������
void USART2_SendString(u8 *str) {
    while (*str) {
        USART_SendData(USART2, *str++);
        while (USART_GetFlagStatus(USART2, USART_FLAG_TXE) == RESET);
    }
}

#ifdef USART2_RX_EN
/*
 * @brief ��ʱ��4�жϷ�����
 * @note �����жϴ��ڽ��ճ�ʱ(10ms)
 */
void TIM4_IRQHandler(void)
{
    if (TIM_GetITStatus(TIM4, TIM_IT_Update) != RESET)  // �������ж�
    {
        USART2_RX_STA |= 1 << 15;  // ��ǽ������
        TIM_ClearITPendingBit(TIM4, TIM_IT_Update);  // ����жϱ�־
        TIM4_Set(0);  // �رն�ʱ��4
    }
}

/**
 * @brief ��ʱ��4���ؿ���
 * @param sta 0-�ر�, 1-����
 */
void TIM4_Set(u8 sta)
{
    if(sta)
    {
        TIM_SetCounter(TIM4, 0);  // ����������
        TIM_Cmd(TIM4, ENABLE);    // ʹ�ܶ�ʱ��
    }
    else
    {
        TIM_Cmd(TIM4, DISABLE);   // �رն�ʱ��
    }
}

/**
 * @brief ��ʱ��4��ʼ��
 * @param arr �Զ���װֵ
 * @param psc ʱ��Ԥ��Ƶ��
 * @note ��ʱ��ʱ��ΪAPB1��2��(72MHz)
 */
void TIM4_Init(u16 arr, u16 psc)
{
    NVIC_InitTypeDef NVIC_InitStructure;
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;

    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);  // TIM4ʱ��ʹ��
    
    // ��ʱ����������
    TIM_TimeBaseStructure.TIM_Period = arr;               // �Զ���װ��ֵ
    TIM_TimeBaseStructure.TIM_Prescaler = psc;            // Ԥ��Ƶֵ
    TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1; // ʱ�ӷָ�
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up; // ���ϼ���
    TIM_TimeBaseInit(TIM4, &TIM_TimeBaseStructure);
    
    // �ж�����
    TIM_ITConfig(TIM4, TIM_IT_Update, ENABLE);  // ʹ�ܸ����ж�
    
    NVIC_InitStructure.NVIC_IRQChannel = TIM4_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1; // ��ռ���ȼ�1
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;       // �����ȼ�2
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
}
#endif

/**
 * @brief USART DMA��������
 * @param DMA_CHx DMAͨ��
 * @param cpar �����ַ
 * @param cmar �洢����ַ
 * @note ����Ϊ�洢��->����ģʽ/8λ���ݿ��/�洢������ģʽ
 */
void UART_DMA_Config(DMA_Channel_TypeDef* DMA_CHx, u32 cpar, u32 cmar)
{
    DMA_InitTypeDef DMA_InitStructure;
    
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);  // DMA1ʱ��ʹ��
    DMA_DeInit(DMA_CHx);  // ��λDMAͨ��
    
    // DMA��ʼ���ṹ������
    DMA_InitStructure.DMA_PeripheralBaseAddr = cpar;   // �����ַ
    DMA_InitStructure.DMA_MemoryBaseAddr = cmar;       // �洢����ַ
    DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralDST; // ���䷽��:�ڴ�->����
    DMA_InitStructure.DMA_BufferSize = 0;              // ��ʼ����������
    DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable; // �����ַ�̶�
    DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;          // �ڴ��ַ����
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte; // 8λ����
    DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;         // 8λ����
    DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;      // ����ģʽ(��ѭ��)
    DMA_InitStructure.DMA_Priority = DMA_Priority_Medium; // �е����ȼ�
    DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;       // ���ڴ浽�ڴ�ģʽ
    
    DMA_Init(DMA_CHx, &DMA_InitStructure);  // ��ʼ��DMA
}

/**
 * @brief ����һ��DMA����
 * @param DMA_CHx DMAͨ��
 * @param len �������ݳ���
 */
void UART_DMA_Enable(DMA_Channel_TypeDef* DMA_CHx, u8 len)
{
    DMA_Cmd(DMA_CHx, DISABLE);               // �ȹر�DMAͨ��
    DMA_SetCurrDataCounter(DMA_CHx, len);    // ���ô���������
    DMA_Cmd(DMA_CHx, ENABLE);                // ʹ��DMA����
}
