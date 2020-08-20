////////////////////////////////////////////////////////////
//
// libwindevblk - Library to read/write partitions
// Copyright (C) 2020 Vincent Richomme
//
// Licensed to the Apache Software Foundation(ASF) under one
// or more contributor license agreements.See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership.The ASF licenses this file
// to you under the Apache License, Version 2.0 (the
// 	"License"); you may not use this file except in compliance
// 	with the License.You may obtain a copy of the License at
// 
// 	http ://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.See the License for the
// specific language governing permissionsand limitations
// under the License.
//
////////////////////////////////////////////////////////////

#include <windows.h>
#include <Strsafe.h>

extern HINSTANCE	g_hInstance;

HWND				g_hWnd = NULL;
static HANDLE		g_hThread = NULL;
static LPCTSTR		g_szWndClassName = TEXT("DevBlkWndClassName");
static LPCTSTR		g_szWndTitle = TEXT("DevBlkWnd");

BOOL CreateOnceMsgLoop();
void DestroyMsgLoop();
DWORD WINAPI ThreadProc(LPVOID lpParam);
BOOL RegisterWindowClass();
LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

/* Registering for Device Notification */
// https://msdn.microsoft.com/fr-fr/library/windows/desktop/aa363432(v=vs.85).aspx
#include <dbt.h>
// This GUID is for all USB serial host PnP drivers, but you can replace it 
// with any valid device class guid.
GUID WceusbshGUID = { 0x25dbce51, 0x6c8f, 0x4a72, 0x8a,0x6d,0xb5,0x4c,0x2b,0x4f,0xc8,0x35 };

BOOL DoRegisterDeviceInterfaceToHwnd(
	IN GUID InterfaceClassGuid,
	IN HWND hWnd,
	OUT HDEVNOTIFY *hDeviceNotify
);

BOOL CreateOnceMsgLoop()
{
	BOOL bRet = TRUE;

	if(g_hThread == NULL)
	{
		g_hThread = CreateThread((LPSECURITY_ATTRIBUTES)NULL, 0, ThreadProc, (LPVOID)NULL, 0, (LPDWORD)NULL);
		bRet = (g_hThread != NULL);
	}

	return bRet;
}

void DestroyMsgLoop()
{
	if (IsWindow(g_hWnd))
	{
		SendMessage(g_hWnd, WM_CLOSE, 0, 0);
		g_hWnd = NULL;
	}
}

BOOL RegisterWindowClass()
{
	WNDCLASSEX wc;
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = 0;
    wc.lpfnWndProc = WindowProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = g_hInstance;
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wc.lpszMenuName = NULL;
    wc.lpszClassName = (LPCTSTR)g_szWndClassName;
    wc.hIconSm = LoadIcon(NULL, IDI_APPLICATION);
	
	if (!RegisterClassEx(&wc))
		return FALSE;
	return TRUE;
}

