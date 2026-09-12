/*************************************************************
 file:	main.c
 author:	Ye Tingkai, Terril
 email:	ye.t@digipen.edu, t.weiliangterril

 brief:	Main entry point for the sample project
			of the CProcessing library

 Copyright � 2024 DigiPen, All rights reserved.
**************************************************************/

#include "cprocessing.h"
#include "loading.h"

// main() the starting point for the program
// CP_Engine_SetNextGameState() tells CProcessing which functions to use for init, update and exit
// CP_Engine_Run() is the core function that starts the simulation
int main(void)
{
	CP_Engine_SetNextGameState(preload_init, preload_update, preload_exit);
	CP_System_SetWindowSize(1600, 900);
	CP_Engine_Run(0);
	return 0;
}