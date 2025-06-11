#include "stm32f10x.h"
#include "dht11.h"
#include "delay.h"

/**
 * @brief  初始化 DHT11 数据引脚为推挽输出模式
 * @param  无
 * @retval 无
 */
void DHT11_GPIO_Config(void)
{		
    GPIO_InitTypeDef GPIO_InitStructure;

    RCC_APB2PeriphClockCmd(DHT11_CLK, ENABLE);        // 使能 DHT11 所在端口时钟

    GPIO_InitStructure.GPIO_Pin = DHT11_PIN;	
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;  // 推挽输出模式
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz; 
    GPIO_Init(DHT11_PORT, &GPIO_InitStructure);		  

    GPIO_SetBits(DHT11_PORT, DHT11_PIN);              // 默认拉高数据线
}

/**
 * @brief  将 DHT11 数据引脚配置为上拉输入模式
 * @param  无
 * @retval 无
 */
static void DHT11_Mode_IPU(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    GPIO_InitStructure.GPIO_Pin = DHT11_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;    // 上拉输入
    GPIO_Init(DHT11_PORT, &GPIO_InitStructure);	 
}

/**
 * @brief  将 DHT11 数据引脚配置为推挽输出模式
 * @param  无
 * @retval 无
 */
static void DHT11_Mode_Out_PP(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    GPIO_InitStructure.GPIO_Pin = DHT11_PIN;	
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;  // 推挽输出
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(DHT11_PORT, &GPIO_InitStructure);	 
}

/**
 * @brief  从 DHT11 读取 1 字节数据（MSB 先行）
 * @param  无
 * @retval 读取到的 1 字节数据
 */
static uint8_t Read_Byte(void) 
{
    uint8_t i, temp = 0;

    for (i = 0; i < 8; i++)    
    {	   
        while (DHT11_DATA_IN() == Bit_RESET);       // 等待低电平结束（起始信号）
        delay_us(40);                               // 延时 40us，超出“0”信号高电平宽度

        if (DHT11_DATA_IN() == Bit_SET)             // 如果仍为高电平，则为数据位 1
        {
            while (DHT11_DATA_IN() == Bit_SET);     // 等待数据位高电平结束
            temp |= (uint8_t)(0x01 << (7 - i));     // 写入当前位（高位先行）
        }
        else                                        // 如果变为低电平，则为数据位 0
        {
            temp &= (uint8_t)~(0x01 << (7 - i));    // 保持为 0
        }
    }

    return temp;
}

/**
 * @brief  读取一次 DHT11 数据（总共 40 位，5 个字节）
 * @param  DHT11_Data : 指向 DHT11_Data_TypeDef 结构体的指针
 * @retval SUCCESS（0）：读取成功
 *         ERROR  （1）：读取失败
 */
uint8_t Read_DHT11(DHT11_Data_TypeDef *DHT11_Data)
{
    DHT11_Mode_Out_PP();     // 配置为推挽输出模式

    DHT11_DATA_OUT(LOW);     // 主机拉低信号
    delay_ms(18);            // 延时至少 18ms，发送起始信号

    DHT11_DATA_OUT(HIGH);    // 主机拉高
    delay_us(30);            // 延时 20~40us，等待 DHT11 响应

    DHT11_Mode_IPU();        // 配置为上拉输入，准备接收响应

    if (DHT11_DATA_IN() == Bit_RESET)               // 检测到 DHT11 的响应低电平
    {
        while (DHT11_DATA_IN() == Bit_RESET);        // 等待响应低电平结束
        while (DHT11_DATA_IN() == Bit_SET);          // 等待准备信号高电平结束

        /* 接收40位数据 */
        DHT11_Data->humi_int  = Read_Byte();         // 湿度整数部分
        DHT11_Data->humi_deci = Read_Byte();         // 湿度小数部分
        DHT11_Data->temp_int  = Read_Byte();         // 温度整数部分
        DHT11_Data->temp_deci = Read_Byte();         // 温度小数部分
        DHT11_Data->check_sum = Read_Byte();         // 校验和

        DHT11_Mode_Out_PP();                         // 读取完毕，切回输出模式
        DHT11_DATA_OUT(HIGH);                        // 总线拉高，准备下一次通信

        /* 校验数据正确性 */
        if (DHT11_Data->check_sum == (DHT11_Data->humi_int + DHT11_Data->humi_deci + 
                                      DHT11_Data->temp_int + DHT11_Data->temp_deci))
        {
            return SUCCESS;   // 校验通过
        }
        else
        {
            return ERROR;     // 校验失败
        }
    }
    else
    {
        return ERROR;         // 无响应
    }
}
