/* deshi Linux Platform Backend
Index:
@vars
@helpers
@callback
@platform
@stopwatch
@file
@modules
@clipboard
@threading
@window
@networking
*/



//~////////////////////////////////////////////////////////////////////////////////////////////////
//// @vars
struct{
	struct{
		// points to the X server
		X11::Display* display;
		// which screen of the display are we using
		int screen;
	}x11;
}linux;

//~////////////////////////////////////////////////////////////////////////////////////////////////
//// @helpers

// handles a range of errors that can be given from errno
#define StartFileErrnoHandler(result_name, err, return_error_value) {\
	if(!result_name) {                                               \
		print_errno(err,"file",__func__,{});                         \
		return return_error_value;                                   \
	}                                                                \
	u32 __feh__errno = err;                                          \
	StartErrorHandler(result_name, FileResult, __feh__errno)

// same, but don't handle printing automatically 
#define StartFileErrnoHandlerX(result_name, err, return_error_value) {\
	u32 __feh__errno = err;                                          \
	StartErrorHandler(result_name, FileResult, __feh__errno)

#define EndFileErrnoHandler()                            \
	EndErrorHandlerAndCatch("linux-file", __feh__errno, {\
		print_errno(__feh__errno, "file", __func__, {}); \
		AssertAlways(false);                             \
	})}

// handles a single error, usually one that doesn't have to do with the system
#define FileHandleErrorL(result_name, result_tag, return_error_value, message, extra)do{\
	if(!result_name){                                                                \
		LogE("file", __func__, "(): ", message);                                     \
		return return_error_value;                                                   \
	}                                                                                \
	*result_name = {result_tag, STR8(message)};                                      \
	extra                                                                            \
	return return_error_value; } while(0);

// handles a single error, usually one that doesn't have to do with the system
// dynamic version
#define FileHandleErrorD(result_name, result_tag, return_error_value, extra, ...)do{\
	if(!result_name){                                                            \
		printf("%s\n", ToString8(deshi_temp_allocator, __func__, "(): ", __VA_ARGS__).str); \
		return return_error_value;                                               \
	}                                                                            \
	*result_name = {result_tag, ToString8(deshi_temp_allocator,__VA_ARGS__)};    \
	extra                                                                        \
	return return_error_value; } while(0)

