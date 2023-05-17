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
void print_errno(u64 err, const char* tag, const char* funcname, str8 message) {
#define errcase(errname, info) case errname: LogE("linux", tag, ": ", funcname, "(): encountered errno ", errname, ": ", STRINGIZE(errname), ". ", info, message); break;
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
}

//~////////////////////////////////////////////////////////////////////////////////////////////////
//// @file
b32
deshi__file_exists(str8 caller_file, upt caller_line, str8 path){
	if(!path || *path.str == 0) {
		LogE("file", "file_exists() was passed an empty 'path' at ", caller_file, "(", caller_line, ")");
		return false;
	}
	return !access((char*)path.str, F_OK);
}

void 
deshi__file_create(str8 caller_file, upt caller_line, str8 path) {
	if(!path || *path.str == 0){
		LogE("file","file_create() was passed an empty `path` at ",caller_file,"(",caller_line,")");
		return;
	}

	// O_CREAT makes open() create the file if it does not exist
	// if it creates it, we make it with the permissions:
	//   S_IRUSR: user may read
	//   S_IWUSR: user may write
	s64 handle = open((char*)path.str, O_CREAT, S_IRUSR|S_IWUSR);

	if(handle == -1){
		u32 e = errno;
		switch(e) {
			case EACCES: print_errno(EACCES, "file", __func__, ToString8(deshi_temp_allocator, "while trying to create path '", path, "'. \npossible reasons include: the protected_fifos or protected_regular sysctl is enabled, the file already exists and is a FIFO or regular file, the owner of the file is neither the current user nor the owner of the containing directory, and the containing directory is both world- or group-writable and sticky.")); return;
			default: print_errno(e,"file",__func__,ToString8(deshi_temp_allocator, "while trying to create path '", path, "'"));
		}
	}

	close(handle);
}

void
deshi__file_delete(str8 caller_file, upt caller_line, str8 path, u32 flags){NotImplemented;
	if(!path || *path.str == 0){
		LogE("file","file_delete() was passed an empty `path` at ",caller_file,"(",caller_line,")");
		return;
	}

	if(!file_exists(path)){
		LogE("file", "file_delete() was a passed a path to a file that doesn't exist.");
		return;
	}

	FileType type = file_get_type_of_path(path);
	if(!type) return;

	if(type == FileType_File) {
		if(!HasFlag(flags, FileDeleteFlags_File)){
			LogE("file", "in ", caller_file, "(",caller_line,"): file_delete() was called on a file, but FileDeleteFlags_File was not specified as a flag.");
			return;
		}
		// we are just deleting a single file, so we don't need to do anything special
		if(unlink((char*)path.str) == -1) {
			print_errno(errno, "file", __func__, ToString8(deshi_temp_allocator, "while trying to delete path '", path, "'"));
		}
		return;
	}

	if(type == FileType_Directory) {
		if(!HasFlag(flags, FileDeleteFlags_Directory)){
			LogE("file", "in ", caller_file, "(", caller_line, "): file_delete() was called on a directory, but FileDeleteFlags_Directory was not specified as a flag.");
			return;
		}

		if(HasFlag(flags, FileDeleteFlags_Recursive_And_I_Promise_I_Am_Using_This_Responsibly)) {
			while(1) {
				errno = 0;
				DIR* dir;
				struct dirent* ep;
				dir = opendir((char*)path.str);
				if(!dir){
					print_errno(errno, "file", __func__, {});
					return;
				}
				while((ep = readdir(dir))){
					if(ep->d_type == DT_REG){

					}
				}
			}
		}
	}


	if(HasAllFlags(flags, FileDeleteFlags_Directory | FileDeleteFlags_File)) {
		// deleting both files and directories
	}


}

void 
deshi__file_rename(str8 caller_file, upt caller_line, str8 old_path, str8 new_path){
	NotImplemented;
}

void 
deshi__file_copy(str8 caller_file, upt caller_line, str8 src_path, str8 dst_path){
	NotImplemented;
}

File 
deshi__file_info(str8 caller_file, upt caller_line, str8 path) {
	NotImplemented;
	return {};
}

File* 
deshi__file_search_directory(str8 caller_file, upt caller_line, str8 directory){
	if(!directory || *directory.str == 0){
		LogE("file","file_search_directory() was passed an empty `directory` at ",caller_file,"(",caller_line,")");
		return 0;
	}


	NotImplemented;

	return 0;
}

str8 
deshi__file_path_absolute(str8 caller_file, upt caller_line, str8 path){
	NotImplemented;
	return {};
}

b32 
deshi__file_path_equal(str8 caller_file, upt caller_line, str8 path1, str8 path2){
	NotImplemented;
	return 0;
}

File* 
deshi__file_init(str8 caller_file, upt caller_line, str8 path, FileAccess access, b32 ignore_nonexistence) {
	NotImplemented;
	return 0;
}

void 
deshi__file_deinit(str8 caller_file, upt caller_line, File* file) {
	NotImplemented;
}

void 
deshi__file_change_access(str8 caller_file, upt caller_line, File* file, FileAccess access){
	NotImplemented;
}

File* 
file_initted_files(){
	NotImplemented;
	return 0;
}

str8
deshi__file_read_simple(str8 caller_file, upt caller_line, str8 path, Allocator* allocator) {
	NotImplemented;
	return {};
}

u64 
deshi__file_write_simple(str8 caller_file, upt caller_line, str8 path, void* data, u64 bytes) {
	NotImplemented;
	return 0;
}

FileType
deshi__file_get_type_of_path(str8 caller_file, upt caller_line, str8 path) {
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

	NotImplemented;

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

