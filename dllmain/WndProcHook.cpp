#include <iostream>
#include "..\includes\stdafx.h"
#include "dllmain.h"

HWND hWindow;

HHOOK WndProcHook;
HHOOK GetMessageHook;

void EnableClipCursor(HWND window)
{
	RECT rect;
	GetClientRect(window, &rect);

	POINT ul;
	ul.x = rect.left;
	ul.y = rect.top;

	POINT lr;
	lr.x = rect.right;
	lr.y = rect.bottom;

	MapWindowPoints(window, nullptr, &ul, 1);
	MapWindowPoints(window, nullptr, &lr, 1);

	rect.left = ul.x;
	rect.top = ul.y;

	rect.right = lr.x;
	rect.bottom = lr.y;

	if (!ClipCursor(&rect)) {
		#ifdef VERBOSE
		std::cout << "Unable to lock cursor" << std::endl;
		#endif
		return;
	}

	#ifdef VERBOSE
	std::cout << "Cursor locked" << std::endl;
	#endif
}

void DisableClipCursor()
{
	ClipCursor(nullptr);

	#ifdef VERBOSE
	std::cout << "Cursor released" << std::endl;
	#endif
}

// Our new hooked WndProc function
LRESULT CALLBACK WndProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	CWPSTRUCT* winMsg;
	winMsg = (CWPSTRUCT*)lParam;

	if (winMsg->message == WM_NCACTIVATE) {
		if (winMsg->wParam == WA_ACTIVE)
			EnableClipCursor(hWindow);
		else
			DisableClipCursor();
	}

	return CallNextHookEx(WndProcHook, nCode, wParam, lParam);
}

/* We don't need this, but I'm leaving it here anyway for reference.
LRESULT CALLBACK GetMessageH(int nCode, WPARAM wParam, LPARAM lParam)
{
	MSG* pMsg = (MSG*)lParam;

	if (pMsg->message == WM_NCACTIVATE)
		std::cout << "test" << std::endl;

	return CallNextHookEx(GetMessageHook, nCode, wParam, lParam);
}
*/

// CreateWindowExA hook to get the hWindow and set up the WndProc hook
HWND __stdcall CreateWindowExA_Hook(DWORD dwExStyle, LPCSTR lpClassName, LPCSTR lpWindowName, DWORD dwStyle, int X, int Y, int nWidth, int nHeight, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam)
{
	HWND result = CreateWindowExA(dwExStyle, lpClassName, lpWindowName, dwStyle, X, Y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam);
	hWindow = result;

	WndProcHook = SetWindowsHookEx(WH_CALLWNDPROC, WndProc, nullptr, GetCurrentThreadId());

	//GetMessageHook = SetWindowsHookEx(WH_GETMESSAGE, GetMessageH, nullptr, GetCurrentThreadId());

	return result;
}

void Init_WndProcHook()
{
	//CreateWindowEx hook
	auto pattern = hook::pattern("FF 15 ? ? ? ? 89 07 FF 15 ? ? ? ? 8B 56 ? 89 97");
	injector::MakeNOP(pattern.get_first(0), 6);
	injector::MakeCALL(pattern.get_first(0), CreateWindowExA_Hook, true);
}