str8
get_errno_print(u64 err, const char* tag, const char* funcname, str8 message) {
#define errcase(errname, info) case errname: return ToString8(stl_allocator, tag, ": ", funcname, "() encountered errno ", errname, ": ", info, " ", message, "\n"); break;
	switch(err){
		errcase(EPERM,          "operation not permitted")
		errcase(ENOENT,         "no such file or directory")
		errcase(ESRCH,          "no such process")
		errcase(EINTR,          "interrupted system call")
		errcase(EIO,            "unspecified I/O error")
		errcase(ENXIO,          "no such device or address")
		errcase(E2BIG,          "argument list too long")
		errcase(ENOEXEC,        "exec format error")
		errcase(EBADF,          "bad file number")
		errcase(ECHILD,         "no child processes")
		errcase(EAGAIN,         "try again")
		errcase(ENOMEM,         "kernel out of memory")
		errcase(EACCES,         "permission denied")
		errcase(EFAULT,         "bad address")
		errcase(ENOTBLK,        "block device required")
		errcase(EBUSY,          "device or resource busy")
		errcase(EEXIST,         "file exists")
		errcase(EXDEV,          "cross-device link")
		errcase(ENODEV,         "no such device")
		errcase(ENOTDIR,        "not a directory")
		errcase(EISDIR,         "is a directory")
		errcase(EINVAL,         "invalid argument")
		errcase(ENFILE,         "file table overflow")
		errcase(EMFILE,         "too many open files")
		errcase(ENOTTY,         "not a typewriter")
		errcase(ETXTBSY,        "text file busy")
		errcase(EFBIG,          "file too large")
		errcase(ENOSPC,         "no space left on device")
		errcase(ESPIPE,         "illegal seek")
		errcase(EROFS,          "read only file system")
		errcase(EMLINK,         "too many links")
		errcase(EPIPE,          "broken pipe")
		errcase(EDOM,           "math argument out of domain of func")
		errcase(ERANGE,         "math result not representable")
		errcase(ENOSYS,         "invalid system call number")
		errcase(ENOTEMPTY,      "directory not empty")
		errcase(ELOOP,          "too many symbolic links encountered")
		//errcase(EWOULDBLOCK,    "operation would block")
		errcase(ENOMSG,         "no message of desired type")
		errcase(EIDRM,          "identifier removed")
		errcase(ECHRNG,         "channel number out of range")
		errcase(EL2NSYNC,       "level 2 not synchronized")
		errcase(EL3HLT,         "level 3 halted")
		errcase(EL3RST,         "level 3 reset")
		errcase(ELNRNG,         "link number out of range")
		errcase(EUNATCH,        "protocol driver not attached")
		errcase(ENOCSI,         "no CSI structure available")
		errcase(EL2HLT,         "level 2 halted")
		errcase(EBADE,          "invalid exchange")
		errcase(EBADR,          "invalid request descriptor")
		errcase(EXFULL,         "exchange full")
		errcase(ENOANO,         "no anode")
		errcase(EBADRQC,        "invalid request code")
		errcase(EBADSLT,        "invalid slot")
		errcase(EBFONT,         "bad font file format")
		errcase(ENOSTR,         "device not a stream")
		errcase(ENODATA,        "no data available")
		errcase(ETIME,          "timer expired")
		errcase(ENOSR,          "out of streams resources")
		errcase(ENONET,         "machine is not on the network")
		errcase(ENOPKG,         "package not installed")
		errcase(EREMOTE,        "object is remote")
		errcase(ENOLINK,        "link has been severed")
		errcase(EADV,           "advertise error")
		errcase(ESRMNT,         "srmount error")
		errcase(ECOMM,          "communication error on send")
		errcase(EPROTO,         "protocol error")
		errcase(EMULTIHOP,      "multihop attempted")
		errcase(EDOTDOT,        "rFS specific error")
		errcase(EBADMSG,        "not a data message")
		errcase(EOVERFLOW,      "value too large for defined data type")
		errcase(ENOTUNIQ,       "name not unique on network")
		errcase(EBADFD,         "file descriptor in bad state")
		errcase(EREMCHG,        "remote address changed")
		errcase(ELIBACC,        "can not access a needed shared library")
		errcase(ELIBBAD,        "accessing a corrupted shared library")
		errcase(ELIBSCN,        ".lib section in a.out corrupted")
		errcase(ELIBMAX,        "attempting to link in too many shared libraries")
		errcase(ELIBEXEC,       "cannot exec a shared library directly")
		errcase(EILSEQ,         "illegal byte sequence")
		errcase(ERESTART,       "interrupted system call should be restarted")
		errcase(ESTRPIPE,       "streams pipe error")
		errcase(EUSERS,         "too many users")
		errcase(ENOTSOCK,       "socket operation on non-socket")
		errcase(EDESTADDRREQ,   "destination address required")
		errcase(EMSGSIZE,       "message too long")
		errcase(EPROTOTYPE,     "protocol wrong type for socket")
		errcase(ENOPROTOOPT,    "protocol not available")
		errcase(EPROTONOSUPPORT,"protocol not supported")
		errcase(ESOCKTNOSUPPORT,"socket type not supported")
		errcase(EOPNOTSUPP,     "operation not supported on transport endpoint")
		errcase(EPFNOSUPPORT,   "protocol family not supported")
		errcase(EAFNOSUPPORT,   "address family not supported by protocol")
		errcase(EADDRINUSE,     "address already in use")
		errcase(EADDRNOTAVAIL,  "cannot assign requested address")
		errcase(ENETDOWN,       "network is down")
		errcase(ENETUNREACH,    "network is unreachable")
		errcase(ENETRESET,      "network dropped connection because of reset")
		errcase(ECONNABORTED,   "software caused connection abort")
		errcase(ECONNRESET,     "connection reset by peer")
		errcase(ENOBUFS,        "no buffer space available")
		errcase(EISCONN,        "transport endpoint is already connected")
		errcase(ENOTCONN,       "transport endpoint is not connected")
		errcase(ESHUTDOWN,      "cannot send after transport endpoint shutdown")
		errcase(ETOOMANYREFS,   "too many references: cannot splice")
		errcase(ETIMEDOUT,      "connection timed out")
		errcase(ECONNREFUSED,   "connection refused")
		errcase(EHOSTDOWN,      "host is down")
		errcase(EHOSTUNREACH,   "no route to host")
		errcase(EALREADY,       "operation already in progress")
		errcase(EINPROGRESS,    "operation now in progress")
		errcase(ESTALE,         "stale file handle")
		errcase(EUCLEAN,        "structure needs cleaning")
		errcase(ENOTNAM,        "not a XENIX named type file")
		errcase(ENAVAIL,        "no XENIX semaphores available")
		errcase(EISNAM,         "is a named type file")
		errcase(EREMOTEIO,      "remote I/O error")
		errcase(EDQUOT,         "quota exceeded")
		errcase(ENOMEDIUM,      "no medium found")
		errcase(EMEDIUMTYPE,    "wrong medium type")
		errcase(ECANCELED,      "operation Canceled")
		errcase(ENOKEY,         "required key not available")
		errcase(EKEYEXPIRED,    "key has expired")
		errcase(EKEYREVOKED,    "key has been revoked")
		errcase(EKEYREJECTED,   "key was rejected by service")
		errcase(EOWNERDEAD,     "owner died")
		errcase(ENOTRECOVERABLE,"state not recoverable")
		errcase(ERFKILL,        "operation not possible due to RF-kill")
		errcase(EHWPOISON,      "memory page has hardware error")
	}
#undef errcase
	printf("Unknown errno: %llu", err);
	return {};
}

void print_errno(u64 err, const char* tag, const char* funcname, str8 message) {
	str8 r = get_errno_print(err, tag, funcname, message);
	if(HasFlag(deshiStage, DS_LOGGER)){
		LogE("linux", r);
	}else{
		printf("%s\n", (u8*)r.str);
	}
	free(r.str);
}

//~////////////////////////////////////////////////////////////////////////////////////////////////
//// @file
b32
deshi__file_exists(str8 caller_file, upt caller_line, str8 path, FileResult* result){
	if(!path || *path.str == 0) {
		FileHandleErrorD(result, FileResult_EmptyPath, false,, "file_exists() was passed an empty 'path' at ", caller_file, "(", caller_line, ")");
	}
	return !access((char*)path.str, F_OK);
}

