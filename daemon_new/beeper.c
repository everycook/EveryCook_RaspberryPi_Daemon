/*
This is the EveryCook Raspberry Pi daemon. It reads inputs from the EveryCook Raspberry Pi shield and controls the outputs.
EveryCook is an open source platform for collecting all data about food and make it available to all kinds of cooking devices.

This program is copyright (C) by EveryCook. Written by Samuel Werder, Peter Turczak and Alexis Wiasmitinow.

This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.

See GPLv3.htm in the main folder for details.
*/
#include "beeper.h"


const uint8_t ALLWAYS_STOP_BUZZING = 0;

bool beeperdoRememberBeep;//if it have to beep
uint32_t beeperbeepEndTime;//time to stop beeping
bool beeperisBuzzing;//if beeping
uint8_t beeperBeepStepEnd;


void beeperBeep(){
	if(beeperGetDoRememberBeep()==1){
		if (daemonGetTimeValuesRunTime()>daemonGetSettingsTimeValuesStepEndTime()){//if step is ended
			if (daemonGetCurrentCommandValuesMode()>=MIN_STATUS_MODE && daemonGetCurrentCommandValuesMode()<=MAX_STATUS_MODE){
				int toLateTime = daemonGetTimeValuesRunTime()-daemonGetSettingsTimeValuesStepEndTime();
				if (toLateTime==1){
					beeperSetBeepEndTime(daemonGetTimeValuesRunTime()+1);
				} else if (toLateTime==5){
					beeperSetBeepEndTime(daemonGetTimeValuesRunTime()+1);
				} else if (toLateTime==30){
					beeperSetBeepEndTime(daemonGetTimeValuesRunTime()+1);
				} else if (toLateTime==60){
					beeperSetBeepEndTime(daemonGetTimeValuesRunTime()+1);
				} else if (toLateTime>0 && toLateTime%300==0){ //each 5 min
					beeperSetBeepEndTime(daemonGetTimeValuesRunTime()+5);
				}
			}
		}
	}
	if (daemonGetTimeValuesRunTime()<beeperGetBeepEndTime()){
		if (!beeperGetIsBuzzing()){
			beeperSetIsBuzzing(true);
			uint32_t duration = beeperGetBeepEndTime()-daemonGetTimeValuesRunTime();
			beeperBeepSeconde(duration);
		}
	} else {
		if (beeperGetIsBuzzing() || ALLWAYS_STOP_BUZZING){
			beeperSetIsBuzzing(false);
		}
	}
}

void beeperBeepSeconde(uint32_t durationS){
	char command[100];
	sprintf(command, "speaker-test -f 500 -t sine -p 1000 -l %d 1>/dev/null 2>&1 &", durationS);
	printf("%s\n",command);
	system(command);
}



void beeperSetDoRememberBeep(bool doRememberBeep){
	beeperdoRememberBeep=doRememberBeep;
}
void beeperSetBeepEndTime(uint32_t beepEndTime){
	beeperbeepEndTime=beepEndTime;
}
void beeperSetIsBuzzing(bool isBuzzing){
	beeperisBuzzing=isBuzzing;
}
void beeperSetSettingsBeepStepEnd(uint8_t BeepStepEnd){
	beeperBeepStepEnd=BeepStepEnd;
}

bool beeperGetDoRememberBeep(){
	return beeperdoRememberBeep;
}
uint32_t beeperGetBeepEndTime(){
	return beeperbeepEndTime;
}
bool beeperGetIsBuzzing(){
	return beeperisBuzzing;
}
uint8_t beeperGetSettingsBeepStepEnd(){
	return beeperBeepStepEnd;
}
