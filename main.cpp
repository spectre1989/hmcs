#include <windows.h>
#include <stdio.h>

#include <vulkan/vulkan.h>

#include "mdl_file.h"
#include "types.h"


LRESULT window_proc(HWND window, UINT msg, WPARAM w_param, LPARAM l_param)
{
	return DefWindowProcA(window, msg, w_param, l_param);
}

int WinMain(_In_ HINSTANCE instance, _In_opt_ HINSTANCE prev_instance, _In_ LPSTR cmd_line, _In_ int show_cmd)
{
	WNDCLASSA window_class;
	window_class.style = 0;
	window_class.lpfnWndProc = window_proc;
	window_class.cbClsExtra = 0;
	window_class.cbWndExtra = 0;
	window_class.hInstance = instance;
	window_class.hIcon = nullptr;
	window_class.hCursor = nullptr;
	window_class.hbrBackground = nullptr;
	window_class.lpszMenuName = nullptr;
	window_class.lpszClassName = "HMCSClass";

	ATOM window_class_atom = RegisterClassA(&window_class);
	// TODO check errors

	HWND window = CreateWindowA(
		(LPCSTR)window_class_atom, //lpClassName
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
	// TODO check errors

	ShowWindow(window, SW_SHOW);
	// TODO check errors

	FILE* file;
	fopen_s(&file, "cstrike_hd/models/player/sas/sas.mdl", "rb");
	fseek(file, 0, SEEK_END);
	const int64 file_size = ftell(file);
	uint8* buffer = new uint8[file_size];
	fseek(file, 0, SEEK_SET);
	fread(buffer, file_size, 1, file);
	fclose(file);

	MDL_Header* header = (MDL_Header*)buffer;

	vkCreateInstance(0, 0, 0);

	while (true)
	{
		MSG msg;
		while (PeekMessageA(&msg, window, 0, 0, PM_REMOVE))
		{
			DispatchMessageA(&msg);
		}
	}
}