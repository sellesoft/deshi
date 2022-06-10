/* deshi Filesystem Module
Notes:
The only way to create a directory is with file_create().
Function implementations are either in this file or in the platform backend file.
Functions take in both kind of path separators \\ or /, but only output / as a separator.
The path variable on file objects for directories will always end with a /.
Usage of fopen() needs to be in the platform backend because MSVC only accepts ANSI without _wfopen().
Currently this module shoud be considered NOT thread-safe 

Index:
@file_types
  FileAccess
  File
@file_system
  file_exists(str8 path) -> void
  file_create(str8 path) -> void
  file_delete(str8 path) -> void
  file_rename(str8 old_path, str8 new_path) -> void
file_copy(str8 src_path, str8 dst_path) -> void
  file_info(str8 path) -> File
  file_search_directory(str8 directory) -> carray<File>
  file_path_absolute(str8 path) -> str8
  file_path_equal(str8 path1, str8 path2) -> b32
@file_init
  file_init(str8 path, FileAccess access) -> File*
  file_init_if_exists(str8 path, FileAccess access) -> File*
  file_deinit(File* file) -> void
  file_change_access(File* file, FileAccess access) -> void
  file_cursor(File* file, s64 offset) -> void
@file_read
  file_read(File* file, void* buffer, u64 bytes) -> str8
  file_read_alloc(File* file, u64 bytes, Allocator* allocator) -> str8
  file_read_line(File* file, void* buffer, u64 max_bytes) -> str8
  file_read_line_alloc(File* file, Allocator* allocator) -> str8
  file_read_simple(str8 path, Allocator* allocator) -> str8
@file_write
  file_write(File* file, void* data, u64 bytes) -> u64
  file_write_line(File* file, str8 line) -> u64
  file_write_simple(str8 path, void* data, u64 bytes) -> u64
  file_append(File* file, void* data, u64 bytes) -> u64
  file_append_line(File* file, str8 line) -> u64
  file_append_simple(str8 path, void* data, u64 bytes) -> u64
  TODO file_set_size(File* file, s64 bytes) -> void
@file_shared_variables
@file_tests

TODOs:
track when files are changed (NOTE its not really safe right now to keep a file open for a long time unless you know nothing will change it)
use CreateFile instead of FindFirstFile to tell if a file exists (b/c FindFirstFile's returned filename sucks)
replace win32 error messages with our own

References:
https://en.cppreference.com/w/c/io
https://en.cppreference.com/w/cpp/filesystem
*/


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#pragma once
#ifndef DESHI_FILE_H
#define DESHI_FILE_H

#include "kigu/common.h"
#include "kigu/unicode.h"


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @file_types
typedef Flags FileAccess; enum{
	FileAccess_Read     = 1 << 0, //opens the file for reading
	FileAccess_Write    = 1 << 1, //opens the file for writing
	//FileAccess_Exec     = 1 << 2, //TODO(delle) figure out what this means in Win32
	
	FileAccess_Append   = 1 << 4, //opens the file with the cursor at the end
	FileAccess_Create   = 1 << 5, //creates the file if it doesn't exist
	FileAccess_Truncate = 1 << 6, //truncates the file to zero bytes
	
	FileAccess_ReadWrite               = FileAccess_Read|FileAccess_Write,
	FileAccess_ReadAppend              = FileAccess_Read|                 FileAccess_Append,
	FileAccess_ReadWriteCreate         = FileAccess_Read|FileAccess_Write|                                      FileAccess_Create,
	FileAccess_ReadWriteAppend         = FileAccess_Read|FileAccess_Write|FileAccess_Append,
	FileAccess_ReadWriteAppendCreate   = FileAccess_Read|FileAccess_Write|FileAccess_Append|                    FileAccess_Create,
	FileAccess_ReadWriteTruncate       = FileAccess_Read|FileAccess_Write|                  FileAccess_Truncate,
	FileAccess_ReadWriteTruncateCreate = FileAccess_Read|FileAccess_Write|                  FileAccess_Truncate|FileAccess_Create,
	FileAccess_WriteCreate             =                 FileAccess_Write|                                      FileAccess_Create,
	FileAccess_WriteAppend             =                 FileAccess_Write|FileAccess_Append,
	FileAccess_WriteAppendCreate       =                 FileAccess_Write|FileAccess_Append|                    FileAccess_Create,
	FileAccess_WriteTruncate           =                 FileAccess_Write|                  FileAccess_Truncate,
	FileAccess_WriteTruncateCreate     =                 FileAccess_Write|                  FileAccess_Truncate|FileAccess_Create,
};

struct File{
	FILE* handle;
	
	u64 creation_time;
	u64 last_access_time;
	u64 last_write_time;
	u64 bytes;
	b32 is_directory;
	//b32 changed;
	
	str8 path;  //full path
	str8 name;  //filename, dot, and extension
	str8 front; //filename, no extension, no dot
	str8 ext;   //extension, no dot
	
	FileAccess access;
	u64 cursor;
};


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @file_system
//Returns true if a file/directory exists at `path`
external b32 deshi__file_exists(str8 caller_file, upt caller_line, str8 path);
#define file_exists(path) deshi__file_exists(str8_lit(__FILE__),__LINE__, (path))

//Creates an empty file/directory at `path` if one doesn't exist already
//    if needed, this will create multiple directories to make the path valid
//    to create a directory, `path` must end with a '\' or '/'
external void deshi__file_create(str8 caller_file, upt caller_line, str8 path);
#define file_create(path) deshi__file_create(str8_lit(__FILE__),__LINE__, (path))

//Deletes the file/directory (and sub-directories) at `path` if it exists
external void deshi__file_delete(str8 caller_file, upt caller_line, str8 path);
#define file_delete(path) deshi__file_delete(str8_lit(__FILE__),__LINE__, (path))

//Renames the file/directory at `old_path` if it exists to `new_path`
external void deshi__file_rename(str8 caller_file, upt caller_line, str8 old_path, str8 new_path);
#define file_rename(old_path,new_path) deshi__file_rename(str8_lit(__FILE__),__LINE__, (old_path),(new_path))

//Copies the file/directory at `src_path` to `dst_path`
//    does not init the destination file if the source is init
external void deshi__file_copy(str8 caller_file, upt caller_line, str8 src_path, str8 dst_path);
#define file_copy(src_path,dst_path) deshi__file_copy(str8_lit(__FILE__),__LINE__, (src_path),(dst_path))

//Returns a temporary `File` containing information about the file/directory at `path` if it exists
external File deshi__file_info(str8 caller_file, upt caller_line, str8 path);
#define file_info(path) deshi__file_info(str8_lit(__FILE__),__LINE__, (path))

//Returns a temporary array of files/directories in the `directory` if it exists
//TODO rework the return type so this function is C compatible
carray<File> deshi__file_search_directory(str8 caller_file, upt caller_line, str8 directory);
#define file_search_directory(directory) deshi__file_search_directory(str8_lit(__FILE__),__LINE__, (directory))