DWORD WINAPI ThreadProc(LPVOID lpParam)
{
	MSG msg;
	int retVal;

	if (RegisterWindowClass())
	{
		g_hWnd = CreateWindowEx(WS_EX_CLIENTEDGE,
			g_szWndClassName, g_szWndTitle,
            WS_OVERLAPPEDWINDOW,
			CW_USEDEFAULT, CW_USEDEFAULT,
			240, 120,
			NULL, NULL,
			g_hInstance,
			NULL);
		ShowWindow(g_hWnd, SW_HIDE);
        UpdateWindow(g_hWnd);
       
		// Get all messages for any window that belongs to this thread,
		// without any filtering. Potential optimization could be
		// obtained via use of filter values if desired.
		while ((retVal = GetMessage(&msg, NULL, 0, 0)) != 0)
		{
			if (retVal == -1)
			{
				break;
			}
			else
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
	}

	return 0;
}

LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	LRESULT lResult = -1;
	static ULONGLONG msgCount = 0;
	static HDEVNOTIFY hDeviceNotify;

#if 0 
    lResult = DefWindowProc(hWnd, uMsg, wParam, lParam);
    return lResult;
#else

	switch (uMsg)
	{
	case WM_CREATE:
		if (!DoRegisterDeviceInterfaceToHwnd(WceusbshGUID, hWnd,&hDeviceNotify))
		{
			// Terminate on failure.
			//ErrorHandler(TEXT("DoRegisterDeviceInterfaceToHwnd"));
			//ExitProcess(1);
		}
		break;

	case WM_DEVICECHANGE:
	{
		//
		// This is the actual message from the interface via Windows messaging.
		// This code includes some additional decoding for this particular device type
		// and some common validation checks.
		//
		// Note that not all devices utilize these optional parameters in the same
		// way. Refer to the extended information for your particular device type 
		// specified by your GUID.
		//
		PDEV_BROADCAST_DEVICEINTERFACE b = (PDEV_BROADCAST_DEVICEINTERFACE)lParam;
		TCHAR strBuff[256];

		// Output some messages to the window.
		switch (wParam)
		{
		case DBT_DEVICEARRIVAL:
			msgCount++;
			StringCchPrintf(strBuff, 256,
				TEXT("Message %d: DBT_DEVICEARRIVAL\n"), msgCount);
			break;
		case DBT_DEVICEREMOVECOMPLETE:
			msgCount++;
			StringCchPrintf(strBuff, 256,
				TEXT("Message %d: DBT_DEVICEREMOVECOMPLETE\n"), msgCount);
			break;
		case DBT_DEVNODES_CHANGED:
			msgCount++;
			StringCchPrintf(strBuff, 256,
				TEXT("Message %d: DBT_DEVNODES_CHANGED\n"), msgCount);
			break;
		default:
			msgCount++;
			StringCchPrintf(strBuff, 256,
				TEXT("Message %d: WM_DEVICECHANGE message received, value %d unhandled.\n"),
				msgCount, wParam);
			break;
		}
        OutputDebugString(strBuff);
	}
    break;

	case WM_CLOSE:
		if (!UnregisterDeviceNotification(hDeviceNotify))
		{
			//ErrorHandler(TEXT("UnregisterDeviceNotification"));
		}
		DestroyWindow(hWnd);
		break;

	case WM_DESTROY:
		PostQuitMessage(0);
		break;

	default:
		break;
	}

    lResult = DefWindowProc(hWnd, uMsg, wParam, lParam);
	return lResult;
#endif
}




BOOL DoRegisterDeviceInterfaceToHwnd(
	IN GUID InterfaceClassGuid,
	IN HWND hWnd,
	OUT HDEVNOTIFY *hDeviceNotify
)
// Routine Description:
//     Registers an HWND for notification of changes in the device interfaces
//     for the specified interface class GUID. 

// Parameters:
//     InterfaceClassGuid - The interface class GUID for the device 
//         interfaces. 

//     hWnd - Window handle to receive notifications.

//     hDeviceNotify - Receives the device notification handle. On failure, 
//         this value is NULL.

// Return Value:
//     If the function succeeds, the return value is TRUE.
//     If the function fails, the return value is FALSE.

// Note:
//     RegisterDeviceNotification also allows a service handle be used,
//     so a similar wrapper function to this one supporting that scenario
//     could be made from this template.
{
	DEV_BROADCAST_DEVICEINTERFACE NotificationFilter;

	ZeroMemory(&NotificationFilter, sizeof(NotificationFilter));
	NotificationFilter.dbcc_size = sizeof(DEV_BROADCAST_DEVICEINTERFACE);
	NotificationFilter.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
	NotificationFilter.dbcc_classguid = InterfaceClassGuid;

	*hDeviceNotify = RegisterDeviceNotification(
		hWnd,                       // events recipient
		&NotificationFilter,        // type of device
		DEVICE_NOTIFY_WINDOW_HANDLE // type of recipient handle
	);

	if (NULL == *hDeviceNotify)
	{
		//ErrorHandler(TEXT("RegisterDeviceNotification"));
		return FALSE;
	}

	return TRUE;
}