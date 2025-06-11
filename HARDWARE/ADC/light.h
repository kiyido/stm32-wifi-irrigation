#ifndef __LIGHT_H
#define __LIGHT_H

#include "sys.h"

void Adc_Init_Light(void);
u16 Get_Adc_Average_Light(u8 ch,u8 times); 
u8 Lsens_Get_Val(void);
uint16_t Lsens_Get_Lux(void);

#endif
