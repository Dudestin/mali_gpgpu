#pragma once

#define WIN32_LEAN_AND_MEAN 1

#define DUMMY_WINDOW_SIZE_HEIGHT 1
#define DUMMY_WINDOW_SIZE_WIDTH 1

#include <windows.h>

HWND create_window(int uiWIdth, int uiHeight);
LRESULT CALLBACK process_window(HWND hWnd, UINT uiMsg, WPARAM wParam, LPARAM lParam);