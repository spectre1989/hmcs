#include "file.h"

#include <Windows.h>



buffer_t file_read(const char* path)
{
	wchar_t path_wide[MAX_PATH];
	MultiByteToWideChar(CP_UTF8, 0, path, -1, path_wide, MAX_PATH);

	HANDLE file = CreateFileW(path_wide, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL); // todo check return

	DWORD file_size = GetFileSize(file, NULL);
	uint8_t* file_data = malloc(file_size);
	ReadFile(file, file_data, file_size, NULL, NULL); // todo check return?

	CloseHandle(file);

	return (buffer_t){
		.data = file_data,
		.size = file_size
	};
}