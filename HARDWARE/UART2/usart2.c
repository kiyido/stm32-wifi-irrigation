#include "delay.h"
#include "usart2.h"
#include "stdarg.h"
#include "stdio.h"
#include "string.h"

/* 发送缓冲区 - 8字节对齐 */
__align(8) u8 USART2_TX_BUF[USART2_MAX_SEND_LEN];

#ifdef USART2_RX_EN
/* 接收缓冲区 */
u8 USART2_RX_BUF[USART2_MAX_RECV_LEN];

/* 接收状态寄存器
 * [15]: 0-未接收到数据, 1-接收到一批数据
 * [14:0]: 接收到的数据长度
 */
u16 USART2_RX_STA = 0;

/*
 * @brief USART2中断服务函数
 * @note 处理接收中断，使用定时器4判断数据帧结束(10ms超时)
 */
void USART2_IRQHandler(void)
{
    u8 res;
    
    // 检查是否接收到数据
    if(USART_GetITStatus(USART2, USART_IT_RXNE) != RESET)
    {
        res = USART_ReceiveData(USART2);  // 读取接收到的数据
        
        if(USART2_RX_STA < USART2_MAX_RECV_LEN)  // 缓冲区未满
        {
            TIM_SetCounter(TIM4, 0);              // 重置定时器计数器
            if(USART2_RX_STA == 0)               // 如果是第一个字节
            {
                TIM4_Set(1);                     // 启动定时器4
            }
            USART2_RX_BUF[USART2_RX_STA++] = res; // 存储数据并递增计数器
        }
        else  // 缓冲区已满
        {
            USART2_RX_STA |= 1 << 15;            // 强制标记接收完成
        }
    }
}
#endif

/*
 * @brief 初始化USART2串口
 * @param bound 波特率
 */
void USART2_Init(u32 bound)
{
    NVIC_InitTypeDef NVIC_InitStructure;
    GPIO_InitTypeDef GPIO_InitStructure;
    USART_InitTypeDef USART_InitStructure;

    // 1. 时钟使能
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);  // GPIOA时钟
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE); // USART2时钟

    // 2. GPIO配置
    USART_DeInit(USART2);  // 复位串口2
    
    // TX引脚(PA2)配置：复用推挽输出
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    
    // RX引脚(PA3)配置：浮空输入
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    // 3. USART参数配置
    USART_InitStructure.USART_BaudRate = bound;           // 波特率
    USART_InitStructure.USART_WordLength = USART_WordLength_8b; // 8位数据
    USART_InitStructure.USART_StopBits = USART_StopBits_1;     // 1位停止位
    USART_InitStructure.USART_Parity = USART_Parity_No;        // 无校验
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None; // 无流控
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx; // 收发模式
    
    USART_Init(USART2, &USART_InitStructure);  // 初始化串口2

    // 4. DMA发送配置
    USART_DMACmd(USART2, USART_DMAReq_Tx, ENABLE);  // 使能串口2的DMA发送
    UART_DMA_Config(DMA1_Channel7, (u32)&USART2->DR, (u32)USART2_TX_BUF); // 配置DMA
    USART_Cmd(USART2, ENABLE);  // 使能串口

#ifdef USART2_RX_EN
    // 5. 接收中断配置
    USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);  // 使能接收中断
    
    NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2; // 抢占优先级2
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;       // 子优先级3
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
    
    // 6. 定时器4初始化(用于判断数据帧结束)
    TIM4_Init(999, 7199);  // 10ms中断
    USART2_RX_STA = 0;     // 清零接收状态
    TIM4_Set(0);            // 初始关闭定时器4
#endif
}

/**
 * @brief 串口2格式化输出函数
 * @param fmt 格式化字符串
 * @note 确保一次发送数据不超过USART2_MAX_SEND_LEN字节
 */
void u2_printf(char* fmt, ...)
{
    va_list ap;
    
    va_start(ap, fmt);
    vsprintf((char*)USART2_TX_BUF, fmt, ap);  // 格式化字符串到发送缓冲区
    va_end(ap);
    
    // 等待上一次DMA传输完成
    while(DMA_GetCurrDataCounter(DMA1_Channel7) != 0);
    
    // 启动DMA传输
    UART_DMA_Enable(DMA1_Channel7, strlen((const char*)USART2_TX_BUF));
}

