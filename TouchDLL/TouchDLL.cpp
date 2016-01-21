#include <windows.h>
#include <windowsx.h>
#include <CommCtrl.h>
#include <stdio.h>

/*
* Hankerspace
* 
* Touch handling windows DLL
* Initialize the DLL with Initialize function with handled window as parameter
* Then, retrieve pointCount & touchData using exported funcs
* Works with windows 7, 8, 8.1
* Note : only touches in handled window are correctly retrieved
*/
extern "C"
{
	// Global Handles
	HWND g_hRemoteWnd;
	HINSTANCE g_hInst;
	HHOOK g_HookPosted = NULL;
	HHOOK g_HookCalled = NULL;

	#define MAX_TOUCH 128

	// Storage for touch tracking
	struct tTouchData
	{
		int m_x;
		int m_y;
		int m_ID;
		int m_Time;
	};

	// Storage for window handling
	struct tWindowData
	{
		char *pName;
		HWND Handle;
	};

	tTouchData g_TouchData[MAX_TOUCH];
	tTouchData g_CopyTouchPoints[MAX_TOUCH];

	// Clear all to -1 IDs at start
	void ClearData()
	{
		for (int i=0;i<MAX_TOUCH;i++)
		{
			g_TouchData[i].m_ID = -1;
		}
	}

	// Touch handler
	// WARNING - ERRORS NOT HANDLED!!!!!
	LRESULT OnTouch(HWND hWnd, WPARAM wParam, LPARAM lParam)
	{
		BOOL bHandled = FALSE;
		UINT cInputs = LOWORD(wParam);
		PTOUCHINPUT pInputs = new TOUCHINPUT[cInputs];
		if (pInputs)
		{
			if (GetTouchInputInfo((HTOUCHINPUT)lParam, cInputs, pInputs, sizeof(TOUCHINPUT)))
			{
				for (UINT i=0; i < cInputs; i++)
				{
					TOUCHINPUT ti = pInputs[i];

					if (ti.dwFlags & TOUCHEVENTF_DOWN)	// Add to list
					{
						for (int p=0;p<MAX_TOUCH;p++)
						{
							if (g_TouchData[p].m_ID == -1)
							{
								g_TouchData[p].m_ID = ti.dwID;
								g_TouchData[p].m_x = ti.x;
								g_TouchData[p].m_y = ti.y;
								g_TouchData[p].m_Time = ti.dwTime;
								break;
							}
						}
					}
					if (ti.dwFlags & TOUCHEVENTF_UP)	// Remove from list
					{
						for (int p=0;p<MAX_TOUCH;p++)
						{
							if (g_TouchData[p].m_ID == ti.dwID)
							{
								g_TouchData[p].m_ID = -1;
								break;
							}
						}
					}
					if (ti.dwFlags & TOUCHEVENTF_MOVE)	// Move point
					{
						for (int p=0;p<MAX_TOUCH;p++)
						{
							if (g_TouchData[p].m_ID == ti.dwID)
							{
								g_TouchData[p].m_x = ti.x;
								g_TouchData[p].m_y = ti.y;
								g_TouchData[p].m_Time = ti.dwTime;
								break;
							}
						}
					}
				}            
				bHandled = TRUE;
			}
			else
			{
				/* TODO : handle the error here */
			}
			delete [] pInputs;
		}
		else
		{
			/* TODO : handle the error here, probably out of memory */
		}
		if (bHandled)
		{
			// if you handled the message, close the touch input handle and return
			CloseTouchInputHandle((HTOUCHINPUT)lParam);
			return 0;
		}
		else
		{
			// if you didn't handle the message, let DefWindowProc handle it
			return DefWindowProc(hWnd, WM_TOUCH, wParam, lParam);
		}
	}


	// Hook prok to capture calls to Unity Window function by System
	LRESULT HookProcCalled(int code, WPARAM wParam, LPARAM lParam)
	{
		CWPSTRUCT *Msg = (CWPSTRUCT *)lParam;
		if (code >= 0)
		{
			if (Msg->hwnd == g_hRemoteWnd)
			{
				switch (Msg->message)
				{
				case WM_TOUCH:
					OnTouch(Msg->hwnd, Msg->wParam, Msg->lParam);
					break;
				}
			}
		}
		// Always call this - we dont actually "Use" any of the messages
		return CallNextHookEx(0, code, wParam, lParam); 
	}


	// Hook prok to capture messages Posted to Unity Window
	LRESULT HookProcPosted(int code, WPARAM wParam, LPARAM lParam)
	{
		MSG *Msg = (MSG *)lParam;
		if (code >= 0)
		{
			if (Msg->hwnd == g_hRemoteWnd)
			{
				switch (Msg->message)
				{
				case WM_TOUCH:
					OnTouch(Msg->hwnd, Msg->wParam, Msg->lParam);
					break;
				}
			}
		}
		// Always call this - we dont actually "Use" any of the messages
		return CallNextHookEx(0, code, wParam, lParam); 
	}

	// loop through the Windows on the desktop,
	// Stop when we find the Unity Window
	BOOL CALLBACK EnumWindowsFunc(
	  _In_  HWND hwnd,
	  _In_  LPARAM lParam
	)
	{
		tWindowData *pData = (tWindowData *)lParam;
		char NewName[128];
		GetWindowText(hwnd, NewName, 128); // Cast error? Set project unicode settings to : not set
		if (!strcmp(pData->pName, NewName)) //Find the right window
		{
			pData->Handle = hwnd;
			return false;
		}
		return true;
	}

	__declspec(dllexport) void __cdecl Test(int Val)
	{
		char Str[128];
		sprintf(Str, "Test: Received %d\n", Val);
		OutputDebugString(Str);
	}
	__declspec(dllexport) void __cdecl Test2(int Val)
	{
		char Str[128];
		sprintf(Str, "Test2: Received %d\n", Val);
		OutputDebugString(Str);
	}

	// Exported func to set up the hook
	// PName : window name (not the process name)
	__declspec(dllexport) int __cdecl Initialise(WCHAR *PName)
	{
		char Str[128];
		int Len = lstrlenW(PName);
		WideCharToMultiByte(CP_ACP, 0, PName, Len, Str, 127, NULL, NULL);
		Str[Len] = 0;

		tWindowData WindowData;
		WindowData.pName = Str;
		WindowData.Handle = NULL;

		// Retrieve current windows
		EnumDesktopWindows(NULL, EnumWindowsFunc, (LPARAM)&WindowData);

		if (WindowData.Handle == NULL)
			return -1;
		g_hRemoteWnd = WindowData.Handle; 

		// Get thread ID for Unity window
		DWORD ProcessID;
		int ID = GetWindowThreadProcessId(g_hRemoteWnd, &ProcessID);  
		if (ID == 0)
			return -2;

		// Set the Hooks
		// One for Posted Messages
		g_HookPosted = SetWindowsHookEx(WH_GETMESSAGE, (HOOKPROC)HookProcPosted, g_hInst, ID); 
		if (g_HookPosted == NULL)
		{
			return -3;
		}

		// One hook for System calls (Win8)
		g_HookCalled = SetWindowsHookEx(WH_CALLWNDPROC, (HOOKPROC)HookProcCalled, g_hInst, ID); 
		if (g_HookCalled == NULL)
		{
			UnhookWindowsHookEx(g_HookPosted);
			return -4;
		}

		// Setup our data
		ClearData();	

		// Register the Unity window for touch
		int Res = RegisterTouchWindow(g_hRemoteWnd, TWF_WANTPALM); 
		if (Res == 0)
		{
			UnhookWindowsHookEx(g_HookPosted);
			UnhookWindowsHookEx(g_HookCalled);
			return -5;
		}
		return 0;
	}

	__declspec(dllexport) void __cdecl ShutDown(char *PName)
	{
		UnhookWindowsHookEx(g_HookPosted);
		UnhookWindowsHookEx(g_HookCalled);
	}

	// Retrieve a touchpoint data
	__declspec(dllexport) void __cdecl GetTouchPoint(int i, tTouchData *p)
	{
		*p = g_CopyTouchPoints[i];
	}

	// Retrieve touch point count
	__declspec(dllexport) int __cdecl GetTouchPointCount()
	{
		int NumPoints = 0;
		for (int i=0;i<MAX_TOUCH;i++)
		{
			if (g_TouchData[i].m_ID != -1)
			{
				g_CopyTouchPoints[NumPoints] = g_TouchData[i];
				NumPoints++;
			}
		}
		return NumPoints;
	}

	// Dll export
	BOOL APIENTRY DllMain( HMODULE hModule,
						   DWORD  ul_reason_for_call,
						   LPVOID lpReserved
						 )
	{
		g_hInst = hModule;
		switch (ul_reason_for_call)
		{
		case DLL_PROCESS_ATTACH:
		case DLL_THREAD_ATTACH:
		case DLL_THREAD_DETACH:
		case DLL_PROCESS_DETACH:
			break;
		}
		return TRUE;
	}

}