#include <stdio.h>
#include "sys.h"
#include "delay.h" // FreeRTOS 友好型^_^
#include "usart.h"
#include "usart2.h"

#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"

#include "led.h"
#include "lcd.h"
#include "Picture.h"
#include "dht11.h"
#include "light.h"
#include "soil.h"
#include "key.h"
#include "pump.h"
#include "esp8266.h"

#include "mode.h"
#include "sensor_data.h"

// 任务优先级
#define START_TASK_PRIO     1
#define TEST01_TASK_PRIO    3
#define TEST02_TASK_PRIO    3
#define TEST03_TASK_PRIO    2
#define TEST04_TASK_PRIO    3
#define TEST05_TASK_PRIO    4
#define TEST06_TASK_PRIO    2

// 任务堆栈大小（单位：字，4字节/字）
#define START_STK_SIZE      128
#define TEST01_STK_SIZE     128
#define TEST02_STK_SIZE     128
#define TEST03_STK_SIZE     128
#define TEST04_STK_SIZE     64
#define TEST05_STK_SIZE     128
#define TEST06_STK_SIZE     64

// 任务句柄
TaskHandle_t StartTask_Handler;
TaskHandle_t TEST01Task_Handler;
TaskHandle_t TEST02Task_Handler;
TaskHandle_t TEST03Task_Handler;
TaskHandle_t TEST04Task_Handler;
TaskHandle_t TEST05Task_Handler;
TaskHandle_t TEST06Task_Handler;

// 全局互斥锁句柄
SemaphoreHandle_t xMutex_GSensorData;

// 任务函数声明
void start_task(void *pvParameters);
void test01_task(void *pvParameters); // 板载小灯泡闪烁
void test02_task(void *pvParameters); // 显示屏画面更新
void test03_task(void *pvParameters); // 发送数据给上位机
void test04_task(void *pvParameters); // 板载按键扫描
void test05_task(void *pvParameters); // 传感器数据更新
void test06_task(void *pvParameters); // 处理上位机指令及网络状态更新

int main(void) 
{
    delay_init();
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);
    USART1_Init(115200);
	USART2_Init(115200);
	
	// 从Flash中加载系统配置
	Load_Config();
    
    LCD_Init();
	LCD_BLK = 0; // 背光
    LED_GPIO_Init(); 
    DHT11_GPIO_Config();
    Adc_Init_Light();
	Adc_Init_Soil();
	KEY_GPIO_Init();
	PUMP_GPIO_Init();
	ESP8266_Mode_Init(); // 内嵌加载画面喵

    // 创建开始任务
    xTaskCreate(start_task, "start_task", START_STK_SIZE, NULL, START_TASK_PRIO, &StartTask_Handler);
    vTaskStartScheduler();  // 启动 FreeRTOS 调度器
}

// 开始任务：创建其他任务后删除自身
void start_task(void *pvParameters) 
{
    taskENTER_CRITICAL();
    xTaskCreate(test01_task, "test01_task", TEST01_STK_SIZE, NULL, TEST01_TASK_PRIO, &TEST01Task_Handler);
    xTaskCreate(test02_task, "test02_task", TEST02_STK_SIZE, NULL, TEST02_TASK_PRIO, &TEST02Task_Handler);
    xTaskCreate(test03_task, "test03_task", TEST03_STK_SIZE, NULL, TEST03_TASK_PRIO, &TEST03Task_Handler);
    xTaskCreate(test04_task, "test04_task", TEST04_STK_SIZE, NULL, TEST04_TASK_PRIO, &TEST04Task_Handler);
    xTaskCreate(test05_task, "test05_task", TEST05_STK_SIZE, NULL, TEST05_TASK_PRIO, &TEST05Task_Handler);
    xTaskCreate(test06_task, "test06_task", TEST06_STK_SIZE, NULL, TEST06_TASK_PRIO, &TEST06Task_Handler);
	xMutex_GSensorData = xSemaphoreCreateMutex();
	if (xMutex_GSensorData == NULL) {
		// 锁创建失败
		printf("xSemaphoreCreateMutexErr\r\n");
		while(1);
	}
    vTaskDelete(NULL);
    taskEXIT_CRITICAL();
}

