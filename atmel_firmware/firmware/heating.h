#ifndef heating_h
#define heating_h

#include "mytypes.h"

#define HEATING_PULSE_INTERVAL 500

//namespace Heating
//{
	extern void Heating_controlIHTemp();
	extern void Heating_init();
	extern void Heating_heatControl();
	extern void Heating_setHeating(boolean on);
	extern int Heating_getLastOnPWM();
	extern boolean Heating_isRunning();
	
	extern uint8_t IHPowerPWM_TIMER;
	extern uint8_t IHFanPWM_TIMER;
	extern uint8_t ihTemp8bit;
	extern int outputValueIH;
	extern uint8_t lastIHFanPWM;
//}

#endif


