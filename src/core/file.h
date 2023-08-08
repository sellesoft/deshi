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
  FileType
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
  file_set_cursor(File* file, s64 offset) -> void
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
@file_utils
  file_get_type_of_path(str8 path) -> FileType
@file_shared_variables
@file_tests

TODOs:
track when files are changed (NOTE its not really safe right now to keep a file open for a long time unless you know nothing will change it)
use CreateFile instead of FindFirstFile to tell if a file exists (b/c FindFirstFile's returned filename sucks)
replace win32 error messages with our own
rename "_if_exists" function alternatives to "_noerror"
add "_noerror" version of file_delete(), file_info()
add FileResult_PathBusy tests

References:
https://en.cppreference.com/w/c/io
https://en.cppreference.com/w/cpp/filesystem
*/


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
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

typedef Type FileType; enum {
	FileType_ERROR,
	FileType_File,
	FileType_Directory,
	FileType_SymbolicLink,
	FileType_CharacterDevice, // stdin/stdout, etc.
	FileType_Unknown,
};

struct File{
	FILE* handle; 
	
	u64 creation_time;
	u64 last_access_time;
	u64 last_write_time;
	u64 bytes;
	FileType type;
	//b32 changed;
	
	str8 path;  //full path
	str8 name;  //filename, dot, and extension
	str8 front; //filename, no extension, no dot
	str8 ext;   //extension, no dot
	
	FileAccess access;
	u64 cursor;
};

typedef File* FileArray;

// TODO(sushi) move this somewhere else
// we reserve a name for internal use, so that the separate case defines use the right name without needing to define it repeatedly
#define StartErrorHandler(name, errtype, err) {      \
    errtype* __errhandler__internal__pointer = name; \
    switch(err) {                                    \

#define ErrorCaseF(err) case err:
#define ErrorCaseL(err, resulttag, message, extra) case err: *__errhandler__internal__pointer = {resulttag, STR8(message)}; extra; break;
// !LEAK: this makes a dynamic string with deshi's temp allocator, so if an app doesn't use that, then we're in trouble
// TODO(sushi) decide to remove this or not 
#define ErrorCaseD(err, resulttag, extra, ...) case err: *__errhandler__internal__pointer = {resulttag, to_dstr8v(deshi_temp_allocator, __VA_ARGS__ ).fin}; extra; break;


#define EndErrorHandlerAndCatch(tag, err, unhandlederr_code) default: {LogE(tag, "unhandled errno in ", __func__, "(): ", err); unhandlederr_code} } }

#define SetResultInfoD(resulttag, ...) *result = {resulttag, to_dstr8v(deshi_temp_allocator, __VA_ARGS__).fin}
#define SetResultInfoL(resulttag, message) *result = {resulttag, STR8(message)}


typedef Type FileResultTag; enum {
	FileResult_Ok,
	// when an error occurs with a file, but the function used
	// doesn't give any useful information about why it failed
	// context can be found in the returned message 
	FileResult_UnspecifiedError,
	// the function called was given an empty path
	FileResult_EmptyPath,
	// the path given does not exist 
	FileResult_PathDoesNotExist,
	// the called function was given an invalid argument
	// uses 'invalid arg' to indicate which argument was invalid.
	// more information should be included in the message.
	FileResult_InvalidArgument,
	// the user does not have access to something
	FileResult_AccessDenied,
	// tried to create something, but something with the same name already exists
	FileResult_FileExists,
	// the user tried to give a path, but when resolved resulted in a loop of symbolic links
	FileResult_SymbolicLinkLoop,
	// the link count of the parent directory would exceed LINK_MAX
	FileResult_MaxLinks,
	// some part of a path is too long
	FileResult_NameTooLong,
	// the file system has run out of space
	FileResult_OutOfSpace,
	// something is not a directory
	FileResult_NotADirectory,
	// something is a directory
	FileResult_IsADirectory,
	// something is read only
	FileResult_ReadOnly,
	// something is too big
	FileResult_TooBig,
	// the user has opened more handles than the OS allows
	FileResult_TooManyHandles,
	// the system is out of RAM
	FileResult_SystemOutOfMemory,
	// the operation failed because the path is currently in use 
	// by the system or another process
	FileResult_PathBusy,
	// the OS gave an unspecified IO error
	// if there is more information given in the documentation of a function
	// that causes this, you should include that information in the returned message!
	FileResult_IOError,
	// the handle used for the file is invalid for the attempted operation
	FileResult_InvalidHandleType,
	// a File was passed that does not have an open handle, but the requested
	// operation needs a handle.
	FileResult_FileNotOpen,
};

struct FileResult{
	FileResultTag tag; // determines what kind of result this is 
	str8 message; // a custom message that may or may not be given by the returning function
	// a collection of unioned data that various tags may want to return along with the tag
	union { 
		u64 invalid_arg;
	} error;
};

//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @file_system
//Returns true if a file/directory exists at `path`
// PLATFORM
external b32 deshi__file_exists(str8 caller_file, upt caller_line, str8 path, FileResult* result);
#define file_exists(path) deshi__file_exists(str8_lit(__FILE__),__LINE__, (path), 0)
#define file_exists_result(path, result) deshi__file_exists(str8_lit(__FILE__),__LINE__, (path), (result))

//Creates an empty file/directory at `path` if one doesn't exist already
//    if needed, this will create multiple directories to make the path valid
//    to create a directory, `path` must end with a '\' or '/'
//    returns false if the function fails
// PLATFORM
external b32 deshi__file_create(str8 caller_file, upt caller_line, str8 path, FileResult* result);
#define file_create(path) deshi__file_create(str8_lit(__FILE__),__LINE__, (path), 0)
#define file_create_result(path,res) deshi__file_create(str8_lit(__FILE__),__LINE__, (path), res)

enum{
	FileDeleteFlags_File = 1 << 0,
	FileDeleteFlags_Directory = 1 << 1,
	// BE CAREFUL!
	FileDeleteFlags_Recursive = 1 << 2,
};

// Deletes the file at the given path.
//    returns false if the function fails
//    flags:
//      FileDeleteFlags_File: 
//        delete only files
//      FileDeleteFlags_Directory: 
//        delete only directories if they are empty, or, if the recursive flag is given, 
//        delete the directory and everything in it
//     FileDeleteFlags_Recursive:
//        if directories are set to be deleted, this will allow deletion of non empty
//        directories. USE RESPONSIBLY!
// PLATFORM
external b32 deshi__file_delete(str8 caller_file, upt caller_line, str8 path, u32 flags, FileResult* result);
#define file_delete(path, flags) deshi__file_delete(str8_lit(__FILE__),__LINE__, (path), flags,0)
#define file_delete_result(path, flags,res) deshi__file_delete(str8_lit(__FILE__),__LINE__, (path), flags,(res))

//Renames the file/directory at `old_path` if it exists to `new_path`
//    returns false if the function fails
// PLATFORM
external b32 deshi__file_rename(str8 caller_file, upt caller_line, str8 old_path, str8 new_path, FileResult* result);
#define file_rename(old_path,new_path) deshi__file_rename(str8_lit(__FILE__),__LINE__, (old_path),(new_path),0)
#define file_rename_result(old_path,new_path,res) deshi__file_rename(str8_lit(__FILE__),__LINE__, (old_path),(new_path),(res))

//Copies the file/directory at `src_path` to `dst_path`
//    does not init the destination file if the source is init
//    returns false if the function fails
// PLATFORM
external b32 deshi__file_copy(str8 caller_file, upt caller_line, str8 src_path, str8 dst_path, FileResult* result);
#define file_copy(src_path,dst_path) deshi__file_copy(str8_lit(__FILE__),__LINE__, (src_path),(dst_path),0)
#define file_copy_result(src_path,dst_path,res) deshi__file_copy(str8_lit(__FILE__),__LINE__, (src_path),(dst_path),(res))

//Returns a temporary `File` containing information about the file/directory at `path` if it exists
//    If the file is a directory, `File.path` will have a trailing slash "/", and both `File.front` and `File.ext` will be empty
// PLATFORM
external File deshi__file_info(str8 caller_file, upt caller_line, str8 path, FileResult* result);
#define file_info(path) deshi__file_info(str8_lit(__FILE__),__LINE__, (path),0)
#define file_info_result(path,res) deshi__file_info(str8_lit(__FILE__),__LINE__, (path),(res))

//Returns a temporary array of files/directories in the `directory` if it exists
//    returns an array compatible with kigu/array.h
//    If a file is a directory, `File.path` will have a trailing slash "/", and both `File.front` and `File.ext` will be empty
//    NOTE when constructing filepaths for storage on the File, we dynamically allocate the names
//      meaning that if you use this, you have to make sure that the name is freed
//      the allocated string is JUST 'path', all of the following str8's are views on this str8
// PLATFORM
external FileArray deshi__file_search_directory(str8 caller_file, upt caller_line, str8 directory, FileResult* result);
#define file_search_directory(directory) deshi__file_search_directory(str8_lit(__FILE__),__LINE__, (directory),0)
#define file_search_directory_result(directory,res) deshi__file_search_directory(str8_lit(__FILE__),__LINE__, (directory),(res))

//Returns a temporary string of the absolute path to the file/directory at `path` if it exists
//    If the file is a directory, the absolute path will have a trailing slash "/"
// PLATFORM
external str8 deshi__file_path_absolute(str8 caller_file, upt caller_line, str8 path, FileResult* result);
#define file_path_absolute(path) deshi__file_path_absolute(str8_lit(__FILE__),__LINE__, (path),0)
#define file_path_absolute_result(path,res) deshi__file_path_absolute(str8_lit(__FILE__),__LINE__, (path),(res))

//Returns true if `path1` and `path2` represent the same file/directory on disk
//    returns false and the result tag will be `FileResult_PathDoesNotExist` if one of the paths is a not valid file
//    `ignore_nonexistence` prevents error messages and an error result tag if the file doesn't exist (will still return false)
// PLATFORM
external b32 deshi__file_path_equal(str8 caller_file, upt caller_line, str8 path1, str8 path2, b32 ignore_nonexistence, FileResult* result);
#define file_path_equal(path1,path2) deshi__file_path_equal(str8_lit(__FILE__),__LINE__, (path1),(path2),false,0)
#define file_path_equal_result(path1,path2,res) deshi__file_path_equal(str8_lit(__FILE__),__LINE__, (path1),(path2),false,(res))
#define file_path_equal_if_exists(path1,path2) deshi__file_path_equal(str8_lit(__FILE__),__LINE__, (path1),(path2),true,0)
#define file_path_equal_if_exists_result(path1,path2,res) deshi__file_path_equal(str8_lit(__FILE__),__LINE__, (path1),(path2),true,(res))


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @file_init
//Returns a `File` pointer initialized with `access`
//    `FileAccess_Append`, `FileAccess_Create`, and `FileAccess_Truncate` do not get applied to `File.access`
//    this call is equivalent to `file_change_access()` if there already is an initialized `File` for `path`
//    `ignore_nonexistence` just prevents error messages if the file doesn't exist (will still return 0)
//    If the file is a directory, `File.path` will have a trailing slash "/", and both `File.front` and `File.ext` will be empty
// PLATFORM
external File* deshi__file_init(str8 caller_file, upt caller_line, str8 path, FileAccess access, b32 ignore_nonexistence, FileResult* result);
#define file_init(path,access) deshi__file_init(str8_lit(__FILE__),__LINE__, path,(access),false,0)
#define file_init_result(path,access,res) deshi__file_init(str8_lit(__FILE__),__LINE__, path,(access),false,(res))
#define file_init_if_exists(path,access) deshi__file_init(str8_lit(__FILE__),__LINE__, (path),(access),true,0)
#define file_init_if_exists_result(path,access,res) deshi__file_init(str8_lit(__FILE__),__LINE__, (path),(access),true,(res))

//Closes a previously init `file` if it has `FileAccess_Read` or `FileAccess_Write` and deletes the internal `File` object
//    this does not delete the file on disk, call `file_delete()` to do that
//    returns false if the function fails
// PLATFORM
external b32 deshi__file_deinit(str8 caller_file, upt caller_line, File* file, FileResult* result);
#define file_deinit(file) deshi__file_deinit(str8_lit(__FILE__),__LINE__, (file),0)
#define file_deinit_result(file,res) deshi__file_deinit(str8_lit(__FILE__),__LINE__, (file),(res))

//Handles conversion of `access` for `file`
//    if new access includes `FileAccess_Read` or `FileAccess_Write`, the file is opened internally if it wasn't already open
//    if new access doesn't include `FileAccess_Read` or `FileAccess_Write`, the file is closed internally if it was open
//    `FileAccess_Append` and `FileAccess_Truncate` are only performed if the file is not already open and does not get applied to `File.access`
//    returns false if the function fails
// PLATFORM
external b32 deshi__file_change_access(str8 caller_file, upt caller_line, File* file, FileAccess access, FileResult* result);
#define file_change_access(file,access) deshi__file_change_access(str8_lit(__FILE__),__LINE__, (file),(access),0)
#define file_change_access_result(file,access,res) deshi__file_change_access(str8_lit(__FILE__),__LINE__, (file),(access),(res))

//Returns a view over the internally initialized files
//    returns an array compatible with kigu/array.h
// LOCAL
File* file_initted_files();

//Sets the `File.cursor` of `file` (if it's been init) to `offset` from the beginning if positive
//    this function prevents the `File.cursor` from going above `File.bytes`
//    writing in the middle overwrites rather then inserts
//    returns false if the function fails
// LOCAL
external b32 deshi__file_set_cursor(str8 caller_file, upt caller_line, File* file, u64 offset, FileResult* result);
#define file_set_cursor(file,offset) deshi__file_set_cursor(str8_lit(__FILE__),__LINE__, (file),(offset),0)
#define file_set_cursor_result(file,offset,res) deshi__file_set_cursor(str8_lit(__FILE__),__LINE__, (file),(offset),(res))

//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @file_read
//Copies `bytes` (at most) from `file` (if it's been init) into a `buffer` and returns a str8 view of the bytes written
//    starts reading at `File.cursor` from the beginning and advances the `File.cursor` forward by `bytes`
//    if the end of file is reached before reading all `bytes`, no error occurs and `File.cursor` is set to `File.size`
//    if an error occurs, the `File.cursor` is not affected and an empty str8 is returned
//    writes a null-terminating character to the `buffer` at `bytes+1`
// LOCAL
external str8 deshi__file_read(str8 caller_file, upt caller_line, File* file, void* buffer, u64 bytes, FileResult* result);
#define file_read(file,buffer,bytes) deshi__file_read(str8_lit(__FILE__),__LINE__, (file),(buffer),(bytes),0)
#define file_read_result(file,buffer,bytes,res) deshi__file_read(str8_lit(__FILE__),__LINE__, (file),(buffer),(bytes),(res))

//Copies `bytes` (at most) from `file` (if it's been init) into a str8 allocated with `allocator`
//    starts reading at `File.cursor` from the beginning and advances the `File.cursor` forward by `bytes`
//    if the end of file is reached before reading all `bytes`, no error occurs and `File.cursor` is set to `File.size`
//    if an error occurs, the `File.cursor` is not affected and an empty str8 is returned
//    allocates an extra byte for a null-terminating character
// LOCAL
external str8 deshi__file_read_alloc(str8 caller_file, upt caller_line, File* file, u64 bytes, Allocator* allocator, FileResult* result);
#define file_read_alloc(file,bytes,allocator) deshi__file_read_alloc(str8_lit(__FILE__),__LINE__, (file),(bytes),(allocator),0)
#define file_read_alloc_result(file,bytes,allocator,res) deshi__file_read_alloc(str8_lit(__FILE__),__LINE__, (file),(bytes),(allocator),(res))

//Copies until the next line ending or file end from `file` (if it's been init) into a `buffer` (capped at `max_bytes` written) and returns a str8 view of the bytes written
//    starts reading at `File.cursor` from the beginning and advances the `File.cursor` past the line ending
//    does not copy the line endings to `buffer`
//    if the end of file is reached, no error occurs and `File.cursor` is set to `File.size`
//    if an error occurs, the `File.cursor` is not affected and an empty str8 is returned
//    writes a null-terminating character to the `buffer` after the string (even if it has to cut off part of the string to fit into `max_bytes`)
// LOCAL
external str8 deshi__file_read_line(str8 caller_file, upt caller_line, File* file, void* buffer, u64 max_bytes, FileResult* result);
#define file_read_line(file,buffer,bytes) deshi__file_read_line(str8_lit(__FILE__),__LINE__, (file),(buffer),(bytes),0)
#define file_read_line_result(file,buffer,bytes,res) deshi__file_read_line(str8_lit(__FILE__),__LINE__, (file),(buffer),(bytes),(res))

//Copies until the next line ending or file end from `file` (if it's been init) into a str8 allocated with `a`
//    starts reading at `File.cursor` from the beginning and advances the `File.cursor` past the line ending
//    does not copy the line endings to the str8
//    if the end of file is reached, no error occurs and `File.cursor` is set to `File.size`
//    if an error occurs, the `File.cursor` is not affected and an empty str8 is returned
//    allocates an extra byte for a null-terminating character
// LOCAL
external str8 deshi__file_read_line_alloc(str8 caller_file, upt caller_line, File* file, Allocator* allocator, FileResult* result);
#define file_read_line_alloc(file,allocator) deshi__file_read_line_alloc(str8_lit(__FILE__),__LINE__, (file),(allocator),0)
#define file_read_line_alloc_result(file,allocator,res) deshi__file_read_line_alloc(str8_lit(__FILE__),__LINE__, (file),(allocator),(res))

//Returns the entire contents (allocated with `allocator`) of the file at `path` if it exists
//    allocatoes and extra bytes for a null-terminating character
// PLATFORM
external str8 deshi__file_read_simple(str8 caller_file, upt caller_line, str8 path, Allocator* allocator, FileResult* result);
#define file_read_simple(path,allocator) deshi__file_read_simple(str8_lit(__FILE__),__LINE__, (path),(allocator),0)
#define file_read_simple_result(path,allocator,res) deshi__file_read_simple(str8_lit(__FILE__),__LINE__, (path),(allocator),(res))


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @file_write
//Writes `bytes` from `data` to `file` (if it's been init) and returns the number of bytes written (which may be less if an error occurred)
//    starts writing at `File.cursor` and advances the `File.cursor` forward by `bytes`
// LOCAL
external u64 deshi__file_write(str8 caller_file, upt caller_line, File* file, const void* data, u64 bytes, FileResult* result);
#define file_write(file,data,bytes) deshi__file_write(str8_lit(__FILE__),__LINE__, (file),(data),(bytes),0)
#define file_write_result(file,data,bytes,res) deshi__file_write(str8_lit(__FILE__),__LINE__, (file),(data),(bytes),(res))

//Writes `line` and a newline to `file` (if it's been init) and returns the number of bytes written (which may be less if an error occurred)
//    starts writing at `File.cursor` and advances the `File.cursor` past the line ending
//    regardless of platform, this only writes a single newline character '\n'
// LOCAL
external u64 deshi__file_write_line(str8 caller_file, upt caller_line, File* file, str8 line, FileResult* result);
#define file_write_line(file,line) deshi__file_write_line(str8_lit(__FILE__),__LINE__, (file),(line),0)
#define file_write_line_result(file,line,res) deshi__file_write_line(str8_lit(__FILE__),__LINE__, (file),(line),(res))

//Creates a file at `path` and writes `bytes` of `data` to it then closes the file and returns the number of bytes written (which may be less if an error occurred)
//    this truncates the file at `path` if it exists
// PLATFORM
external u64 deshi__file_write_simple(str8 caller_file, upt caller_line, str8 path, void* data, u64 bytes, FileResult* result);
#define file_write_simple(path,data,bytes) deshi__file_write_simple(str8_lit(__FILE__),__LINE__, (path),(data),(bytes),0)
#define file_write_simple_result(path,data,bytes,res) deshi__file_write_simple(str8_lit(__FILE__),__LINE__, (path),(data),(bytes),(res))

//Writes `bytes` from `data` to the end of `file` (if it's been init) and returns the number of bytes written (which may be less if an error occurred)
//    always writes at the end of the file and doesn't modify the `File.cursor`
// LOCAL
external u64 deshi__file_append(str8 caller_file, upt caller_line, File* file, void* data, u64 bytes, FileResult* result);
#define file_append(file,data,bytes) deshi__file_append(str8_lit(__FILE__),__LINE__, (file),(data),(bytes),0)
#define file_append_result(file,data,bytes,res) deshi__file_append(str8_lit(__FILE__),__LINE__, (file),(data),(bytes),(res))

//Writes `line` and a newline to the end of `file` (if it's been init) and returns the number of bytes written (which may be less if an error occurred)
//    always writes at the end of the file and doesn't modify the `File.cursor`
//    regardless of platform, this only writes a single newline character '\n'
// LOCAL
external u64 deshi__file_append_line(str8 caller_file, upt caller_line, File* file, str8 line, FileResult* result);
#define file_append_line(file,line) deshi__file_append_line(str8_lit(__FILE__),__LINE__, (file),(line),0)
#define file_append_line_result(file,line,res) deshi__file_append_line(str8_lit(__FILE__),__LINE__, (file),(line),(res))

//Opens the file at `path` and writes `bytes` of `data` to the end of it then closes the file
//    this creates the file at `path` if one doesn't exist
// LOCAL
external u64 deshi__file_append_simple(str8 caller_file, upt caller_line, str8 path, void* data, u64 bytes, FileResult* result);
#define file_append_simple(path,data,bytes) deshi__file_append_simple(str8_lit(__FILE__),__LINE__, (path),(data),(bytes),0)
#define file_append_simple_result(path,data,bytes,res) deshi__file_append_simple(str8_lit(__FILE__),__LINE__, (path),(data),(bytes),(res))

//Resizes a file to a specified size
//external void deshi__file_set_size(str8 caller_file, upt caller_line, File* file, s64 bytes);
//#define file_set_size(file, bytes) deshi__file_set_size(str8_lit(__FILE__),__LINE__,(file),(bytes));

//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @file_utils

// gets the type of a file and returns it
// PLATFORM
external FileType deshi__file_get_type_of_path(str8 caller_file, upt caller_line, str8 path, FileResult* result);
#define file_get_type_of_path(path) deshi__file_get_type_of_path(str8_lit(__FILE__),__LINE__,path,0)
#define file_get_type_of_path_result(path,res) deshi__file_get_type_of_path(str8_lit(__FILE__),__LINE__,path,(res))


#endif //DESHI_FILE_H
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#if defined(DESHI_IMPLEMENTATION) && !defined(DESHI_FILE_IMPL)
#define DESHI_FILE_IMPL
#include "logger.h"
#include "kigu/arrayT.h"

// handles a range of errors that can be given from errno
#define StartFileErrnoHandler(result_name, err, return_error_value) {\
    if(!result_name) {                                               \
        LogE("file", "unhandled errno: ", err);                      \
        return return_error_value;                                   \
    }                                                                \
    u32 __feh__errno = err;                                          \
    StartErrorHandler(result_name, FileResult, __feh__errno)

#define EndFileErrnoHandler()                            \
    EndErrorHandlerAndCatch("linux-file", __feh__errno, {\
        LogE("file", "unhandled errno: ", __feh__errno); \
        AssertAlways(false);                             \
    })}

// handles a single error, usually one that doesn't have to do with the system
#define FileHandleErrorL(result_name, result_tag, return_error_value, message, extra)\
    if(!result_name){                                                                \
        LogE("file", __func__, "(): ", message);                                     \
        return return_error_value;                                                   \
    }                                                                                \
    *result_name = {result_tag, STR8(message)};                                      \
    extra                                                                            \
    return return_error_value;

// handles a single error, usually one that doesn't have to do with the system
// dynamic version
// !LEAK: this makes a dynamic string with deshi's temp allocator, so if an app doesn't use that, then we're in trouble
// TODO(sushi) decide to remove this or not 
#define FileHandleErrorD(result_name, result_tag, return_error_value, extra, ...)\
    if(!result_name){                                                            \
        LogE("file", __func__, "(): ", __VA_ARGS__);                             \
        return return_error_value;                                               \
    }                                                                            \
    *result_name = {result_tag, to_dstr8v(deshi_temp_allocator,__VA_ARGS__).fin};\
    extra                                                                        \
    return return_error_value; 

//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @file_shared_variables
local struct{
	FileArray files;
	b32 crash_on_error;
} file_shared;

//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @file_init
File*
file_initted_files(){
	return file_shared.files;
}

b32
deshi__file_set_cursor(str8 caller_file, upt caller_line, File* file, u64 offset, FileResult* result){
	if(file == 0){
		FileHandleErrorD(result, FileResult_InvalidArgument,0,,"file_set_cursor() was passed a null `file` pointer at ",caller_file,"(",caller_line,")");
	}
	
	if(file->handle){
		offset = ClampMax(offset, file->bytes);
		if(fseek(file->handle, offset, SEEK_SET) == 0){
			file->cursor = offset;
		}else{
			StartFileErrnoHandler(result, errno, 0)
				ErrorCaseD(EINVAL, FileResult_InvalidArgument,, "The offset provided to fseek resulted in a negative. Offset was ", offset, ".")
				ErrorCaseL(ESPIPE, FileResult_InvalidHandleType, "The file descriptor is not seekable, eg. it refers to a pipe, FIFO, or socket.",);
			EndFileErrnoHandler();
		}
	}else{
		FileHandleErrorD(result, FileResult_FileNotOpen,0,,"file_set_cursor() called on a closed file '",file->path,"' at ",caller_file,"(",caller_line,")")
	}
	return 1;
}


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @file_read
str8
deshi__file_read(str8 caller_file, upt caller_line, File* file, void* buffer, u64 bytes, FileResult* result){
	if(file == 0){
		FileHandleErrorD(result, FileResult_InvalidArgument,{},,"file_read() was passed a null `file` pointer at ",caller_file,"(",caller_line,")");
	}
	if(buffer == 0 && bytes > 0){
		FileHandleErrorD(result, FileResult_InvalidArgument,{},,"file_read() was passed a null `buffer` pointer (with non-zero `bytes` to read) at ",caller_file,"(",caller_line,")");
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
				if(!result){
					LogE("file","fgetc() failed in file_read_line() call on file '",file->path,"(",file->cursor,")' at ",caller_file,"(",caller_line,")");
				}else{
					SetResultInfoD(FileResult_UnspecifiedError, "fgetc() failed in file_read_line() call on file '",file->path,"(",file->cursor,")' at ",caller_file,"(",caller_line,")");
				}
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
		FileHandleErrorD(result, FileResult_FileNotOpen,{},,"file_read() called on a closed file '",file->path,"' at ",caller_file,"(",caller_line,")")
	}
}

str8
deshi__file_read_alloc(str8 caller_file, upt caller_line, File* file, u64 bytes, Allocator* allocator, FileResult* result){
	if(file == 0){
		FileHandleErrorD(result, FileResult_InvalidArgument,{},, "file_read_alloc() was passed a null `file` pointer at ",caller_file,"(",caller_line,")");
	}
	if(allocator == 0 && bytes > 0){
		FileHandleErrorD(result, FileResult_InvalidArgument,{},, "file_read_alloc() was passed a null `allocator` pointer (with non-zero `bytes` to read) at ",caller_file,"(",caller_line,")");
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
				if(!result){
					LogE("file","fgetc() failed in file_read_line() call on file '",file->path,"(",file->cursor,")' at ",caller_file,"(",caller_line,")");
				}else{
					SetResultInfoD(FileResult_UnspecifiedError, "fgetc() failed in file_read_line() call on file '",file->path,"(",file->cursor,")' at ",caller_file,"(",caller_line,")");
				}
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
		FileHandleErrorD(result, FileResult_FileNotOpen,{},,"file_read_alloc() called on a closed file '",file->path,"' at ",caller_file,"(",caller_line,")")
	}
}

str8
deshi__file_read_line(str8 caller_file, upt caller_line, File* file, void* buffer, u64 max_bytes, FileResult* result){
	if(file == 0){
		FileHandleErrorD(result, FileResult_InvalidArgument,{},, "file_read_line() was passed a null `file` pointer at ",caller_file,"(",caller_line,")"); 
	}
	if(buffer == 0){
		FileHandleErrorD(result, FileResult_InvalidArgument,{},, "file_read_line() was passed a null `buffer` pointer at ",caller_file,"(",caller_line,")");
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
				if(!result){
					LogE("file","fgetc() failed in file_read_line() call on file '",file->path,"(",file->cursor,")' at ",caller_file,"(",caller_line,")");
				}else{
					SetResultInfoD(FileResult_UnspecifiedError, "fgetc() failed in file_read_line() call on file '",file->path,"(",file->cursor,")' at ",caller_file,"(",caller_line,")");
				}
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
		FileHandleErrorD(result, FileResult_FileNotOpen,{},,"file_read_line() called on a closed file '",file->path,"' at ",caller_file,"(",caller_line,")")
	}
}

str8
deshi__file_read_line_alloc(str8 caller_file, upt caller_line, File* file, Allocator* allocator, FileResult* result){
	if(file == 0){
		FileHandleErrorD(result, FileResult_InvalidArgument,{},, "file_read_line_alloc() was passed a null `file` pointer at ",caller_file,"(",caller_line,")");
	}
	if(allocator == 0){
		FileHandleErrorD(result, FileResult_InvalidArgument,{},, "file_read_line_alloc() was passed a null `allocator` pointer at ",caller_file,"(",caller_line,")");
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
				if(!result){
					LogE("file","fgetc() failed in file_read_line_alloc() call on file '",file->path,"(",file->cursor,")' at ",caller_file,"(",caller_line,")");
				}else{
					SetResultInfoD(FileResult_UnspecifiedError, "fgetc() failed in file_read_line() call on file '",file->path,"(",file->cursor,")' at ",caller_file,"(",caller_line,")");
				}
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
		FileHandleErrorD(result, FileResult_FileNotOpen,{},,"file_read_line_alloc() called on a closed file '",file->path,"' at ",caller_file,"(",caller_line,")");
	}
}


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @file_write
u64
deshi__file_write(str8 caller_file, upt caller_line, File* file, const void* data, u64 bytes, FileResult* result){
	if(file == 0){
		FileHandleErrorD(result, FileResult_InvalidArgument,0,, "file_write() was passed a null `file` pointer at ",caller_file,"(",caller_line,")");
	}
	if(data == 0 && bytes > 0){
		FileHandleErrorD(result, FileResult_InvalidArgument,0,, "file_write() was passed a null `data` pointer (with non-zero `bytes` to read) at ",caller_file,"(",caller_line,")");
	}
	
	if(file->handle){
		size_t bytes_written = fwrite(data, 1,bytes, file->handle);
		fflush(file->handle);
		file->cursor += bytes_written;
		if(file->cursor > file->bytes) file->bytes = file->cursor;
		return bytes_written;
	}else{
		FileHandleErrorD(result, FileResult_FileNotOpen,0,,"file_write() called on a closed file '",file->path,"' at ",caller_file,"(",caller_line,")");
	}
}

u64
deshi__file_write_line(str8 caller_file, upt caller_line, File* file, str8 line, FileResult* result){
	if(file == 0){
		FileHandleErrorD(result, FileResult_InvalidArgument,0,, "file_write_line() was passed a null `file` pointer at ",caller_file,"(",caller_line,")");
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
		FileHandleErrorD(result, FileResult_FileNotOpen,0,, "file_write_line() called on a closed file '",file->path,"' at ",caller_file,"(",caller_line,")");
	}
}

u64
deshi__file_append(str8 caller_file, upt caller_line, File* file, void* data, u64 bytes, FileResult* result){
	if(file == 0){
		FileHandleErrorD(result, FileResult_InvalidArgument,0,, "file_append() was passed a null `file` pointer at ",caller_file,"(",caller_line,")");
	}
	if(data == 0 && bytes > 0){
		FileHandleErrorD(result, FileResult_InvalidArgument,0,, "file_append() was passed a null `data` pointer (with non-zero `bytes` to read) at ",caller_file,"(",caller_line,")");
	}
	
	if(file->handle){
		fseek(file->handle, file->bytes,  SEEK_SET);
		size_t bytes_written = fwrite(data, 1,bytes, file->handle);
		fseek(file->handle, file->cursor, SEEK_SET);
		file->bytes += bytes_written;
		return bytes_written;
	}else{
		FileHandleErrorD(result, FileResult_FileNotOpen,0,,"file_append() called on a closed file '",file->path,"' at ",caller_file,"(",caller_line,")");
	}
}

u64
deshi__file_append_line(str8 caller_file, upt caller_line, File* file, str8 line, FileResult* result){
	if(file == 0){
		FileHandleErrorD(result, FileResult_InvalidArgument,0,, "file_append_line() was passed a null `file` pointer at ",caller_file,"(",caller_line,")");
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
		FileHandleErrorD(result, FileResult_FileNotOpen,0,,"file_append_line() called on a closed file '",file->path,"' at ",caller_file,"(",caller_line,")");
	}
}

#undef FileHandleErrorL
#undef FileHandleErrorD
#undef EndFileErrnoHandler
#undef StartFileErrnoHandler

#endif //DESHI_IMPLEMENTATION
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//// @file_tests
#if defined(DESHI_TESTS) && !defined(DESHI_TESTS_FILE)
#define DESHI_TESTS_FILE

#include "test.h"
#include "kigu/array.h"

void TEST_deshi_file(b32 verbose = 0){
	if(file_exists(str8_lit("data/test_deshi_file"))){
		file_delete(str8_lit("data/test_deshi_file"), FileDeleteFlags_Directory|FileDeleteFlags_Recursive);
	}
	
	TestStartEnvironment(file, FileResult, result, verbose);
	
	TestStartSection("file/system");{
		//// exists ////
		TestStartFunction(file_exists);{
			TestLog("deshi's data file should already exist.");
			TestReturn(file_exists(STR8("data")), 1);
			TestReturn(file_exists(STR8("data/")), 1);
			TestReturn(file_exists(STR8("data\\")), 1);
			
			TestLog("test directories that dont exist.");
			TestReturn(file_exists(STR8("datadbasjkdabskjdasbkjdasbjkds")), 0);
			TestReturn(file_exists(STR8("datadbasjkdabskjdasbkjdasbjkds/")), 0);
			TestReturn(file_exists(STR8("datadbasjkdabskjdasbkjdasbjkds\\")), 0);
			
			TestLog("exists should reject empty paths.");
			TestResult(file_exists_result(str8{},result), FileResult_EmptyPath);
			TestResult(file_exists_result(STR8(""), result), FileResult_EmptyPath);
		}TestEndFunction(file_exists);
		
		//// create ////
		TestStartFunction(file_create);{
			TestLog("create an already existing file (data/).");
			TestOk(file_create_result(STR8("data/"), result));
			TestReturnTrue(file_exists(STR8("data")));
			
			TestLog("create data/test_deshi_file.");
			TestOk(file_create_result(STR8("data/test_deshi_file/"), result));
			TestReturnTrue(file_exists(STR8("data/test_deshi_file")));
			
			TestLog("create data/test_deshi_file/food/apple.txt, should create the dir 'food' and the file.");
			TestOk(file_create_result(STR8("data/test_deshi_file/food/apple.txt"), result));
			TestReturnTrue(file_exists(STR8("data/test_deshi_file/food/apple.txt")));
			
			TestLog("create a directory with a unicode name.");
			TestOk(file_create_result(STR8("data/test_deshi_file/不明誘惑/"), result));
			TestReturnTrue(file_exists(STR8("data/test_deshi_file/不明誘惑")));
			
			TestLog("create a file with a unicode name.");
			TestOk(file_create_result(STR8("data/test_deshi_file/不明誘惑/悪徳.市"), result));
			TestReturnTrue(file_exists(STR8("data/test_deshi_file/不明誘惑/悪徳.市")));
			
			TestLog("create should reject an empty path.");
			TestResult(file_create_result(str8{}, result), FileResult_EmptyPath);
			TestResult(file_create_result(STR8(""), result), FileResult_EmptyPath);
		}TestEndFunction(file_create);
		
		//// delete ////
		TestStartFunction(file_delete);{
			TestLog("recursively delete the previously created food directory.");
			TestOk(file_delete_result(STR8("data/test_deshi_file/food"), FileDeleteFlags_Directory|FileDeleteFlags_Recursive, result));
			TestReturn(file_exists(STR8("data/test_deshi_file/food/apple.txt")), 0);
			TestReturn(file_exists(STR8("data/test_deshi_file/food")), 0);
			
			TestLog("recursively delete the previously created directory with a unicode name.");
			TestOk(file_delete_result(STR8("data/test_deshi_file/不明誘惑"), FileDeleteFlags_Directory|FileDeleteFlags_Recursive, result));
			TestReturn(file_exists(STR8("data/test_deshi_file/不明誘惑/悪徳.市")), 0);
			TestReturn(file_exists(STR8("data/test_deshi_file/不明誘惑")), 0);
			
			TestLog("delete should reject empty paths.");
			TestResult(file_delete_result(str8{}, 0, result), FileResult_EmptyPath);
			TestResult(file_delete_result(STR8(""), 0, result), FileResult_EmptyPath);
			
			if(!file_create(STR8("data/dummydir/"))) return;
			if(!file_create(STR8("data/dummydir/dummyfile"))) return;
			
			TestLog("delete should reject calls that do not give the right flags for the given path.");
			TestResult(file_delete_result(STR8("data/dummydir"), 0, result), FileResult_InvalidArgument);
			TestResult(file_delete_result(STR8("data/dummydir"), FileDeleteFlags_File, result), FileResult_IsADirectory);
			TestResult(file_delete_result(STR8("data/dummydir/dummyfile"), FileDeleteFlags_Directory, result), FileResult_InvalidArgument);
			TestResult(file_delete_result(STR8("data/dummydir"), FileDeleteFlags_Directory, result), FileResult_InvalidArgument);
			TestResult(file_delete_result(STR8("data/dummydir"), FileDeleteFlags_Directory|FileDeleteFlags_Recursive, result), FileResult_Ok);
		}TestEndFunction(file_delete);
		
		//// rename ////
		TestStartFunction(file_rename);{
			TestLog("renaming a file.");
			file_create(STR8("data/test_deshi_file/food/apple.txt"));
			TestOk(file_rename_result(STR8("data/test_deshi_file/food/apple.txt"), STR8("data/test_deshi_file/food/banana.txt"), result));
			TestReturn(file_exists(STR8("data/test_deshi_file/food/apple.txt")), 0);
			TestReturn(file_exists(STR8("data/test_deshi_file/food/banana.txt")), 1);
			
			TestLog("renaming a directory.");
			TestOk(file_rename_result(STR8("data/test_deshi_file/food/"), STR8("data/test_deshi_file/fruits/"), result));
			TestReturn(file_exists(STR8("data/test_deshi_file/food/apple.txt")), 0);
			TestReturn(file_exists(STR8("data/test_deshi_file/food/banana.txt")), 0);
			TestReturn(file_exists(STR8("data/test_deshi_file/food")), 0);
			TestReturn(file_exists(STR8("data/test_deshi_file/fruits")), 1);
			TestReturn(file_exists(STR8("data/test_deshi_file/fruits/banana.txt")), 1);
			
			TestLog("rename should reject empty paths in either of its arguments.");
			TestResult(file_rename_result(str8{}, str8{}, result), FileResult_EmptyPath);
			TestResult(file_rename_result(STR8(""), STR8(""), result), FileResult_EmptyPath);
			TestResult(file_rename_result(STR8("test"), str8{}, result), FileResult_EmptyPath);
			TestResult(file_rename_result(STR8("test"), STR8(""), result), FileResult_EmptyPath);
			
			TestLog("rename should fail when given a non existant path.");
			TestResult(file_rename_result(STR8("data/test_deshi_file/food/apple.txt"), STR8("data/test_deshi_file/food/banana.txt"), result), FileResult_PathDoesNotExist);
		}TestEndFunction(file_rename);
		
		//// info ////
		TestStartFunction(file_info);{
			TestLog("get info about data directory.");
			// TODO(sushi) TestResult doesn't work on calls that need to be assigned. an expression oriented grammar would solve this :)
			File file = file_info_result(STR8("data/"), result);
			TestResultSeparate(FileResult_Ok);
			Test(file.creation_time != 0);
			Test(file.last_access_time >= file.creation_time);
			Test(file.last_write_time >= file.creation_time);
#if DESHI_WINDOWS
			Test(file.bytes == 0);
#elif DESHI_LINUX
			//TODO(delle) the API should not return different values on different platforms
			Test(file.bytes == 4096); // on linux, directories actually hold a size
#else
#  error "unhandled platform"
#endif
			Test(file.type == FileType_Directory);
			//TestResult(!file.changed);
			Test(str8_ends_with(file.path, STR8("data/")));
			Test(str8_equal_lazy(file.name, STR8("data")));
			Test(str8_equal_lazy(file.front, str8{}));
			Test(str8_equal_lazy(file.ext, str8{}));
			Test(file.access == 0);
			Test(file.cursor == 0);
			
			TestLog("get info about file banana.txt.");
			// TODO(sushi) TestResult doesn't work on calls that need to be assigned. an expression oriented grammar would solve this :)
			file = file_info_result(STR8("data/test_deshi_file/fruits/banana.txt"), result);
			TestResultSeparate(FileResult_Ok);
			Test(file.creation_time != 0);
			Test(file.last_access_time >= file.creation_time);
			Test(file.last_write_time >= file.creation_time);
			Test(file.bytes == 0);
			Test(file.type != FileType_Directory);
			//TestResult(!file.changed);
			Test(str8_ends_with(file.path, STR8("data/test_deshi_file/fruits/banana.txt")));
			Test(str8_equal_lazy(file.name, STR8("banana.txt")));
			Test(str8_equal_lazy(file.front, STR8("banana")));
			Test(str8_equal_lazy(file.ext, STR8("txt")));
			Test(file.access == 0);
			Test(file.cursor == 0);
			
			TestLog("info should return a zero'd struct and fail with FileResult_PathDoesNotExist on non-existing file.");
			file = file_info_result(STR8("data/test_deshi_file/fruits/apple.txt"), result);
			TestResultSeparate(FileResult_PathDoesNotExist);
			Test(file.creation_time == 0);
		}TestEndFunction(file_info);
		
		//// search_directory ////
		TestStartFunction(file_search_directory);{
			if(!file_create(STR8("data/test_deshi_file/fruits/apple.bin"))) return;
			
			TestLog("create file 'apple.bin' in fruits/ and verify data on it and banana.txt.");
			File* files = file_search_directory(STR8("data/test_deshi_file/fruits/"));
			Test(array_count(files) == 2);
			u32 apple_index = 0, banana_index = 1;
			if(str8_equal_lazy(files[0].front, STR8("banana"))) Swap(apple_index, banana_index);
			Test(files[banana_index].creation_time != 0);
			Test(files[banana_index].last_access_time >= files[banana_index].creation_time);
			Test(files[banana_index].last_write_time >= files[banana_index].creation_time);
			Test(files[banana_index].bytes == 0);
			Test(files[banana_index].type != FileType_Directory);
			//Test(!files[banana_index].changed);
			Test(str8_ends_with(files[banana_index].path, STR8("data/test_deshi_file/fruits/banana.txt")));
			Test(str8_equal_lazy(files[banana_index].name, STR8("banana.txt")));
			Test(str8_equal_lazy(files[banana_index].front, STR8("banana")));
			Test(str8_equal_lazy(files[banana_index].ext, STR8("txt")));
			Test(files[banana_index].access == 0);
			Test(files[banana_index].cursor == 0);
			Test(files[apple_index].creation_time != 0 && files[apple_index].creation_time >= files[banana_index].creation_time);
			Test(files[apple_index].last_access_time >= files[apple_index].creation_time && files[apple_index].last_access_time >= files[banana_index].last_access_time);
			Test(files[apple_index].last_write_time >= files[apple_index].creation_time && files[apple_index].last_write_time >= files[banana_index].last_write_time);
			Test(files[apple_index].bytes == 0);
			Test(files[apple_index].type != FileType_Directory);
			//Test(!files[apple_index].changed);
			Test(str8_ends_with(files[apple_index].path, STR8("data/test_deshi_file/fruits/apple.bin")));
			Test(str8_equal_lazy(files[apple_index].name, STR8("apple.bin")));
			Test(str8_equal_lazy(files[apple_index].front, STR8("apple")));
			Test(str8_equal_lazy(files[apple_index].ext, STR8("bin")));
			Test(files[apple_index].access == 0);
			Test(files[apple_index].cursor == 0);
			
			TestLog("search_directories should return a null pointer for a non existent path.");
			files = file_search_directory_result(STR8("data/test_deshi_file/food/"), result);
			TestResultSeparate(FileResult_PathDoesNotExist);
			Test(!files);
		}TestEndFunction(file_search_directory);
		
		
		//// path_absolute ////
		TestStartFunction(file_path_absolute);{
			str8 out;
			
			TestLog("the returned absolute path should end with what it was provided with.");
			out = file_path_absolute_result(STR8("data/test_deshi_file/fruits/apple.bin"), result);
			TestResultSeparate(FileResult_Ok);
			Test(str8_ends_with(out, STR8("data/test_deshi_file/fruits/apple.bin")));
			out = file_path_absolute_result(STR8("data/test_deshi_file/fruits"), result);
			TestResultSeparate(FileResult_Ok);
			Test(str8_ends_with(out, STR8("data/test_deshi_file/fruits/")));
			out = file_path_absolute_result(STR8("data/test_deshi_file/"), result);
			TestResultSeparate(FileResult_Ok);
			Test(str8_ends_with(out, STR8("data/test_deshi_file/")));
			
			TestLog("should return an empty str8 and FileResult_PathDoesNotExist when given a non-existant path.");
			out = file_path_absolute_result(STR8("data/test_deshi_file/food/"), result);
			TestResultSeparate(FileResult_PathDoesNotExist);
			Test(!out);
			
#if 0 //NOTE(delle) user specific path testing
			Test(str8_equal_lazy(file_path_absolute(STR8("data/test_deshi_file/fruits/apple.bin")), STR8("W:/suugu/data/test_deshi_file/fruits/apple.bin")));
			Test(str8_equal_lazy(file_path_absolute(STR8("data/test_deshi_file/fruits")), STR8("W:/suugu/data/test_deshi_file/fruits/")));
#endif
		}TestEndFunction(file_path_absolute);
		
		//// path_equal ////
		TestStartFunction(file_path_equal);{
			b32 e;
			
			TestLog("different variations of paths should be equal.");
			e = file_path_equal_result(STR8("data/test_deshi_file/fruits/apple.bin"), STR8("data/test_deshi_file/fruits/apple.bin"), result);
			TestResultSeparate(FileResult_Ok);
			Test(e);
			e = file_path_equal_result(STR8("data\\test_deshi_file\\fruits\\apple.bin"), STR8("data/test_deshi_file/fruits/apple.bin"), result);
			TestResultSeparate(FileResult_Ok);
			Test(e);
			e = file_path_equal_result(STR8("data/test_deshi_file/fruits/apple.bin"), STR8("data\\test_deshi_file\\fruits\\apple.bin"), result);
			TestResultSeparate(FileResult_Ok);
			Test(e);
			e = file_path_equal_result(STR8("data/test_deshi_file/fruits/apple.bin"), STR8("data\\test_deshi_file/fruits\\apple.bin"), result);
			TestResultSeparate(FileResult_Ok);
			Test(e);
			e = file_path_equal_result(STR8("data/test_deshi_file/fruits/"), STR8("data/test_deshi_file/fruits"), result);
			TestResultSeparate(FileResult_Ok);
			Test(e);
			e = file_path_equal_result(STR8("data/test_deshi_file/fruits"), STR8("data/test_deshi_file/fruits/"), result);
			TestResultSeparate(FileResult_Ok);
			Test(e);
			e = file_path_equal_result(STR8("data/test_deshi_file/fruits\\"), STR8("data/test_deshi_file/fruits/"), result);
			TestResultSeparate(FileResult_Ok);
			Test(e);
			e = file_path_equal_result(STR8("data/test_deshi_file/fruits/"), STR8("data/test_deshi_file/fruits\\"), result);
			TestResultSeparate(FileResult_Ok);
			Test(e);
			
			TestLog("different variations of paths should not be equal.");
			e = !file_path_equal_result(STR8("data/test_deshi_file/fruits/apple.bin"), STR8("data/test_deshi_file/fruits/banana.txt"), result);
			TestResultSeparate(FileResult_Ok);
			Test(e);
			e = !file_path_equal_result(STR8("data/test_deshi_file/fruits/apple.bin"), STR8("data/test_deshi_file/fruits/apple.bi"), result);
			TestResultSeparate(FileResult_PathDoesNotExist);
			Test(e);
			e = !file_path_equal_result(STR8("data/test_deshi_file/fruits/apple.bin"), STR8("data/test_deshi_file/fruits/"), result);
			TestResultSeparate(FileResult_Ok);
			Test(e);
		}TestEndFunction(file_path_equal);
	}TestEndSection();
	
	TestStartSection("file/simple read,write, and append");{
		File file1 = file_info_result(STR8("data/test_deshi_file/fruits/banana.txt"), result);
		if(!file1.creation_time) return;
		
		TestLog("writing to banana.txt.");
		str8 s1 = STR8("woah, this is a banana!");
		u32 count = file_write_simple(STR8("data/test_deshi_file/fruits/banana.txt"), s1.str, s1.count);
		File file2 = file_info(STR8("data/test_deshi_file/fruits/banana.txt"));
		Test(count == s1.count);
		Test(file2.bytes == s1.count);
		
		TestLog("truncating banana.txt.");
		str8 s2 = STR8("アクアマン");
		count = file_write_simple(STR8("data/test_deshi_file/fruits/banana.txt"), s2.str, s2.count);
		file1 = file_info(STR8("data/test_deshi_file/fruits/banana.txt"));
		Test(count == s2.count);
		Test(file1.bytes == s2.count);
		
		TestLog("appending banana.txt.");
		count = file_append_simple(STR8("data/test_deshi_file/fruits/banana.txt"), s1.str, s1.count);
		file2 = file_info(STR8("data/test_deshi_file/fruits/banana.txt"));
		Test(count == s1.count);
		Test(file2.bytes == s2.count + s1.count);
		
		str8 read = file_read_simple(STR8("data/test_deshi_file/fruits/banana.txt"), deshi_temp_allocator);
		file1 = file_info(STR8("data/test_deshi_file/fruits/banana.txt"));
		Test(read.count == s2.count + s1.count);
		Test(str8_begins_with(read, s2));
		Test(str8_ends_with(read, s1));
		Test(file1.bytes == s2.count + s1.count);
	}TestEndSection();
	
	File* file = 0;
	TestStartSection("file/init");{
		TestStartFunction(file_init);{
			TestLog("initializing a new file and checking its info.");
			file = file_init_result(STR8("data/test_deshi_file/不明誘惑/悪徳.市"), FileAccess_ReadWriteCreate, result);
			TestResultSeparate(FileResult_Ok);
			Test(file->handle != 0);
			Test(file->creation_time != 0);
			Test(file->last_access_time >= file->creation_time);
			Test(file->last_write_time >= file->creation_time);
			Test(file->bytes == 0);
			Test(file->type != FileType_Directory);
			//Test(!file->changed);
			Test(str8_ends_with(file->path, STR8("data/test_deshi_file/不明誘惑/悪徳.市")));
			Test(str8_equal_lazy(file->name, STR8("悪徳.市")));
			Test(str8_equal_lazy(file->front, STR8("悪徳")));
			Test(str8_equal_lazy(file->ext, STR8("市")));
			Test(file->access == FileAccess_ReadWrite);
			Test(file->cursor == 0);
			
			TestLog("initializing the same file again.");
			file = file_init_result(STR8("data/test_deshi_file/不明誘惑/悪徳.市"), FileAccess_ReadWriteCreate, result);
			TestResultSeparate(FileResult_Ok);
			Test(file->handle != 0);
			Test(file->creation_time != 0);
			Test(file->last_access_time >= file->creation_time);
			Test(file->last_write_time >= file->creation_time);
			Test(file->bytes == 0);
			Test(file->type != FileType_Directory);
			//Test(!file->changed);
			Test(str8_ends_with(file->path, STR8("data/test_deshi_file/不明誘惑/悪徳.市")));
			Test(str8_equal_lazy(file->name, STR8("悪徳.市")));
			Test(str8_equal_lazy(file->front, STR8("悪徳")));
			Test(str8_equal_lazy(file->ext, STR8("市")));
			Test(file->access == FileAccess_ReadWrite);
			Test(file->cursor == 0);
			
			TestLog("file_init should error when given a non-existant path and isn't given FileAccess_Create");
			File* file2 = file_init_result(STR8("data/test_deshi_file/fruits/apple.txt"), FileAccess_ReadWrite, result);
			TestResultSeparate(FileResult_PathDoesNotExist);
			Test(file2 == 0);
		}TestEndFunction(file_init);
		
		TestStartFunction(file_change_access);{
			TestLog("changing access to read/append.");
			TestOk(file_change_access_result(file, FileAccess_ReadAppend, result));
			Test(file->handle != 0);
			Test(file->access == FileAccess_Read);
			Test(file->cursor == 0);
			
			TestLog("changing access to write/truncate.");
			TestOk(file_change_access_result(file, FileAccess_WriteTruncate, result));
			Test(file->handle != 0);
			Test(file->access == FileAccess_Write);
			Test(file->cursor == 0);
			
			TestLog("changing access on a null File Pointer.");
			TestResult(file_change_access_result(0, FileAccess_WriteTruncate, result), FileResult_InvalidArgument);
		}TestEndFunction(file_change_access);
		
		TestStartFunction(file_deinit);{
			TestLog("deinitializing file normally.");
			TestOk(file_deinit_result(file, result));
			
			TestLog("deinitializing a null File pointer.");
			TestResult(file_deinit_result(0, result), FileResult_InvalidArgument);
		}TestEndFunction(file_deinit);
		
		//// append ////
		TestLog("appending.");
		str8 s1 = STR8("aaabbbccc");
		file_write_simple(STR8("data/test_deshi_file/不明誘惑/悪徳.市"), s1.str, s1.count);
		file = file_init(STR8("data/test_deshi_file/不明誘惑/悪徳.市"), FileAccess_ReadWriteAppend);
		Test(file->handle != 0);
		Test(file->bytes == 9);
		Test(file->access == FileAccess_ReadWrite);
		Test(file->cursor == 9);
		file_deinit(file);
		
		//// truncate ////
		TestLog("truncating.");
		str8 s2 = STR8("aaabbbcccddd");
		file_write_simple(STR8("data/test_deshi_file/不明誘惑/悪徳.市"), s2.str, s2.count);
		file = file_init(STR8("data/test_deshi_file/不明誘惑/悪徳.市"), FileAccess_WriteTruncate);
		Test(file->handle != 0);
		Test(file->bytes == 0);
		Test(file->access == FileAccess_Write);
		Test(file->cursor == 0);
		file_deinit(file);
		
		//// cursor ////
		TestLog("testing cursor movement.");
		file_write_simple(STR8("data/test_deshi_file/不明誘惑/悪徳.市"), s1.str, s1.count);
		file = file_init(STR8("data/test_deshi_file/不明誘惑/悪徳.市"), FileAccess_ReadAppend);
		Test(file->bytes == 9);
		Test(file->cursor == 9);
		
		TestOk(file_set_cursor_result(file, 0, result));
		Test(file->cursor == 0);
		TestOk(file_set_cursor_result(file, 2, result));
		Test(file->cursor == 2);
		TestOk(file_set_cursor_result(file, 16, result));
		Test(file->cursor == 9);
		
		if(!file_deinit(file)) return;
	}TestEndSection();
	
	file = file_init(STR8("data/test_deshi_file/不明誘惑/悪徳.市"), FileAccess_WriteTruncateCreate);
	str8 s1 = STR8("If death is what it seems");
	str8 s2 = STR8("なぜ夢の中でこんなに鮮明に描かれているのか");
	str8 s3 = STR8("理解することへの恐れ");
	str8 s4 = STR8("悪魔の仕業だ");
	str8 s5 = STR8("Hatred's not received, it's coming straight from the source");
	str8 s  = STR8("If death is what it seems\n"
				   "なぜ夢の中でこんなに鮮明に描かれているのか\n"
				   "理解することへの恐れ\n"
				   "悪魔の仕業だ\n"
				   "Hatred's not received, it's coming straight from the source");
	
	TestStartSection("file/writing");{
		TestLog("checking that file is empty and cursor is at beginning.");
		Test(file->bytes == 0);
		Test(file->cursor == 0);
		
		TestLog("writing first string: \"", s1, "\"");
		u32 count = file_write_result(file, s1.str, s1.count, result);
		TestResultSeparate(FileResult_Ok);
		Test(count == s1.count);
		Test(file->bytes  == s1.count);
		Test(file->cursor == s1.count);
		
		TestLog("writing newline.");
		count = file_write_result(file, "\n", 1, result);
		TestResultSeparate(FileResult_Ok);
		Test(count == 1);
		Test(file->bytes  == s1.count+1);
		Test(file->cursor == s1.count+1);
		fflush(file->handle);
		
		TestLog("writing third string as a line: \"", s3, "\"");
		count = file_write_line_result(file, s3, result);
		TestResultSeparate(FileResult_Ok);
		Test(count == s3.count+1);
		Test(file->bytes  == s1.count+1+s3.count+1);
		Test(file->cursor == s1.count+1+s3.count+1);
		fflush(file->handle);
		
		TestLog("setting cursor to after first string and writing the second one: \"", s2, "\"");
		file_set_cursor(file, s1.count+1);
		Test(file->cursor == s1.count+1);
		count = file_write_line(file, s2);
		Test(count == s2.count+1);
		Test(file->bytes  == s1.count+1+s2.count+1);
		Test(file->cursor == s1.count+1+s2.count+1);
		fflush(file->handle);
		
		TestLog("moving cursor to the end of the file and writing the third string again.");
		file_set_cursor(file, -1);
		count = file_write_line(file, s3);
		Test(count == s3.count+1);
		Test(file->bytes  == s1.count+1+s2.count+1+s3.count+1);
		Test(file->cursor == s1.count+1+s2.count+1+s3.count+1);
		fflush(file->handle);
		
		TestLog("using append_line to write the fourth string: \"", s4, "\"");
		count = file_append_line(file, s4);
		Test(count == s4.count+1);
		Test(file->bytes  == s1.count+1+s2.count+1+s3.count+1+s4.count+1);
		Test(file->cursor == s1.count+1+s2.count+1+s3.count+1);
		fflush(file->handle);
		
		TestLog("using file_append to write the last string: \"", s5, "\"");
		count = file_append(file, s5.str, s5.count);
		Test(count == s5.count);
		Test(file->bytes  == s1.count+1+s2.count+1+s3.count+1+s4.count+1+s5.count);
		Test(file->cursor == s1.count+1+s2.count+1+s3.count+1);
		fflush(file->handle);
		
		TestLog("verifying that the file was written to properly.");
		if(!file_change_access(file, 0)) return;
		str8 sanity = file_read_simple(file->path, deshi_temp_allocator);
		Test(str8_equal_lazy(sanity, s));
		
		TestLog("testing invalid Files.");
		File invalid{};
		invalid.path = STR8("an/invalid/path");
		count = file_write_result(&invalid, s1.str, s1.count, result);
		TestResultSeparate(FileResult_FileNotOpen);
		Test(!count);
		count = file_write_line_result(&invalid, s1, result);
		TestResultSeparate(FileResult_FileNotOpen);
		Test(!count);
		count = file_append_result(&invalid, s1.str, s1.count, result);
		TestResultSeparate(FileResult_FileNotOpen);
		Test(!count);
		count = file_append_line_result(&invalid, s1, result);
		TestResultSeparate(FileResult_FileNotOpen);
		Test(!count);
	}TestEndSection();
	
	TestStartSection("file/reading");{
		TestLog("reopening file for reading.");
		file_change_access(file, FileAccess_ReadAppend);
		Test(file->cursor == file->bytes);
		Test(file->cursor == s.count);
		file_set_cursor(file, 0);
		
		TestLog("reading first line of file into preallocated buffer.");
		u8 buffer[256];
		str8 read = file_read(file, buffer, s1.count);
		Test(str8_equal_lazy(read, s1));
		Test(file->cursor == s1.count);
		read = file_read(file, buffer, 1);
		Test(str8_equal_lazy(read, STR8("\n")));
		Test(file->cursor == s1.count+1);
		
		TestLog("reading second line of file with file_read_alloc.");
		read = file_read_alloc(file, s2.count, deshi_temp_allocator);
		Test(str8_equal_lazy(read, s2));
		Test(file->cursor == s1.count+1+s2.count);
		read = file_read_alloc(file, 1, deshi_temp_allocator);
		Test(str8_equal_lazy(read, STR8("\n")));
		Test(file->cursor == s1.count+1+s2.count+1);
		
		TestLog("reading third line using file_read_line.");
		read = file_read_line(file, buffer, 255);
		Test(str8_equal_lazy(read, s3));
		Test(file->cursor == s1.count+1+s2.count+1+s3.count+1);
		
		TestLog("reading third line using file_read_line_alloc.");
		read = file_read_line_alloc(file, deshi_temp_allocator);
		Test(str8_equal_lazy(read, s4));
		Test(file->cursor == s1.count+1+s2.count+1+s3.count+1+s4.count+1);
		
		TestLog("reading entire file into preallocated buffer.");
		read = file_read(file, buffer, -1);
		Test(str8_equal_lazy(read, s5));
		Test(file->cursor == s1.count+1+s2.count+1+s3.count+1+s4.count+1+s5.count);
		Test(file->cursor == file->bytes);
		
		TestLog("reading entire file using file_read_alloc.");
		file_set_cursor(file, 0);
		read = file_read_alloc(file, s.count, deshi_temp_allocator);
		Test(str8_equal_lazy(read, s));
		Test(file->cursor == file->bytes);
		
		TestLog("testing a File with an invalid path.");
		File invalid{};
		invalid.path = STR8("an/invalid/path");
		read = file_read_result(&invalid, s1.str, s1.count, result);
		TestResultSeparate(FileResult_FileNotOpen);
		Test(!read.count && !read.str);
		read = file_read_alloc_result(&invalid, s1.count, deshi_temp_allocator, result);
		TestResultSeparate(FileResult_FileNotOpen);
		Test(!read.count && !read.str);
		read = file_read_line_result(&invalid, s1.str, -1, result);
		TestResultSeparate(FileResult_FileNotOpen);
		Test(!read.count && !read.str);
		read = file_read_line_alloc_result(&invalid, deshi_temp_allocator, result);
		TestResultSeparate(FileResult_FileNotOpen);
		Test(!read.count && !read.str);
	}TestEndSection();
	
	file_deinit(file);
	file_delete(STR8("data/test_deshi_file"), FileDeleteFlags_Directory|FileDeleteFlags_Recursive);
}


#endif //DESHI_TESTS