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
#include "heater.h"


/** @brief check something
 * 	@param * leds :
 *  @param errNo
 *  @param * state
 *  @param * lastTime
 *  @param runTime
 @return true if no probleme
*/
bool heaterCheckIsState(uint32_t* leds, uint8_t errNo, bool* state, uint32_t* lastTime, uint32_t runTime);//checkIsHeaterState
/** @brief calculate the status of heater in according to the shield Version
*/
void *heaterLedEvaluation(void *ptr);

void heaterAtmelSetHeating(bool on);

uint32_t heaterHourCounter=0.0;
uint32_t heaterStartTime=0; 
uint32_t heaterStopTime=0; 
bool heaterPowerStatus=false; 
struct Heater_Leds_Values heaterStatus = {};

uint8_t heaterErrorPattern[7][6] = {{0, 0, 0, 0, 0, 0},{0, 0, 1, 1, 1, 1},{1, 0, 1, 1, 1, 1},{0, 1, 1, 1, 1, 1},{1, 0, 0, 1, 1, 1},{0, 0, 0, 1, 1, 1}};
char* heaterErrorCodes[] = {"No Pan", "Temperature Sensor out of work", "IGBT Sensor out of work","Voltage To High(>260V)", "Voltage to Low(<85V)","The bow out of water","IGBT Temperature To High"};
uint8_t heaterErrorTimeout[7] = {1, 4, 4, 2, 2, 3, 2};

pthread_t threadHeaterLedReader;


//Public function
bool heaterOn(){//HeatOn
	if(daemonGetSettingsShieldVersion() < 4){
		//if (daemonGetSettingsDebug_enabled()|| daemonGetRunningModeSimulationMode()){printf("HeatOn, was: %d\n", state->heatPowerStatus);}
		if (heaterGetPowerStatus() && !heaterGetStatusIsOn()){
			if (( daemonGetTimeValuesRunTimeMillis() - heaterGetStatusIsOnLastTime()) > 6000){
				heaterSetPowerStatus(false);
			}
		}
		if (daemonGetAdcConfigRestartingAdc()){
			return false;
		}
	
		if (!heaterGetPowerStatus()) { //if its off
			if (daemonGetSettingsDebug_enabled()||  daemonGetRunningModeSimulationMode()){printf("HeatOn status was: %d, led is heating: %d\n", heaterGetPowerStatus(), heaterGetStatusIsOn());}
			if (!daemonGetRunningModeSimulationMode()){
				if (!heaterGetStatusIsOn()){
					writeControllButtonPin(IND_KEY_POWER, 0); //"press" the power button
					delay(500);
					writeControllButtonPin(IND_KEY_POWER, 1);
					if ( daemonGetSettingsDebug3_enabled()){printf("-->HeatOn\n");}
				} else {
					printf("ERROR: should turn HeatOn, but led's say it is already on, so nothing done\n");
				}
			}
			heaterSetPowerStatus(true); //save that we turned it on
			heaterStartTime = daemonGetTimeValuesRunTime();
			return true;
		} else {
			return false;
		}
	} else {
		if (heaterGetPowerStatus() && !heaterGetStatusIsOn()){
			heaterSetPowerStatus(false);
		}

		if (daemonGetAdcConfigRestartingAdc()){
			return false;
		}

		if (!heaterGetPowerStatus()) { //if its off
			if (daemonGetSettingsDebug_enabled()|| daemonGetRunningModeSimulationMode()){printf("HeatOn status was: %d, led is heating: %d\n", heaterGetPowerStatus(), heaterGetStatusIsOn());}
			if (!daemonGetRunningModeSimulationMode()){
				if (!heaterGetStatusIsOn()){
					heaterAtmelSetHeating(true);
					if (daemonGetSettingsDebug3_enabled()){printf("-->HeatOn\n");}
				} else {
					printf("ERROR: should turn HeatOn, but ATMega644 status say it's already on, so nothing done\n");
				}
			}
			heaterSetPowerStatus(true); //save that we turned it on
			heaterStartTime = daemonGetTimeValuesRunTime();
			return true;
		} else {
			return false;
		}
	}
}

