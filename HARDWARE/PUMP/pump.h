#ifndef __PUMP_H
#define __PUMP_H

#include "sys.h"

// �̵����������Ŷ���
#define PUMP_GPIO_PORT     GPIOB
#define PUMP_GPIO_PIN      GPIO_Pin_1

// �̵������ƺ�
#define PUMP_OFF()         GPIO_SetBits(PUMP_GPIO_PORT, PUMP_GPIO_PIN)
#define PUMP_ON()          GPIO_ResetBits(PUMP_GPIO_PORT, PUMP_GPIO_PIN)
#define PUMP_TOGGLE()      GPIO_ReadOutputDataBit(PUMP_GPIO_PORT, PUMP_GPIO_PIN) ? PUMP_OFF() : PUMP_ON()

// ��ʼ����������
void PUMP_GPIO_Init(void);

#endif
