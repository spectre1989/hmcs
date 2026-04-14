#pragma once

#include "types.h"


// todo probably move this to somewhere else
typedef struct buffer_t
{
	void* data;
	uint32_t size;
} buffer_t;

buffer_t file_read(const char* path);