b32
deshi__file_create(str8 caller_file, upt caller_line, str8 path, FileResult* result) {
	if(!path || *path.str == 0){
		FileHandleErrorD(result, FileResult_EmptyPath, 0,, "file_create() was passed an empty `path` at ",caller_file,"(",caller_line,")");
	}

	str8 scan = {path.str,0};
	while(1) {
		if(scan.count == path.count) break;
		for(u32 i = 0;;i++) {
			if(*(scan.str+scan.count) == '/' || *(scan.str+scan.count) == '\\' || scan.count == path.count - 1){
				scan.count++;
				break;
			}
			scan.count++;
		}

		// because the OS functions use null terminated strings,
		// we have to make a copy of each iteration
		// we could also temporarily set a 0 after the end of 'scan'
		// and replace it when done, but I think that would come with other problems
		str8b temp;
		str8_builder_init(&temp, scan, deshi_allocator);

		if(!file_exists(temp.fin)){
			if(str8_ends_with(temp.fin, STR8("/")) || str8_ends_with(temp.fin, STR8("\\"))) {
				if(mkdir((char*)temp.fin.str, S_IRWXO|S_IRWXG|S_IRWXU) == -1) {
					// TODO(sushi) this is scuffed
					u32 err = errno;
					if(!result && err != EEXIST) {                                               
						print_errno(err,"file",__func__,{});                         
						return 0;                                   
					}                                                                
					if (err != EEXIST) {
						StartFileErrnoHandlerX(result, errno,)
							ErrorCaseL(EACCES,       FileResult_AccessDenied,     "Search permission is denied on a component of the path prefix, or write permission is denied on the parent directory of the directory to be created.",) 
							ErrorCaseL(EEXIST,       FileResult_FileExists,       "Attemp.finted to create a directory, but a file with the same name already exists at the given path.",) 
							ErrorCaseL(ELOOP,        FileResult_SymbolicLinkLoop, "Resolving the given path resulted in a loop of symbolic links.",)
							ErrorCaseL(EMLINK,       FileResult_MaxLinks,         "Creating this directory would cause the link count of the parent directory to exceed LINK_MAX",)
							ErrorCaseL(ENAMETOOLONG, FileResult_NameTooLong,      "Either the given path or the resulting path is too long, path length must not exceed " STRINGIZE(PATH_MAX) " characters.",)
							ErrorCaseL(ENOENT,       FileResult_PathDoesNotExist, "Some part of the resulting path does not exist.",)
							ErrorCaseL(ENOSPC,       FileResult_OutOfSpace,       "The filesystem does not have enough space to create the directory.",)
							ErrorCaseL(ENOTDIR,      FileResult_NotADirectory,    "Some part of the given path is not a directory.",)
							ErrorCaseL(EROFS,        FileResult_ReadOnly,         "Some parent directory is read only.",)
						EndFileErrnoHandler()
						return 0;
					}
				}
			}else{
				// O_CREAT makes open() create the file if it does not exist
				// if it creates it, we make it with the permissions:
				//   S_IRUSR: user may read
				//   S_IWUSR: user may write
				s64 handle = open((char*)temp.fin.str, O_CREAT, S_IRWXO|S_IRWXG|S_IRWXU);
				if(handle == -1){
					// TODO(sushi) this is scuffed
					u32 err = errno;
					if(!result && err != EEXIST) {                                               
						print_errno(err,"file",__func__,{});                         
						return 0;                                   
					}                                                                
					if (err != EEXIST) {
						StartFileErrnoHandlerX(result, errno,)
							ErrorCaseL(EACCES,        FileResult_AccessDenied,      "It is possible that the file already exists and is FIFO or a regular file, the owner of the file is netiher the current user nor the owner of the containing directory, or the containing directory is both world- or group-writable and sticky.",);
							ErrorCaseF(ENOSPC)
							ErrorCaseL(EDQUOT,        FileResult_OutOfSpace,        "The file does not already exist, but the file system is full.",);
							ErrorCaseL(EISDIR,        FileResult_IsADirectory,      "The given path already exists and is a directory.",);
							ErrorCaseL(ENOTDIR,       FileResult_NotADirectory,     "Some component of the given path is not a directory.",);
							ErrorCaseL(ELOOP,         FileResult_SymbolicLinkLoop,  "Too many symbolic links were encountered when resolving the path.",);
							ErrorCaseL(EMFILE,        FileResult_TooManyHandles,    "The per-process limit on the amount of handles opened has been reached.",);
							ErrorCaseL(ENAMETOOLONG,  FileResult_NameTooLong,       "The given path is too long.",);
							ErrorCaseL(ENFILE,        FileResult_TooManyHandles,    "The system-wide limit on the total number of open handles has been reached.",);
							ErrorCaseL(ENOENT,        FileResult_PathDoesNotExist,  "Some part of the given path does not exist.",);
							ErrorCaseL(ENOMEM,        FileResult_SystemOutOfMemory, "The kernel has insufficient memory to create a file.",);
							ErrorCaseF(EFBIG)
							ErrorCaseL(EOVERFLOW,     FileResult_TooBig,            "The file is too large to open.",);
							ErrorCaseL(EFAULT,        FileResult_InvalidArgument,   "The given path is outside of the accessible address space.", result->error.invalid_arg = 1;);
							ErrorCaseL(EINVAL,        FileResult_InvalidArgument,   "The given path is invalid, eg. it contains characters not permitted by the underlying filesystem.", result->error.invalid_arg = 1;);
						EndFileErrnoHandler()
						return 0;
					}
				}
				close(handle);
			}
		}
		str8_builder_deinit(&temp);
	} 

	
	return 1;
}

