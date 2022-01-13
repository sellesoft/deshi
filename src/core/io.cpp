local b32   io_crash_on_error = false;
local Arena file_data_arena;


#if   DESHI_WINDOWS
void Win32LogLastError(const char* func_name){
	LPVOID msg_buffer;
	DWORD error = GetLastError();
	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM|FORMAT_MESSAGE_IGNORE_INSERTS|FORMAT_MESSAGE_MAX_WIDTH_MASK, 
				  0, error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&msg_buffer, 0, 0);
	LogfE("io-win32","%s failed with error %d: %s", func_name,(u32)error,(const char*)msg_buffer);
	LocalFree(msg_buffer);
	if(io_crash_on_error) ExitProcess(error);
}
#elif DESHI_LINUX //DESHI_WINDOWS
#error "not implemented yet for linux"
#elif DESHI_MAC   //DESHI_LINUX
#error "not implemented yet for mac"
#endif            //DESHI_MAC

//TODO binary file loading if needed
//initializes a FileReader for a given File
FileReader init_reader(const File& file) {
	FileReader fr;
	if (!file.handle ) { LogE("IO", "attempted to initalize a FileReader on a file that's not initialized"); fr.failed = true; return fr; }
	if (!HasFlag(file.flags, FileAccess_Read)) { LogE("IO", "attempted to initialize a FileReader on a file that doesn't have read access"); fr.failed = true; return fr; }
	fr.raw.str = (char*)memalloc(file.bytes_size); //TODO eventually arena file data allocations
	fr.raw.count = file.bytes_size;

#if DESHI_WINDOWS /////////////////////////////////////////////////////////////////// Windows

	u32 bytes_read = 0;
	if (!ReadFile(file.handle, fr.raw.str, file.bytes_size, (LPDWORD)&bytes_read, 0)) {
		Win32LogLastError("ReadFile");
		fr.failed = true; return fr;
	}
	if (bytes_read != file.bytes_size) LogW("IO-WIN32", "ReadFile failed to read the entire file '", get_file_name((File)file), "' \nfile's size is ", file.bytes_size, " but ", bytes_read, " were read");

#elif DESHI_LINUX /////////////////////////////////////////////////////////////////// Linux
#error "File not implemented for linux platforms"
#elif DESHI_MAC ///////////////////////////////////////////////////////////////////// Mac
#error "File not implemented for macOS"
#endif

	//gather lines
	//TODO maybe make a way to disable this
	//maybe make a function gather_lines or something and only cache lines when thats called
	char* start = fr.raw.str, end = 0;
	cstring raw = fr.raw;
	forI(bytes_read+1) {
		if (raw[0] == '\n' || raw[0] == '\0') {
			fr.lines.add({ start, u32(raw.str - start) });
			start = raw.str + 1;
		}
		advance(&raw);
	}
	if (!fr.lines.count) fr.lines.add(fr.raw);

	fr.file = &file;
	fr.read = { fr.raw.str, 0 };
	return fr;
}

FileReader init_reader(char* data, u32 datasize) {
	FileReader fr;
	fr.raw.str = data;
	fr.raw.count = datasize;
	fr.read.str = fr.raw.str;

	char* start = fr.raw.str, end = 0;
	cstring raw = fr.raw;
	forI(datasize + 1) {
		if (raw[0] == '\n' || raw[0] == '\0') {
			fr.lines.add({ start, u32(raw.str - start) });
			start = raw.str + 1;
		}
		advance(&raw);
	}
	if (!fr.lines.count) fr.lines.add(fr.raw);

	return fr;
}

b32 next_char(FileReader& reader) {
	reader.read.str++;
	reader.read.count = 1;
	if (reader.read.str > reader.raw.str + reader.raw.count) return false;
}

b32 next_line(FileReader& reader) {
	if (reader.line_number >= reader.lines.count) return false;
	reader.read = reader.lines[reader.line_number];
	reader.line_number++;
	return true;
	
	
	//old code for reading lines incase we dont want to cache it, or if we want to be able to disable caching
	//char*& readstr = reader.read.str;
	//upt& readcount = reader.read.count;
	//char*   rawstr = reader.raw.str;
	//upt   rawcount = reader.raw.count;

	////ensure that for the first line read points to starts at the beginnng of the file 
	////and we dont skip the first line
	//if (!reader.line_number) {
	//	readstr = rawstr;
	//	readcount = rawcount;
	//	u32 idx = find_first_char(reader.read, '\n');
	//	if (idx != npos) {
	//		readcount = idx;
	//		if (readstr[readcount - 1] == '\r') readcount--;
	//	}
	//}
	//else {
	//	readstr += readcount;
	//	if (*readstr == '\r') readstr += 2;
	//	else readstr++;
	//	if (readstr >= rawstr + rawcount) return false;
	//	u32 index = find_first_char(readstr, rawcount - (readstr - rawstr), '\n');
	//	if (index == npos) readcount = rawcount - (readstr - rawstr);
	//	else readcount = index;
	//	if (reader.read[readcount - 1] == '\r') readcount--;
	//}
	//reader.line_number++;
	//return true;
}

