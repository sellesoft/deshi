#pragma once
#ifndef DESHI_IO_H
#define DESHI_IO_H

#include "../utils/array.h"
#include "../utils/string.h"

enum FileAccessFlags{
	FileAccessFlag_Read  = 1 << 0,
	FileAccessFlag_Write = 1 << 1,
	FileAccessFlag_Exec  = 1 << 2,
};

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

//returns a temporary array of the files in the target directory
array<File> get_directory_files(const char* directory);
FORCE_INLINE array<File> get_directory_files(cstring directory){ return get_directory_files(directory.str); }

void delete_file(const char* filepath);
FORCE_INLINE void delete_file(cstring filepath){ return delete_file(filepath.str); }
FORCE_INLINE void delete_file(File* file){ return delete_file(file->path); }

//returns true if a filepath is valid
b32 file_exists(const char* filepath);
FORCE_INLINE b32 file_exists(cstring filepath){ return file_exists(filepath.str); }
FORCE_INLINE b32 file_exists(File* file){ return file_exists(file->path); }

#endif //DESHI_IO_H
