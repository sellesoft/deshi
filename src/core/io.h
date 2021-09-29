#pragma once
#ifndef DESHI_IO_H
#define DESHI_IO_H

#include "../utils/cstring.h"

#define MAX_FILEPATH_SIZE 1024
#define MAX_FILENAME_SIZE 256
struct File{
	void* handle = 0;
	u64 time_creation = 0;
	u64 time_last_access = 0;
	u64 time_last_write = 0;
	u64 bytes_size = 0;
	b32 is_directory = false;
	
	u32 path_length  = 0;
	u32 name_length  = 0;
	u32 short_length = 0;
	u32 ext_length   = 0;
	char path[MAX_FILEPATH_SIZE] = {};
	char name[MAX_FILENAME_SIZE] = {};
};

struct FileReader{
	cstring data = {};
	cstring raw  = {};
	u32 line_number = 0;
	u8  comment_character = '#';
	b32 skip_comments_and_whitespace = true;
	b32 failed = false;
};

array<File> get_directory_files(const char* directory);

void delete_file(const char* filepath);
FORCE_INLINE void delete_file(File* file){ delete_file(file->path); }

#endif //DESHI_IO_H
