#pragma once
#ifndef DESHI_IO_H
#define DESHI_IO_H

#include "../utils/array.h"
#include "../utils/string.h"

enum FileAccessFlags{
	FileAccess_Read  = 1 << 0,
	FileAccess_Write = 1 << 1,
	FileAccess_Exec  = 1 << 2,
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
	FileAccessFlags flags;
	
	u32 path_length  = 0;
	u32 name_length  = 0;
	u32 short_length = 0;
	u32 ext_length   = 0;
	char path[MAX_FILEPATH_SIZE] = {};
	char name[MAX_FILENAME_SIZE] = {};
};

FORCE_INLINE cstring get_file_name(File& file)      { return cstring{ file.name, file.short_length }; }
FORCE_INLINE cstring get_file_extension(File& file) { return cstring{ file.name + file.short_length+1, file.ext_length }; }
FORCE_INLINE cstring get_file_path(File& file)      { return cstring{ file.path, file.path_length }; }

struct FileReader{
	cstring data = {};
	cstring raw  = {};
	u32 line_number = 0;
	u8  comment_character = '#';
	b32 skip_comments_and_whitespace = true;
	b32 failed = false;

	operator bool() const { return !failed; }
};

FileReader init_reader(const File& file);
FileReader init_reader(const char* data);
cstring    get_line(FileReader& reader);
void       goto_line(FileReader& reader, u32 linenum);
void       goto_idx(FileReader& reader, u32 charnum);
cstring    get_chunk(FileReader& reader, char delimiter);
cstring    get_value_from_key(FileReader& reader, const char* key, char value_delimiter);

File open_file(const char* path, FileAccessFlags flags);

//returns a temporary array of the files in the target directory
array<File> get_directory_files(const char* directory);
FORCE_INLINE array<File> get_directory_files(cstring directory){ return get_directory_files(directory.str); }

void delete_file(const char* filepath);
FORCE_INLINE void delete_file(File* file){ return delete_file(file->path); }

//returns true if a filepath is valid
b32 file_exists(const char* filepath);
FORCE_INLINE b32 file_exists(File* file){ return file_exists(file->path); }

void rename_file(const char* old_filepath, const char* new_filepath);



#endif //DESHI_IO_H
