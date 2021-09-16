inline void Logging::
Init(u32 log_count, bool mirror, bool fileline){
    mirror_to_stdout = mirror;
    log_file_and_line = fileline;
    
    //delete all but last 'log_count' files in logs directory
    Assert(log_count, "log_count must be at least 1");
    array<File> log_files = get_directory_files(Assets::dirLogs().c_str());
    if(log_files.count >= log_count){
        //sort logs ascending based on last write time
        b32 swapped = false;
        forX(i,log_count){
            swapped = false;
            forX(j,log_files.count-1-i){
                if(log_files[j].time_last_write > log_files[j+1].time_last_write){
                    Swap(log_files[j], log_files[j+1]);
                    swapped = true;
                }
            }
            if(!swapped) break;
        }
        
        //delete logs
        forI((log_files.count-log_count)+1) delete_file(&log_files[i]);
    }
    
    //create log file named as current time
    time_t rawtime = time(0);
    int cursor = snprintf(log_path,LOG_PATH_SIZE,"%s/log_%llu.txt",Assets::dirLogs().c_str(),(u64)rawtime);
    file = fopen(log_path,"a");
    Assert(file, "logger failed to open file");
    
    //write date at top of file
    tm* timeinfo = localtime(&rawtime);
    strftime(log_buffer,LOG_BUFFER_SIZE,"%c",timeinfo);
    fprintf(file,"%s\n\n",log_buffer);
#if DESHI_SLOW
    //write immediately when debugging so that a log() right before Assert() still writes
    setvbuf(file,0,_IONBF,0);
#endif //DESHI_SLOW
}

//TODO maybe flush every X seconds/frames instead of every update?
inline void Logging::
Update(){
    Assert(fflush(file) == 0, "logger failed to flush file");
}

inline void Logging::
Cleanup(){
    fclose(file);
}