// 简单字符串发送
void USART2_SendString(u8 *str) {
    while (*str) {
        USART_SendData(USART2, *str++);
        while (USART_GetFlagStatus(USART2, USART_FLAG_TXE) == RESET);
    }
}

#ifdef USART2_RX_EN
/*
 * @brief 定时器4中断服务函数
 * @note 用于判断串口接收超时(10ms)
 */
void TIM4_IRQHandler(void)
{
    if (TIM_GetITStatus(TIM4, TIM_IT_Update) != RESET)  // 检查更新中断
    {
        USART2_RX_STA |= 1 << 15;  // 标记接收完成
        TIM_ClearITPendingBit(TIM4, TIM_IT_Update);  // 清除中断标志
        TIM4_Set(0);  // 关闭定时器4
    }
}

/**
 * @brief 定时器4开关控制
 * @param sta 0-关闭, 1-开启
 */
void TIM4_Set(u8 sta)
{
    if(sta)
    {
        TIM_SetCounter(TIM4, 0);  // 计数器清零
        TIM_Cmd(TIM4, ENABLE);    // 使能定时器
    }
    else
    {
        TIM_Cmd(TIM4, DISABLE);   // 关闭定时器
    }
}

/**
 * @brief 定时器4初始化
 * @param arr 自动重装值
 * @param psc 时钟预分频数
 * @note 定时器时钟为APB1的2倍(72MHz)
 */
void TIM4_Init(u16 arr, u16 psc)
{
    NVIC_InitTypeDef NVIC_InitStructure;
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;

    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);  // TIM4时钟使能
    
    // 定时器基础配置
    TIM_TimeBaseStructure.TIM_Period = arr;               // 自动重装载值
    TIM_TimeBaseStructure.TIM_Prescaler = psc;            // 预分频值
    TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1; // 时钟分割
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up; // 向上计数
    TIM_TimeBaseInit(TIM4, &TIM_TimeBaseStructure);
    
    // 中断配置
    TIM_ITConfig(TIM4, TIM_IT_Update, ENABLE);  // 使能更新中断
    
    NVIC_InitStructure.NVIC_IRQChannel = TIM4_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1; // 抢占优先级1
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;       // 子优先级2
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
}
#endif

/**
 * @brief USART DMA发送配置
 * @param DMA_CHx DMA通道
 * @param cpar 外设地址
 * @param cmar 存储器地址
 * @note 配置为存储器->外设模式/8位数据宽度/存储器增量模式
 */
void UART_DMA_Config(DMA_Channel_TypeDef* DMA_CHx, u32 cpar, u32 cmar)
{
    DMA_InitTypeDef DMA_InitStructure;
    
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);  // DMA1时钟使能
    DMA_DeInit(DMA_CHx);  // 复位DMA通道
    
    // DMA初始化结构体配置
    DMA_InitStructure.DMA_PeripheralBaseAddr = cpar;   // 外设地址
    DMA_InitStructure.DMA_MemoryBaseAddr = cmar;       // 存储器地址
    DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralDST; // 传输方向:内存->外设
    DMA_InitStructure.DMA_BufferSize = 0;              // 初始传输数据量
    DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable; // 外设地址固定
    DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;          // 内存地址递增
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte; // 8位数据
    DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;         // 8位数据
    DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;      // 正常模式(非循环)
    DMA_InitStructure.DMA_Priority = DMA_Priority_Medium; // 中等优先级
    DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;       // 非内存到内存模式
    
    DMA_Init(DMA_CHx, &DMA_InitStructure);  // 初始化DMA
}

/**
 * @brief 启动一次DMA传输
 * @param DMA_CHx DMA通道
 * @param len 传输数据长度
 */
void UART_DMA_Enable(DMA_Channel_TypeDef* DMA_CHx, u8 len)
{
    DMA_Cmd(DMA_CHx, DISABLE);               // 先关闭DMA通道
    DMA_SetCurrDataCounter(DMA_CHx, len);    // 设置传输数据量
    DMA_Cmd(DMA_CHx, ENABLE);                // 使能DMA传输
}
