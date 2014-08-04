#ifndef NEWMODE_C_
#define NEWMODE_C_
#include "newMode.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include <math.h>
#include <stdarg.h>
#include "daemon.c"
#include "solenoid.h"
#include "display.h"
#include "newMode.h"

#define HELP 0
#define QUIT 1
#define TEST_SERVO 2
#define TEST_TEMPPRESS 3
#define TEST_VALVE 4
#define TEST_BOUTON 5
#define TEST_DISPLAY 6
#define TEST_FAN 7
#define TEST_INDUCTION 8
#define TEST_MOTOR 9
#define TEST_WEIGHT 10

#define WIDHTCOLUMN 10
pthread_t readInput;// thread that read input on keyboard

int newModeMain();
void printColumnVAr( uint16_t widthColumn, uint16_t nbColumn,...);
void printMiddle(uint16_t space, uint32_t nb);
void *readInputFunction(void *ptr);
void helpPrintf();
void quitPrintf();
void testServoPrintf();
void testTempPressPrintf();
void testValvePrintf();
void testBoutonPrintf();
void testInductionPrintf();
void testSDisplayPrintf();
void testFanPrintf();
void testMotorPrintf();
void testWeightPrintf();

char * modeNom = "mode test"; //name of the mode for the display
int modeNum = 0; //number of mode for the switch
int running=1;
//TEST_FAN
int fanPwm=0;
int fantemp=0;
//TEST_MOTOR
int motorRPMTrue=0;
int motorRPMDesired=0;
int motorSensor=0;
//TEST_INDUCTION
int inductionPwm=0;
bool inductionIsRunning=false;
//TEST_BOUTON
bool boutonState[6]={false,false,false,false,false,false};
//TEST_DISPLAY
bool displayLedShined[8]={false,false,false,false,false,false,false,false};
int displayMode=0;
//TEST_VALVE
int solenoidPwm=0;
bool solenoidIsOpen=false;
//TEST_WEIGHT
#define NBWEIGHTLINE 25
struct Weight_Data {
	uint32_t frontL;
	uint32_t frontR;
	uint32_t backL;
	uint32_t backR;
	uint32_t average;
	uint32_t averageG;
};
struct Weight_Data_tab{
	struct Weight_Data max;
	struct	Weight_Data min;
	struct Weight_Data delta;
	struct	Weight_Data avg;
	uint32_t nbData;
};
struct Weight_Data allWeightData[NBWEIGHTLINE]; 
struct Weight_Data newWeightData;
struct Weight_Data_tab tabWeightData;
//TEST_TEMPPRESS
#define NBTEMPPRESSLINE 25
struct Temppress_Data{
	uint32_t tempInput;
	uint32_t tempDeg;
	uint32_t pressInput;
	uint32_t pressPasc;
};
struct Temppress_Data_tab{
	struct Temppress_Data max;
	struct	Temppress_Data min;
	struct Temppress_Data delta;
	struct	Temppress_Data avg;
	uint32_t nbData;
};
struct Temppress_Data allTemppressData[NBTEMPPRESSLINE]; 
struct Temppress_Data newTemppressData;
struct Temppress_Data_tab tabTemppressData;


int newModeMain(){
	srand(time(NULL));
	int t=pthread_create(&readInput, NULL, readInputFunction, NULL);
	if(t==0){
		printf("merde");
	}
	while(running){
		system("clear");
		printf("\n********************************************************************************");
		printf("\n Help : h  ********* Mode actuel : %s",modeNom);
		printf("\n********************************************************************************");
		switch (modeNum){
			case HELP :
				helpPrintf();
			break;
			case QUIT  :
				quitPrintf();
			break;
			case TEST_SERVO  :
				testServoPrintf();
			break;
			case TEST_TEMPPRESS  :
				testTempPressPrintf();
			break;
			case TEST_VALVE  :
				testValvePrintf();
			break;
			case TEST_BOUTON  :
				testBoutonPrintf();
			break;
			case TEST_DISPLAY  :
				testSDisplayPrintf();
			break;
			case TEST_FAN  :
				testFanPrintf();
			break;
			case TEST_INDUCTION  :
				testInductionPrintf();
			break;
			case TEST_MOTOR  :
				testMotorPrintf();
			break;
			case TEST_WEIGHT  :
				testWeightPrintf();
			break;
			default :
				printf("ERROR = No modeNum");
			break;
			}
		printf("\nWhat do you want to do : ");
		printf("\n");
		usleep(1000000);
		
	};
	return 0;
}