b32
deshi__file_delete(str8 caller_file, upt caller_line, str8 path, u32 flags, FileResult* result){
	if(!path || *path.str == 0){
		FileHandleErrorD(result, FileResult_EmptyPath,0,,"file_delete() was passed an empty `path` at ",caller_file,"(",caller_line,")");
	}

	// NOTE(sushi) if the file already doesn't exist, we consider the function
	//             to have succeeded.
	if(!file_exists_result(path, result)) return 1;

	FileType type = file_get_type_of_path_result(path, result);
	if(!type) return 0;

	if(type == FileType_File) {
		if(!HasFlag(flags, FileDeleteFlags_File)){
			FileHandleErrorD(result, FileResult_InvalidArgument,0,, "in ", caller_file, "(",caller_line,"): file_delete() was called on a file, but FileDeleteFlags_File was not specified as a flag.");
		}
		// we are just deleting a single file, so we don't need to do anything special
		if(unlink((char*)path.str) == -1) {
			StartFileErrnoHandler(result, errno,0)
				ErrorCaseL(EACCES,       FileResult_AccessDenied,      "Write access to the directory containing the given path is not allowed for the current process's user.",);
				ErrorCaseL(EBUSY,        FileResult_PathBusy,          "The given path cannot be unlinked because it is being used by the system or another process.",);
				ErrorCaseL(EFAULT,       FileResult_InvalidArgument,   "The given path pointer points outside of accessible address space.", result->error.invalid_arg = 1;);
				ErrorCaseL(EIO,          FileResult_IOError,           "Linux has encountered an unspecified I/O error.",);
				ErrorCaseL(ELOOP,        FileResult_SymbolicLinkLoop,  "Too many symbolic links were encounted in translating the given path",);
				ErrorCaseL(ENAMETOOLONG, FileResult_NameTooLong,       "The given path is too long.",);
				ErrorCaseL(ENOENT,       FileResult_PathDoesNotExist,  "Some part of the given path does not exist.",);
				ErrorCaseL(ENOMEM,       FileResult_SystemOutOfMemory, "The kernel has insufficient memory to complete the unlinking operation.",);
				ErrorCaseL(ENOTDIR,      FileResult_NotADirectory,     "Some part of the given path is not a directory.",);
				ErrorCaseL(EPERM,        FileResult_AccessDenied,      "The filesystem does not allow unlinking of files or the process does have the correct permissions.",);
				ErrorCaseL(EROFS,        FileResult_ReadOnly,          "The filesystem is read only.",);
			EndFileErrnoHandler()
		}
	}

	if(type == FileType_Directory) {
		if(!HasFlag(flags, FileDeleteFlags_Directory)){
			FileHandleErrorD(result, FileResult_IsADirectory,0,,"in ", caller_file, "(", caller_line, "): file_delete() was called on a directory, but FileDeleteFlags_Directory was not specified as a flag.");
		}

		if(HasFlag(flags, FileDeleteFlags_Recursive)) {
			File* files = file_search_directory_result(path, result);
			defer {array_deinit(files);};
			if(!files) return 0;
			forX_array(file, files){
				if(file->type == FileType_Directory){
					if(!file_delete_result(file->path, FileDeleteFlags_Directory|FileDeleteFlags_Recursive,result))
						return 0;
				}else if(file->type == FileType_File){
					if(!file_delete_result(file->path, FileDeleteFlags_File, result))
						return 0;
				}
			}
		}

		if(rmdir((char*)path.str) == -1) {
			StartFileErrnoHandler(result, errno, 0)
				ErrorCaseL(EACCES, FileResult_AccessDenied,     "Search permission is denied on some component of the path prefix, or write permission is denied on the parent directory of the directory to be removed.",)
				ErrorCaseL(EBUSY,  FileResult_PathBusy,         "The directory to be removed is currently in use by the system or some process.",)
				case ENOTEMPTY:
				ErrorCaseL(EEXIST, FileResult_FileExists,       "The given directory is not empty and the FileResult_Recursive flag was not set.",)
				ErrorCaseL(EINVAL, FileResult_InvalidArgument,  "The path ends with a '.'.",)
				ErrorCaseL(EIO,    FileResult_IOError,          "A physical I/O error has occured.",)
				ErrorCaseL(ELOOP,  FileResult_SymbolicLinkLoop, "Symbolic link loop encountered.",)
			EndFileErrnoHandler();
		}
	}
	return 1;
}

b32 
deshi__file_rename(str8 caller_file, upt caller_line, str8 old_path, str8 new_path, FileResult* result){
	if(!old_path || *old_path.str == 0) FileHandleErrorD(result, FileResult_EmptyPath,0,, "file_rename() was passed an empty `old_path` at ",caller_file,"(",caller_line,")");
	if(!new_path || *new_path.str == 0) FileHandleErrorD(result, FileResult_EmptyPath,0,, "file_rename() was passed an empty `new_path` at ",caller_file,"(",caller_line,")");

	if(rename((char*)old_path.str, (char*)new_path.str) == -1){
		StartFileErrnoHandler(result, errno, 0)
			ErrorCaseL(EACCES,       FileResult_AccessDenied,     "A component of either path prefix denies search permission; or one of the directories containing old or new denies write permissions; or, write permission is required and is denied for a directory pointed to by the old or new arguments. ",)
			ErrorCaseL(EBUSY,        FileResult_PathBusy,         "The directory named by old or new is currently in use by the system or another process, and the implementation considers this an error. ",)
			case EEXIST: 
			ErrorCaseL(ENOTEMPTY,    FileResult_FileExists,       "The link named by new is a directory that is not an empty directory. ",)
			ErrorCaseL(EINVAL,       FileResult_InvalidArgument,  "The new directory pathname contains a path prefix that names the old directory. ",)
			ErrorCaseL(EIO,          FileResult_IOError,          "A physical I/O error has occurred. ",)
			ErrorCaseL(EISDIR,       FileResult_IsADirectory,     "The new argument points to a directory and the old argument points to a file that is not a directory. ",)
			ErrorCaseL(ELOOP,        FileResult_SymbolicLinkLoop, "A loop exists in symbolic links encountered during resolution of the path argument. ",)
			ErrorCaseL(EMLINK,       FileResult_SymbolicLinkLoop, "The file named by old is a directory, and the link count of the parent directory of new would exceed {LINK_MAX}. ",)
			ErrorCaseL(ENAMETOOLONG, FileResult_NameTooLong,      "The length of the old or new argument exceeds {PATH_MAX} or a pathname component is longer than {NAME_MAX}. ",)
			ErrorCaseL(ENOENT,       FileResult_PathDoesNotExist, "The link named by old does not name an existing file, or either old or new points to an empty string. ",)
			ErrorCaseL(ENOSPC,       FileResult_OutOfSpace,       "The directory that would contain new cannot be extended. ",)
			ErrorCaseL(ENOTDIR,      FileResult_NotADirectory,    "A component of either path prefix is not a directory; or the old argument names a directory and new argument names a non-directory file. ",)
		EndFileErrnoHandler();
	}

	return 1;
}