// LED 保持闪烁
void test01_task(void *pvParameters) 
{
    while (1)
	{
        LED = !LED;
		
        delay_ms(1000);
    }
}

// LCD 显示
void test02_task(void *pvParameters) 
{
	// 拷贝缓存，防止任务2在调用数据途中，数据被任务5篡改
	DHT11_Data_TypeDef tmp_dht11;
	u8 tmp_light, tmp_soil;
	u16 tmp_light_lux;
	u8 tmp_pump;
	
	char Temp_str[32]; // 存放格式化后的字符串
	
	// 启动画面
    LCD_Clear(WHITE);
    LCD_Showimage(0, 0, 128, 128, gImage_01);
//	LCD_DrawFont_GBK24(0, 108, BLACK, WHITE, "STM32F103RCT6");
    delay_ms(3000);
    LCD_Clear(WHITE);
	
	// 信息框架
	LCD_DrawFont_GBK24(0, 0, BLACK, WHITE, "欢迎使用");
	LCD_DrawFont_GBK16(0, 40, RED, WHITE, "温度:");
	LCD_DrawFont_GBK16(0, 56, RED, WHITE, "湿度:");
	LCD_DrawFont_GBK16(0, 72, RED, WHITE, "光照:");
	LCD_DrawFont_GBK16(0, 88, RED, WHITE, "土壤:");
	LCD_DrawFont_GBK16(0, 104, RED, WHITE, "模式");
	LCD_DrawFont_GBK16(68, 104, RED, WHITE, "水泵");
	
	while(1) {
		// 读操作上锁
		if (xSemaphoreTake(xMutex_GSensorData, portMAX_DELAY) == pdTRUE)
		{
			// 一次性保存下当前所有传感器数据
			tmp_dht11.temp_int = g_sensor_data.DHT11_Data.temp_int;
			tmp_dht11.temp_deci = g_sensor_data.DHT11_Data.temp_deci;
			tmp_dht11.humi_int = g_sensor_data.DHT11_Data.humi_int;
			tmp_light = g_sensor_data.light;
			tmp_soil = g_sensor_data.soil;
			tmp_light_lux = g_sensor_data.light_lux;
			tmp_pump = pump_threshold;
			
			xSemaphoreGive(xMutex_GSensorData); // 释放锁
		}
		// 处理自动模式控制水泵
		if (mode_flag) 
		{
            if (tmp_soil < pump_threshold) 
			{
                pump_flag = 1;
            } 
			else 
			{
                pump_flag = 0;
            }
        }
		
		sprintf(Temp_str, "%d ", tmp_pump);
		LCD_DrawFont_GBK16(96, 0, BLUE, WHITE, (u8*)Temp_str); // 阈值
		
		sprintf(Temp_str, "%d.%d  ", tmp_dht11.temp_int, tmp_dht11.temp_deci);
		LCD_DrawFont_GBK16(48, 40, BLUE, WHITE, (u8*)Temp_str); // 温度
		
		sprintf(Temp_str, "%d ", tmp_dht11.humi_int);
		LCD_DrawFont_GBK16(48, 56, BLUE, WHITE, (u8*)Temp_str); // 湿度

		if(tmp_light_lux>=10000) // 防止溢出
			sprintf(Temp_str, "%d@%u", tmp_light, tmp_light_lux);
		else if(tmp_light_lux<100)
			sprintf(Temp_str, "%d@%u    ", tmp_light, tmp_light_lux);
		else
			sprintf(Temp_str, "%d@%u  ", tmp_light, tmp_light_lux);
		LCD_DrawFont_GBK16(48, 72, BLUE, WHITE, (u8*)Temp_str); // 相对光照与Lux值
		
		sprintf(Temp_str, "%d ", tmp_soil); // 补一个空格，覆盖旧的第二位
		LCD_DrawFont_GBK16(48, 88, BLUE, WHITE, (u8*)Temp_str); // 土壤
		
		// 局域网连接状态更新，模式与水泵状态显示更新
		if(connection_flag) 
		{
			LCD_DrawFont_GBK16(0, 24, WHITE, GREEN, "   Connected    ");
		} 
		else 
		{
			LCD_DrawFont_GBK16(0, 24, WHITE, RED, "  Disconnected  ");
		}
		if(pump_flag) 
		{
			LCD_DrawFont_GBK16(102, 104, BLUE, WHITE, "开");
			PUMP_ON(); // 打开水泵
		} 
		else 
		{
			LCD_DrawFont_GBK16(102, 104, BLUE, WHITE, "关");
			PUMP_OFF(); // 关闭水泵
		}
		if(mode_flag) 
		{
			LCD_DrawFont_GBK16(34, 104, BLUE, WHITE, "自动");
		} 
		else 
		{
			LCD_DrawFont_GBK16(34, 104, BLUE, WHITE, "手动");
		}
		
		// 保存系统配置到Flash
		Save_Config();
		
		delay_ms(1000);
	}
}

