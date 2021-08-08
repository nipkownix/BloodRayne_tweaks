#include "stdafx.h"
#include "CursorLock.h"
#include <iostream>
#include <string>

// github.com/luluco250/fnv_clipcursor/blob/master/main.cpp

static HWND GameWindow = NULL;
static WNDPROC GameWndProc = NULL;

void setup_clipcursor(HWND window)
{
	RECT rect;
	if (!GetWindowRect(window, &rect)) {
		std::cout << "Failed to get window rect" << std::endl;
		return;
	}
	if (!ClipCursor(&rect))
		std::cout << "Failed lock cursor" << std::endl;
	else
		std::cout << "Cursor locked" << std::endl;
}

void unset_clipcursor() {
	if (!ClipCursor(NULL))
		std::cout << "Failed unlock cursor" << std::endl;
	else
		std::cout << "Cursor unlocked" << std::endl;
}

LRESULT CALLBACK injected_wndproc(HWND window, UINT message, WPARAM w_param, LPARAM l_param)
{
	switch (message) {
		case WM_ACTIVATE: {
			if (w_param == WA_INACTIVE)
				unset_clipcursor();
			else
				setup_clipcursor(window);
			break;
		}
		case WM_MOUSEMOVE: {
			//setup_clipcursor(window); // bro why?
			break;
		}
		case WM_SETFOCUS: {
			setup_clipcursor(window);
			break;
		}
		case WM_KILLFOCUS: {
			unset_clipcursor();
			break;
		}
	}
	return GameWndProc(window, message, w_param, l_param);
}

void inject_wndproc() {
	GameWndProc = reinterpret_cast<WNDPROC>(GetWindowLongPtrA(GameWindow, GWLP_WNDPROC));

	if (GameWndProc == NULL)
	{
		std::cout << "Failed to get game wndproc" << std::endl;
		return;
	}

	if (SetWindowLongPtrA(GameWindow, GWLP_WNDPROC, reinterpret_cast<LONG>(&injected_wndproc)) == 0)
	{
		std::cout << "Failed to set injected wndproc" << std::endl;
		return;
	}
	setup_clipcursor(GameWindow);
}

void SetCursorLock(char* GameClassName)
{
	GameWindow = FindWindowA(GameClassName, NULL);
	if (GameWindow == NULL)
	{
		std::cout << "Failed to find GameWindow" << std::endl;
		return;
	} else {
		std::cout << "GameWindow found" << std::endl;
	}
	inject_wndproc();
}