b32
deshi__file_copy(str8 caller_file, upt caller_line, str8 src_path, str8 dst_path, FileResult* result){
	if(str8_equal(src_path, dst_path)){
		FileHandleErrorL(result, FileResult_InvalidArgument,0,"'src_path' and 'dst_path' may not be the same.",);
	}

	if(!file_create_result(dst_path, result)) return 0;

	File* src = file_init_result(src_path, FileAccess_Read, result);
	defer {file_deinit(src);};
	if(!src) return 0;

	File* dst = file_init_result(dst_path, FileAccess_Write, result);
	defer {file_deinit(dst);};
	if(!dst) return 0;

	str8 buffer = file_read_alloc_result(src, src->bytes, deshi_allocator, result);
	defer {memzfree(buffer.str);};
	if(!buffer.str) return 0;

	u64 c = file_write_result(dst, buffer.str, buffer.count, result);
	if(c != buffer.count) return 0;

	return 1;
}

File 
deshi__file_info(str8 caller_file, upt caller_line, str8 path, FileResult* result) {
	if(!path || *path.str == 0){
		FileHandleErrorD(result, FileResult_EmptyPath, {},, 
			"file_info() was passed an empty `path` at ",caller_file,"(",caller_line,")"
		);
	}

	if(!file_exists(path)){
		FileHandleErrorD(result, FileResult_PathDoesNotExist, {},, 
			"at ", caller_file, "(", caller_line, "): file_init() was given a path to a file that doesn't exist"
		);
	}

	File out = {};

	// gather absolute path and format into different parts
	out.path.str = (u8*)memalloc(PATH_MAX);
	out.path.str = (u8*)realpath((char*)path.str, (char*)out.path.str);
	if(!out.path.str) {
		StartFileErrnoHandler(result, errno, {})
			ErrorCaseL(EACCES, FileResult_AccessDenied,      "Read or search permission was denied for a component of the path prefix.",)
			ErrorCaseL(EIO,    FileResult_IOError,           "An I/O error occurred while reading from the filesystem.",)
			ErrorCaseL(ELOOP,  FileResult_SymbolicLinkLoop,  "Too many symbolic links were encountered in translating the pathname.",)
			ErrorCaseL(ENAMETOOLONG, FileResult_NameTooLong, "A component of a pathname exceeded NAME_MAX characters, or an entire pathname exceeded PATH_MAX characters.",)
			ErrorCaseL(ENOENT, FileResult_PathDoesNotExist,  "The named file does not exist.",)
			ErrorCaseL(ENOMEM, FileResult_SystemOutOfMemory, "System ran out of memory.",)
			ErrorCaseL(ENOTDIR, FileResult_NotADirectory,    "A component of the path prefix is not a directory.",)
		EndFileErrnoHandler();
	}
	out.path.count = strlen((char*)out.path.str);
	out.path.str = (u8*)memrealloc(out.path.str, out.path.count);

	u32 name_length = 0;
	u8* scan = out.path.str+out.path.count;
	while(name_length != out.path.count && 
		 *scan != '\\' && 
		 *scan != '/') {
		scan--;
		name_length++;
	}
	name_length--;

	out.name = {out.path.str + out.path.count - name_length, name_length};
	out.front = out.name;
	forI_reverse(name_length) {
		// on linux, files starting with a '.' are hidden
		// so we don't want to say the entire name is the extension
		if(i && out.name.str[i] == '.'){ 
			out.front.count = i;
		}
	}
	if(out.front.count != out.name.count)
		out.ext = {out.front.str+out.front.count+1, out.path.str+out.path.count-(out.front.str+out.front.count+1)};

	// get time information
	struct statx s;
	if(statx(0, (char*)out.path.str, 0, STATX_ATIME|STATX_BTIME|STATX_MTIME|STATX_SIZE|STATX_MODE, &s) == -1) {
		// NOTE(sushi) I believe that this should not error in most cases, because we just checked a lot of this
		//             while opening the file, but let me know if it does throw something not here.
		StartFileErrnoHandler(result, errno, {})
			ErrorCaseL(ENOMEM, FileResult_SystemOutOfMemory, "The system is out of memory.",)
		EndFileErrnoHandler();
	}

	out.creation_time = s.stx_btime.tv_sec;
	out.last_access_time = s.stx_atime.tv_sec;
	out.last_write_time = s.stx_mtime.tv_sec;
	out.bytes = s.stx_size;

	if     (S_ISREG(s.stx_mode)) out.type = FileType_File;
	else if(S_ISDIR(s.stx_mode)) out.type = FileType_Directory;
	else if(S_ISLNK(s.stx_mode)) out.type = FileType_SymbolicLink;	
	else{
		FileHandleErrorD(result, FileResult_InvalidArgument,{},, 
			"attempted to gather info for a path that is not a file, directory, or symbolic link. path: ", path
		);
	}

	return out;
}

File*
deshi__file_search_directory(str8 caller_file, upt caller_line, str8 directory, FileResult* result){
	if(!directory || *directory.str == 0){
		FileHandleErrorD(result, FileResult_EmptyPath, 0, , "file_search_directory() was passed an empty `directory` at ",caller_file,"(",caller_line,")");
	}

	if(!file_exists(directory)) {
		FileHandleErrorD(result, FileResult_PathDoesNotExist, 0,, "file_search_directory() was given a path that does not exist at ",caller_file,"(",caller_line,")");
	}

	File* out;
	array_init(out, 4, deshi_temp_allocator);

	DIR* dir = opendir((char*)directory.str);
	if(!dir) {
		StartFileErrnoHandler(result, errno, 0)
			ErrorCaseL(EACCES, FileResult_AccessDenied,      "This process does not have permission to open the given directory",)
			ErrorCaseL(EMFILE, FileResult_TooManyHandles,    "This process has too many handles open.",)
			ErrorCaseL(ENFILE, FileResult_TooManyHandles,    "The system-wide limit on open handles has been reached.",)
			ErrorCaseL(ENOENT, FileResult_PathDoesNotExist,  "The given path doesn't exist",)
			ErrorCaseL(ENOMEM, FileResult_SystemOutOfMemory, "Not enough memory to open directory.",)
		EndFileErrnoHandler()
	}


	struct dirent* entry;
	while((entry = readdir(dir))){
		if(!strcmp(entry->d_name, "..") || !strcmp(entry->d_name, ".")) continue;
		str8 filepath = ToString8(deshi_allocator, directory, "/", entry->d_name);
		defer {memzfree(filepath.str);};

		File file = file_info_result(filepath, result);
		if(!file.creation_time) return 0;

		*array_push(out) = file;
	}	

	return out;
}