// ESP8266，发送数据给上位机
void test03_task(void *pvParameters) 
{
    while (1) 
	{
		// 有连接则发送数据给上位机
		if(connection_flag) 
		{
			ESP8266_Send_Data();
		}

		delay_ms(500);
    }
}

// 板载按键扫描
void test04_task(void *pvParameters) 
{
	// 每次只执行一次操作，防止使用组合键时，单键功能被触发
	u8 combo_triggered = 0;
	// 防止使用组合键后，松开前置键时单键功能被触发
	u8 combo_active = 0;
	
	// 连续多次检测到按键状态一致后，才确认状态变化，否则认为是抖动
	// 当前去抖次数 (3 * 10ms = 30ms)
    while (1) 
	{
        key1.now = GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_9);
        key2.now = GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_8);

		// 扫描按键，确认状态
        scan_key(&key1, key1.now);
        scan_key(&key2, key2.now);
		
        // 组合键1：先按 KEY1，再按 KEY2 抬起时触发
        if (key2.last == 0 && key2.stable == 1 && key1.stable == 0) 
        {
			if(pump_threshold < 95)
				pump_threshold += 5;
			else if (pump_threshold >= 95)
				pump_threshold = 100;
			combo_triggered = 1;
			combo_active = 1;
        }

        // 组合键2：先按 KEY2，再按 KEY1 抬起时触发
        if (key1.last == 0 && key1.stable == 1 && key2.stable == 0) 
        {
			if(pump_threshold > 5)
				pump_threshold -= 5;
			else if (pump_threshold <= 5)
				pump_threshold = 0;
			combo_triggered = 1;
			combo_active = 1;
        }
		
		// 组合键全部松开，才能执行单键功能
		if (combo_active == 0) 
		{
			// 上次为按下，这次确认是松开，抬起触发
			if (key1.last == 0 && key1.stable == 1 && combo_triggered == 0) 
			{
				mode_flag = !mode_flag;
			}
			if (key2.last == 0 && key2.stable == 1 && combo_triggered == 0) 
			{
				pump_flag = !pump_flag;
			}
		}

		// 所有按键都松开，才执行清除
		if (key1.stable == 1 && key2.stable == 1)
		{
			combo_active = 0;
		}
		// 记录上次状态
        key1.last = key1.stable;
        key2.last = key2.stable;
		
		combo_triggered = 0;

        delay_ms(10);
    }
}

// 传感器数据更新到全局变量
void test05_task(void *pvParameters) 
{
    while (1) 
	{
		// 写操作上锁
        if (xSemaphoreTake(xMutex_GSensorData, portMAX_DELAY) == pdTRUE) 
		{
			Read_DHT11(&g_sensor_data.DHT11_Data);
			g_sensor_data.light = Lsens_Get_Val();
			g_sensor_data.light_lux = Lsens_Get_Lux();
			g_sensor_data.soil = Soil_Get_Val();
			
			xSemaphoreGive(xMutex_GSensorData); // 释放锁
		}
        delay_ms(1000);
    }
}

// ESP8266，检测上位机的命令并执行，检测连接状态并更新 connection_flag 状态
void test06_task(void *pvParameters) 
{
	while(1) 
	{
		if (USART2_RX_STA & 0x8000) 
		{ // 判断是否接收完成
			USART2_RX_BUF[USART2_RX_STA & 0x7FFF] = 0; // 添加结束符 '\0'
//			printf("TIPTIP:%s\r\n",USART2_RX_BUF);
			ESP8266_Parse_Command(USART2_RX_BUF); // 解析命令
			USART2_RX_STA = 0; // 清空状态，准备下一帧
		}

		delay_ms(50);
	}
}