//old chunking code for if we want a non cached version
//it works, but only if you dont use any other instruction before it
//becuase of me doing readstr += readcount; to pass the previous chunk
#if 0
b32 next_chunk(FileReader& reader, char delimiter, b32 include_delimiter) {
	char*& readstr = reader.read.str;
	upt& readcount = reader.read.count;
	char*   rawstr = reader.raw.str;
	upt   rawcount = reader.raw.count;

	readstr += readcount;
	if (readstr >= rawstr + rawcount) return false;
	if (*readstr == delimiter) readstr++;
	u32 index = find_first_char(readstr, rawcount - (readstr - rawstr), delimiter);
	if (index == npos) readcount = rawcount - (readstr - rawstr);
	else readcount = index + (include_delimiter ? 1 : 0);
	return true;
}

b32 next_chunk(FileReader& reader, char chunk_begin, char chunk_end, b32 include_delimiters) {
	char*& readstr = reader.read.str;
	upt& readcount = reader.read.count;
	char*   rawstr = reader.raw.str;
	upt   rawcount = reader.raw.count;
	
	if (readstr >= rawstr + rawcount) return false;
	if (*readstr == chunk_begin) readstr++;
	u32 index = find_first_char(readstr, chunk_begin);
	if (index == npos) return false; //TODO handle this better
	readstr += index + (include_delimiters ? 0 : 1);
	index = find_first_char(readstr, chunk_end);
	if (index == npos) readcount = rawcount - (readstr - rawstr);
	else readcount = index + (include_delimiters ? 1 : 0);

	return true;
}
#endif

//TODO maybe make key a cstring instead 
//you may pass 0 for inbetween char if there is none
b32 next_value_from_key(FileReader& reader, const char* key, char inbetween_char, char value_delimiter) {
	char*& readstr = reader.read.str;
	upt& readcount = reader.read.count;
	char*   rawstr = reader.raw.str;
	upt   rawcount = reader.raw.count;

	if (readstr >= rawstr + rawcount) return false;
	u32 index = find_first_string(readstr, rawcount - (readstr - rawstr), key, strlen(key));
	if (index == npos) return false;
	readstr += index + strlen(key);
	if (inbetween_char) {
		//if an inbetween char is passed we pass it 
		index = find_first_char(readstr, rawcount - (readstr - rawstr), inbetween_char);
		if (index == npos) { LogE("IO", "next_value_from_key could not find an inbetween character for a key and value pair on line ", reader.line_number, " of file ", get_file_name(*reader.file)); return false; }
		readstr += index +1;
	}
	//eat spaces
	index = find_first_char_not(readstr, ' ');
	readstr += index;
	//find end of value
	index = find_first_char(readstr, rawcount - (readstr - rawstr), value_delimiter);
	if (index == npos) readcount = rawcount - (readstr - rawstr);
	else readcount = index;

	return true;
}

void chunk_file(FileReader& reader, char delimiter, b32 stop_on_newline){
	reader.chunks.clear();
	cstring raw = reader.raw;
	char* start = raw.str;
	forI(reader.file->bytes_size+1) {
		if (raw[0] == delimiter || raw[0] == 0 || (stop_on_newline ? raw[0] == '\n' : 0)) {
			reader.chunks.add({ start, u32(raw.str - start) });
			start = raw.str+1;
		}
		advance(&raw);
	}
}

void chunk_file(FileReader& reader, char begin_delimiter, char end_delimiter, b32 stop_on_newline){
	reader.chunks.clear();
	cstring raw = reader.raw;
	char* start = 0;
	forI(reader.file->bytes_size) {
		if (raw[0] == begin_delimiter && !start) {
			start = raw.str + 1;
		}
		else if ((raw[0] == end_delimiter || (stop_on_newline ? raw[0] == '\n' : 0)) && start && start != raw.str + 1) {
			reader.chunks.add({ start, u32(raw.str - start) });
			start = 0;
		}
		else {
			advance(&raw);
		}
	}
}