str8 
deshi__file_path_absolute(str8 caller_file, upt caller_line, str8 path, FileResult* result){
	str8 out;
	out.str = (u8*)memtalloc(PATH_MAX);
	out.str = (u8*)realpath((char*)path.str, (char*)out.str);
	if(!out.str) {
		StartFileErrnoHandler(result, errno, {})
			ErrorCaseL(EACCES,       FileResult_AccessDenied,      "Read or search permission was denied for a component of the path prefix.",)
			ErrorCaseL(EIO,          FileResult_IOError,           "An I/O error occurred while reading from the filesystem.",)
			ErrorCaseL(ELOOP,        FileResult_SymbolicLinkLoop,  "Too many symbolic links were encountered in translating the pathname.",)
			ErrorCaseL(ENAMETOOLONG, FileResult_NameTooLong,       "A component of a pathname exceeded NAME_MAX characters, or an entire pathname exceeded PATH_MAX characters.",)
			ErrorCaseL(ENOENT,       FileResult_PathDoesNotExist,  "The named file does not exist.",)
			ErrorCaseL(ENOMEM,       FileResult_SystemOutOfMemory, "System ran out of memory.",)
			ErrorCaseL(ENOTDIR,      FileResult_NotADirectory,     "A component of the path prefix is not a directory.",)
		EndFileErrnoHandler();
		return {};
	}
	out.count = strlen((char*)out.str);
	out.str = (u8*)memtrealloc(out.str, out.count);
	return out;
}

b32 
deshi__file_path_equal(str8 caller_file, upt caller_line, str8 path1, str8 path2, FileResult* result){
	if(!path1 || *path1.str == 0) FileHandleErrorD(result, FileResult_EmptyPath, 0,, "file_path_equal() was passed an empty `path1` at ",caller_file,"(",caller_line,")");
	if(!path2 || *path2.str == 0) FileHandleErrorD(result, FileResult_EmptyPath, 0,, "file_path_equal() was passed an empty `path2` at ",caller_file,"(",caller_line,")");

	str8b path1b;
	str8_builder_init(&path1b, path1, deshi_allocator);
	str8_builder_replace_codepoint(&path1b, '\\', '/');
	defer {str8_builder_deinit(&path1b);};

	str8b path2b;
	str8_builder_init(&path2b, path2, deshi_allocator);
	str8_builder_replace_codepoint(&path2b, '\\', '/');
	defer {str8_builder_deinit(&path2b);};

	str8 p1normalized = file_path_absolute_result(path1b.fin, result);
	if(!p1normalized) return 0;
	str8 p2normalized = file_path_absolute_result(path2b.fin, result);
	if(!p2normalized) return 0;

	return str8_equal(p1normalized, p2normalized);
}

File* 
deshi__file_init(str8 caller_file, upt caller_line, str8 path, FileAccess access, b32 ignore_nonexistence, FileResult* result) {
	if(!path || *path.str == 0){
		FileHandleErrorD(result, FileResult_EmptyPath, 0,, 
			"file_init() was passed an empty `path` at ",caller_file,"(",caller_line,")"
		);
	}

	b32 exists = file_exists(path);

	if(exists) forX_array(f, file_shared.files) {
		if(file_path_equal(path, f->path)){
			if(!file_change_access_result(f, access, result)) return 0;
			return f;
		}
	}

	File* out = array_push(file_shared.files);

	if(HasFlag(access, FileAccess_Create)) {
		if(!file_create_result(path, result)) return 0;
	}

	*out = file_info_result(path, result);
	if(!out->path.str) return 0;
	if(!file_change_access_result(out, access, result)) return 0;

	return out;
}

b32
deshi__file_deinit(str8 caller_file, upt caller_line, File* file, FileResult* result) {
	if(!file){
		FileHandleErrorD(result, FileResult_InvalidArgument,0,,"file_deinit() was given a null File pointer.");
	}
	memzfree(file->path.str);

	if(file->handle) fclose(file->handle);
	forI(array_count(file_shared.files)){
		if(file_shared.files + i == file){
			array_remove_unordered(file_shared.files, i);
			break;
		}
	}
	return 1;
}

