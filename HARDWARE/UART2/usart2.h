#ifndef __USART2_H
#define __USART2_H

#include "sys.h"

//////////////////////////////////////////////////////////////////////////////////
// ����2����ͷ�ļ�
// ���ܣ��ṩUSART2���ڵĳ�ʼ�������ͺͽ��չ���
// �汾��V1.0
// ����޸ģ�2014/3/29
// ע�⣺���ͺͽ��ջ�������С�ɸ�����Ҫ����
//////////////////////////////////////////////////////////////////////////////////

/* ���ò��� */
#define USART2_MAX_RECV_LEN     1024    // �����ջ����ֽ���
#define USART2_MAX_SEND_LEN     1024    // ����ͻ����ֽ���
#define USART2_RX_EN           1       // ���չ���ʹ�ܣ�0-���ã�1-����

/* �ⲿ�������� */
extern u8 USART2_RX_BUF[USART2_MAX_RECV_LEN];  // ���ջ�����
extern u8 USART2_TX_BUF[USART2_MAX_SEND_LEN];  // ���ͻ�����
extern u16 USART2_RX_STA;                      // ����״̬�Ĵ���
                                               // bit15: ������ɱ�־
                                               // bit14-0: �������ݳ���

/* �������� */
void USART2_Init(u32 bound);                   // ����2��ʼ��
void TIM4_Set(u8 sta);                         // ��ʱ��4���ؿ���
void TIM4_Init(u16 arr, u16 psc);              // ��ʱ��4��ʼ��
void UART_DMA_Config(DMA_Channel_TypeDef* DMA_CHx, u32 cpar, u32 cmar); // DMA����
void UART_DMA_Enable(DMA_Channel_TypeDef* DMA_CHx, u8 len);              // DMA����ʹ��
void u2_printf(char* fmt, ...);                // ��ʽ���������(����printf)
void USART2_SendString(u8 *str);

#endif