//Returns a temporary string of the absolute path to the file/directory at `path` if it exists
external str8 deshi__file_path_absolute(str8 caller_file, upt caller_line, str8 path);
#define file_path_absolute(path) deshi__file_path_absolute(str8_lit(__FILE__),__LINE__, (path))

//Returns true if `path1` and `path2` represent the same file/directory on disk
external b32 deshi__file_path_equal(str8 caller_file, upt caller_line, str8 path1, str8 path2);
#define file_path_equal(path1,path2) deshi__file_path_equal(str8_lit(__FILE__),__LINE__, (path1),(path2))


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @file_init
//Returns a `File` pointer initialized with `access`
//    `FileAccess_Append`, FileAccess_Create`, and `FileAccess_Truncate` do not get applied to `File.access`
//    this call is equivalent to `file_change_access()` if there already is an initialized `File` for `path`
//    `ignore_nonexistence` just prevents error messages if the file doesn't exist (will still return 0)
external File* deshi__file_init(str8 caller_file, upt caller_line, str8 path, FileAccess access, b32 ignore_nonexistence);
#define file_init(path,access) deshi__file_init(str8_lit(__FILE__),__LINE__, path,(access),false)
#define file_init_if_exists(path,access) deshi__file_init(str8_lit(__FILE__),__LINE__, (path),(access),true)

//Closes a previously init `file` if it has `FileAccess_Read` or `FileAccess_Write` and deletes the internal `File` object
//    this does not delete the file on disk, call `file_delete()` to do that
external void deshi__file_deinit(str8 caller_file, upt caller_line, File* file);
#define file_deinit(file) deshi__file_deinit(str8_lit(__FILE__),__LINE__, (file))

//Handles conversion of `access` for `file`
//    if new access includes `FileAccess_Read` or `FileAccess_Write`, the file is opened internally if it wasn't already open
//    if new access doesn't include `FileAccess_Read` or `FileAccess_Write`, the file is closed internally if it was open
//    `FileAccess_Append` and `FileAccess_Truncate` are only performed if the file is not already open and do not get applied to `File.access`
external void deshi__file_change_access(str8 caller_file, upt caller_line, File* file, FileAccess access);
#define file_change_access(file,access) deshi__file_change_access(str8_lit(__FILE__),__LINE__, (file),(access))

//Returns a view over the internally initialized files
//TODO rework the return type so this function is C compatible
carray<File*> file_initted_files(); 

//Sets the `File.cursor` of `file` (if it's been init) to `offset` from the beginning if positive
//    this function prevents the `File.cursor` from going above `File.bytes`
//    writing in the middle overwrites rather then inserts
external void deshi__file_cursor(str8 caller_file, upt caller_line, File* file, u64 offset);
#define file_cursor(file,offset) deshi__file_cursor(str8_lit(__FILE__),__LINE__, (file),(offset))


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @file_read
//Copies `bytes` (at most) from `file` (if it's been init) into a `buffer` and returns a str8 view of the bytes written
//    starts reading at `File.cursor` from the beginning and advances the `File.cursor` forward by `bytes`
//    if the end of file is reached before reading all `bytes`, no error occurs and `File.cursor` is set to `File.size`
//    if an error occurs, the `File.cursor` is not affected and an empty str8 is returned
//    writes a null-terminating character to the `buffer` at `bytes+1`
external str8 deshi__file_read(str8 caller_file, upt caller_line, File* file, void* buffer, u64 bytes);
#define file_read(file,buffer,bytes) deshi__file_read(str8_lit(__FILE__),__LINE__, (file),(buffer),(bytes))

//Copies `bytes` (at most) from `file` (if it's been init) into a str8 allocated with `allocator`
//    starts reading at `File.cursor` from the beginning and advances the `File.cursor` forward by `bytes`
//    if the end of file is reached before reading all `bytes`, no error occurs and `File.cursor` is set to `File.size`
//    if an error occurs, the `File.cursor` is not affected and an empty str8 is returned
//    allocates an extra byte for a null-terminating character
external str8 deshi__file_read_alloc(str8 caller_file, upt caller_line, File* file, u64 bytes, Allocator* allocator);
#define file_read_alloc(file,bytes,allocator) deshi__file_read_alloc(str8_lit(__FILE__),__LINE__, (file),(bytes),(allocator))

//Copies until the next line ending or file end from `file` (if it's been init) into a `buffer` (capped at `max_bytes` written) and returns a str8 view of the bytes written
//    starts reading at `File.cursor` from the beginning and advances the `File.cursor` past the line ending
//    does not copy the line endings to `buffer`
//    if the end of file is reached, no error occurs and `File.cursor` is set to `File.size`
//    if an error occurs, the `File.cursor` is not affected and an empty str8 is returned
//    writes a null-terminating character to the `buffer` after the string (even if it has to cut off part of the string to fit into `max_bytes`)
external str8 deshi__file_read_line(str8 caller_file, upt caller_line, File* file, void* buffer, u64 max_bytes);
#define file_read_line(file,buffer,bytes) deshi__file_read_line(str8_lit(__FILE__),__LINE__, (file),(buffer),(bytes))

//Copies until the next line ending or file end from `file` (if it's been init) into a str8 allocated with `a`
//    starts reading at `File.cursor` from the beginning and advances the `File.cursor` past the line ending
//    does not copy the line endings to the str8
//    if the end of file is reached, no error occurs and `File.cursor` is set to `File.size`
//    if an error occurs, the `File.cursor` is not affected and an empty str8 is returned
//    allocates an extra byte for a null-terminating character
external str8 deshi__file_read_line_alloc(str8 caller_file, upt caller_line, File* file, Allocator* allocator);
#define file_read_line_alloc(file,allocator) deshi__file_read_line_alloc(str8_lit(__FILE__),__LINE__, (file),(allocator))

//Returns the entire contents (allocated with `allocator`) of the file at `path` if it exists
//    allocatoes and extra bytes for a null-terminating character
external str8 deshi__file_read_simple(str8 caller_file, upt caller_line, str8 path, Allocator* allocator);
#define file_read_simple(path,allocator) deshi__file_read_simple(str8_lit(__FILE__),__LINE__, (path),(allocator))


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @file_write
//Writes `bytes` from `data` to `file` (if it's been init) and returns the number of bytes written (which may be less if an error occurred)
//    starts writing at `File.cursor` and advances the `File.cursor` forward by `bytes`
external u64 deshi__file_write(str8 caller_file, upt caller_line, File* file, void* data, u64 bytes);
#define file_write(file,data,bytes) deshi__file_write(str8_lit(__FILE__),__LINE__, (file),(data),(bytes))