void chunk_line(FileReader& reader, u32 line, char delimiter){
	Assert(line < reader.lines.count);
	reader.chunks.clear();
	cstring raw = reader.lines[line];
	char* start = raw.str;
	forI(reader.lines[line].count+1) {
		if (raw[0] == delimiter || i == reader.lines[line].count) {
			reader.chunks.add({ start, u32(raw.str - start) });
			start = raw.str + 1;
		}
		advance(&raw);
	}
}

void chunk_line(FileReader& reader, u32 line, char begin_delimiter, char end_delimiter){
	Assert(line < reader.lines.count);
	reader.chunks.clear();
	cstring raw = reader.lines[line];
	char* start = 0;
	forI(reader.lines[line].count + 1) {
		if (raw[0] == begin_delimiter && !start) {
			start = raw.str + 1;
		}
		else if (raw[0] == end_delimiter && start && start != raw.str + 1) {
			reader.chunks.add({ start, u32(raw.str - start) });
			start = 0;
		}
		else {
			advance(&raw);
		}
	}
}



void goto_line(FileReader& reader, u32 linenum) {
	//TODO goto_line
	//TODO maybe count lines on file load
}

void goto_char(FileReader& reader, u32 charnum) {
	Assert(charnum < reader.file->bytes_size);
	reader.read.str = reader.raw.str + charnum;
	reader.read.count = 1;
}

void reset_reader(FileReader& reader) {
	reader.read.str = reader.raw.str;
	reader.read.count = 0;
}