void *readInputFunction(void *ptr){
	int isNumber;
	while(running){
	char str[50]; // 50 caractères + '\0' terminal
	fgets(str, 50, stdin);
	str[strlen(str)-1]='\0'; //enlève le '\n'
	char* temp= &str;
	 switch(*temp)
    {
		case 'h' :
        case 'H' :
			modeNom="help";
			modeNum=HELP;
            break;
        case 's' :
        case 'S' :
			modeNom="test servo";
			modeNum=TEST_SERVO;
            break;
        case 't' :
        case 'T' :
			modeNom="test temp/press";
			modeNum=TEST_TEMPPRESS;
			tabTemppressData.nbData=0;
            break;
        case 'v' :
        case 'V' :
			modeNom="test valve";
			modeNum=TEST_VALVE;
            break;
        case 'b' :
        case 'B' :
			modeNom="test bouton";
			modeNum=TEST_BOUTON;
            break;
        case 'd' :
        case 'D' :
			modeNom="test display";
			modeNum=TEST_DISPLAY;
            break;
        case 'q' :
        case 'Q' :
			modeNom="quit";
			modeNum=QUIT;
            break;
		case 'f' :
        case 'F' :
			modeNom="test fan";
			modeNum=TEST_FAN;
			break;
		case 'i' :
        case 'I' :
			modeNom="test induction";
			modeNum=TEST_INDUCTION;
			break;
		case 'm' :
        case 'M' :
			modeNom="test motor";
			modeNum=TEST_MOTOR;
			break;
		case 'w' :
        case 'W' :
			modeNom="test weight";
			modeNum=TEST_WEIGHT;
			tabWeightData.nbData=0;
            break;
        case 'p' :
        case 'P' :
			displayMode=4;
			displayShowText("I love everycook");
        break;
		case 'y' :
        case 'Y' :
			switch (modeNum){
				case QUIT :
					running=0;
				break;
				case TEST_DISPLAY :
					displayMode=1;
					displayFill();
				break;
				default :
				break;
			}
		break;
		case 'n' :
        case 'N' :
			switch (modeNum){
				case QUIT :
					modeNom="help";
					modeNum=HELP;
				break;
				case TEST_DISPLAY :
					displayMode=0;
					displayClear();
				break;
				default :
				break;
			}
		break;
        case 'o' :
        case 'O' :
			switch (modeNum){
				case TEST_VALVE :
					solenoidSetOpen(true);
					solenoidIsOpen=true;
				break;
				default :
				break;
			}
		break;
		case 'c' :
		case 'C' :
			switch (modeNum){
				case TEST_VALVE :
					solenoidSetOpen(false);
					solenoidIsOpen=false;
				break;
				default :
				break;
			}
		break;
		case '0':
			switch (modeNum){
				case TEST_VALVE :
					solenoidPwm=0;
					//PWM 0 pour servo moteur
				break;
				case TEST_INDUCTION :
					inductionPwm=0;
					heaterOff();
				break;
				case TEST_FAN :
					fanPwm=0;
					//PWM 0 of Fan
				break;
				case TEST_MOTOR:
					motorRPMDesired=0;
					motorSetCommandRPM(0);
				break;
				default:
				break;
			}
		
		break;
		default :
		break;
    }
		isNumber=atoi(temp);
		if(isNumber!=0){
			switch (modeNum){
				case TEST_VALVE :
					solenoidPwm=isNumber;
					//PWM pour servo moteur
				break;
				case TEST_DISPLAY:
					if(isNumber==1){
						if(displayLedShined[isNumber-1]){
							displaySetTop(false);
						}else{
							displaySetTop(true);
						}
						displayMode=2;
						if(daemonGetSettingsShieldVersion()>3){
							uint16_t picture2[9]  = {	0b0010101010101010,
											0b0001010101010101,
											0b0010101010101010,
											0b0001010101010101,
											0b0010101010101010,
											0b0001010101010101,
											0b0010101010101010,
											0b0001010101010101,
											0b0010101010101010};
							displayShowPicture(&picture2[0]);
						}
						displayLedShined[isNumber-1]=!displayLedShined[isNumber-1];
					}else if(isNumber==2){
						if(displayLedShined[isNumber-1]){
							displaySetTopLeft(false);
						}else{
							displaySetTopLeft(true);
						}
						displayMode=3;
						if(daemonGetSettingsShieldVersion()>3){
							uint16_t picture[9] = {	0b0000000111110000,
										0b0000001000001000,
										0b0000010000000100,
										0b0000010100010100,
										0b0000010000000100,
										0b0000010100010100,
										0b0000010011100100,
										0b0000001000001000,
										0b0000000111110000};
							displayShowPicture(&picture[0]);
						}
						displayLedShined[isNumber-1]=!displayLedShined[isNumber-1];
					}else if(isNumber==3){
						if(displayLedShined[isNumber-1]){
							displaySetBottomLeft(false);
						}else{
							displaySetBottomLeft(true);
						}
						displayLedShined[isNumber-1]=!displayLedShined[isNumber-1];
					}else if(isNumber==4){
						if(displayLedShined[isNumber-1]){
							displaySetBottom(false);
						}else{
							displaySetBottom(true);
						}
						displayLedShined[isNumber-1]=!displayLedShined[isNumber-1];
					}else if(isNumber==5){
						if(displayLedShined[isNumber-1]){
							displaySetBottomRight(false);
						}else{
							displaySetBottomRight(true);
						}
						displayLedShined[isNumber-1]=!displayLedShined[isNumber-1];
					}else if(isNumber==6){
						if(displayLedShined[isNumber-1]){
							displaySetTopRight(false);
						}else{
							displaySetTopRight(true);
						}
						displayLedShined[isNumber-1]=!displayLedShined[isNumber-1];
					}else if(isNumber==7){
						if(displayLedShined[isNumber-1]){
							displaySetCenter(false);
						}else{
							displaySetCenter(true);
						}
						displayLedShined[isNumber-1]=!displayLedShined[isNumber-1];
					}else if(isNumber==8){
						if(displayLedShined[isNumber-1]){
							displaySetPeriod(false);
						}else{
							displaySetPeriod(true);
						}
						displayLedShined[isNumber-1]=!displayLedShined[isNumber-1];
					} 
				break;
				case TEST_INDUCTION :
					inductionPwm=isNumber;
					//PWM pour le heater
				break;
				case TEST_FAN :
					fanPwm=isNumber;
					//PWM of Fan
				break;
				case TEST_MOTOR:
						motorRPMDesired=isNumber;
						//RPM of Motor
				break;
				default :
				break;
			}
		}
	}
	return 0;
}

