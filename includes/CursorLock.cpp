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
	setup_clipcursor(GameWindow);
}