//opens a file if it already exists or creates a new one if it doesnt
//this does not load any data, you must use FileReader to do that!
File open_file(const char* path, FileAccessFlags flags) {
	Assert(flags, "attempt to open_file without specifing access flags");
	File file;
	file.flags = flags;

#if DESHI_WINDOWS
	DWORD access = (HasFlag(flags, FileAccess_Write) ? GENERIC_WRITE : 0) | (HasFlag(flags, FileAccess_Read) ? GENERIC_READ : 0);
	file.handle = CreateFileA(path, access, FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE, 0, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
	if (GetLastError() != ERROR_FILE_EXISTS && GetLastError()) Win32LogLastError("CreateFileA");
	
	//read data of opened file
	BY_HANDLE_FILE_INFORMATION data;
	ULARGE_INTEGER time;
	ULARGE_INTEGER size;
	
	if (!GetFileInformationByHandle(file.handle, &data)) Win32LogLastError("GetFileInformationByHandle");

	time.LowPart = data.ftCreationTime.dwLowDateTime; time.HighPart = data.ftCreationTime.dwHighDateTime;
	file.time_creation = WindowsTimeToUnixTime(time.QuadPart);
	time.LowPart = data.ftLastAccessTime.dwLowDateTime; time.HighPart = data.ftLastAccessTime.dwHighDateTime;
	file.time_last_access = WindowsTimeToUnixTime(time.QuadPart);
	time.LowPart = data.ftLastWriteTime.dwLowDateTime; time.HighPart = data.ftLastWriteTime.dwHighDateTime;
	file.time_last_write = WindowsTimeToUnixTime(time.QuadPart);
	size.LowPart = data.nFileSizeLow; size.HighPart = data.nFileSizeHigh;
	file.bytes_size = size.QuadPart;
	file.is_directory = (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);

	u32 pathlength = GetFinalPathNameByHandleA(file.handle, file.path, MAX_FILEPATH_SIZE, FILE_NAME_NORMALIZED);
	if (pathlength > MAX_FILEPATH_SIZE) LogW("IO", "file path for '", path, "' has a length greater than MAX_FILEPATH_SIZE\npath length was ", pathlength);

	//file.path_length = Min(pathlength, u32(MAX_FILEPATH_SIZE));

	string pathstr(file.path);
	//remove \\?\ prefix, however this may cause issues in the future with network paths, so TODO add checking for that
	//see https://stackoverflow.com/questions/31439011/getfinalpathnamebyhandle-result-without-prepended
	pathstr = pathstr.substr(4); 
	pathstr.replace('\\', "/");
	strcpy(file.path, pathstr.str);
	file.path_length = pathstr.count;
	pathstr = pathstr.substr(pathstr.findLastChar('/')+1);
	strcpy(file.name, pathstr.str);
	file.name_length = pathstr.count;
	file.short_length = pathstr.findFirstChar('.');
	file.ext_length = pathstr.count - file.short_length - 1;

#elif DESHI_LINUX
#error "File not implemented for linux platforms"
#elif DESHI_MAC
#error "File not implemented for macOS"
#endif

	return file;
}

//TODO(delle) search filters
array<File>
get_directory_files(const char* directory){
	array<File> result(deshi_temp_allocator);
	if(directory == 0) return result;
#if   DESHI_WINDOWS
	string pattern(directory); //TODO add allocator to string
	pattern += (pattern[pattern.count-1] != '/') ? "/*" : "*";
	WIN32_FIND_DATAA data; HANDLE next;
	ULARGE_INTEGER size;   ULARGE_INTEGER time;
	
	next = FindFirstFileA(pattern.str, &data);
	if(next == INVALID_HANDLE_VALUE || !(data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)){
		Win32LogLastError("FindFirstFileA");
		LogaE("io-win32","'$' is not a valid directory.",directory);
		FindClose(next);
		return result;
	}
	while(next != INVALID_HANDLE_VALUE){
		if((strcmp(data.cFileName,".") == 0) || (strcmp(data.cFileName,"..") == 0)){
			if(FindNextFileA(next, &data) == 0) break;
			continue;
		}
		
		File file;
		file.handle = next;
		time.LowPart = data.ftCreationTime.dwLowDateTime; time.HighPart = data.ftCreationTime.dwHighDateTime;
		file.time_creation = WindowsTimeToUnixTime(time.QuadPart);
		time.LowPart = data.ftLastAccessTime.dwLowDateTime; time.HighPart = data.ftLastAccessTime.dwHighDateTime;
		file.time_last_access = WindowsTimeToUnixTime(time.QuadPart);
		time.LowPart = data.ftLastWriteTime.dwLowDateTime; time.HighPart = data.ftLastWriteTime.dwHighDateTime;
		file.time_last_write = WindowsTimeToUnixTime(time.QuadPart);
		size.LowPart = data.nFileSizeLow; size.HighPart = data.nFileSizeHigh;
		file.bytes_size = size.QuadPart;
		file.is_directory = (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
		
		u32 path_len  = pattern.count-1;
		u32 name_len  = strlen(data.cFileName);
		u32 short_len = name_len;
		while(short_len && data.cFileName[short_len--] != '.');
		file.path_length  = path_len+name_len;
		file.name_length  = name_len;
		file.short_length = short_len+1;
		file.ext_length   = name_len-short_len-1;
		Assert(file.path_length < MAX_FILEPATH_SIZE);
		memcpy(file.path, pattern.str, pattern.count-1);
		memcpy(file.path+path_len, data.cFileName, name_len);
		memcpy(file.name, data.cFileName, name_len);
		
		result.add(file);
		if(FindNextFileA(next, &data) == 0) break;
	}
	DWORD error = GetLastError(); 
	if(error != ERROR_NO_MORE_FILES){
		Win32LogLastError("FindNextFileA");
	}
	FindClose(next);
#elif DESHI_LINUX //DESHI_WINDOWS
	
#elif DESHI_MAC   //DESHI_LINUX
	
#endif            //DESHI_MAC
	return result;
}

//TODO(delle) add safety checks so deletion only happens within the data folder
void
delete_file(const char* filepath){
	if(filepath == 0) return;
#if   DESHI_WINDOWS
	WIN32_FIND_DATAA data;
	HANDLE next = FindFirstFileA(filepath, &data);
	if(next == INVALID_HANDLE_VALUE){
		Win32LogLastError("FindFirstFileA");
		return;
	}
	
	//if directory, recursively delete all files and directories
	if(data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY){
		array<File> dir_files = get_directory_files(filepath);
		forE(dir_files) delete_file(it->path);
		BOOL success = RemoveDirectoryA(filepath);
		if(!success) Win32LogLastError("RemoveDirectoryA");
	}else{
		BOOL success = DeleteFileA(filepath);
		if(!success) Win32LogLastError("DeleteFile");
	}
	FindClose(next);
#elif DESHI_LINUX //DESHI_WINDOWS
	
#elif DESHI_MAC   //DESHI_LINUX
	
#endif            //DESHI_MAC
}

b32
file_exists(const char* filepath){
	if(filepath == 0) return false;
	WIN32_FIND_DATAA data; 
	HANDLE handle = FindFirstFileA(filepath, &data);
	if(handle != INVALID_HANDLE_VALUE){
		FindClose(handle);
		return true;
	}
	return false;
}

void
rename_file(const char* old_filepath, const char* new_filepath){
	BOOL success = MoveFileA(old_filepath, new_filepath);
	if(!success) Win32LogLastError("MoveFileA");
}