//Writes `line` and a newline to `file` (if it's been init) and returns the number of bytes written (which may be less if an error occurred)
//    starts writing at `File.cursor` and advances the `File.cursor` past the line ending
//    regardless of platform, this only writes a single newline character '\n'
external u64 deshi__file_write_line(str8 caller_file, upt caller_line, File* file, str8 line);
#define file_write_line(file,line) deshi__file_write_line(str8_lit(__FILE__),__LINE__, (file),(line))

//Creates a file at `path` and writes `bytes` of `data` to it then closes the file and returns the number of bytes written (which may be less if an error occurred)
//    this truncates the file at `path` if it exists
external u64 deshi__file_write_simple(str8 caller_file, upt caller_line, str8 path, void* data, u64 bytes);
#define file_write_simple(path,data,bytes) deshi__file_write_simple(str8_lit(__FILE__),__LINE__, (path),(data),(bytes))

//Writes `bytes` from `data` to the end of `file` (if it's been init) and returns the number of bytes written (which may be less if an error occurred)
//    always writes at the end of the file and doesn't modify the `File.cursor`
external u64 deshi__file_append(str8 caller_file, upt caller_line, File* file, void* data, u64 bytes);
#define file_append(file,data,bytes) deshi__file_append(str8_lit(__FILE__),__LINE__, (file),(data),(bytes))

//Writes `line` and a newline to the end of `file` (if it's been init) and returns the number of bytes written (which may be less if an error occurred)
//    always writes at the end of the file and doesn't modify the `File.cursor`
//    regardless of platform, this only writes a single newline character '\n'
external u64 deshi__file_append_line(str8 caller_file, upt caller_line, File* file, str8 line);
#define file_append_line(file,line) deshi__file_append_line(str8_lit(__FILE__),__LINE__, (file),(line))

//Opens the file at `path` and writes `bytes` of `data` to the end of it then closes the file
//    this creates the file at `path` if one doesn't exist
external u64 deshi__file_append_simple(str8 caller_file, upt caller_line, str8 path, void* data, u64 bytes);
#define file_append_simple(path,data,bytes) deshi__file_append_simple(str8_lit(__FILE__),__LINE__, (path),(data),(bytes))

//Resizes a file to a specified size
//external void deshi__file_set_size(str8 caller_file, upt caller_line, File* file, s64 bytes);
//#define file_set_size(file, bytes) deshi__file_set_size(str8_lit(__FILE__),__LINE__,(file),(bytes));


#endif //DESHI_FILE_H
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#ifdef DESHI_IMPLEMENTATION
#include "logger.h"
#include "kigu/array.h"


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @file_shared_variables
local b32 file_crash_on_error = false;
local array<File*> file_files;


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @file_init
void
deshi__file_cursor(str8 caller_file, upt caller_line, File* file, u64 offset){
	if(file == 0){
		LogE("file","file_cursor() was passed a null `file` pointer at ",caller_file,"(",caller_line,")");
		return;
	}
	
	if(file->handle){
		offset = ClampMax(offset, file->bytes);
		if(fseek(file->handle, offset, SEEK_SET) == 0){
			file->cursor = offset;
		}else{
			LogE("file","fseek() failed in file_cursor() call on file '",file->path,"' with offset: ",offset);
		}
	}else{
		LogE("file","file_cursor() called on a closed file '",file->path,"' at ",caller_file,"(",caller_line,")");
	}
}


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @file_read
str8
deshi__file_read(str8 caller_file, upt caller_line, File* file, void* buffer, u64 bytes){
	if(file == 0){
		LogE("file","file_read() was passed a null `file` pointer at ",caller_file,"(",caller_line,")");
		return str8{};
	}
	if(buffer == 0 && bytes > 0){
		LogE("file","file_read() was passed a null `buffer` pointer (with non-zero `bytes` to read) at ",caller_file,"(",caller_line,")");
		return str8{};
	}
	
	if(file->handle){
		int c = 0;
		s64 bytes_read = 0;
		while(bytes_read < bytes){
			c = fgetc(file->handle);
			if(c == EOF) break;
			bytes_read += 1;
		}
		
		u64 restore_cursor = file->cursor;
		if(c == EOF){
			if(feof(file->handle)){
				clearerr(file->handle);
				file->cursor = file->bytes;
			}else if(ferror(file->handle)){
				LogE("file","fgetc() failed in file_read_line() call on file '",file->path,"(",file->cursor,")' at ",caller_file,"(",caller_line,")");
				fseek(file->handle, file->cursor, SEEK_SET);
				clearerr(file->handle);
				return str8{};
			}
		}else{
			file->cursor += bytes_read;
		}
		
		fseek(file->handle, restore_cursor, SEEK_SET);
		fread(buffer, 1,bytes_read, file->handle);
		fseek(file->handle, file->cursor,   SEEK_SET);
		((u8*)buffer)[bytes_read] = '\0';
		return str8{(u8*)buffer, bytes_read};
	}else{
		LogE("file","file_read() called on a closed file '",file->path,"' at ",caller_file,"(",caller_line,")");
		return str8{};
	}
}

str8
deshi__file_read_alloc(str8 caller_file, upt caller_line, File* file, u64 bytes, Allocator* allocator){
	if(file == 0){
		LogE("file","file_read_alloc() was passed a null `file` pointer at ",caller_file,"(",caller_line,")");
		return str8{};
	}
	if(allocator == 0 && bytes > 0){
		LogE("file","file_read_alloc() was passed a null `allocator` pointer (with non-zero `bytes` to read) at ",caller_file,"(",caller_line,")");
		return str8{};
	}
	
	if(file->handle){
		int c = 0;
		s64 bytes_read = 0;
		while(bytes_read < bytes){
			c = fgetc(file->handle);
			if(c == EOF) break;
			bytes_read += 1;
		}
		
		u64 restore_cursor = file->cursor;
		if(c == EOF){
			if(feof(file->handle)){
				clearerr(file->handle);
				file->cursor = file->bytes;
			}else if(ferror(file->handle)){
				LogE("file","fgetc() failed in file_read_line() call on file '",file->path,"(",file->cursor,")' at ",caller_file,"(",caller_line,")");
				fseek(file->handle, file->cursor, SEEK_SET);
				clearerr(file->handle);
				return str8{};
			}
		}else{
			file->cursor += bytes_read;
		}
		
		str8 result{(u8*)allocator->reserve(bytes_read+1), bytes_read};
		fseek(file->handle, restore_cursor, SEEK_SET);
		fread(result.str, 1,bytes_read, file->handle);
		fseek(file->handle, file->cursor,   SEEK_SET);
		result.str[bytes_read] = '\0';
		return result;
	}else{
		LogE("file","file_read_alloc() called on a closed file '",file->path,"' at ",caller_file,"(",caller_line,")");
		return str8{};
	}
}

