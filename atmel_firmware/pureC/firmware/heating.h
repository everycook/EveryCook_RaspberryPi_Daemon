#ifndef heating_h
#define heating_h

#include "mytypes.h"

//namespace Heating
//{
	extern void Heating_controlIHTemp();
	extern void Heating_init();
	extern void Heating_heatControl();
	extern void Heating_setHeating(boolean on);
	extern int Heating_getLastOnPWM();
	extern boolean Heating_isRunning();
//}

#endif