bool heaterOff(){//HeatOff
	if(daemonGetSettingsShieldVersion() < 4 ){
		if (!heaterGetPowerStatus() && heaterGetStatusIsOn()){
			if ((daemonGetTimeValuesRunTimeMillis() - heaterGetStatusIsOnLastTime()) < 3000){
				heaterSetPowerStatus(true);
			}
		}
		if (daemonGetAdcConfigRestartingAdc()){
			return false;
		}
	
		//if (daemonGetSettingsDebug_enabled()|| daemonGetRunningModeSimulationMode()){printf("HeatOff, was: %d\n", state->heatPowerStatus);}
		if (heaterGetPowerStatus()) { //if its on
			if (daemonGetSettingsDebug_enabled()|| daemonGetRunningModeSimulationMode()){printf("HeatOff status was: %d, led is heating: %d\n", heaterGetPowerStatus(), heaterGetStatusIsOn());}
			if (!daemonGetRunningModeSimulationMode()){
				if (heaterGetStatusIsOn()){
					writeControllButtonPin(IND_KEY_POWER, 0); //"press" the power button
					delay(500);
					writeControllButtonPin(IND_KEY_POWER, 1);
					if (daemonGetSettingsDebug3_enabled()){printf("-->HeatOff\n");}
				} else {
					printf("ERROR: should turn HeatOff, but led's say it is already off, so nothing done\n");
				}
			}
			heaterSetPowerStatus(false); //save that we turned it off
			heaterStopTime = daemonGetTimeValuesRunTime();
			return true;
		} else {
			return false;
		}
	} else {
		if (!heaterGetPowerStatus() && heaterGetStatusIsOn()){
			heaterSetPowerStatus(true);
		}

		if (daemonGetAdcConfigRestartingAdc()){
			return false;
		}

		if (heaterGetPowerStatus()) { //if its on
			if (daemonGetSettingsDebug_enabled()|| daemonGetRunningModeSimulationMode()){printf("HeatOff status was: %d, led is heating: %d\n", heaterGetPowerStatus(), heaterGetStatusIsOn());}
			if (!daemonGetRunningModeSimulationMode()){
				if (heaterGetStatusIsOn()){
					heaterAtmelSetHeating(false);
										if (daemonGetSettingsDebug3_enabled()){printf("-->HeatOff\n");}
				} else {
					printf("ERROR: should turn HeatOff, but ATMega644 status say it is already off, so nothing done\n");
				}
			}
			heaterSetPowerStatus(false); //save that we turned it off
			heaterStopTime = daemonGetTimeValuesRunTime();
			return true;
		} else {
			return false;
		}
	}
}

void heaterUpdateTime(){//updateHeaterTime
	//if (currentCommandValues.motorRpm>0){
	if (heaterGetPowerStatus()){
		if (heaterStartTime < daemonGetTimeValuesLastLogSaveTime()){
			heaterHourCounter = heaterHourCounter + (daemonGetTimeValuesRunTime() - daemonGetTimeValuesLastLogSaveTime());
		} else {
			heaterHourCounter = heaterHourCounter + (daemonGetTimeValuesRunTime() - heaterStartTime);
		}
	} else {
		if (heaterStopTime > daemonGetTimeValuesLastLogSaveTime()){
			heaterHourCounter = heaterHourCounter + (heaterStopTime - daemonGetTimeValuesLastLogSaveTime());
		}
	}
}

void heaterThreadLedReader(){
	pthread_create(&threadHeaterLedReader, NULL, heaterLedEvaluation, NULL); 
}

void heaterStoptThreadLedReader(){
	pthread_join(threadHeaterLedReader, NULL);
}
	
