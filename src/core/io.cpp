local b32 io_crash_on_error = false;

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

//TODO(delle) search filter
global_ array<File>
get_directory_files(const char* directory){
    array<File> result;
#if   DESHI_WINDOWS
    string pattern = directory;
    pattern += (pattern[pattern.count-1] != '/') ? "/*" : "*";
    WIN32_FIND_DATAA data; HANDLE next;
    ULARGE_INTEGER size;   ULARGE_INTEGER time;
    
    next = FindFirstFileA(pattern.str, &data);
    if(next == INVALID_HANDLE_VALUE || !(data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)){
        Win32LogLastError("FindFirstFileA");
        LogaE("io-win32","'$' is not a valid directory.",directory);
        return result;
    }
    while(next != INVALID_HANDLE_VALUE){
        if((strcmp(data.cFileName,".") == 0) || (strcmp(data.cFileName,"..") == 0)){
            if(FindNextFileA(next, &data) == 0) break;
            continue;
        }
        
        File file;
        time.LowPart = data.ftCreationTime.dwLowDateTime; time.HighPart = data.ftCreationTime.dwHighDateTime;
        file.time_creation = WindowsTimeToUnixTime(time.QuadPart);
        time.LowPart = data.ftLastAccessTime.dwLowDateTime; time.HighPart = data.ftLastAccessTime.dwHighDateTime;
        file.time_last_access = WindowsTimeToUnixTime(time.QuadPart);
        time.LowPart = data.ftLastWriteTime.dwLowDateTime; time.HighPart = data.ftLastWriteTime.dwHighDateTime;
        file.time_last_write = WindowsTimeToUnixTime(time.QuadPart);
        size.LowPart = data.nFileSizeLow; size.HighPart = data.nFileSizeHigh;
        file.bytes_size = size.QuadPart;
        file.is_directory = (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
        
        u32 path_len = pattern.count-1;
        u32 name_len = strlen(data.cFileName);
        file.path_length = path_len+name_len;
        file.name_length = name_len;
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
global_ void 
delete_file(const char* filepath){
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
