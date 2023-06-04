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
		// the root window of the X windowing system
		X11::Window root;
	}x11;
}linux;

//~////////////////////////////////////////////////////////////////////////////////////////////////
//// @helpers

int linux_error_handler(X11::Display* display, X11::XErrorEvent* event) {
	switch(event->error_code){
		case BadAccess: 			printf("A client attempts to grab a key/button combination already grabbed by another client.\n"
							                "A client attempts to free a colormap entry that it had not already allocated or to free an entry in a colormap that was created with all entries writable.\n"
							                "A client attempts to store into a read-only or unallocated colormap entry.\n"
							                "A client attempts to modify the access control list from other than the local (or otherwise authorized) host.\n"
							                "A client attempts to select an event type that another client has already selected."); break;
		case BadAlloc: 			    printf("The server fails to allocate the requested resource. Note that the explicit listing of BadAlloc errors in requests only covers allocation errors at a very coarse level and is not intended to (nor can it in practice hope to) cover all cases of a server running out of allocation space in the middle of service. The semantics when a server runs out of allocation space are left unspecified, but a server may generate a BadAlloc error on any request for this reason, and clients should be prepared to receive such errors and handle or discard them."); break;
		case BadAtom: 			    printf("A value for an atom argument does not name a defined atom."); break;
		case BadColor: 			    printf("A value for a colormap argument does not name a defined colormap."); break;
		case BadCursor: 			printf("A value for a cursor argument does not name a defined cursor."); break;
		case BadDrawable: 		    printf("A value for a drawable argument does not name a defined window or pixmap."); break;
		case BadFont: 			    printf("A value for a font argument does not name a defined font (or, in some cases, GContext)."); break;
		case BadGC: 				printf("A value for a GContext argument does not name a defined GContext ."); break;
		case BadIDChoice: 		    printf("The value chosen for a resource identifier either is not included in the range assigned to the client or is already in use. Under normal circumstances, this cannot occur and should be considered a server or Xlib error."); break;
		case BadImplementation: 	printf("The server does not implement some aspect of the request. A server that generates this error for a core request is deficient. As such, this error is not listed for any of the requests, but clients should be prepared to receive such errors and handle or discard them."); 
		case BadLength: 			printf("The length of a request is shorter or longer than that required to contain the arguments. This is an internal Xlib or server error.\n" 
							                "The length of a request exceeds the maximum length accepted by the server."); break;
		case BadMatch: 			    printf("In a graphics request, the root and depth of the graphics context does not match that of the drawable.\n"
							                "An InputOnly window is used as a drawable.\n"
							                "Some argument or pair of arguments has the correct type and range, but it fails to match in some other way required by the request.\n"
							                "An InputOnly window lacks this attribute."); break;
		case BadName: 			    printf("A font or color of the specified name does not exist."); break;
		case BadPixmap: 			printf("A value for a pixmap argument does not name a defined pixmap."); break;
		case BadRequest: 			printf("The major or minor opcode does not specify a valid request. This usually is an Xlib or server error."); break;
		case BadValue: 			    printf("Some numeric value falls outside of the range of values accepted by the request. Unless a specific range is specified for an argument, the full range defined by the argument's type is accepted. Any argument defined as a set of alternatives typically can generate this error (due to the encoding)."); break;
		case BadWindow: 			printf("A value for a window argument does not name a defined window. "); break;
	}
	return 1;
}