void heaterTestHeatLed(){
	daemonSetStateDelay(10);//delay's initialization
		uint32_t led[6];//leds' initilazation
		led[0] = 0;
		led[1] = 0;
		led[2] = 0;
		led[3] = 0;
		led[4] = 0;
		led[5] = 0;
		
		heaterSetStatusErrorMsg(NULL);
		heaterSetStatusIsOn(false);
		heaterSetStatusHasError(false);
		heaterSetPowerStatus(false);
		heaterOn();
		
		if (daemonGetSettingsShieldVersion() == 1){
			uint32_t lastHeatLedTime = time(NULL);
			uint32_t heatLedValuesSameCount = 0;
			uint32_t noPanLedLastTime = 0;
			while(daemonGetStateRunning()){
				daemonSetTimeValuesRunTime(time(NULL));
				led[0] = readSignPin(0);
				led[1] = readSignPin(1);
				led[2] = readSignPin(2);
				led[3] = readSignPin(3);
				led[4] = readSignPin(4);
				led[5] = readSignPin(5);
				
				bool different = false;
				
				if (led[3] == 0){
					heaterSetStatusIsOnLastTime(daemonGetTimeValuesRunTime());
					if (!heaterGetStatusIsOn()){
						heaterSetStatusIsOn(true);
						different = true;
					}
				} else if (heaterGetStatusIsOn() && heaterGetStatusIsOnLastTime() + 3 < daemonGetTimeValuesRunTime()){
					heaterSetStatusIsOn(false);
						different = true;
				}
				
				if (led[0] == 1 && led[1] == 1 && led[2] == 1 && led[3] == 1 && led[4] == 1 && led[5] == 1){
					if (heaterGetStatusHasPower() && heaterGetStatusHasPowerLedOnLastTime() + 4 < daemonGetTimeValuesRunTime()){
						heaterSetStatusHasPower(false);
						different = true;
					}
				} else {
					heaterSetStatusHasPowerLedOnLastTime(daemonGetTimeValuesRunTime());
					if (!heaterGetStatusHasPower()){
						heaterSetStatusHasPower(true);
						different = true;
					}
				}
				
				if (led[0] == 0 && led[1] == 0 && led[2] == 0 && led[3] == 0 && led[4] == 0 && led[5] == 0){
					noPanLedLastTime = daemonGetTimeValuesRunTime();
					if (!heaterGetStatusNoPanError()){
						heaterSetStatusNoPanError(true);
						different = true;
					}
				} else {
					if (heaterGetStatusNoPanError() && noPanLedLastTime + 1 < daemonGetTimeValuesRunTime()){
						heaterSetStatusNoPanError(false);
						different = true;
					}
				}
				
				uint8_t i = 0;
				for (; i<6; ++i){
					if (heaterGetStatusLedValuesI(i) != led[i]){
						different = true;
						break;
					}
				}
				
				if (different){
					char* errorMsg;
					bool errorFound = false;
					
					uint8_t errNr = 1;
					for (; errNr<7; ++errNr){
						bool isSame = true;
						for (i = 0; i<6; ++i){
							if (heaterErrorPattern[errNr][i] != led[i]){
								isSame = false;
								break;
							}
						}
						if (isSame){
							errorFound = true;
							errorMsg = heaterErrorCodes[errNr];
							break;
						}
					}
					if (errorFound){
						heaterStatus.errorMsg = errorMsg;
						printf("leds:\t%d\t%d\t%d\t%d\t%d\t%d\t|\thas power:%d\tis heating:%d\tnoPan:%d\t\telapsed Time:%d\tprev amount:%d\t\tError:%s\n",led[0],led[1],led[2],led[3],led[4],led[5], heaterStatus.hasPower,heaterStatus.isOn,heaterStatus.noPanError, daemonGetTimeValuesRunTime() - lastHeatLedTime, heatLedValuesSameCount, errorMsg);
					} else {
						printf("leds:\t%d\t%d\t%d\t%d\t%d\t%d\t|\thas power:%d\tis heating:%d\tnoPan:%d\t\telapsed Time:%d\tprev amount:%d\n",led[0],led[1],led[2],led[3],led[4],led[5], heaterStatus.hasPower,heaterStatus.isOn,heaterStatus.noPanError, daemonGetTimeValuesRunTime() - lastHeatLedTime, heatLedValuesSameCount);
						heaterStatus.errorMsg = NULL;
					}
					i = 0;
					for (; i<6; ++i){
						heaterStatus.ledValues[i] = led[i];
					}
					lastHeatLedTime = time(NULL);
					heatLedValuesSameCount = 0;
				} else {
					heatLedValuesSameCount = heatLedValuesSameCount+1;
				}
				delay(daemonGetStateDelay());
			}
		} else if (daemonGetSettingsShieldVersion() == 2 || daemonGetSettingsShieldVersion() == 3){
			uint32_t lastHeatLedTime = millis();
			uint32_t heatLedValuesSameCount = 0;
			while(daemonGetStateRunning()){
				bool different = false;
				uint8_t i = 0;
				bool newHasError = false;
				
				uint32_t leds, led1,led2,led3;
				uint32_t multiplexer = readSignPin(0);
				delayMicroseconds(200);
				leds = readSignPin(0);
				led1 = readSignPin(1);
				led2 = readSignPin(2);
				led3 = readSignPin(3);
				delayMicroseconds(200);
				daemonSetTimeValuesRunTimeMillis(millis());
				if (multiplexer == leds && leds == readSignPin(0) && led1 == readSignPin(1) && led2 == readSignPin(2) && led3 == readSignPin(3)){
					if (multiplexer==0){
						led[0] = led1;
						led[1] = led2;
						led[2] = led3;
						if (heaterStatus.hasPower && (daemonGetTimeValuesRunTimeMillis() - heaterStatus.multiplexerLastTime1) > 200){
							//multiplexer value not changed for a amount of time -> No Power
							led[3] = 0;
							led[4] = 0;
							led[5] = 0;
						}
					} else {
						led[3] = led1;
						led[4] = led2;
						led[5] = led3;
						heaterStatus.multiplexerLastTime1 = daemonGetTimeValuesRunTimeMillis();
					}
					
					if (daemonGetSettingsDebug_enabled()){printf("%d\t%d\tleds:\t%d\t%d\t%d\t%d\t%d\t%d\n",daemonGetTimeValuesRunTimeMillis(),multiplexer, led[0],led[1],led[2],led[3],led[4],led[5]);}
				} else {
					delay(daemonGetStateDelay());
					continue;
				}
				bool multiplexerChanged = false;
				if (multiplexer != heaterStatus.lastMultiplexer){
					heaterStatus.lastMultiplexer = multiplexer;
					multiplexerChanged = true;
				}
				
				for (; i<6; ++i){
					if (heaterStatus.ledValues[i] != led[i]){
						different = true;
						break;
					}
				}
				
				if (!different && heaterStatus.lastDiffMultiplexer != multiplexer){
					//OK
					if (daemonGetSettingsDebug_enabled()){printf("diff changed complete %d\n", multiplexer);}
					heaterStatus.lastDiffMultiplexer = multiplexer;
				} else {
					if (different){
						heaterStatus.lastDiffMultiplexer = multiplexer;
						if (daemonGetSettingsDebug_enabled()){
							printf("there was a difference on multiplexer %d\n", multiplexer);
							printf("before\tleds:\t%d\t%d\t%d\t%d\t%d\t%d\n",heaterStatus.ledValues[0],heaterStatus.ledValues[1],heaterStatus.ledValues[2],heaterStatus.ledValues[3],heaterStatus.ledValues[4],heaterStatus.ledValues[5]);
							printf("after\tleds:\t%d\t%d\t%d\t%d\t%d\t%d\n",led[0],led[1],led[2],led[3],led[4],led[5]);
						}
						i = 0;
						for (; i<6; ++i){
							heaterStatus.ledValues[i] = led[i];
						}
						delay(daemonGetStateDelay());
						continue;
					} else if (!multiplexerChanged){
						//if (daemonGetSettingsDebug_enabled()){printf("multiplexer not changed %d\n", multiplexer);}
						delay(daemonGetStateDelay());
						continue;
					}
				}
				if (daemonGetSettingsDebug_enabled()){printf("do check\n");}
				if (led[IND_LED_POWER]){
					heaterStatus.hasPowerLedOnLastTime = daemonGetTimeValuesRunTimeMillis();
					if (!heaterStatus.hasPower){
						heaterStatus.hasPower = true;
						different = true;
					}
				} else {
					heaterStatus.hasPowerLedOffLastTime = daemonGetTimeValuesRunTimeMillis();
					if (heaterStatus.hasPower && ((daemonGetTimeValuesRunTimeMillis() - heaterStatus.hasPowerLedOnLastTime) > 1000 || (daemonGetTimeValuesRunTimeMillis() - heaterStatus.multiplexerLastTime1) > 200)){ //if not heating, blinking in about 500ms cycles
						heaterStatus.hasPower = false;
						different = true;
						
						//reset all values, there could not be any state if no power
						heaterStatus.isOn = false;
						heaterStatus.isModeHeating = false;
						heaterStatus.isModeKeepwarm = false;
						heaterStatus.level = 0;
					}
				}
				if (heaterStatus.hasPower){
					if(led[IND_LED_TEMP_MAX] ^ led[IND_LED_TEMP_MIDDLE] ^ led[IND_LED_TEMP_MIN]){
						//one(only one!) of the level leds are on
						if (led[IND_LED_MODE_HEATING] ^ led[IND_LED_MODE_KEEPWARM]){
							heaterStatus.isOnLastTime = daemonGetTimeValuesRunTimeMillis();
							heaterStatus.errorMsg = NULL;
							if (!heaterStatus.isOn || heaterStatus.hasError){
								heaterStatus.isOn = true;
								heaterStatus.isModeHeating = led[IND_LED_MODE_HEATING];
								heaterStatus.isModeKeepwarm = led[IND_LED_MODE_KEEPWARM];
								different = true;
							}
							if(led[IND_LED_TEMP_MAX]){
								heaterStatus.level = 3;
							} else if(led[IND_LED_TEMP_MIDDLE]){
								heaterStatus.level = 2;
							} else if(led[IND_LED_TEMP_MIN]){
								heaterStatus.level = 1;
							}
						} else {
							newHasError = true;
						}
					} else {
						newHasError = true;
					}
				}
				char* errorMsg = NULL;
				if(newHasError){
					if(heaterStatus.hasPower){
						if (heaterStatus.isOn && (daemonGetTimeValuesRunTimeMillis() - heaterStatus.hasPowerLedOffLastTime) < 1000){ //if not heating, blinking in about 500ms cycles
							if (daemonGetSettingsDebug_enabled()){printf("Set isOn to false\n");}
							heaterStatus.isOn = false;
							different = true;
						} else if (!heaterStatus.isOn && (daemonGetTimeValuesRunTimeMillis() - heaterStatus.hasPowerLedOffLastTime) > 1000){
							//This one is needed for correct error handling, if it is set to false in couse of error, but is still on
							if (daemonGetSettingsDebug_enabled()){printf("Set isOn to true\n");}
							heaterStatus.isOn = true;
							different = true;
						}
					}
					if (heaterStatus.isOn){
						if(different || !heaterStatus.hasError || (daemonGetTimeValuesRunTimeMillis() - heaterStatus.errorLastTime) > 5000){
							if(!led[IND_LED_MODE_HEATING] && !led[IND_LED_MODE_KEEPWARM] && !led[IND_LED_TEMP_MAX] && !led[IND_LED_TEMP_MIDDLE] && !led[IND_LED_TEMP_MIN]){
								//Error leds off blink state
								if (daemonGetSettingsDebug_enabled()){printf("Error leds off blink state\n");}
								heaterStatus.ledsOffBlinkState = true;
							} else {
								//There could only be on error at a time
								heaterStatus.hasError = true;
								heaterStatus.ledsOffBlinkState = false;
								heaterStatus.level = 0;
								heaterStatus.isModeHeating = false;
								heaterStatus.isModeKeepwarm = false;
								different = true;
								
								if (daemonGetSettingsDebug_enabled()){printf("check witch error it is...\n");}
								if(led[IND_LED_MODE_HEATING] && led[IND_LED_MODE_KEEPWARM] && led[IND_LED_TEMP_MAX] && led[IND_LED_TEMP_MIDDLE] && led[IND_LED_TEMP_MIN]){
									heaterStatus.noPanError = true;
									errorMsg = "No Pan";
								} else if(led[IND_LED_TEMP_MAX] && led[IND_LED_TEMP_MIDDLE] && led[IND_LED_TEMP_MIN]){
									heaterStatus.IGBTTempToHeightError = true;
									errorMsg = "IGBT Temperature To High";
								} else if(led[IND_LED_TEMP_MAX] && !led[IND_LED_TEMP_MIDDLE] && !led[IND_LED_TEMP_MIN]){
									heaterStatus.tempSensorError = true;
									errorMsg = "Temperature Sensor out of work";
								} else if(!led[IND_LED_TEMP_MAX] && led[IND_LED_TEMP_MIDDLE] && led[IND_LED_TEMP_MIN]){
									heaterStatus.IGBTSensorError = true;
									errorMsg = "IGBT Sensor out of work";
								} else if(!led[IND_LED_TEMP_MAX] && led[IND_LED_TEMP_MIDDLE] && !led[IND_LED_TEMP_MIN]){
									heaterStatus.voltageToHeightError = true;
									errorMsg = "Voltage To High(>260V)";
								} else if(!led[IND_LED_TEMP_MAX] && !led[IND_LED_TEMP_MIDDLE] && led[IND_LED_TEMP_MIN]){
									heaterStatus.voltageToLowError = true;
									errorMsg = "Voltage to Low(<85V)";
								} else if(led[IND_LED_TEMP_MAX] && led[IND_LED_TEMP_MIDDLE] && !led[IND_LED_TEMP_MIN]){
									heaterStatus.bowOutOfWaterError = true;
									errorMsg = "The bow out of water";
								}
								heaterStatus.errorLastTime = daemonGetTimeValuesRunTimeMillis();
							}
						}
					}
				} else {
					if(heaterStatus.hasError){
						heaterStatus.hasError = false;
						heaterStatus.noPanError = false;
						heaterStatus.IGBTTempToHeightError = false;
						heaterStatus.tempSensorError = false;
						heaterStatus.IGBTSensorError = false;
						heaterStatus.voltageToHeightError = false;
						heaterStatus.voltageToLowError = false;
						heaterStatus.bowOutOfWaterError = false;
						heaterStatus.ledsOffBlinkState = false;
					}
				}
				if (different){
					if (heaterStatus.hasError){
						if (errorMsg != NULL){
							heaterStatus.errorMsg = errorMsg;
						}
						if (heaterStatus.errorMsg != NULL){
							printf("leds:\t%d\t%d\t%d\t%d\t%d\t%d\t|\thas power:%d\tis heating:%d\tnoPan:%d\t\telapsed Time:%d  \tprev amount:%d\t\tError:%s\n",led[0],led[1],led[2],led[3],led[4],led[5], heaterStatus.hasPower,heaterStatus.isOn,heaterStatus.noPanError, daemonGetTimeValuesRunTimeMillis() - lastHeatLedTime, heatLedValuesSameCount, heaterStatus.errorMsg);
						}
					} else {
						printf("leds:\t%d\t%d\t%d\t%d\t%d\t%d\t|\thas power:%d\tis heating:%d\tnoPan:%d\t\telapsed Time:%d  \tprev amount:%d\n",led[0],led[1],led[2],led[3],led[4],led[5], heaterStatus.hasPower,heaterStatus.isOn,heaterStatus.noPanError, daemonGetTimeValuesRunTimeMillis() - lastHeatLedTime, heatLedValuesSameCount);
						heaterStatus.errorMsg = NULL;
					}
					i = 0;
					for (; i<6; ++i){
						heaterStatus.ledValues[i] = led[i];
					}
					lastHeatLedTime = daemonGetTimeValuesRunTimeMillis();
					heatLedValuesSameCount = 0;
				} else {
					heatLedValuesSameCount = heatLedValuesSameCount+1;
				}
				delay(daemonGetStateDelay());
			}
		} else {
			printf("No longer China Board (on shieldVersion %d), so no need to read heater-leds. Logic now on ATMega644 firmware\n", daemonGetSettingsShieldVersion());
		}
}
	
