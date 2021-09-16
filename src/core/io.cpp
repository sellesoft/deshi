#if   DESHI_WINDOWS
//void Win32LogLastError(){}
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
    pattern += (pattern[pattern.size-1] != '/') ? "/*" : "*";
    WIN32_FIND_DATAA data; HANDLE next;
    ULARGE_INTEGER size;   ULARGE_INTEGER time;
    
    next = FindFirstFileA(pattern.str, &data);
    if(next == INVALID_HANDLE_VALUE || !(data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)){
        logaE("io-win32","'$' is not a valid directory",pattern);
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
        
        upt path_len = pattern.size-1;
        upt name_len = strlen(data.cFileName);
        file.path_length = path_len+name_len;
        file.dir_length  = pattern.size-2;
        file.name_length = name_len;
        Assert(file.path_length < MAX_FILEPATH_SIZE);
        memcpy(file.path, pattern.str, pattern.size-1);
        memcpy(file.path+path_len, data.cFileName, name_len);
        
        result.add(file);
        if(FindNextFileA(next, &data) == 0) break;
    }
    DWORD error = GetLastError(); 
    if(error != ERROR_NO_MORE_FILES){ //TODO(delle) error messages
        logfE("io-win32","FindNextFileA or FindFirstFileA failed: %#x",(u32)error);
    }
    FindClose(next);
#elif DESHI_LINUX //DESHI_WINDOWS
    
#elif DESHI_MAC   //DESHI_LINUX
    
#endif            //DESHI_MAC
    return result;
}

global_ void 
delete_file(const char* filepath){
#if   DESHI_WINDOWS
    
#elif DESHI_LINUX //DESHI_WINDOWS
    
#elif DESHI_MAC   //DESHI_LINUX
    
#endif            //DESHI_MAC
}