FORCE_INLINE KeyCode
linux_keysym_to_key(X11::KeySym k) {
	switch(k){
		case XK_a: return Key_A; case XK_b: return Key_B; case XK_c: return Key_C; case XK_d: return Key_D; case XK_e: return Key_E;
		case XK_f: return Key_F; case XK_g: return Key_G; case XK_h: return Key_H; case XK_i: return Key_I; case XK_j: return Key_J;
		case XK_k: return Key_K; case XK_l: return Key_L; case XK_m: return Key_M; case XK_n: return Key_N; case XK_o: return Key_O;
		case XK_p: return Key_P; case XK_q: return Key_Q; case XK_r: return Key_R; case XK_s: return Key_S; case XK_t: return Key_T;
		case XK_u: return Key_U; case XK_v: return Key_V; case XK_w: return Key_W; case XK_x: return Key_X; case XK_y: return Key_Y;
		case XK_z: return Key_Z;
		case XK_0: return Key_0; case XK_1: return Key_1; case XK_2: return Key_2; case XK_3: return Key_3; case XK_4: return Key_4;
		case XK_5: return Key_5; case XK_6: return Key_6; case XK_7: return Key_7; case XK_8: return Key_8; case XK_9: return Key_9;
		case XK_F1: return Key_F1; case XK_F2:  return Key_F2;  case XK_F3:  return Key_F3;  case XK_F4:  return Key_F4;
		case XK_F5: return Key_F5; case XK_F6:  return Key_F6;  case XK_F7:  return Key_F7;  case XK_F8:  return Key_F8;
		case XK_F9: return Key_F9; case XK_F10: return Key_F10; case XK_F11: return Key_F11; case XK_F12: return Key_F12;
		case XK_Up: return Key_UP; case XK_Down: return Key_DOWN; case XK_Left: return Key_LEFT; case XK_Right: return Key_RIGHT;
		case XK_Escape:       return Key_ESCAPE;     case XK_asciitilde: return Key_TILDE;        case XK_Tab:         return Key_TAB;
		case XK_Caps_Lock:    return Key_CAPSLOCK;   case XK_Shift_L:    return Key_LSHIFT;       case XK_Control_L:   return Key_LCTRL;
		case XK_Alt_L:        return Key_LALT;       case XK_BackSpace:  return Key_BACKSPACE;    case XK_Return:      return Key_ENTER;
		case XK_Shift_R:      return Key_RSHIFT;     case XK_Control_R:  return Key_RCTRL;        case XK_Alt_R:       return Key_RALT;
		case XK_minus:        return Key_MINUS;      case XK_plus:       return Key_EQUALS;       case XK_bracketleft: return Key_LBRACKET;
		case XK_bracketright: return Key_RBRACKET;   case XK_slash:      return Key_FORWARDSLASH; case XK_semicolon:   return Key_SEMICOLON;
		case XK_apostrophe:   return Key_APOSTROPHE; case XK_comma:      return Key_COMMA;        case XK_period:      return Key_PERIOD;
		case XK_backslash:    return Key_BACKSLASH;  case XK_space:      return Key_SPACE;        case XK_Insert:      return Key_INSERT;
		case XK_Delete:       return Key_DELETE;     case XK_Home:       return Key_HOME;         case XK_End:         return Key_END;
		case XK_Prior:        return Key_PAGEUP;     case XK_Page_Down:  return Key_PAGEDOWN;     case XK_Pause:       return Key_PAUSEBREAK;
		case XK_Scroll_Lock:  return Key_SCROLLLOCK; case XK_Meta_L:     return Key_LMETA;        case XK_Meta_R:      return Key_RMETA;
		case XK_Print:  return Key_PRINTSCREEN;
		case XK_KP_0: return Key_NP0; case XK_KP_1: return Key_NP1; case XK_KP_2: return Key_NP2; case XK_KP_3: return Key_NP3;
		case XK_KP_4: return Key_NP4; case XK_KP_5: return Key_NP5; case XK_KP_6: return Key_NP6; case XK_KP_7: return Key_NP7;
		case XK_KP_8: return Key_NP8; case XK_KP_9: return Key_NP9;
		case XK_KP_Multiply: return Key_NPMULTIPLY; case XK_KP_Divide:  return Key_NPDIVIDE; case XK_KP_Add:     return Key_NPPLUS;
		case XK_KP_Subtract: return Key_NPMINUS;    case XK_KP_Decimal: return Key_NPPERIOD; case XK_Num_Lock: return Key_NUMLOCK;
		default: return Key_NONE;
	}
}



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
			"at ", caller_file, "(", caller_line, "): file_init() was given a path to a file that doesn't exist: ", path
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
	File f = deshi__file_info(caller_file, caller_line, path, result);
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
	return (current.tv_sec*1000000000+current.tv_nsec)/1e6;
}

