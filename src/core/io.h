#pragma once
#ifndef DESHI_IO_H
#define DESHI_IO_H

#include "kigu/array.h"
#include "kigu/string.h"
#include "kigu/string_utils.h"

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
	u32 name_length  = 0; //filename, dot, and extension
	u32 short_length = 0; //filename without extension and dot
	u32 ext_length   = 0; //extension length (without the dot)
	char path[MAX_FILEPATH_SIZE] = {}; //C:/directory/filename.ext
	char name[MAX_FILENAME_SIZE] = {}; //filename.ext
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
	
	array<cstring> lines;
	array<cstring> chunks;
	
	operator bool() const { return !failed; }
};

FileReader init_reader(const File& file);
FileReader init_reader(char* data, u32 datasize);
b32        next_char(FileReader& reader);
b32        next_line(FileReader& reader); //next_ functions advance the reader's internal read cstring
b32        next_value_from_key(FileReader& reader, const char* key, char inbetween_char, char value_delimiter);
void       read_line(FileReader& reader, cstring& out); //read_ functions place data into an external buffer
void       read_chunk(FileReader& reader, cstring& out, char delimiter); //TODO maybe implement these
void       read_value_from_key(FileReader& reader, cstring& out, const char* key, char value_delimiter);
void       chunk_file(FileReader& reader, char delimiter, b32 stop_on_newline = false);
void       chunk_file(FileReader& reader, char begin_delimiter, char end_delimiter, b32 stop_on_newline = false);
void       chunk_line(FileReader& reader, u32 line, char delimiter);
void       chunk_line(FileReader& reader, u32 line, char begin_delimiter, char end_delimiter);
void       goto_char(FileReader& reader, u32 charnum);
void       goto_line(FileReader& reader, u32 linenum);
void       reset_reader(FileReader& reader);

//opens a file if it already exists or creates a new one if it doesnt
//this does not load any data, you must use FileReader to do that!
File open_file(const char* path, FileAccessFlags flags);

//returns a temporary array of the files in the target directory
//TODO return carray instead of array
array<File> get_directory_files(const char* directory);
FORCE_INLINE array<File> get_directory_files(cstring directory){ return get_directory_files(directory.str); }
FORCE_INLINE array<File> get_directory_files(File* directory){ return (directory->is_directory) ? get_directory_files(directory->path) : array<File>();}

void delete_file(const char* filepath);
FORCE_INLINE void delete_file(cstring filepath){ return delete_file(filepath.str); }
FORCE_INLINE void delete_file(File* file){ return delete_file(file->path); }

//returns true if a filepath is valid
b32 file_exists(const char* filepath);
FORCE_INLINE b32 file_exists(cstring filepath){ return file_exists(filepath.str); }
FORCE_INLINE b32 file_exists(File* file){ return file_exists(file->path); }

void rename_file(const char* old_filepath, const char* new_filepath);

//returns a temporary cstring of the absolute path to the file or directory specified, empty cstring if it doesn't exist
cstring absolute_path(const char* relative_path);
FORCE_INLINE cstring absolute_path(cstring relative_path){ return absolute_path(relative_path.str); }
FORCE_INLINE cstring absolute_path(File* file){ return absolute_path(file->path); }

//returns a pointer to a temporary File object, 0 if the file does not exist
File* file_info(const char* filepath);
FORCE_INLINE File* file_info(cstring filepath){ return file_info(filepath.str); }

#endif //DESHI_IO_H