uint8_t heaterAtmelGetHeatingOutputLevel(){
	SPIAtmelWrite(SPI_MODE_GET_HEATING_OUTPUT_LEVEL);
return getResult();
}



//Private function
void *heaterLedEvaluation(void *ptr){
	if (daemonGetRunningModeSimulationMode()){//if simulation mode
		heaterStatus.hasPower = true;
		heaterStatus.noPanError = false;
		while (daemonGetStateRunning()){
			heaterStatus.isOn = heaterGetPowerStatus();
			delay(500);
		}
		return 0;
	}
	if (daemonGetSettingsShieldVersion() == 1){//if n 1 shield Version
		uint32_t lastTime_V1[7];
		while (daemonGetStateRunning()){
			uint32_t runTime = time(NULL);
			//return high or low
			heaterStatus.ledValues[0] = readSignPin(0);
			heaterStatus.ledValues[1] = readSignPin(1);
			heaterStatus.ledValues[2] = readSignPin(2);
			heaterStatus.ledValues[3] = readSignPin(3);
			heaterStatus.ledValues[4] = readSignPin(4);
			heaterStatus.ledValues[5] = readSignPin(5);
			
			if (heaterStatus.ledValues[0] && heaterStatus.ledValues[1] && heaterStatus.ledValues[2]  && heaterStatus.ledValues[3] && heaterStatus.ledValues[4]  && heaterStatus.ledValues[5]){ //all leds are turn on
				if (heaterStatus.hasPower && heaterStatus.hasPowerLedOnLastTime + 4 < daemonGetTimeValuesRunTime()){
					heaterStatus.hasPower = false;
				}
			} else {// all leds are not turn on
				heaterStatus.hasPowerLedOnLastTime = daemonGetTimeValuesRunTime();//initialization of time
				if (!heaterStatus.hasPower){
					heaterStatus.hasPower = true;
				}
			}
			
			if (heaterStatus.ledValues[3] == 0){
				heaterStatus.isOnLastTime = daemonGetTimeValuesRunTime();//initialization of time
				if (!heaterStatus.isOn){
					heaterStatus.isOn = true;
				}
			} else if (heaterStatus.isOn && heaterStatus.isOnLastTime + 4 < daemonGetTimeValuesRunTime()){
				heaterStatus.isOn = false;
			}
			
			
			bool errorFound = false;
			if(heaterCheckIsState(&heaterStatus.ledValues[0], 0, &heaterStatus.noPanError, &lastTime_V1[0], runTime)){
				heaterStatus.errorMsg = heaterErrorCodes[0];
				errorFound = true;
			}
			
			if(heaterCheckIsState(&heaterStatus.ledValues[0], 1, &heaterStatus.tempSensorError, &lastTime_V1[1], runTime)){
				heaterStatus.errorMsg = heaterErrorCodes[1];
				errorFound = true;
			}
			if(heaterCheckIsState(&heaterStatus.ledValues[0], 2, &heaterStatus.IGBTSensorError, &lastTime_V1[2], runTime)){
				heaterStatus.errorMsg = heaterErrorCodes[2];
				errorFound = true;
			}
			if(heaterCheckIsState(&heaterStatus.ledValues[0], 3, &heaterStatus.voltageToHeightError, &lastTime_V1[3], runTime)){
				heaterStatus.errorMsg = heaterErrorCodes[3];
				errorFound = true;
			}
			if(heaterCheckIsState(&heaterStatus.ledValues[0], 4, &heaterStatus.voltageToLowError, &lastTime_V1[4], runTime)){
				heaterStatus.errorMsg = heaterErrorCodes[4];
				errorFound = true;
			}
			if(heaterCheckIsState(&heaterStatus.ledValues[0], 5, &heaterStatus.bowOutOfWaterError, &lastTime_V1[5], runTime)){
				heaterStatus.errorMsg = heaterErrorCodes[5];
				errorFound = true;
			}
			if(heaterCheckIsState(&heaterStatus.ledValues[0], 6, &heaterStatus.IGBTTempToHeightError, &lastTime_V1[6], runTime)){
				heaterStatus.errorMsg = heaterErrorCodes[6];
				errorFound = true;
			}
			
			if (!errorFound){//si error
				heaterStatus.errorMsg = NULL;
			}
			//if (daemonGetSettingsDebug_enabled()){
			//	printf("leds:\t%d\t%d\t%d\t%d\t%d\t%d\t|\thas power:%d\tis heating:%d\tNoPan:%d\t\tError:%s\n",heaterStatus.ledValues[0],heaterStatus.ledValues[1],heaterStatus.ledValues[2],heaterStatus.ledValues[3],heaterStatus.ledValues[4],heaterStatus.ledValues[5], heaterStatus.hasPower,heaterStatus.isHeating,heaterStatus.noPan, heaterStatus.errorMsg);
			//}
			delay(10);
		}
	} else if (daemonGetSettingsShieldVersion() == 2 || daemonGetSettingsShieldVersion() == 3){//if n 2 or 3 shield Version
		uint32_t led[6];
		led[0] = 0;
		led[1] = 0;
		led[2] = 0;
		led[3] = 0;
		led[4] = 0;
		led[5] = 0;
		
		uint32_t leds, led1,led2,led3;
		uint32_t updateDelay = 10;
		uint32_t runTimeMillis;
		while (daemonGetStateRunning()){
			bool different = false;
			uint8_t i = 0;
			bool newHasError = false;
			
			uint32_t multiplexer = readSignPin(0);
			delayMicroseconds(200);
			leds = readSignPin(0);
			led1 = readSignPin(1);
			led2 = readSignPin(2);
			led3 = readSignPin(3);
			delayMicroseconds(200);
			runTimeMillis = millis();
			if (multiplexer == leds && leds == readSignPin(0) && led1 == readSignPin(1) && led2 == readSignPin(2) && led3 == readSignPin(3)){
				if (multiplexer==0){
					led[0] = led1;
					led[1] = led2;
					led[2] = led3;
					if (heaterStatus.hasPower && (runTimeMillis - heaterStatus.multiplexerLastTime1) > 200){
						//multiplexer value not changed for a amount of time -> No Power
						led[3] = 0;
						led[4] = 0;
						led[5] = 0;
					}
				} else {
					led[3] = led1;
					led[4] = led2;
					led[5] = led3;
					heaterStatus.multiplexerLastTime1 = runTimeMillis;
				}
				
				if (daemonGetSettingsDebug_enabled()){printf("%d\t%d\tleds:\t%d\t%d\t%d\t%d\t%d\t%d\n",runTimeMillis,multiplexer, led[0],led[1],led[2],led[3],led[4],led[5]);}
			} else {
				delay(updateDelay);
				continue;
			}
			bool multiplexerChanged = false;
			if (multiplexer != heaterStatus.lastMultiplexer){
				heaterStatus.lastMultiplexer = multiplexer;
				multiplexerChanged = true;
			}
			
			for (; i<6; ++i){
				if (heaterStatus.ledValues[i] != led[i]){
					different = true;
					break;
				}
			}
			
			if (!different && heaterStatus.lastDiffMultiplexer != multiplexer){
				//OK
				if (daemonGetSettingsDebug_enabled()){printf("diff changed complete %d\n", multiplexer);}
				heaterStatus.lastDiffMultiplexer = multiplexer;
			} else {
				if (different){
					heaterStatus.lastDiffMultiplexer = multiplexer;
					if (daemonGetSettingsDebug_enabled()){
						printf("there was a difference on multiplexer %d\n", multiplexer);
						printf("before\tleds:\t%d\t%d\t%d\t%d\t%d\t%d\n",heaterStatus.ledValues[0],heaterStatus.ledValues[1],heaterStatus.ledValues[2],heaterStatus.ledValues[3],heaterStatus.ledValues[4],heaterStatus.ledValues[5]);
						printf("after\tleds:\t%d\t%d\t%d\t%d\t%d\t%d\n",led[0],led[1],led[2],led[3],led[4],led[5]);
					}
					i = 0;
					for (; i<6; ++i){
						heaterStatus.ledValues[i] = led[i];
					}
					delay(updateDelay);
					continue;
				} else if (!multiplexerChanged){
					//if (daemonGetSettingsDebug_enabled()){printf("multiplexer not changed %d\n", multiplexer);}
					delay(updateDelay);
					continue;
				}
			}
			
			if (daemonGetSettingsDebug_enabled()){printf("do check\n");}
			if (led[IND_LED_POWER]){
				heaterStatus.hasPowerLedOnLastTime = runTimeMillis;
				if (!heaterStatus.hasPower){
					heaterStatus.hasPower = true;
					different = true;
				}
			} else {
				heaterStatus.hasPowerLedOffLastTime = runTimeMillis;
				if (heaterStatus.hasPower && ((runTimeMillis - heaterStatus.hasPowerLedOnLastTime) > 1000 || (runTimeMillis - heaterStatus.multiplexerLastTime1) > 200)){ //if not heating, blinking in about 500ms cycles
					heaterStatus.hasPower = false;
					different = true;
					
					//reset all values, there could not be any state if no power
					heaterStatus.isOn = false;
					heaterStatus.isModeHeating = false;
					heaterStatus.isModeKeepwarm = false;
					heaterStatus.level = 0;
				}
			}
			if (heaterStatus.hasPower){
				if(led[IND_LED_TEMP_MAX] ^ led[IND_LED_TEMP_MIDDLE] ^ led[IND_LED_TEMP_MIN]){
					//one(only one!) of the level leds are on
					if (led[IND_LED_MODE_HEATING] ^ led[IND_LED_MODE_KEEPWARM]){
						heaterStatus.isOnLastTime = runTimeMillis;
						heaterStatus.errorMsg = NULL;
						if (!heaterStatus.isOn || heaterStatus.hasError){
							heaterStatus.isOn = true;
							heaterStatus.isModeHeating = led[IND_LED_MODE_HEATING];
							heaterStatus.isModeKeepwarm = led[IND_LED_MODE_KEEPWARM];
							different = true;
						}
						if(led[IND_LED_TEMP_MAX]){
							heaterStatus.level = 3;
						} else if(led[IND_LED_TEMP_MIDDLE]){
							heaterStatus.level = 2;
						} else if(led[IND_LED_TEMP_MIN]){
							heaterStatus.level = 1;
						}
					} else {
						newHasError = true;
					}
				} else {
					newHasError = true;
				}
			}
			
			if(newHasError){
				if(heaterStatus.hasPower){
					if (heaterStatus.isOn && (runTimeMillis - heaterStatus.hasPowerLedOffLastTime) < 1000){ //if not heating, blinking in about 500ms cycles
						if (daemonGetSettingsDebug_enabled()){printf("Set isOn to false\n");}
						heaterStatus.isOn = false;
						different = true;
					} else if (!heaterStatus.isOn && (runTimeMillis - heaterStatus.hasPowerLedOffLastTime) > 1000){
						//This one is needed for correct error handling, if it is set to false in couse of error, but is still on
						if (daemonGetSettingsDebug_enabled()){printf("Set isOn to true\n");}
						heaterStatus.isOn = true;
						different = true;
					}
				}
				if (heaterStatus.isOn){
					if(different || !heaterStatus.hasError || (runTimeMillis - heaterStatus.errorLastTime) > 5000){
						if(!led[IND_LED_MODE_HEATING] && !led[IND_LED_MODE_KEEPWARM] && !led[IND_LED_TEMP_MAX] && !led[IND_LED_TEMP_MIDDLE] && !led[IND_LED_TEMP_MIN]){
							//Error leds off blink state
							if (daemonGetSettingsDebug_enabled()){printf("Error leds off blink state\n");}
							heaterStatus.ledsOffBlinkState = true;
						} else {
							//There could only be on error at a time
							heaterStatus.hasError = true;
							heaterStatus.ledsOffBlinkState = false;
							heaterStatus.level = 0;
							heaterStatus.isModeHeating = false;
							heaterStatus.isModeKeepwarm = false;
							different = true;
							
							if (daemonGetSettingsDebug_enabled()){printf("check witch error it is...\n");}
							if(led[IND_LED_MODE_HEATING] && led[IND_LED_MODE_KEEPWARM] && led[IND_LED_TEMP_MAX] && led[IND_LED_TEMP_MIDDLE] && led[IND_LED_TEMP_MIN]){
								heaterStatus.noPanError = true;
								heaterStatus.errorMsg = "No Pan";
							} else if(led[IND_LED_TEMP_MAX] && led[IND_LED_TEMP_MIDDLE] && led[IND_LED_TEMP_MIN]){
								heaterStatus.IGBTTempToHeightError = true;
								heaterStatus.errorMsg = "IGBT Temperature To High";
							} else if(led[IND_LED_TEMP_MAX] && !led[IND_LED_TEMP_MIDDLE] && !led[IND_LED_TEMP_MIN]){
								heaterStatus.tempSensorError = true;
								heaterStatus.errorMsg = "Temperature Sensor out of work";
							} else if(!led[IND_LED_TEMP_MAX] && led[IND_LED_TEMP_MIDDLE] && led[IND_LED_TEMP_MIN]){
								heaterStatus.IGBTSensorError = true;
								heaterStatus.errorMsg = "IGBT Sensor out of work";
							} else if(!led[IND_LED_TEMP_MAX] && led[IND_LED_TEMP_MIDDLE] && !led[IND_LED_TEMP_MIN]){
								heaterStatus.voltageToHeightError = true;
								heaterStatus.errorMsg = "Voltage To High(>260V)";
							} else if(!led[IND_LED_TEMP_MAX] && !led[IND_LED_TEMP_MIDDLE] && led[IND_LED_TEMP_MIN]){
								heaterStatus.voltageToLowError = true;
								heaterStatus.errorMsg = "Voltage to Low(<85V)";
							} else if(led[IND_LED_TEMP_MAX] && led[IND_LED_TEMP_MIDDLE] && !led[IND_LED_TEMP_MIN]){
								heaterStatus.bowOutOfWaterError = true;
								heaterStatus.errorMsg = "The bow out of water";
							}
							heaterStatus.errorLastTime = runTimeMillis;
						}
					}
				}
			} else {
				if(heaterStatus.hasError){
					heaterStatus.hasError = false;
					heaterStatus.noPanError = false;
					heaterStatus.IGBTTempToHeightError = false;
					heaterStatus.tempSensorError = false;
					heaterStatus.IGBTSensorError = false;
					heaterStatus.voltageToHeightError = false;
					heaterStatus.voltageToLowError = false;
					heaterStatus.bowOutOfWaterError = false;
					heaterStatus.ledsOffBlinkState = false;
				}
			}
			delay(updateDelay);
		}
	} else if (daemonGetSettingsShieldVersion() >= 4){
		printf("No longer China Board (on shieldVersion %d), so no need to read heater-leds. Logic now on ATMega644 firmware\n", daemonGetSettingsShieldVersion());
	}
	return 0;
}