void helpPrintf(){
	printf("\nMode enable :");
	printf("\ns : test servo");
	printf("\nt : test temp/press");
	printf("\nb : test bouton");
	printf("\nd : test display");
	printf("\nf : test fan");
	printf("\ni : test induction");
	printf("\nm : test motor");
	printf("\nw : test weight");
	printf("\nv : test valve");
	printf("\nq : quit");
}
void quitPrintf(){
	printf("Do you want to quit? (y,n)");
}
void testServoPrintf(){
}
void testTempPressPrintf(){
	int i;
	newTemppressData.pressInput=adc_values.Press.adc_value;
	newTemppressData.tempInput=adc_values.Temp.adc_value;
	newTemppressData.pressPasc=adc_values.Press.valueByOffset;
	newTemppressData.tempDeg=adc_values.Temp.valueByOffset;
	if(tabTemppressData.nbData<NBTEMPPRESSLINE){
		allTemppressData[tabTemppressData.nbData].pressInput=newTemppressData.pressInput;
		allTemppressData[tabTemppressData.nbData].tempInput=newTemppressData.tempInput;
		allTemppressData[tabTemppressData.nbData].pressPasc=newTemppressData.pressPasc;
		allTemppressData[tabTemppressData.nbData].tempDeg=newTemppressData.tempDeg;
	}else{
		for(i=1;i<NBTEMPPRESSLINE;i++){
			allTemppressData[i-1].pressInput=allTemppressData[i].pressInput;
			allTemppressData[i-1].tempInput=allTemppressData[i].tempInput;
			allTemppressData[i-1].pressPasc=allTemppressData[i].pressPasc;
			allTemppressData[i-1].tempDeg=allTemppressData[i].tempDeg;

		}
		allTemppressData[NBTEMPPRESSLINE-1].pressInput=newTemppressData.pressInput;
		allTemppressData[NBTEMPPRESSLINE-1].tempInput=newTemppressData.tempInput;
		allTemppressData[NBTEMPPRESSLINE-1].pressPasc=newTemppressData.pressPasc;
		allTemppressData[NBTEMPPRESSLINE-1].tempDeg=newTemppressData.tempDeg;
	}
	if(tabTemppressData.nbData==0){
		tabTemppressData.max=newTemppressData;
		tabTemppressData.min=newTemppressData;
		tabTemppressData.avg=newTemppressData;
	}else{
		if(tabTemppressData.min.tempInput>newTemppressData.tempInput){
			tabTemppressData.min.tempInput=newTemppressData.tempInput;
		}
		if(tabTemppressData.min.tempDeg>newTemppressData.tempDeg){
			tabTemppressData.min.tempDeg=newTemppressData.tempDeg;
		}
		if(tabTemppressData.min.pressInput>newTemppressData.pressInput){
			tabTemppressData.min.pressInput=newTemppressData.pressInput;
		}
		if(tabTemppressData.min.pressPasc>newTemppressData.pressPasc){
			tabTemppressData.min.pressPasc=newTemppressData.pressPasc;
		}
		if(tabTemppressData.max.tempInput<newTemppressData.tempInput){
			tabTemppressData.max.tempInput=newTemppressData.tempInput;
		}
		if(tabTemppressData.max.tempDeg<newTemppressData.tempDeg){
			tabTemppressData.max.tempDeg=newTemppressData.tempDeg;
		}
		if(tabTemppressData.max.pressInput<newTemppressData.pressInput){
			tabTemppressData.max.pressInput=newTemppressData.pressInput;
		}
		if(tabTemppressData.max.pressPasc<newTemppressData.pressPasc){
			tabTemppressData.max.pressPasc=newTemppressData.pressPasc;
		}

		tabTemppressData.delta.tempInput=tabTemppressData.max.tempInput-tabTemppressData.min.tempInput;
		tabTemppressData.delta.tempDeg=tabTemppressData.max.tempDeg-tabTemppressData.min.tempDeg;
		tabTemppressData.delta.pressInput=tabTemppressData.max.pressInput-tabTemppressData.min.pressInput;
		tabTemppressData.delta.pressPasc=tabTemppressData.max.pressPasc-tabTemppressData.min.pressPasc;

		tabTemppressData.avg.tempInput=newTemppressData.tempInput+tabTemppressData.avg.tempInput;
		tabTemppressData.avg.tempDeg=newTemppressData.tempDeg+tabTemppressData.avg.tempDeg;
		tabTemppressData.avg.pressInput=newTemppressData.pressInput+tabTemppressData.avg.pressInput;
		tabTemppressData.avg.pressPasc=newTemppressData.pressPasc+tabTemppressData.avg.pressPasc;
	}
	tabTemppressData.nbData++;
	printf("\n  Value :PressInput| PressPasc| TempInput|  TempDeg |");
	
	printf("\n  Max   :");
	printColumnVAr(WIDHTCOLUMN,4,tabTemppressData.max.pressInput,tabTemppressData.max.pressPasc,tabTemppressData.max.tempInput,tabTemppressData.max.tempDeg);
	printf("\n  Min   :");
	printColumnVAr(WIDHTCOLUMN,4,tabTemppressData.min.pressInput,tabTemppressData.min.pressPasc,tabTemppressData.min.tempInput,tabTemppressData.min.tempDeg);
	printf("\n Delta  :");
	printColumnVAr(WIDHTCOLUMN,4,tabTemppressData.delta.pressInput,tabTemppressData.delta.pressPasc,tabTemppressData.delta.tempInput,tabTemppressData.delta.tempDeg);
	printf("\nAverage :");
	printColumnVAr(WIDHTCOLUMN,4,tabTemppressData.avg.pressInput/tabTemppressData.nbData,tabTemppressData.avg.pressPasc/tabTemppressData.nbData,tabTemppressData.avg.tempInput/tabTemppressData.nbData,tabTemppressData.avg.tempDeg/tabTemppressData.nbData);
	printf("\n___________________|__________|__________|__________|");
	if(tabTemppressData.nbData<NBWEIGHTLINE){
		for(i=0;i<tabTemppressData.nbData;i++){
			printf("\n         ");
			printColumnVAr(WIDHTCOLUMN,4,allTemppressData[i].pressInput,allTemppressData[i].pressPasc,allTemppressData[i].tempInput,allTemppressData[i].tempDeg);	
		}
	}else{
		for(i=0;i<NBWEIGHTLINE;i++){
			printf("\n         ");
			printColumnVAr(WIDHTCOLUMN,4,allTemppressData[i].pressInput,allTemppressData[i].pressPasc,allTemppressData[i].tempInput,allTemppressData[i].tempDeg);
		}
	}
	
}
void testValvePrintf(){
	printf("\nPress 'o' for open the valve or 'c' for close the valve.");
	if(solenoidIsOpen==true){
		printf("\nThe valve is opened");
	}else{
		printf("\nThe valve is closed");
	}
	if(daemonGetSettingsShieldVersion()<4){
		printf("\nEnter number betwenn 0 and 100 to activate the PWM : %d",solenoidPwm);
	}
}
void testBoutonPrintf(){
	boutonState[0]=readButton(0);
	boutonState[1]=readButton(1);
	boutonState[2]=readButton(2);
	boutonState[3]=state.lidClosed;
	boutonState[4]=state.lidLocked;
	boutonState[5]=state.pusherLocked;
	printf("\nBouton pressed 1 and release 0");
	if(boutonState[0]){
		printf("\nBouton 1 = 1");
	}else{
		printf("\nBouton 1 = 0");
	}
	if(boutonState[1]){
		printf("\nBouton 2 = 1");
	}else{
		printf("\nBouton 2 = 0");
	}
	if(boutonState[2]){
		printf("\nBouton 3 = 1");
	}else{
		printf("\nBouton 3 = 0");
	}
	if(boutonState[3]){
		printf("\nBouton lid closed = 1");
	}else{
		printf("\nBouton lid closed = 0");
	}
	if(boutonState[4]){
		printf("\nBouton lid locked = 1");
	}else{
		printf("\nBouton lid locked = 0");
	}
	if(boutonState[5]){
		printf("\nBouton pusher locked = 1");
	}else{
		printf("\nBouton pusher locked = 0");
	}

}
void testInductionPrintf(){
	printf("\nEnter number betwenn 0 and 100 to activate the PWM : %d",inductionPwm);
	if(inductionIsRunning){
		printf("\nThe heater is running");
	}else{
		printf("\nThe heater is not running");
	}
	//if(heaterStatus.errorMsg == NULL){
		printf("\nThere aren't error");
	//}else{
		//printf("The error is %s",heaterStatus.errorMsg);
	//}
}
void testSDisplayPrintf(){
	printf("\nthe Shield version is %d",daemonGetSettingsShieldVersion());
	if(daemonGetSettingsShieldVersion()<4){
	printf("\nwitch led would you want to switch on or switch off? (1-8)");
	if(displayLedShined[0]){
		printf("\n                       ___1___        Led shined :   1      ");
	}else{
		printf("\n                       ___1___        Led shined :          ");
	}
	if(displayLedShined[1]){
		printf("\n                      |       |                      2      ");
	}else{
		printf("\n                      |       |                             ");
	}
	if(displayLedShined[2]){
		printf("\n                     2|       |6                     3      ");
	}else{
		printf("\n                     2|       |6                            ");
	}
	if(displayLedShined[3]){
		printf("\n                      |_______|                      4      ");
	}else{
		printf("\n                      |_______|                             ");
	}
	if(displayLedShined[4]){
		printf("\n                      |   7   |                      5      ");
	}else{
		printf("\n                      |   7   |                             ");
	}
	if(displayLedShined[5]){
		printf("\n                     3|       |5                     6      ");
	}else{
		printf("\n                     3|       |5                            ");
	}
	if(displayLedShined[6]){
		printf("\n                      |_______|                      7      ");
	}else{
		printf("\n                      |_______|                             ");
	}
	if(displayLedShined[7]){
		printf("\n                          4    *8                    8      ");	
	}else{
		printf("\n                          4    *8                           ");	
	}	
	}else{
	printf("\nWould you want to turn on or off all Leds? (y/n)");
	printf("\nWhat picture would you want to show? (Chess = 1/Smilly = 2)");
	printf("\nPress 'p' to show the following text 'I love everycook' parades before your eyes");
	
	switch (displayMode){
		case 0 :
			printf("\n\n'_'=led turn off    '0'=led turn on\n");
			printf("\n     ______________");
			printf("\n     ______________");
			printf("\n     ______________");
			printf("\n     ______________");
			printf("\n     ______________");
			printf("\n     ______________");
			printf("\n     ______________");
			printf("\n     ______________");
			printf("\n     ______________");
		break;
		case 1 :
			printf("\n\n'_'=led turn off    '0'=led turn on\n");
			printf("\n     00000000000000");
			printf("\n     00000000000000");
			printf("\n     00000000000000");
			printf("\n     00000000000000");
			printf("\n     00000000000000");
			printf("\n     00000000000000");
			printf("\n     00000000000000");
			printf("\n     00000000000000");
			printf("\n     00000000000000");
		break;
		case 2 :
			printf("\n\n'_'=led turn off    '0'=led turn on\n");
			printf("\n     0_0_0_0_0_0_0_");
			printf("\n     _0_0_0_0_0_0_0");
			printf("\n     0_0_0_0_0_0_0_");
			printf("\n     _0_0_0_0_0_0_0");
			printf("\n     0_0_0_0_0_0_0_");
			printf("\n     _0_0_0_0_0_0_0");
			printf("\n     0_0_0_0_0_0_0_");
			printf("\n     _0_0_0_0_0_0_0");
			printf("\n     0_0_0_0_0_0_0_");
		break;
		case 3 :
			printf("\n\n'_'=led turn off    '0'=led turn on\n");
			printf("\n     _____00000____");
			printf("\n     ____0_____0___");
			printf("\n     ___0_______0__");
			printf("\n     ___0_0___0_0__");
			printf("\n     ___0_______0__");
			printf("\n     ___0_0___0_0__");
			printf("\n     ___0__000__0__");
			printf("\n     ____0_____0___");
			printf("\n     _____00000____");
		break;
		case 4 :
			printf("\n\nYour are going to see 'I love everycook' on the display");
		break;
		}
		
	}
}
void testFanPrintf(){
	//fantemp=
	printf("\nEnter number betwenn 0 and 100 to activate the PWM : %d",fanPwm);
	printf("\nThe temperature of the transistor is %d",fantemp);
}
void testMotorPrintf(){
	motorRPMTrue=motorGetMotorRPM();
	motorSensor=motorGetPosSensor();
	printf("\nHow many RPM would you want?");
	printf("\nThe motor turn at %d RPM per second and your command is %d",motorRPMTrue,motorRPMDesired);
	printf("\nState of position sensor %d",motorSensor);
}
void testWeightPrintf(){
	int i;
	newWeightData.backL=adc_values.LoadCellBackLeft.adc_value;
	newWeightData.backR=adc_values.LoadCellBackRight.adc_value;
	newWeightData.frontL=adc_values.LoadCellFrontLeft.adc_value;
	newWeightData.frontR=adc_values.LoadCellFrontRight.adc_value;
	newWeightData.average=(newWeightData.backL+newWeightData.backR+newWeightData.frontL+newWeightData.frontR)/4;//change
	newWeightData.averageG=adc_values.Weight.valueByOffset;
	if(tabWeightData.nbData<NBWEIGHTLINE){
		allWeightData[tabWeightData.nbData].backL=newWeightData.backL;
		allWeightData[tabWeightData.nbData].backR=newWeightData.backR;
		allWeightData[tabWeightData.nbData].frontL=newWeightData.frontL;
		allWeightData[tabWeightData.nbData].frontR=newWeightData.frontR;
		allWeightData[tabWeightData.nbData].average=newWeightData.average;
		allWeightData[tabWeightData.nbData].averageG=newWeightData.averageG;
	}else{
		for(i=1;i<NBWEIGHTLINE;i++){
			allWeightData[i-1].backL=allWeightData[i].backL;
			allWeightData[i-1].backR=allWeightData[i].backR;
			allWeightData[i-1].frontL=allWeightData[i].frontL;
			allWeightData[i-1].frontR=allWeightData[i].frontR;
			allWeightData[i-1].average=allWeightData[i].average;
			allWeightData[i-1].averageG=allWeightData[i].averageG;
		}
		allWeightData[NBWEIGHTLINE-1].backL=newWeightData.backL;
		allWeightData[NBWEIGHTLINE-1].backR=newWeightData.backR;
		allWeightData[NBWEIGHTLINE-1].frontL=newWeightData.frontL;
		allWeightData[NBWEIGHTLINE-1].frontR=newWeightData.frontR;
		allWeightData[NBWEIGHTLINE-1].average=newWeightData.average;
		allWeightData[NBWEIGHTLINE-1].averageG=newWeightData.averageG;
	}
	if(tabWeightData.nbData==0){
		tabWeightData.max=newWeightData;
		tabWeightData.min=newWeightData;
		tabWeightData.avg=newWeightData;
	}else{
		if(tabWeightData.min.backL>newWeightData.backL){
			tabWeightData.min.backL=newWeightData.backL;
		}
		if(tabWeightData.min.backR>newWeightData.backR){
			tabWeightData.min.backR=newWeightData.backR;
		}
		if(tabWeightData.min.frontL>newWeightData.frontL){
			tabWeightData.min.frontL=newWeightData.frontL;
		}
		if(tabWeightData.min.frontR>newWeightData.frontR){
			tabWeightData.min.frontR=newWeightData.frontR;
		}
		if(tabWeightData.min.average>newWeightData.average){
			tabWeightData.min.average=newWeightData.average;
		}
		if(tabWeightData.min.averageG>newWeightData.averageG){
			tabWeightData.min.averageG=newWeightData.averageG;
		}
		if(tabWeightData.max.backL<newWeightData.backL){
			tabWeightData.max.backL=newWeightData.backL;
		}
		if(tabWeightData.max.backR<newWeightData.backR){
			tabWeightData.max.backR=newWeightData.backR;
		}
		if(tabWeightData.max.frontL<newWeightData.frontL){
			tabWeightData.max.frontL=newWeightData.frontL;
		}
		if(tabWeightData.max.frontR<newWeightData.frontR){
			tabWeightData.max.frontR=newWeightData.frontR;
		}
		if(tabWeightData.max.average<newWeightData.average){
			tabWeightData.max.average=newWeightData.average;
		}
		if(tabWeightData.max.averageG<newWeightData.averageG){
			tabWeightData.max.averageG=newWeightData.averageG;
		}
		tabWeightData.delta.backL=tabWeightData.max.backL-tabWeightData.min.backL;
		tabWeightData.delta.backR=tabWeightData.max.backR-tabWeightData.min.backR;
		tabWeightData.delta.frontL=tabWeightData.max.frontL-tabWeightData.min.frontL;
		tabWeightData.delta.frontR=tabWeightData.max.frontR-tabWeightData.min.frontR;
		tabWeightData.delta.average=tabWeightData.max.average-tabWeightData.min.average;
		tabWeightData.delta.averageG=tabWeightData.max.backL-tabWeightData.min.averageG;

		tabWeightData.avg.backL=newWeightData.backL+tabWeightData.avg.backL;
		tabWeightData.avg.backR=newWeightData.backR+tabWeightData.avg.backR;
		tabWeightData.avg.frontL=newWeightData.frontL+tabWeightData.avg.frontL;
		tabWeightData.avg.frontR=newWeightData.frontR+tabWeightData.avg.frontR;
		tabWeightData.avg.average=newWeightData.average+tabWeightData.avg.average;
		tabWeightData.avg.averageG=newWeightData.averageG+tabWeightData.avg.averageG;
	}
	tabWeightData.nbData++;
	printf("\n Sensor :  FrontL  |  FrontR  |   BackL  |   BackR  |  Average | Average/G|");
	
	printf("\n  Max   :");
	printColumnVAr(WIDHTCOLUMN,6,tabWeightData.max.frontL,tabWeightData.max.frontR,tabWeightData.max.backL,tabWeightData.max.backR,tabWeightData.max.average,tabWeightData.max.averageG);
	printf("\n  Min   :");
	printColumnVAr(WIDHTCOLUMN,6,tabWeightData.min.frontL,tabWeightData.min.frontR,tabWeightData.min.backL,tabWeightData.min.backR,tabWeightData.min.average,tabWeightData.min.averageG);
	printf("\n Delta  :");
	printColumnVAr(WIDHTCOLUMN,6,tabWeightData.delta.frontL,tabWeightData.delta.frontR,tabWeightData.delta.backL,tabWeightData.delta.backR,tabWeightData.delta.average,tabWeightData.delta.averageG);
	printf("\nAverage :");
	printColumnVAr(WIDHTCOLUMN,6,tabWeightData.avg.frontL/tabWeightData.nbData,tabWeightData.avg.frontR/tabWeightData.nbData,tabWeightData.avg.backL/tabWeightData.nbData,tabWeightData.avg.backR/tabWeightData.nbData,tabWeightData.avg.average/tabWeightData.nbData,tabWeightData.avg.averageG/tabWeightData.nbData);
	printf("\n___________________|__________|__________|__________|__________|________________");
	if(tabWeightData.nbData<NBWEIGHTLINE){
		for(i=0;i<tabWeightData.nbData;i++){
			printf("\n         ");
			printColumnVAr(WIDHTCOLUMN,6,allWeightData[i].frontL,allWeightData[i].frontR,allWeightData[i].backL,allWeightData[i].backR,allWeightData[i].average,allWeightData[i].averageG);	
		}
	}else{
		for(i=0;i<NBWEIGHTLINE;i++){
			printf("\n         ");
			printColumnVAr(WIDHTCOLUMN,6,allWeightData[i].frontL,allWeightData[i].frontR,allWeightData[i].backL,allWeightData[i].backR,allWeightData[i].average,allWeightData[i].averageG);
		}
	}
}