str8
deshi__file_read_line(str8 caller_file, upt caller_line, File* file, void* buffer, u64 max_bytes){
	if(file == 0){
		LogE("file","file_read_line() was passed a null `file` pointer at ",caller_file,"(",caller_line,")");
		return str8{};
	}
	if(buffer == 0){
		LogE("file","file_read_line() was passed a null `buffer` pointer at ",caller_file,"(",caller_line,")");
		return str8{};
	}
	
	max_bytes -= 1; //NOTE(delle) leave space for \0
	if(file->handle){
		int c = 0;
		s64 bytes_read = 0;
		while(true && max_bytes){
			c = fgetc(file->handle);
			if(c == EOF || c == '\r' || c == '\n') break;
			bytes_read += 1;
		}
		
		u64 restore_cursor = file->cursor;
		if(c == EOF){
			if(feof(file->handle)){
				clearerr(file->handle);
				file->cursor = file->bytes;
			}else if(ferror(file->handle)){
				LogE("file","fread() failed in file_read_line_alloc() call on file '",file->path,"(",file->cursor,")' at ",caller_file,"(",caller_line,")");
				fseek(file->handle, file->cursor, SEEK_SET);
				clearerr(file->handle);
				return str8{};
			}
		}else if(c == '\r'){
			file->cursor += bytes_read+1;
			if((c = fgetc(file->handle)) == '\n') file->cursor += 1;
		}else if(c == '\n'){
			file->cursor += bytes_read+1;
		}else{
			file->cursor += bytes_read;
		}
		
		fseek(file->handle, restore_cursor, SEEK_SET);
		fread(buffer, 1,bytes_read, file->handle);
		fseek(file->handle, file->cursor,   SEEK_SET);
		((u8*)buffer)[bytes_read] = '\0';
		return str8{(u8*)buffer, bytes_read};
	}else{
		LogE("file","file_read_line() called on a closed file '",file->path,"' at ",caller_file,"(",caller_line,")");
		return str8{};
	}
}

str8
deshi__file_read_line_alloc(str8 caller_file, upt caller_line, File* file, Allocator* allocator){
	if(file == 0){
		LogE("file","file_read_line_alloc() was passed a null `file` pointer at ",caller_file,"(",caller_line,")");
		return str8{};
	}
	if(allocator == 0){
		LogE("file","file_read_line_alloc() was passed a null `allocator` pointer at ",caller_file,"(",caller_line,")");
		return str8{};
	}
	
	if(file->handle){
		int c = 0;
		s64 bytes_read = 0;
		while(true){
			c = fgetc(file->handle);
			if(c == EOF || c == '\r' || c == '\n') break;
			bytes_read += 1;
		}
		
		u64 restore_cursor = file->cursor;
		if(c == EOF){
			if(feof(file->handle)){
				clearerr(file->handle);
				file->cursor = file->bytes;
			}else if(ferror(file->handle)){
				LogE("file","fread() failed in file_read_line_alloc() call on file '",file->path,"(",file->cursor,")' at ",caller_file,"(",caller_line,")");
				fseek(file->handle, file->cursor, SEEK_SET);
				clearerr(file->handle);
				return str8{};
			}
		}else if(c == '\r'){
			file->cursor += bytes_read+1;
			if((c = fgetc(file->handle)) == '\n') file->cursor += 1;
		}else if(c == '\n'){
			file->cursor += bytes_read+1;
		}else{
			file->cursor += bytes_read;
		}
		
		str8 result{(u8*)allocator->reserve(bytes_read+1), bytes_read};
		fseek(file->handle, restore_cursor, SEEK_SET);
		fread(result.str, 1,bytes_read, file->handle);
		fseek(file->handle, file->cursor,   SEEK_SET);
		return result;
	}else{
		LogE("file","file_read_line_alloc() called on a closed file '",file->path,"' at ",caller_file,"(",caller_line,")");
		return str8{};
	}
}


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @file_write
u64
deshi__file_write(str8 caller_file, upt caller_line, File* file, void* data, u64 bytes){
	if(file == 0){
		LogE("file","file_write() was passed a null `file` pointer at ",caller_file,"(",caller_line,")");
		return 0;
	}
	if(data == 0 && bytes > 0){
		LogE("file","file_write() was passed a null `data` pointer (with non-zero `bytes` to read) at ",caller_file,"(",caller_line,")");
		return 0;
	}
	
	if(file->handle){
		size_t bytes_written = fwrite(data, 1,bytes, file->handle);
		fflush(file->handle);
		file->cursor += bytes_written;
		if(file->cursor > file->bytes) file->bytes = file->cursor;
		return bytes_written;
	}else{
		LogE("file","file_write() called on a closed file '",file->path,"' at ",caller_file,"(",caller_line,")");
		return 0;
	}
}

u64
deshi__file_write_line(str8 caller_file, upt caller_line, File* file, str8 line){
	if(file == 0){
		LogE("file","file_write_line() was passed a null `file` pointer at ",caller_file,"(",caller_line,")");
		return 0;
	}
	if(!line){
		return 0;
	}
	
	if(file->handle){
		size_t bytes_written = fwrite(line.str, 1,line.count, file->handle);
		if(fputc('\n', file->handle) != EOF) bytes_written += 1;
		file->cursor += bytes_written;
		if(file->cursor > file->bytes) file->bytes = file->cursor;
		return bytes_written;
	}else{
		LogE("file","file_write_line() called on a closed file '",file->path,"' at ",caller_file,"(",caller_line,")");
		return 0;
	}
}

u64
deshi__file_append(str8 caller_file, upt caller_line, File* file, void* data, u64 bytes){
	if(file == 0){
		LogE("file","file_append() was passed a null `file` pointer at ",caller_file,"(",caller_line,")");
		return 0;
	}
	if(data == 0 && bytes > 0){
		LogE("file","file_append() was passed a null `data` pointer (with non-zero `bytes` to read) at ",caller_file,"(",caller_line,")");
		return 0;
	}
	
	if(file->handle){
		fseek(file->handle, file->bytes,  SEEK_SET);
		size_t bytes_written = fwrite(data, 1,bytes, file->handle);
		fseek(file->handle, file->cursor, SEEK_SET);
		file->bytes += bytes_written;
		return bytes_written;
	}else{
		LogE("file","file_append() called on a closed file '",file->path,"' at ",caller_file,"(",caller_line,")");
		return 0;
	}
}

u64
deshi__file_append_line(str8 caller_file, upt caller_line, File* file, str8 line){
	if(file == 0){
		LogE("file","file_append_line() was passed a null `file` pointer at ",caller_file,"(",caller_line,")");
		return 0;
	}
	if(!line){
		return 0;
	}
	
	if(file->handle){
		fseek(file->handle, file->bytes,  SEEK_SET);
		size_t bytes_written = fwrite(line.str, 1,line.count, file->handle);
		if(fputc('\n', file->handle) != EOF) bytes_written += 1;
		fseek(file->handle, file->cursor, SEEK_SET);
		file->bytes += bytes_written;
		return bytes_written;
	}else{
		LogE("file","file_append_line() called on a closed file '",file->path,"' at ",caller_file,"(",caller_line,")");
		return 0;
	}
}


