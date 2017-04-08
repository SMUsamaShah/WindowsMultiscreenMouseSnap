/*******************************************************************************************************/
// File Name	:		 WindowsMultiscreenMouseFix.cpp
// Created By   :		 SMUsamaShah
// Created Date	:		 09 April, 2017
// Log			:
/*******************************************************************************************************/

/*******************************************************************************************************/
// Headers
/*******************************************************************************************************/

#include "stdafx.h"
#include <Windows.h>

/*******************************************************************************************************/
// 
/*******************************************************************************************************/

HHOOK hLowLevelMouseProc;
MSLLHOOKSTRUCT ms;
RECT* monitors;
int nMonitorIndex = 0;
int nMonitors;
BOOL bClipped = false;
DWORD dwTime_;
DWORD dwDelay;
DWORD dwMaxDelay = 200;
int nCurrentMonitor = 0;
int nUpcomingMonitor = 0;

/*******************************************************************************************************/
// Callback for EnumDisplayMonitors. Stores coordinates of each monitor.
/*******************************************************************************************************/

BOOL CALLBACK MonitorEnumProc(HMONITOR hMonitor,
	HDC      hdcMonitor,
	LPRECT   lprcMonitor,
	LPARAM   dwData)
{
	MONITORINFO info;
	info.cbSize = sizeof(info);
	if (GetMonitorInfo(hMonitor, &info)) {
		monitors[nMonitorIndex] = info.rcMonitor;
		nMonitorIndex++;

		/*printf("Monitor(%d) = %d,%d-%d,%d \n", nMonitorIndex,
			monitors[nMonitorIndex].left,
			monitors[nMonitorIndex].top,
			monitors[nMonitorIndex].right,
			monitors[nMonitorIndex].bottom);*/
	}

	return TRUE;  // continue enumerating
}

/*******************************************************************************************************/
// Calculate display index when given mouse 
/*******************************************************************************************************/

int GetMouseDisplay(POINT &mousePt, RECT display[], int &n) {
	for (int i = 0; i < n; i++) {
		if ((display[i].left <= mousePt.x && mousePt.x <= display[i].right) &&
			(display[i].top <= mousePt.y && mousePt.y <= display[i].bottom)) {
			return i;
		}
	}
}

/*******************************************************************************************************/
// Tell on which monitor mouse currently is
/*******************************************************************************************************/

LRESULT CALLBACK LowLevelMouseProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	ms = *((MSLLHOOKSTRUCT*)lParam);
	dwDelay = ms.time - dwTime_;
	dwTime_ = ms.time;

	if (nCode >= 0 && wParam == WM_MOUSEMOVE && ms.flags != LLMHF_INJECTED)
	{
		if (nCurrentMonitor != (nUpcomingMonitor = GetMouseDisplay(ms.pt, monitors, nMonitors))) {
			if (dwDelay < dwMaxDelay && !bClipped) {
				bClipped = ClipCursor(&monitors[nCurrentMonitor]);
				//printf("FAST > bClipped to %d\n", nCurrentMonitor);
			}
			else if (dwDelay >= dwMaxDelay) {
				bClipped = ClipCursor(&monitors[nUpcomingMonitor]);
				nCurrentMonitor = nUpcomingMonitor;
				//printf("SLOW > bClipped to %d\n", nUpcomingMonitor);
			}
		}

		//printf("Mouse position X = %d  Mouse Position Y = %d\n", ms.pt.x, ms.pt.y);
	}

	return CallNextHookEx(hLowLevelMouseProc, nCode, wParam, lParam);
}

/*******************************************************************************************************/
// Message Loop for SetWindowsHookEx
/*******************************************************************************************************/

void MessageLoop()
{
	MSG message;
	while (GetMessage(&message, NULL, 0, 0) > 0)
	{
		//TranslateMessage(&message);
		DispatchMessage(&message);
	}
}

/*******************************************************************************************************/
// 
/*******************************************************************************************************/

BOOL InstallHook() {
	HINSTANCE hInstance = GetModuleHandle(NULL);

	if (!hInstance)
		return FALSE;

	hLowLevelMouseProc = SetWindowsHookEx(
		WH_MOUSE_LL,
		(HOOKPROC)LowLevelMouseProc,
		hInstance,
		NULL
	);

	return TRUE;
}

/*******************************************************************************************************/
// Install low level mouse and wait in message loop
/*******************************************************************************************************/

DWORD WINAPI MouseDetectThread(LPVOID lpParm)
{
	if (InstallHook() == FALSE) {
		return 1;
	}
	MessageLoop();
	UnhookWindowsHookEx(hLowLevelMouseProc);

	return 0;
}

/*******************************************************************************************************/
// 
/*******************************************************************************************************/

int main(int argc, char** argv)
{
	SetPriorityClass(GetCurrentProcess(), REALTIME_PRIORITY_CLASS);
	HANDLE hThread;
	DWORD dwThread;
	nMonitors = GetSystemMetrics(SM_CMONITORS);
	monitors = new RECT[nMonitors];
	EnumDisplayMonitors(NULL, NULL, MonitorEnumProc, 0);

//	printf("num monitors = %d\n", nMonitors);
//	printf(" xyvirtual screen = %d,%d\n", GetSystemMetrics(SM_XVIRTUALSCREEN), GetSystemMetrics(SM_YVIRTUALSCREEN));
//	printf("cxyvirtual screen = %d,%d\n", GetSystemMetrics(SM_CXVIRTUALSCREEN), GetSystemMetrics(SM_CYVIRTUALSCREEN));

	hThread = CreateThread(
		NULL,
		NULL,
		(LPTHREAD_START_ROUTINE)MouseDetectThread,
		(LPVOID)argv[0],
		NULL,
		&dwThread
	);

	if (hThread)
	{
		return WaitForSingleObject(hThread, INFINITE);
	}
	else
	{
		return 1;
	}
	return 0;
}