void  printColumnVAr( uint16_t widthColumn, uint16_t nbColumn,...){
	va_list ap;
	int i;
    va_start(ap, nbColumn);
    for( i = 0 ; i < nbColumn ; i++){
		printMiddle(widthColumn,va_arg(ap,uint32_t));
		printf("|");
    }
    va_end(ap);
}

void printMiddle(uint16_t space, uint32_t nb){
	int nbsize = 0;
	int nbSpace=0;
	uint32_t nbtemp=nb;
	int i=0;
	if(nbtemp == 0) {
    nbsize=1;
	}else if(nbtemp < 0)  { //si tu compte le moins dans la longueur
		++nbsize;
	}
	while(nbtemp != 0) {
		nbtemp /= 10;
		++nbsize;
	}
	nbSpace=space-nbsize;
	if(nbSpace>-1){
		if(nbSpace%2==0){
			for(i=0;i<(nbSpace/2);i++){
				printf(" ");
			}
			printf("%d",nb);
			for(i=0;i<(nbSpace/2);i++){
				printf(" ");
			}
		}else{
			for(i=0;i<((nbSpace/2)+1);i++){
				printf(" ");
			}
			printf("%d",nb);
			for(i=0;i<(nbSpace/2);i++){
				printf(" ");
			}
		}
	}else{
		printf("The number is bigger than numberSpace");
	}
}

#endif /*----NEWMODE_C_-----*/
