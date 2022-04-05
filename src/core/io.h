#pragma once
#ifndef DESHI_IO_H
#define DESHI_IO_H

#include "kigu/array.h"
#include "kigu/string.h"
#include "kigu/unicode.h"
#include "kigu/string_utils.h"

enum FileAccessFlags_ {
	FileAccess_Read  = 1 << 0,
	FileAccess_Write = 1 << 1,
	FileAccess_Exec  = 1 << 2,
}; typedef u32 FileAccessFlags;

#define MAX_FILEPATH_SIZE 1024
#define MAX_FILENAME_SIZE 256
//TODO maybe implement a callback function feature that allows defining a function to call when a file has been changed externally
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

//filename.ext -> filename.ext
FORCE_INLINE cstring get_file_name(const File& file)      { return cstring{ (char*)file.name, file.name_length }; }
//filename.ext -> filename
FORCE_INLINE cstring get_file_short(const File& file)     { return cstring{ (char*)file.name, file.short_length }; }
//filename.ext -> ext
FORCE_INLINE cstring get_file_extension(const File& file) { return cstring{ (char*)file.name + file.short_length+1, file.ext_length }; }
//filename.ext -> C:/path/to/file/filename.ext
FORCE_INLINE cstring get_file_path(const File& file)      { return cstring{ (char*)file.path, file.path_length }; }

//a helper for reading data from a file or locally stored data.
//
//          raw - cstring that points to the entire buffer of data
//         read - cstring that points to things inside the buffer such as chunks, lines, etc.
//                manipulated either manually or through file reader functions declared below
//  line_number - current line number the read ptr is on
//       failed - set when the file reader fails to initialize
//         file - points to a File struct if the reader if initialized using File
//	
//  operator bool just returns if the FileReader's fail flag is set
//
//  comment_character - UNUSED TODO use this somewhere or remove it
//  skip_comments_and_whitespace - UNUSED TODO use this or remove it
struct FileReader{
	cstring raw  = {}; //points to the entire buffer of data
	cstring read = {}; //points to things inside the buffer such as chunks, lines, etc. use next_ functions to manipulate it
	u32 line_number = 0; //current line that the read pointer is pointing to TODO test this
	u8  comment_character = '#'; //TODO unused
	b32 skip_comments_and_whitespace = true; //TODO unused
	b32 failed = false; //set when the reader fails to open a file
	const File* file = 0; //points to a File struct if one is used to init the reader
	
	array<cstring> lines; //caches lines in the file
	array<cstring> chunks; //stores chunked parts of the file. use chunk_file or chunk_line
	
	operator bool() const { return !failed; }
};

//initializes a new FileReader from an opened File. 
//read starts at the beginning of the file and it's size is equal to the size of the file
//by default caches lines TODO make a way to disable line caching
FileReader init_reader(const File& file);
//initializes a new FileReader from locally allocated data. read starts at the beginning of the data
FileReader init_reader(char* data, u32 datasize);
//ends the reader, freeing any allocated data
void       end_reader(FileReader& reader);
//moves read to the next character and sets it's size to 1
b32        next_char(FileReader& reader);
//moves read to the beginning of the next line and sets it's size to the length of it
b32        next_line(FileReader& reader); 
//moves read to the beginning of a value found from a given key
//if there is no inbetween character you can pass 0 to ignore it
//TODO handle inbetween strings
b32        next_value_from_key(FileReader& reader, const char* key, char inbetween_char, char value_delimiter);
//reads the next line into an externally provided buffer
void       read_line(FileReader& reader, cstring& out); //read_ functions place data into an external buffer
void       read_chunk(FileReader& reader, cstring& out, char delimiter); //TODO maybe implement these
void       read_value_from_key(FileReader& reader, cstring& out, const char* key, char value_delimiter);
//tells the reader to chunk the entire file based on a given delimiter. the chunks are put into the chunks array on the reader
void       chunk_file(FileReader& reader, char delimiter, b32 stop_on_newline = false);
//tells the reader to chunk the entire file based on a given start and end delimiter. this will only create a chunk if it ends and begins with the given delimiters
void       chunk_file(FileReader& reader, char begin_delimiter, char end_delimiter, b32 stop_on_newline = false);
//tells the reader to chunk the entire file by lines and put it in it's lines array
void       chunk_file_by_lines(FileReader& reader);
//tells the reader to chunk the entire current line base on a given delimiter. chunks are placed into the reader's chunks array
void       chunk_line(FileReader& reader, u32 line, char delimiter);
//tells the reader to chunk the entire current line based on a given start and end delimiter. chunks are placed into the reader's chunks array 
void       chunk_line(FileReader& reader, u32 line, char begin_delimiter, char end_delimiter);
//seeks to the given index TODO rename this and its args
void       goto_char(FileReader& reader, u32 charnum);
//seeks to the beginning of a given line number
void       goto_line(FileReader& reader, u32 linenum);
void       reset_reader(FileReader& reader);

//opens a file if it already exists or creates a new one if it doesnt
//this does not load any data, you must use FileReader or read_file to do that
File open_file(const char* path, FileAccessFlags flags);

//closes an opened file
void close_file(File* file);

//reads a file into a char buffer. this allocates a buffer for it and its the user's responsibility to free it
//TODO decide if its necessary to implement this. for ex cases where you dont need to use a FileReader
cstring read_file(const File& file);

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

//returns a temporary cstring containing the entire file's contents
//TODO update this to support Unicode, only ASCII atm
cstring read_entire_file(const char* filepath);

#endif //DESHI_IO_H