f64 peek_stopwatch(Stopwatch watch) {
	timespec current;
	if(clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &current) == -1){
		switch(errno) {
			case EFAULT: LogE("stopwatch", "in peek_stopwatch(), clock_gettime() was passed a bad address. ERRNO 14: EFAULT"); return 0;
			case EINVAL: LogE("stopwatch", "in peek_stopwatch(), clock_gettime() was given a clk_id that is not supported on this system. ERRNO 22: EINVAL"); return 0;
		}
	}
	return (current.tv_sec*1000000000+current.tv_nsec)/1e6 - watch;
}

//~////////////////////////////////////////////////////////////////////////////////////////////////
//// @platform


void
platform_init() {
	DeshiStageInitStart(DS_PLATFORM, DS_MEMORY, "Attempted to initialize Platform module before initializing the Memory module");

	DeshTime->stopwatch = start_stopwatch();

	array_init(file_shared.files, 16, deshi_allocator);
	file_create(STR8("data/"));
	file_create(STR8("data/cfg/"));
	file_create(STR8("data/temp/"));

	X11::XSetErrorHandler(&linux_error_handler);
	
	// initialize display and screen
	X11::Display* display = linux.x11.display = X11::XOpenDisplay(0);
	s32 screen = linux.x11.screen = X11::XDefaultScreen(display);
	X11::Window root = linux.x11.root = X11::XRootWindow(display, screen);

	X11::XWindowAttributes wa;
	X11::XGetWindowAttributes(display, root, &wa);


	// forI(n_monitors) {
	// 	X11::XRRMonitorInfo monitor = monitors[i];
	// 	printf("Monitor %u: \n", i);
	// 	printf("  size: %u x %u\n", monitor.width, monitor.height);
	// 	printf("   pos: (%u, %u)\n", monitor.x, monitor.y);
	// }

	// u32 n_screens = X11::XScreenCount(linux.x11.display);
	// Log("", "found ", n_screens, " screens for display 0.");
	// forI(n_screens) {
	// 	X11::Screen* screen = X11::XScreenOfDisplay(linux.x11.display, i);
	// 	u32 w = X11::XWidthOfScreen(screen);
	// 	u32 h = X11::XHeightOfScreen(screen);
	// 	Log("", "screen ", i, ": ");
	// 	Log("", "	width: ", w);
	// 	Log("", "  height: ", h);
	// }

	int min,max;
	X11::XDisplayKeycodes(linux.x11.display, &min,&max);
	//Log("", "min: ", min, ", max: ", max);

	DeshiStageInitEnd(DS_PLATFORM);
}