bool heaterCheckIsState(uint32_t* leds, uint8_t errNo, bool* state, uint32_t* lastTime, uint32_t runTime){//checkIsHeaterState
	bool isSame = true;
	uint8_t i;
	for (i = 0; i<6; ++i){
		if (heaterErrorPattern[errNo][i] != leds[i]){
			isSame = false;
			break;
		}
	}
	if (isSame){
		*lastTime = runTime;
		*state = true;
	} else {
		if (*state && *lastTime + heaterErrorTimeout[errNo] < runTime){
			*state = false;
		}
	}
	return *state;
}

void heaterheaterAtmelSetHeating(bool on){
	if (atmelGetDebug2()) {printf("--->heaterAtmelSetHeating\n");}
	SPIAtmelWrite(SPI_MODE_HEATING);
	if(on){
		SPIAtmelWrite(0x01);
	} else{
		SPIAtmelWrite(0x00);
	}
	getValidResultOrReset();
}

// Set function
void heaterSetStatusErrorMsg(char* msg){
	heaterStatus.errorMsg=msg;
}
void heaterSetStatusIsOn(bool ison){
	heaterStatus.isOn=ison;
}
void heaterSetStatusHasError(bool error){
	heaterStatus.hasError=error;
}
void heaterSetPowerStatus(bool status){
	heaterPowerStatus=status;
}
void heaterSetStatusIsOnLastTime(uint32_t lasttime){
	heaterStatus.isOnLastTime=lasttime;
}
void heaterSetStatusHasPower(bool haspower){
	heaterStatus.hasPower=haspower;
}
void heaterSetStatusHasPowerLedOnLastTime(uint32_t lastime){
	heaterStatus.hasPowerLedOnLastTime=lastime;
}
void heaterSetStatusNoPanError(bool paserror){
	heaterStatus.noPanError=paserror;
}
void heaterSetHourCounter(uint32_t hourcounter){
	heaterHourCounter=hourcounter;
}
void heaterSetStatusLedValuesI(uint8_t i,uint32_t ledvalues){
	heaterStatus.ledValues[i]=ledvalues;
}

// Get function
bool heaterGetStatusIsOn(){
	return heaterStatus.isOn;
}
bool heaterGetStatusIsOnLastTime(){
	return heaterStatus.isOnLastTime;
}
bool heaterGetStatusHasPower(){
	return heaterStatus.hasPower;
}
bool heaterGetStatusHasPowerLedOnLastTime(){
	return heaterStatus.hasPowerLedOnLastTime;
}
bool heaterGetStatusNoPanError(){
	return heaterStatus.noPanError;
}
uint32_t heaterGetStatusLedValuesI(uint8_t i){
	return heaterStatus.ledValues[i];
}
bool heaterGetPowerStatus(){
	return heaterPowerStatus;
}
uint32_t heaterGetHourCounter(){
	return heaterHourCounter;
}