b32
deshi__file_change_access(str8 caller_file, upt caller_line, File* file, FileAccess access, FileResult* result){
	if(file == 0){
		FileHandleErrorD(result, FileResult_InvalidArgument, 0,,"file_change_access() was passed a null `file` pointer at ",caller_file,"(",caller_line,")");
	}

	RemoveFlag(access, FileAccess_Create); // the file already exists
	if(!file->handle && HasFlag(access, FileAccess_ReadWrite)) {
		Flags open_flags = access & ~(FileAccess_Append);
		char* open_mode = 0;
		switch(open_flags) {
			case FileAccess_Read:              open_mode = "rb";  break;
			case FileAccess_WriteTruncate:     open_mode = "wb";  file->bytes = 0; break;
			case FileAccess_Write:
			case FileAccess_ReadWrite:         open_mode = "rb+"; break;
			case FileAccess_ReadWriteTruncate: open_mode = "wb+"; file->bytes = 0; break;
		}
		
		file->handle = fopen((char*)file->path.str, open_mode);
		if(!file->handle) {
			StartFileErrnoHandler(result, errno, 0)
				ErrorCaseL(EACCES,        FileResult_AccessDenied,      "It is possible that the file already exists and is FIFO or a regular file, the owner of the file is netiher the current user nor the owner of the containing directory, or the containing directory is both world- or group-writable and sticky.",);
				ErrorCaseF(ENOSPC)
				ErrorCaseL(EDQUOT,        FileResult_OutOfSpace,        "The file does not already exist, but the file system is full.",);
				ErrorCaseL(EISDIR,        FileResult_IsADirectory,      "The given path already exists and is a directory.",);
				ErrorCaseL(ENOTDIR,       FileResult_NotADirectory,     "Some component of the given path is not a directory.",);
				ErrorCaseL(ELOOP,         FileResult_SymbolicLinkLoop,  "Too many symbolic links were encountered when resolving the path.",);
				ErrorCaseL(EMFILE,        FileResult_TooManyHandles,    "The per-process limit on the amount of handles opened has been reached.",);
				ErrorCaseL(ENAMETOOLONG,  FileResult_NameTooLong,       "The given path is too long.",);
				ErrorCaseL(ENFILE,        FileResult_TooManyHandles,    "The system-wide limit on the total number of open handles has been reached.",);
				ErrorCaseL(ENOENT,        FileResult_PathDoesNotExist,  "Some part of the given path does not exist.",);
				ErrorCaseL(ENOMEM,        FileResult_SystemOutOfMemory, "The kernel has insufficient memory to create a file.",);
				ErrorCaseF(EFBIG)
				ErrorCaseL(EOVERFLOW,     FileResult_TooBig,            "The file is too large to open.",);
				ErrorCaseL(EFAULT,        FileResult_InvalidArgument,   "The given path is outside of the accessible address space.", result->error.invalid_arg = 1;);
				ErrorCaseL(EINVAL,        FileResult_InvalidArgument,   "The given path is invalid, eg. it contains characters not permitted by the underlying filesystem.", result->error.invalid_arg = 1;);
			EndFileErrnoHandler()
			return 0;
		}
	} else if(file->handle && !HasFlag(access, FileAccess_ReadWrite)) {
		fclose(file->handle);
		file->handle = 0;
	}

	if(file->handle && HasFlag(access, FileAccess_Append)) {
		fseek(file->handle, file->bytes, SEEK_SET);
		file->cursor = file->bytes;
	}

	if(file->handle && HasFlag(access, FileAccess_Truncate)) {
		fclose(file->handle);
		file->handle = 0;
		if     (HasFlag(access, FileAccess_ReadWrite)) file->handle = fopen((char*)file->path.str, "wb+");
		else if(HasFlag(access, FileAccess_Write))     file->handle = fopen((char*)file->path.str, "wb");
		else {
			// just truncate
			fclose(fopen((char*)file->path.str, "wb"));
		}
	}

	file->access = access & ~(FileAccess_Append|FileAccess_Truncate);

	return 1;
}

File* 
file_initted_files(){
	return file_shared.files;
}

str8
deshi__file_read_simple(str8 caller_file, upt caller_line, str8 path, Allocator* allocator, FileResult* result) {
	if(!path){
		FileHandleErrorD(result, FileResult_EmptyPath,{},,"at ", caller_file, ":", caller_line, ": file_read_simple() was given an empty path.");
	}

	// very scuffed way of going about this
	// TODO(sushi) change this to just use the platform stuff directly
	File f = file_info_result(path, result);
	if(!f.creation_time) return {};
	if(!file_change_access_result(&f, FileAccess_Read, result)) return {};

	str8 out = file_read_alloc_result(&f, f.bytes, allocator, result);

	if(!file_change_access_result(&f, ~FileAccess_ReadWrite, result)) return {};

	return out;
}

u64 
deshi__file_write_simple(str8 caller_file, upt caller_line, str8 path, void* data, u64 bytes, FileResult* result) {
	if(!path){
		FileHandleErrorD(result, FileResult_EmptyPath,{},,"at ", caller_file, ":", caller_line, ": file_write_simple() was given an empty path.");
	}

	File* f = file_init_result(path, FileAccess_WriteTruncateCreate, result);
	if(!f) return 0;

	u64 c = file_write_result(f, data, bytes, result);

	file_deinit_result(f, result);
	return c;
}

u64 
deshi__file_append_simple(str8 caller_file, upt caller_line, str8 path, void* data, u64 bytes, FileResult* result) {
	if(!path) {
		FileHandleErrorD(result, FileResult_EmptyPath,0,, "at ", caller_file, ":", caller_line, ": file_append_simple() was given an empty path.");
	}

	File* f = file_init_result(path,FileAccess_WriteAppendCreate,result);
	if(!f) return 0;

	u64 c = file_append_result(f, data, bytes, result);

	file_deinit_result(f, result);
	return c;
}

