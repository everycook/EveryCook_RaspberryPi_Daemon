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
#include "speaker.h"

#define FRANCAIS 1
#define ENGLISH	 2
#define DEUTSCH	 3

int language = ENGLISH;
char * sounds="/home/pi/codes/sounds/";

void speakerSpeakLanguage(char * text){
	int result;
	char command[400];
	switch (language) {
		case FRANCAIS :
			sprintf(command, "mpg123 -q %sfr%s.mp3",sounds, text);
		break;
		case ENGLISH :
			sprintf(command, "mpg123 -q %sen%s.mp3",sounds, text);
		break;
		case DEUTSCH :
			sprintf(command, "mpg123 -q %sde%s.mp3",sounds, text);
		break;
	}
	result=system(command);
	if(result!=0){
		switch (language) {
			case FRANCAIS :
				sprintf(command, "mpg123 -q %sfrmp3filedoesnotexist.mp3",sounds);
			break;
			case ENGLISH :
				sprintf(command, "mpg123 -q %senmp3filedoesnotexist.mp3",sounds);
			break;
			case DEUTSCH :
				sprintf(command, "mpg123 -q %sdemp3filedoesnotexist.mp3",sounds);
			break;
		}
		system(command);
	}
}
void speakerLanguageFrancais(){
	language=FRANCAIS;
}
void speakerLanguageEnglish(){
	language=ENGLISH;
}
void speakerLanguageDeutsch(){
	language=DEUTSCH;
}
char* speakerCurrentLabguage(){
	char* currentLanguage;
	switch (language) {
		case FRANCAIS :
			currentLanguage="francais";
		break;
		case ENGLISH :
			currentLanguage="english";
		break;
		case DEUTSCH :
			currentLanguage="deutsch";
		break;
		default :
			currentLanguage="no language";
		break;
	}
	return currentLanguage;
}

void speakerSpeak(char* text){
/*
	if (fork() == 0){
		char[400] command;
		sprintf(command, "echo \"%s\" | espeak -a 200 -v %s 2>&1", text, state.language);
		int result = system( command );	
		exit(0);
	}
*/
	if (daemonGetSettingsDebug_enabled() || daemonGetSettingsDebug3_enabled() || daemonGetSettingsDebug4_enabled()){printf("start speak thread\n");}
	pthread_t threadSpeak;
	//pthread_create(&threadSpeak, NULL, speakThreadFunc, NULL);
	char* textpointer = &text[0];
	//pthread_create(&threadSpeak, NULL, speakThreadFunc, (void *) textpointer);
	//pthread_join(threadSpeak, NULL);
}

/** @brief fonction wich speaks
 *  @param *ptr : text that is spoken
*/
void *speakThreadFunc(void *ptr);


void *speakThreadFunc(void *ptr){
	char* text = (char *) ptr;
	char command[400];
	//sprintf(command, "echo \"%s\" | espeak -a 200 -v %s 2>&1", text, state.language);
	sprintf(command, "echo \"%s\" | espeak -a 200 -v german 2>&1 > /dev/nul", text);
	//sprintf(command, "echo \"%s\" | espeak -a 200 2>&1 > /dev/nul", text);
	if (daemonGetSettingsDebug_enabled() || daemonGetSettingsDebug3_enabled() || daemonGetSettingsDebug4_enabled()){printf("%s\n", command);}
	int result = system( command );
	pthread_exit(0);
	return (void *)result;
}
