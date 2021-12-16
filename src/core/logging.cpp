namespace Logging{
#define LOG_BUFFER_SIZE 1024
#define LOG_PATH_SIZE 1024
	local FILE* file = 0;
	local char  log_buffer[LOG_BUFFER_SIZE] = {};
	local char  log_path[LOG_PATH_SIZE] = {};
	local u64   last_message_len = 0;
	local bool  mirror_to_stdout = true;
	local bool  mirror_to_console = false;
	
	void LogF_(const char* filepath, upt line_number, const char* tag, const char* fmt, ...){
		int cursor = (tag && *tag != 0) ? snprintf(log_buffer, LOG_BUFFER_SIZE, "[%s] ", string::toUpper(tag).str) : 0;
		va_list args; va_start(args, fmt);
		cursor += vsnprintf(log_buffer+cursor, LOG_BUFFER_SIZE-cursor, fmt, args);
		va_end(args);
		if(mirror_to_stdout) puts(log_buffer);
		log_buffer[cursor] = '\n'; log_buffer[cursor+1] = '\0';
		cursor += 1;
		
		fputs(log_buffer, file);
		last_message_len = cursor;
		if(mirror_to_console) Console2::AddLog(to_string(LastMessage()));
	}
	
	inline void LogInternal(string& str){
		if(mirror_to_stdout) puts(str.str);
		str += "\n";
		fputs(str.str, file);
		memcpy(log_buffer, str.str, str.count);
		last_message_len = str.count;
		if(mirror_to_console) Console2::AddLog(str);
	}
	
	inline cstring LastMessage(){
		return cstring{log_buffer,last_message_len};
	}
	
	void Init(u32 log_count, bool mirror){
		TIMER_START(t_s);
		
		mirror_to_stdout = mirror;
		
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
		
		Log("deshi","Finished logging initialization in ",TIMER_END(t_s),"ms");
	}
	
	//TODO maybe flush every X seconds/frames instead of every update?
	void Update(){
		Assert(!fflush(file), "logger failed to flush file");
	}
	
	void Cleanup(){
		fclose(file);
	}
}; //namespace Logging