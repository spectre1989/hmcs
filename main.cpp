#include <windows.h>

int WinMain(HINSTANCE instance, HINSTANCE prev_instance, LPSTR cmd_line, int show_cmd)
{
	HWND window = CreateWindowA(
		nullptr, //lpClassName
		"HMCS", //lpWindowName
		WS_OVERLAPPEDWINDOW, //dwStyle
		100, //x
		200, //y
		1600, //nWidth
		900, //nHeight
		nullptr, //hWndParent
		nullptr, //hMenu
		instance, //hInstance
		nullptr //lpParam
	);

	while (true)
	{}
}