FileType
deshi__file_get_type_of_path(str8 caller_file, upt caller_line, str8 path, FileResult* result) {
	if(!path || *path.str == 0){
		LogE("file","file_delete() was passed an empty `path` at ",caller_file,"(",caller_line,")");
		return FileType_ERROR;
	}

	struct stat path_stat;
	if(stat((char*)path.str, &path_stat) == -1) {
		switch(errno){
			case EACCES:       LogE("file", "file_get_type_of_path() cannot open the path '", path, "'. Access to some part of the path is denied. ERRNO 13: EACCES"); break;
			case EFAULT:       LogE("file", "file_get_type_of_path() failed, bad address. ERRNO 14: EFAULT."); break;
			case ELOOP:        LogE("file", "file_get_type_of_path() cannot use the path '", path, "', encountered too many symbolic links. ERRNO 40: ELOOP"); break;
			case ENAMETOOLONG: LogE("file", "file_get_type_of_path() was provided a path that is too long for stat(). ERRNO 36: ENAMETOOLONG"); break;
			case ENOMEM:       LogE("file", "file_get_type_of_path() attempted to call stat(), but the kernel is out of memory. ERRNO 12: ENOMEM"); break;
			default:           LogE("file", "file_get_type_of_path() encounted an error when calling stat(), but this error is not handled or acknowledged."); break;
		}
		return FileType_ERROR;
	}

	if(S_ISREG(path_stat.st_mode)) return FileType_File;
	if(S_ISDIR(path_stat.st_mode)) return FileType_Directory;
	if(S_ISLNK(path_stat.st_mode)) return FileType_SymbolicLink;

	LogE("file", "file_get_type_of_path() encountered unhandled file mode at path '", path, "'. \nhandled modes are regular files, directories, and symbolic links.");
	return FileType_ERROR;
}

//~////////////////////////////////////////////////////////////////////////////////////////////////
//// @stopwatch

Stopwatch
start_stopwatch() {
	timespec current;
	// I am unsure if 'CLOCK_PROCESS_CPUTIME_ID' is appropriate here.
	// it is the high resolution clock available from Linux
	if(clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &current) == -1){
		switch(errno) {
			case EFAULT: LogE("stopwatch", "in start_stopwatch(), clock_gettime() was passed a bad address. ERRNO 14: EFAULT"); return 0;
			case EINVAL: LogE("stopwatch", "in start_stopwatch(), clock_gettime() was given a clk_id that is not supported on this system. ERRNO 22: EINVAL"); return 0;
		}
	}
	return (current.tv_sec*1000000000+current.tv_nsec)/3;
}

f64 peek_stopwatch(Stopwatch watch) {
	timespec current;
	if(clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &current) == -1){
		switch(errno) {
			case EFAULT: LogE("stopwatch", "in peek_stopwatch(), clock_gettime() was passed a bad address. ERRNO 14: EFAULT"); return 0;
			case EINVAL: LogE("stopwatch", "in peek_stopwatch(), clock_gettime() was given a clk_id that is not supported on this system. ERRNO 22: EINVAL"); return 0;
		}
	}
	return (current.tv_sec*1000000000+current.tv_nsec)/3 - watch;
}

//~////////////////////////////////////////////////////////////////////////////////////////////////
//// @platform

void
platform_init() {
	DeshiStageInitStart(DS_PLATFORM, DS_MEMORY, "Attempted to initialize Platform module before initializing the Memory module");

	array_init(file_shared.files, 16, deshi_allocator);

	// NotImplemented;

	DeshiStageInitEnd(DS_PLATFORM);
}

b32 
platform_update() {
	NotImplemented;
	return 0;
}

void 
platform_sleep(u32 time) {
	NotImplemented;
}

void 
platform_cursor_position(s32 x, s32 y) {
	NotImplemented;
}

void* 
platform_load_module(str8 module_path) {
	NotImplemented;
	return 0;
}

void 
platform_free_module(void* module) {
	NotImplemented;
}

void* 
platform_get_module_symbol(void* module, const char* symbol_name) {
	NotImplemented;
	return 0;
}

str8 
platform_get_clipboard() {
	NotImplemented;
	return {};
}

void 
platform_set_clipboard(str8 text) {
	NotImplemented;
}

//~////////////////////////////////////////////////////////////////////////////////////////////////
//// @threading

void 
threader_init(u32 max_jobs) {
	NotImplemented;
}

void
threader_spawn_thread(u32 count){
	NotImplemented;
}

void
threader_close_all_threads(){
	NotImplemented;
}

void
threader_add_job(ThreadJob job){
	NotImplemented;
}

void
threader_add_jobs(carray<ThreadJob> jobs){
	NotImplemented;
}

void
threader_cancel_all_jobs(){
	NotImplemented;
}

void
threader_wake_threads(u32 count){
	NotImplemented;
}

void
threader_set_thread_name(str8 name){
	NotImplemented;
}


//~////////////////////////////////////////////////////////////////////////////////////////////////
//// @window
Window*
window_create(str8 title, s32 width, s32 height, s32 x, s32 y, DisplayMode display_mode, Decoration decorations){
	DeshiStageInitStart(DS_WINDOW, DS_PLATFORM, "Called window_create() before initializing Platform module");
	
	linux.x11.display = X11::XOpenDisplay(0);
	linux.x11.screen = X11::XDefaultScreen(linux.x11.display);
	u32 black = X11::XBlackPixel(linux.x11.display, linux.x11.screen);
	u32 white = X11::XWhitePixel(linux.x11.display, linux.x11.screen);

	Window* window = (Window*)memalloc(sizeof(Window));
	window->handle = (void*)X11::XCreateSimpleWindow(linux.x11.display, X11::XDefaultRootWindow(linux.x11.display), 0,0,200,300,5,white,black);
	
	X11::XSetStandardProperties(linux.x11.display, (X11::Window)window->handle, (const char*)title.str, 0,0,0,0,0);
	DeshiStageInitEnd(DS_WINDOW);
	return window;

} // window_create


void window_close(Window* window){
	NotImplemented;
}

void
window_swap_buffers(Window* window){
	NotImplemented;
}

void window_display_mode(Window* window, DisplayMode displayMode){
	NotImplemented;
}

void 
window_show(Window* window) {
	NotImplemented;
}

void 
window_hide(Window* window) {
	NotImplemented;
}

void 
window_set_title(Window* window, str8 title) {
	NotImplemented;
}

void 
window_set_cursor_mode(Window* window, CursorMode mode){
	NotImplemented;
}

void 
window_set_cursor_type(Window* window, CursorType curtype) {
	NotImplemented;
}

void 
window_set_cursor_position(Window* window, s32 x, s32 y){
	NotImplemented;
}

