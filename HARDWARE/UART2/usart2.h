#ifndef __USART2_H
#define __USART2_H

#include "sys.h"

//////////////////////////////////////////////////////////////////////////////////
// 串口2驱动头文件
// 功能：提供USART2串口的初始化、发送和接收功能
// 版本：V1.0
// 最后修改：2014/3/29
// 注意：发送和接收缓冲区大小可根据需要调整
//////////////////////////////////////////////////////////////////////////////////

/* 配置参数 */
#define USART2_MAX_RECV_LEN     1024    // 最大接收缓存字节数
#define USART2_MAX_SEND_LEN     1024    // 最大发送缓存字节数
#define USART2_RX_EN           1       // 接收功能使能：0-禁用，1-启用

/* 外部变量声明 */
extern u8 USART2_RX_BUF[USART2_MAX_RECV_LEN];  // 接收缓冲区
extern u8 USART2_TX_BUF[USART2_MAX_SEND_LEN];  // 发送缓冲区
extern u16 USART2_RX_STA;                      // 接收状态寄存器
                                               // bit15: 接收完成标志
                                               // bit14-0: 接收数据长度

/* 函数声明 */
void USART2_Init(u32 bound);                   // 串口2初始化
void TIM4_Set(u8 sta);                         // 定时器4开关控制
void TIM4_Init(u16 arr, u16 psc);              // 定时器4初始化
void UART_DMA_Config(DMA_Channel_TypeDef* DMA_CHx, u32 cpar, u32 cmar); // DMA配置
void UART_DMA_Enable(DMA_Channel_TypeDef* DMA_CHx, u8 len);              // DMA传输使能
void u2_printf(char* fmt, ...);                // 格式化输出函数(类似printf)
void USART2_SendString(u8 *str);

#endif