#endif //DESHI_IMPLEMENTATION
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//// @file_tests
#ifdef DESHI_TESTS
#include "test.h"


void TEST_deshi_file(){
	if(file_exists(str8_lit("data/test_deshi_file"))){
		file_delete(str8_lit("data/test_deshi_file"));
	}
	
	{//system
		//// exists ////
		Test(file_exists(str8_lit("data")));
		Test(file_exists(str8_lit("data/")));
		Test(file_exists(str8_lit("data\\")));
		
		Test(!file_exists(str8_lit("datadbasjkdabskjdasbkjdasbjkds")));
		Test(!file_exists(str8_lit("datadbasjkdabskjdasbkjdasbjkds/")));
		Test(!file_exists(str8_lit("datadbasjkdabskjdasbkjdasbjkds\\")));
		
		TestExpectedLog("[FILE-ERROR] file_exists() was passed an empty `path` at "__FILE__"(",__LINE__+1,")");
		Test(!file_exists(str8{}));
		TestExpectedLog("[FILE-ERROR] file_exists() was passed an empty `path` at "__FILE__"(",__LINE__+1,")");
		Test(!file_exists(str8_lit("")));
		
		//// create ////
		file_create(str8_lit("data/"));
		Test(file_exists(str8_lit("data/")));
		
		file_create(str8_lit("data/test_deshi_file/"));
		Test(file_exists(str8_lit("data/test_deshi_file")));
		
		file_create(str8_lit("data/test_deshi_file/food/apple.txt"));
		Test(file_exists(str8_lit("data/test_deshi_file/food/apple.txt")));
		
		file_create(str8_lit("data/test_deshi_file/food/apple.txt"));
		Test(file_exists(str8_lit("data/test_deshi_file/food/apple.txt")));
		
		file_create(str8_lit("data/test_deshi_file/不明誘惑/"));
		Test(file_exists(str8_lit("data/test_deshi_file/不明誘惑")));
		
		file_create(str8_lit("data/test_deshi_file/不明誘惑/悪徳.市"));
		Test(file_exists(str8_lit("data/test_deshi_file/不明誘惑/悪徳.市")));
		
		TestExpectedLog("[FILE-ERROR] file_create() was passed an empty `path` at "__FILE__"(",__LINE__+1,")");
		file_create(str8{});
		TestExpectedLog("[FILE-ERROR] file_create() was passed an empty `path` at "__FILE__"(",__LINE__+1,")");
		file_create(str8_lit(""));
		
		//// delete ////
		file_delete(str8_lit("data/test_deshi_file/food/"));
		Test(!file_exists(str8_lit("data/test_deshi_file/food/apple.txt")));
		Test(!file_exists(str8_lit("data/test_deshi_file/food")));
		
		file_delete(str8_lit("data/test_deshi_file/不明誘惑"));
		Test(!file_exists(str8_lit("data/test_deshi_file/不明誘惑/悪徳.市")));
		Test(!file_exists(str8_lit("data/test_deshi_file/不明誘惑")));
		
		TestExpectedLog("[FILE-ERROR] file_delete() was passed an empty `path` at "__FILE__"(",__LINE__+1,")");
		file_delete(str8{});
		TestExpectedLog("[FILE-ERROR] file_delete() was passed an empty `path` at "__FILE__"(",__LINE__+1,")");
		file_delete(str8_lit(""));
		TestExpectedLog("[FILE-ERROR] File deletion can only occur within the data folder. Input path: apple/");
		file_delete(str8_lit("apple/"));
		
		//// rename ////
		file_create(str8_lit("data/test_deshi_file/food/apple.txt"));
		file_rename(str8_lit("data/test_deshi_file/food/apple.txt"), str8_lit("data/test_deshi_file/food/banana.txt"));
		Test(!file_exists(str8_lit("data/test_deshi_file/food/apple.txt")));
		Test(file_exists(str8_lit("data/test_deshi_file/food/banana.txt")));
		
		file_rename(str8_lit("data/test_deshi_file/food/"), str8_lit("data/test_deshi_file/fruits/"));
		Test(!file_exists(str8_lit("data/test_deshi_file/food/apple.txt")));
		Test(!file_exists(str8_lit("data/test_deshi_file/food/banana.txt")));
		Test(!file_exists(str8_lit("data/test_deshi_file/food")));
		Test(file_exists(str8_lit("data/test_deshi_file/fruits")));
		Test(file_exists(str8_lit("data/test_deshi_file/fruits/banana.txt")));
		
		TestExpectedLog("[FILE-ERROR] file_rename() was passed an empty `old_path` at "__FILE__"(",__LINE__+1,")");
		file_rename(str8{}, str8{});
		TestExpectedLog("[FILE-ERROR] file_rename() was passed an empty `old_path` at "__FILE__"(",__LINE__+1,")");
		file_rename(str8_lit(""), str8_lit(""));
		TestExpectedLog("[FILE-ERROR] file_rename() was passed an empty `new_path` at "__FILE__"(",__LINE__+1,")");
		file_rename(str8_lit("test"), str8{});
		TestExpectedLog("[FILE-ERROR] file_rename() was passed an empty `new_path` at "__FILE__"(",__LINE__+1,")");
		file_rename(str8_lit("test"), str8_lit(""));
		TestExpectedLog("[FILE-ERROR] File renaming can only occur within the data folder. Input old path: test/");
		file_rename(str8_lit("test/"), str8_lit("apple/"));
		TestExpectedLog("[FILE-ERROR] File renaming can only occur within the data folder. Input new path: apple/");
		file_rename(str8_lit("data/"), str8_lit("apple/"));
		TestExpectedLog("[FILE-ERROR] File renaming can only occur within the data folder. Input new path: apple/test.txt");
		file_rename(str8_lit("data/test.txt"), str8_lit("apple/test.txt"));
#if DESHI_WINDOWS
		TestExpectedLog("[WIN32-ERROR] MoveFileW failed with error 3: The system cannot find the path specified.\n data/test_deshi_file/food/apple.txt");
#else
#  error test not setup yet
#endif
		file_rename(str8_lit("data/test_deshi_file/food/apple.txt"), str8_lit("data/test_deshi_file/food/banana.txt"));
		
		//// info ////
		File file = file_info(str8_lit("data/"));
		Test(file.creation_time != 0);
		Test(file.last_access_time >= file.creation_time);
		Test(file.last_write_time >= file.creation_time);
		Test(file.bytes == 0);
		Test(file.is_directory);
		Test(!file.changed);
		Test(str8_ends_with(file.path, str8_lit("data/")));
		Test(str8_equal_lazy(file.name, str8{}));
		Test(str8_equal_lazy(file.front, str8{}));
		Test(str8_equal_lazy(file.ext, str8{}));
		Test(file.access == 0);
		Test(file.cursor == 0);
		
		file = file_info(str8_lit("data/test_deshi_file/fruits/banana.txt"));
		Test(file.creation_time != 0);
		Test(file.last_access_time >= file.creation_time);
		Test(file.last_write_time >= file.creation_time);
		Test(file.bytes == 0);
		Test(!file.is_directory);
		Test(!file.changed);
		Test(str8_ends_with(file.path, str8_lit("data/test_deshi_file/fruits/banana.txt")));
		Test(str8_equal_lazy(file.name, str8_lit("banana.txt")));
		Test(str8_equal_lazy(file.front, str8_lit("banana")));
		Test(str8_equal_lazy(file.ext, str8_lit("txt")));
		Test(file.access == 0);
		Test(file.cursor == 0);
		
#if DESHI_WINDOWS
		TestExpectedLog("[WIN32-ERROR] FindFirstFileW failed with error 2: The system cannot find the file specified.\n data/test_deshi_file/food");
#else
#  error test not setup yet
#endif
		file = file_info(str8_lit("data/test_deshi_file/food"));
#if DESHI_WINDOWS
		TestExpectedLog("[WIN32-ERROR] FindFirstFileW failed with error 2: The system cannot find the file specified.\n data/test_deshi_file/fruits/apple.txt");
#else
#  error test not setup yet
#endif
		file = file_info(str8_lit("data/test_deshi_file/fruits/apple.txt"));
		Test(file.creation_time == 0);
		
		//// search_directory ////
		file_create(str8_lit("data/test_deshi_file/fruits/apple.bin"));
		carray<File> files = file_search_directory(str8_lit("data/test_deshi_file/fruits/"));
		Test(files.count == 2);
		u32 apple_index = 0, banana_index = 1;
		if(str8_equal_lazy(files[0].front, str8_lit("banana"))) Swap(apple_index, banana_index);
		Test(files[banana_index].creation_time != 0);
		Test(files[banana_index].last_access_time >= files[banana_index].creation_time);
		Test(files[banana_index].last_write_time >= files[banana_index].creation_time);
		Test(files[banana_index].bytes == 0);
		Test(!files[banana_index].is_directory);
		Test(!files[banana_index].changed);
		Test(str8_ends_with(files[banana_index].path, str8_lit("data/test_deshi_file/fruits/banana.txt")));
		Test(str8_equal_lazy(files[banana_index].name, str8_lit("banana.txt")));
		Test(str8_equal_lazy(files[banana_index].front, str8_lit("banana")));
		Test(str8_equal_lazy(files[banana_index].ext, str8_lit("txt")));
		Test(files[banana_index].access == 0);
		Test(files[banana_index].cursor == 0);
		Test(files[apple_index].creation_time != 0 && files[apple_index].creation_time >= files[banana_index].creation_time);
		Test(files[apple_index].last_access_time >= files[apple_index].creation_time && files[apple_index].last_access_time >= files[banana_index].last_access_time);
		Test(files[apple_index].last_write_time >= files[apple_index].creation_time && files[apple_index].last_write_time >= files[banana_index].last_write_time);
		Test(files[apple_index].bytes == 0);
		Test(!files[apple_index].is_directory);
		Test(!files[apple_index].changed);
		Test(str8_ends_with(files[apple_index].path, str8_lit("data/test_deshi_file/fruits/apple.bin")));
		Test(str8_equal_lazy(files[apple_index].name, str8_lit("apple.bin")));
		Test(str8_equal_lazy(files[apple_index].front, str8_lit("apple")));
		Test(str8_equal_lazy(files[apple_index].ext, str8_lit("bin")));
		Test(files[apple_index].access == 0);
		Test(files[apple_index].cursor == 0);
		
#if DESHI_WINDOWS
		TestExpectedLog("[WIN32-ERROR] FindFirstFileW failed with error 3: The system cannot find the path specified.\n data/test_deshi_file/food/");
#else
#  error test not setup yet
#endif
		files = file_search_directory(str8_lit("data/test_deshi_file/food/"));
		Test(files.count == 0);
		Test(files.data == 0);
		
		//// path_absolute ////
		Test(str8_ends_with(file_path_absolute(str8_lit("data/test_deshi_file/fruits/apple.bin")), str8_lit("data/test_deshi_file/fruits/apple.bin")));
		Test(str8_ends_with(file_path_absolute(str8_lit("data/test_deshi_file/fruits")), str8_lit("data/test_deshi_file/fruits/")));
		Test(str8_ends_with(file_path_absolute(str8_lit("data/test_deshi_file/")), str8_lit("data/test_deshi_file/")));
#if 0 //NOTE(delle) user specific path testing
		Test(str8_equal_lazy(file_path_absolute(str8_lit("data/test_deshi_file/fruits/apple.bin")), str8_lit("W:/suugu/data/test_deshi_file/fruits/apple.bin")));
		Test(str8_equal_lazy(file_path_absolute(str8_lit("data/test_deshi_file/fruits")), str8_lit("W:/suugu/data/test_deshi_file/fruits/")));
#endif
		
#if DESHI_WINDOWS
		TestExpectedLog("[WIN32-ERROR] FindFirstFileW failed with error 2: The system cannot find the file specified.\n data/test_deshi_file/food/");
#else
#  error test not setup yet
#endif
		file_path_absolute(str8_lit("data/test_deshi_file/food/"));
		
		//// path_equal ////
		Test(file_path_equal(str8_lit("data/test_deshi_file/fruits/apple.bin"), str8_lit("data/test_deshi_file/fruits/apple.bin")));
		Test(file_path_equal(str8_lit("data\\test_deshi_file\\fruits\\apple.bin"), str8_lit("data/test_deshi_file/fruits/apple.bin")));
		Test(file_path_equal(str8_lit("data/test_deshi_file/fruits/apple.bin"), str8_lit("data\\test_deshi_file\\fruits\\apple.bin")));
		Test(file_path_equal(str8_lit("data/test_deshi_file/fruits/apple.bin"), str8_lit("data\\test_deshi_file/fruits\\apple.bin")));
		Test(file_path_equal(str8_lit("data/test_deshi_file/fruits/"), str8_lit("data/test_deshi_file/fruits")));
		Test(file_path_equal(str8_lit("data/test_deshi_file/fruits"), str8_lit("data/test_deshi_file/fruits/")));
		Test(file_path_equal(str8_lit("data/test_deshi_file/fruits\\"), str8_lit("data/test_deshi_file/fruits/")));
		Test(file_path_equal(str8_lit("data/test_deshi_file/fruits/"), str8_lit("data/test_deshi_file/fruits\\")));
		
		Test(!file_path_equal(str8_lit("data/test_deshi_file/fruits/apple.bin"), str8_lit("data/test_deshi_file/fruits/banana.txt")));
		Test(!file_path_equal(str8_lit("data/test_deshi_file/fruits/apple.bin"), str8_lit("data/test_deshi_file/fruits/apple.bi")));
		Test(!file_path_equal(str8_lit("data/test_deshi_file/fruits/apple.bin"), str8_lit("data/test_deshi_file/fruits/")));
		
		TestPassed("core/file/system");
	}
	
	{//simple read/write/append
		File file1 = file_info(str8_lit("data/test_deshi_file/fruits/banana.txt"));
		
		//// write ////
		str8 s1 = str8_lit("woah, this is a banana!");
		u32 count = file_write_simple(str8_lit("data/test_deshi_file/fruits/banana.txt"), s1.str, s1.count);
		File file2 = file_info(str8_lit("data/test_deshi_file/fruits/banana.txt"));
		Test(count == s1.count);
		Test(file2.bytes == s1.count);
		
		//// truncate write ////
		str8 s2 = str8_lit("アクアマン");
		count = file_write_simple(str8_lit("data/test_deshi_file/fruits/banana.txt"), s2.str, s2.count);
		file1 = file_info(str8_lit("data/test_deshi_file/fruits/banana.txt"));
		Test(count == s2.count);
		Test(file1.bytes == s2.count);
		
		//// append ////
		count = file_append_simple(str8_lit("data/test_deshi_file/fruits/banana.txt"), s1.str, s1.count);
		file2 = file_info(str8_lit("data/test_deshi_file/fruits/banana.txt"));
		Test(count == s1.count);
		Test(file2.bytes == s2.count + s1.count);
		
		//// read ////
		str8 read = file_read_simple(str8_lit("data/test_deshi_file/fruits/banana.txt"), deshi_temp_allocator);
		file1 = file_info(str8_lit("data/test_deshi_file/fruits/banana.txt"));
		Test(read.count == s2.count + s1.count);
		Test(str8_begins_with(read, s2));
		Test(str8_ends_with(read, s1));
		Test(file1.bytes == s2.count + s1.count);
		
		TestPassed("core/file/simple");
	}
	
	{//init
		file_create(str8_lit("data/test_deshi_file/不明誘惑/"));
		
		//// init ////
		File* file = file_init(str8_lit("data/test_deshi_file/不明誘惑/悪徳.市"), FileAccess_ReadWriteCreate);
		Test(*file_initted_files().data == file);
		Test(file_initted_files().count == 1);
		Test(file->handle != 0);
		Test(file->creation_time != 0);
		Test(file->last_access_time >= file->creation_time);
		Test(file->last_write_time >= file->creation_time);
		Test(file->bytes == 0);
		Test(!file->is_directory);
		Test(!file->changed);
		Test(str8_ends_with(file->path, str8_lit("data/test_deshi_file/不明誘惑/悪徳.市")));
		Test(str8_equal_lazy(file->name, str8_lit("悪徳.市")));
		Test(str8_equal_lazy(file->front, str8_lit("悪徳")));
		Test(str8_equal_lazy(file->ext, str8_lit("市")));
		Test(file->access == FileAccess_ReadWrite);
		Test(file->cursor == 0);
		
		file = file_init(str8_lit("data/test_deshi_file/不明誘惑/悪徳.市"), FileAccess_ReadWriteCreate);
		Test(*file_initted_files().data == file);
		Test(file_initted_files().count == 1);
		Test(file->handle != 0);
		Test(file->creation_time != 0);
		Test(file->last_access_time >= file->creation_time);
		Test(file->last_write_time >= file->creation_time);
		Test(file->bytes == 0);
		Test(!file->is_directory);
		Test(!file->changed);
		Test(str8_ends_with(file->path, str8_lit("data/test_deshi_file/不明誘惑/悪徳.市")));
		Test(str8_equal_lazy(file->name, str8_lit("悪徳.市")));
		Test(str8_equal_lazy(file->front, str8_lit("悪徳")));
		Test(str8_equal_lazy(file->ext, str8_lit("市")));
		Test(file->access == FileAccess_ReadWrite);
		Test(file->cursor == 0);
		
#if DESHI_WINDOWS
		TestExpectedLog("[WIN32-ERROR] FindFirstFileW failed with error 2: The system cannot find the file specified.\n data/test_deshi_file/fruits/apple.txt");
#else
#  error test not setup yet
#endif
		File* file2 = file_init(str8_lit("data/test_deshi_file/fruits/apple.txt"), FileAccess_ReadWrite);
		Test(file2 == 0);
		
		//// change_access ////
		file_change_access(file, FileAccess_ReadAppend);
		Test(file->handle != 0);
		Test(file->access == FileAccess_Read);
		Test(file->cursor == 0);
		
		file_change_access(file, FileAccess_WriteTruncate);
		Test(file->handle != 0);
		Test(file->access == FileAccess_Write);
		Test(file->cursor == 0);
		
		//// deinit ////
		file_deinit(file);
		Test(file_initted_files().count == 0);
		
		//// append ////
		str8 s1 = str8_lit("aaabbbccc");
		file_write_simple(str8_lit("data/test_deshi_file/不明誘惑/悪徳.市"), s1.str, s1.count);
		file = file_init(str8_lit("data/test_deshi_file/不明誘惑/悪徳.市"), FileAccess_ReadWriteAppend);
		Test(file->handle != 0);
		Test(file->bytes == 9);
		Test(file->access == FileAccess_ReadWrite);
		Test(file->cursor == 9);
		file_deinit(file);
		
		//// truncate ////
		str8 s2 = str8_lit("aaabbbcccddd");
		file_write_simple(str8_lit("data/test_deshi_file/不明誘惑/悪徳.市"), s2.str, s2.count);
		file = file_init(str8_lit("data/test_deshi_file/不明誘惑/悪徳.市"), FileAccess_WriteTruncate);
		Test(file->handle != 0);
		Test(file->bytes == 0);
		Test(file->access == FileAccess_Write);
		Test(file->cursor == 0);
		file_deinit(file);
		
		//// cursor ////
		file_write_simple(str8_lit("data/test_deshi_file/不明誘惑/悪徳.市"), s1.str, s1.count);
		file = file_init(str8_lit("data/test_deshi_file/不明誘惑/悪徳.市"), FileAccess_ReadAppend);
		Test(file->bytes == 9);
		Test(file->cursor == 9);
		
		file_cursor(file, 0);
		Test(file->cursor == 0);
		file_cursor(file, 2);
		Test(file->cursor == 2);
		file_cursor(file, 16);
		Test(file->cursor == 9);
		
		file_deinit(file);
		TestPassed("core/file/init");
	}
	
	File* file = file_init(str8_lit("data/test_deshi_file/不明誘惑/悪徳.市"), FileAccess_WriteTruncateCreate);
	str8 s1 = str8_lit("If death is what it seems");
	str8 s2 = str8_lit("なぜ夢の中でこんなに鮮明に描かれているのか");
	str8 s3 = str8_lit("理解することへの恐れ");
	str8 s4 = str8_lit("悪魔の仕業だ");
	str8 s5 = str8_lit("Hatred's not received, it's coming straight from the source");
	str8 s = str8_lit("If death is what it seems\n"
					  "なぜ夢の中でこんなに鮮明に描かれているのか\n"
					  "理解することへの恐れ\n"
					  "悪魔の仕業だ\n"
					  "Hatred's not received, it's coming straight from the source");
	
	{//write
		Test(file->bytes == 0);
		Test(file->cursor == 0);
		
		u32 count = file_write(file, s1.str, s1.count);
		Test(count == s1.count);
		Test(file->bytes  == s1.count);
		Test(file->cursor == s1.count);
		count = file_write(file, "\n", 1);
		Test(count == 1);
		Test(file->bytes  == s1.count+1);
		Test(file->cursor == s1.count+1);
		fflush(file->handle);
		
		count = file_write_line(file, s3);
		Test(count == s3.count+1);
		Test(file->bytes  == s1.count+1+s3.count+1);
		Test(file->cursor == s1.count+1+s3.count+1);
		fflush(file->handle);
		
		file_cursor(file, s1.count+1);
		Test(file->cursor == s1.count+1);
		count = file_write_line(file, s2);
		Test(count == s2.count+1);
		Test(file->bytes  == s1.count+1+s2.count+1);
		Test(file->cursor == s1.count+1+s2.count+1);
		fflush(file->handle);
		
		file_cursor(file, -1);
		count = file_write_line(file, s3);
		Test(count == s3.count+1);
		Test(file->bytes  == s1.count+1+s2.count+1+s3.count+1);
		Test(file->cursor == s1.count+1+s2.count+1+s3.count+1);
		fflush(file->handle);
		
		count = file_append_line(file, s4);
		Test(count == s4.count+1);
		Test(file->bytes  == s1.count+1+s2.count+1+s3.count+1+s4.count+1);
		Test(file->cursor == s1.count+1+s2.count+1+s3.count+1);
		fflush(file->handle);
		
		count = file_append(file, s5.str, s5.count);
		Test(count == s5.count);
		Test(file->bytes  == s1.count+1+s2.count+1+s3.count+1+s4.count+1+s5.count);
		Test(file->cursor == s1.count+1+s2.count+1+s3.count+1);
		fflush(file->handle);
		
		file_change_access(file, 0);
		str8 sanity = file_read_simple(file->path, deshi_temp_allocator);
		Test(str8_equal_lazy(sanity, s));
		
		File invalid{};
		invalid.path = str8_lit("an/invalid/path");
		TestExpectedLog("[FILE-ERROR] file_write() called on a closed file 'an/invalid/path' at "__FILE__"(",__LINE__+1,")");
		count = file_write(&invalid, s1.str, s1.count);
		Test(count == 0);
		TestExpectedLog("[FILE-ERROR] file_write_line() called on a closed file 'an/invalid/path' at "__FILE__"(",__LINE__+1,")");
		count = file_write_line(&invalid, s1);
		Test(count == 0);
		TestExpectedLog("[FILE-ERROR] file_append() called on a closed file 'an/invalid/path' at "__FILE__"(",__LINE__+1,")");
		count = file_append(&invalid, s1.str, s1.count);
		Test(count == 0);
		TestExpectedLog("[FILE-ERROR] file_append_line() called on a closed file 'an/invalid/path' at "__FILE__"(",__LINE__+1,")");
		count = file_append_line(&invalid, s1);
		Test(count == 0);
		
		TestPassed("core/file/write");
	}
	
	{//read
		file_change_access(file, FileAccess_ReadAppend);
		Test(file->cursor == file->bytes);
		Test(file->cursor == s.count);
		file_cursor(file, 0);
		
		u8 buffer[256];
		str8 read = file_read(file, buffer, s1.count);
		Test(str8_equal_lazy(read, s1));
		Test(file->cursor == s1.count);
		read = file_read(file, buffer, 1);
		Test(str8_equal_lazy(read, str8_lit("\n")));
		Test(file->cursor == s1.count+1);
		
		read = file_read_alloc(file, s2.count, deshi_temp_allocator);
		Test(str8_equal_lazy(read, s2));
		Test(file->cursor == s1.count+1+s2.count);
		read = file_read_alloc(file, 1, deshi_temp_allocator);
		Test(str8_equal_lazy(read, str8_lit("\n")));
		Test(file->cursor == s1.count+1+s2.count+1);
		
		read = file_read_line(file, buffer, 255);
		Test(str8_equal_lazy(read, s3));
		Test(file->cursor == s1.count+1+s2.count+1+s3.count+1);
		
		read = file_read_line_alloc(file, deshi_temp_allocator);
		Test(str8_equal_lazy(read, s4));
		Test(file->cursor == s1.count+1+s2.count+1+s3.count+1+s4.count+1);
		
		read = file_read(file, buffer, -1);
		Test(str8_equal_lazy(read, s5));
		Test(file->cursor == s1.count+1+s2.count+1+s3.count+1+s4.count+1+s5.count);
		Test(file->cursor == file->bytes);
		
		file_cursor(file, 0);
		read = file_read_alloc(file, s.count, deshi_temp_allocator);
		Test(str8_equal_lazy(read, s));
		Test(file->cursor == file->bytes);
		
		File invalid{};
		invalid.path = str8_lit("an/invalid/path");
		TestExpectedLog("[FILE-ERROR] file_read() called on a closed file 'an/invalid/path' at "__FILE__"(",__LINE__+1,")");
		read = file_read(&invalid, s1.str, s1.count);
		Test(read.str == 0 && read.count == 0);
		TestExpectedLog("[FILE-ERROR] file_read_alloc() called on a closed file 'an/invalid/path' at "__FILE__"(",__LINE__+1,")");
		read = file_read_alloc(&invalid, s1.count, deshi_temp_allocator);
		Test(read.str == 0 && read.count == 0);
		TestExpectedLog("[FILE-ERROR] file_read_line() called on a closed file 'an/invalid/path' at "__FILE__"(",__LINE__+1,")");
		read = file_read_line(&invalid, s1.str, -1);
		Test(read.str == 0 && read.count == 0);
		TestExpectedLog("[FILE-ERROR] file_read_line_alloc() called on a closed file 'an/invalid/path' at "__FILE__"(",__LINE__+1,")");
		read = file_read_line_alloc(&invalid, deshi_temp_allocator);
		Test(read.str == 0 && read.count == 0);
		
		TestPassed("core/file/read");
	}
	
	file_deinit(file);
	file_delete(str8_lit("data/test_deshi_file"));
	TestPassed("core/file");
}

#endif //DESHI_TESTS