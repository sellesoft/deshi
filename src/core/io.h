#pragma once
#ifndef DESHI_IO_H
#define DESHI_IO_H

#include "../utils/array.h"
#include "../utils/string.h"
#include "../utils/string_utils.h"

enum FileAccessFlags_ {
	FileAccess_Read  = 1 << 0,
	FileAccess_Write = 1 << 1,
	FileAccess_Exec  = 1 << 2,
}; typedef u32 FileAccessFlags;

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

FORCE_INLINE cstring get_file_name(const File& file)      { return cstring{ (char*)file.name, file.name_length }; }
FORCE_INLINE cstring get_file_short(const File& file)     { return cstring{ (char*)file.name, file.short_length }; }
FORCE_INLINE cstring get_file_extension(const File& file) { return cstring{ (char*)file.name + file.short_length+1, file.ext_length }; }
FORCE_INLINE cstring get_file_path(const File& file)      { return cstring{ (char*)file.path, file.path_length }; }

struct FileReader{
	cstring read = {};
	cstring raw  = {};
	u32 line_number = 0;
	u8  comment_character = '#';
	b32 skip_comments_and_whitespace = true;
	b32 failed = false;
	const File* file = 0;

	operator bool() const { return !failed; }
};

FileReader init_reader(const File& file);
FileReader init_reader(const char* data, u32 datasize);
b32        next_char(FileReader& reader);
b32        next_line(FileReader& reader); //next_ functions advance the reader's internal read cstring
b32        next_chunk(FileReader& reader, char delimiter, b32 include_delimiter = false);
b32        next_value_from_key(FileReader& reader, const char* key, char inbetween_char, char value_delimiter);
void       read_line(FileReader& reader, cstring& out); //read_ functions place data into an external buffer
void       read_chunk(FileReader& reader, cstring& out, char delimiter); //TODO maybe implement these
void       read_value_from_key(FileReader& reader, cstring& out, const char* key, char value_delimiter);
void       goto_char(FileReader& reader, u32 charnum);
void       goto_line(FileReader& reader, u32 linenum);
void       reset_reader(FileReader& reader);

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
