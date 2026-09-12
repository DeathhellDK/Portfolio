/* Start Header ************************************************************************/
/*!
\file		Main.cpp
\author		Hao Peng, TingKai
\date		March, 20, 2025
\brief		File contains the main function to set the main game loop
			

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/* End Header **************************************************************************/

#include <crtdbg.h> // To check for memory leaks
#include "GameStateList.h"
#include "GameStateManager.h"
#include "Main.h"

int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
{
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // Initialize the system
    AESysInit(hInstance, nCmdShow, 1600, 900, 1, 60, true, NULL);
    AESysSetWindowTitle("Final Stand");

    // Initialize the Game State Manager with the LOGO state
    GSM_Initialize(GS_LOGO);

    // Main Game Loop using GSM
    while (current != GS_QUIT)
    {
        GSM_Update();    // Update the GSM (set function pointers)
        fpLoad();        // Load the appropriate state

        fpInitialize();  // Initialize the current state

        while (next == current)  // Continue in the same state until transition
        {
            // Start the frame
            AESysFrameStart();

            // Handle state-specific updates and drawing
            fpUpdate();
            fpDraw();

            // End the frame
            AESysFrameEnd();

            // Check if window still exists
            if (0 == AESysDoesWindowExist())
            {
                next = GS_QUIT;  // Exit if window is closed
            }
        }

        // State is changing, clean up
        fpFree();
        fpUnload();

        // Update state tracking
        previous = current;
        current = next;
    }

    // Cleanup and exit
    AESysExit();
    return 0;
}