b32 
platform_update() {
	Stopwatch update_stopwatch = start_stopwatch();

	DeshTime->prevDeltaTime = DeshTime->deltaTime;
	DeshTime->deltaTime     = reset_stopwatch(&DeshTime->stopwatch);
	DeshTime->totalTime    += DeshTime->deltaTime;
	DeshTime->frame        += 1;
	DeshTime->timeTime = reset_stopwatch(&update_stopwatch);
	
	X11::XEvent event;
	X11::KeySym key;
	char text[255];

	// get the current amount of events in queue
	u64 events_to_handle = X11::XEventsQueued(linux.x11.display, QueuedAlready);

	forI(events_to_handle){
		X11::XNextEvent(linux.x11.display, &event);
		switch(event.type) {
			case Expose: {
				
			}break;

			case MotionNotify: {
				X11:: XMotionEvent motion = event.xmotion;
				//Log("", "mouse -> root:(", motion.x_root, ", ", motion.y_root, "), win:(", motion.x, ", ", motion.y, ")");
			}break;

			case KeyPress: {
				X11::KeySym ks = XLookupKeysym(&event.xkey, 0);
				KeyCode key = linux_keysym_to_key(ks);
				if(key != Key_NONE) {
					DeshInput->realKeyState[key] = 1;
				}
			}break;

			case KeyRelease: {
				X11::KeySym ks = XLookupKeysym(&event.xkey,  0);
				KeyCode key = linux_keysym_to_key(ks);
				if(key != Key_NONE) {
					DeshInput->realKeyState[key] = 0;
				}
			}break;
		}
	}

	return 1;
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
	void* handle = dlopen((char*)module_path.str, RTLD_NOW);
	return handle;
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
	
	// we'll create the window in the monitor that the user's cursor is in 
	X11::Window root,child;
	s32 root_x, root_y;
	s32 win_x, win_y;
	u32 mask;
	X11::XQueryPointer(linux.x11.display, linux.x11.root, &root, &child, &root_x, &root_y, &win_x, &win_y, &mask);

	s32 n_monitors;
	X11::XRRMonitorInfo* monitors = X11::XRRGetMonitors(linux.x11.display, linux.x11.root, 1, &n_monitors);

	X11::XRRMonitorInfo selection;
	forI(n_monitors){
		X11::XRRMonitorInfo monitor = monitors[i];
		if(Math::PointInRectangle(Vec2(root_x,root_y), Vec2(monitor.x,monitor.y), Vec2(monitor.width,monitor.height))) {
			selection = monitor;
			break;
		}
	}

	if(width == -1)  width = selection.width / 2;
	if(height == -1) height = selection.height / 2;
	if(x == -1)      x = selection.x + width / 2;
	if(y == -1)      y = selection.y + height / 2;

	u32 black = X11::XBlackPixel(linux.x11.display, linux.x11.screen);
	u32 white = X11::XWhitePixel(linux.x11.display, linux.x11.screen);

	linux.x11.root = X11::XRootWindow(linux.x11.display, linux.x11.screen);

	Window* window = (Window*)memalloc(sizeof(Window));
	X11::Window handle = X11::XCreateSimpleWindow(linux.x11.display, linux.x11.root, x, y, width, height, 0, white, black);
	window->handle = (void*)handle;
	if(!DeshWindow) DeshWindow = window;

	X11::XSetStandardProperties(linux.x11.display, (X11::Window)window->handle, (const char*)title.str, 0,0,0,0,0);

	X11::XSelectInput(linux.x11.display, (X11::Window)window->handle, 
		  ExposureMask      // caused when an invisible window becomes visible, or when a hidden part of a window becomes visible
		| ButtonPressMask   // mouse button pressed
		| ButtonReleaseMask // mouse button released
		| KeyPressMask      
		| KeyReleaseMask
		| EnterWindowMask
		| LeaveWindowMask
		| PointerMotionMask // mouse movement event
	);
	window->context = X11::XCreateGC(linux.x11.display, (X11::Window)window->handle, 0, 0);
	X11::XSetBackground(linux.x11.display, (X11::GC)window->context, black);
	X11::XSetForeground(linux.x11.display, (X11::GC)window->context, white);

	X11::XClearWindow(linux.x11.display, (X11::Window)window->handle);
	X11::XMapRaised(linux.x11.display, (X11::Window)window->handle);

	u32 bw,d;
	X11::Window groot,gchild;
	X11::XGetGeometry(linux.x11.display, handle, &groot, &window->x, &window->y, (u32*)&window->width, (u32*)&window->height, &bw, &d);

	window_windows.add(window);

	DeshiStageInitEnd(DS_WINDOW);
	return window;

} // window_create


void window_close(Window* window){
	NotImplemented;
}

void
window_swap_buffers(Window* window){
	X11::XFlush(linux.x11.display);
	//X11::XSync(linux.x11.display, 0);
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

