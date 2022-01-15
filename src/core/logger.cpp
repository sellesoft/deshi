namespace Logger{
#define LOG_BUFFER_SIZE 4096
#define LOG_PATH_SIZE 1024
	local FILE* file = 0;
	local char  log_buffer[LOG_BUFFER_SIZE] = {};
	local char  log_path[LOG_PATH_SIZE] = {};
	local u64   last_message_len = 0;
	local b32   mirror_to_stdout = false;
	local b32   mirror_to_console = false;
	local b32   is_logging = false;
	local u32   indent_level = 0;
	local u32   indent_spaces = 2;
	
	void ConsoleMirror(const string& str, u32 file_char_offset) {
		if (str[0] == '[') {
			string modified = "{{";
			u32 rb = str.findFirstChar(']');
			string tag = str.substr(1, rb - 1);
			
			if (rb != npos) {
				if (tag.endsWith("-ERROR")) {
					modified += "e,";
				}
				else if (tag.endsWith("-WARNING")) {
					modified += "w,";
				}
				else if (tag.endsWith("-SUCCESS")) {
					modified += "s,";
				}
				modified += "t=" + tag + "}" + str.substr(rb + 2) + "{}}";
				DeshConsole->LoggerMirror(modified, file_char_offset + tag.count + 3);
				return;
			}
		}
		DeshConsole->LoggerMirror(str, file_char_offset);
	}
	
	void LogF_(const char* filepath, upt line_number, const char* tag, const char* fmt, ...){
		if(!is_logging) return;
		int cursor = (tag && *tag != 0) ? snprintf(log_buffer, LOG_BUFFER_SIZE, "[%s] ", string::toUpper(tag).str) : 0; //tag
		cursor += snprintf(log_buffer+cursor, LOG_BUFFER_SIZE-cursor, "%*s", indent_level*indent_spaces, ""); //indentation
		
		va_list args; //fmt
		va_start(args, fmt);{
		cursor += vsnprintf(log_buffer+cursor, LOG_BUFFER_SIZE-cursor, fmt, args);
		}va_end(args);
		
		if(mirror_to_stdout) puts(log_buffer);
		log_buffer[cursor] = '\n'; log_buffer[cursor+1] = '\0';
		cursor += 1;
		u32 file_char_offset = ftell(file);
		fputs(log_buffer, file);
		last_message_len = cursor;
		if(mirror_to_console && DeshiModuleLoaded(DS_CONSOLE)) ConsoleMirror(toStr(LastMessage()), file_char_offset);
	}
	
	inline void LogInternal(const string& tag, const string& msg){
		if(!is_logging) return;
		
		string str = tag;
		forI(indent_level*indent_spaces) str += " ";
		str += msg;
		if(str.count >= LOG_BUFFER_SIZE) LogW("logger","Attempted to log a message more than 4096 characters long.");
		if(mirror_to_stdout) puts(str.str);
		str += "\n";
		
		u32 file_char_offset = ftell(file);
		fputs(str.str, file);
		memcpy(log_buffer, str.str, ClampMax(str.count, LOG_BUFFER_SIZE));
		last_message_len = ClampMax(str.count, LOG_BUFFER_SIZE);
		if(mirror_to_console && DeshiModuleLoaded(DS_CONSOLE)) ConsoleMirror(str, file_char_offset);
	}
	
	//just a special function called by Console to prevent feedback between 
	//console and logger and to prevent logger from appending a newline
	void LogFromConsole(const string& str) {
		if (!is_logging) return;
		fputs(str.str, file);
		if (mirror_to_stdout) {
			fputs(str.str, stdout);
		}
		memcpy(log_buffer, str.str, str.count);
		last_message_len = str.count;
	}
	
	void PushIndent(u32 count){
		indent_level += count;
	}
	
	void PopIndent(u32 count){
		u32 temp = indent_level - count;
		if(temp > indent_level){
			indent_level = 0;
		}else{
			indent_level = temp;
		}
	}
	
	 FILE* GetFilePtr() {
		return file;
	}
	
	cstring LastMessage(){
		return cstring{log_buffer,last_message_len};
	}
	
	void SetIsLogging(b32 yep) {
		is_logging = yep;
	}
	
	void SetMirrorToConsole(b32 mirrorToConsole){
		mirror_to_console = mirrorToConsole;
	}
	
	void Init(u32 log_count, b32 mirror){
		AssertDS(DS_MEMORY, "Attempt to initialize Logger without loading memory first");
		deshiStage |= DS_LOGGER;
		
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
		file = fopen(log_path,"ab+");
		Assert(file, "logger failed to open file");
		
		//write date at top of file
		tm* timeinfo = localtime(&rawtime);
		strftime(log_buffer,LOG_BUFFER_SIZE,"%c",timeinfo);
		//fprintf(file,"%s\n\n",log_buffer);
#if DESHI_SLOW
		//write immediately when debugging so that a log() right before Assert() still writes
		setvbuf(file,0,_IONBF,0);
#endif //DESHI_SLOW
		
		is_logging = true; //NOTE this prevents errors in calls to Log before Logger is finished initializing
		LogS("deshi","Finished logging initialization in ",TIMER_END(t_s),"ms");
	}
	
	//TODO maybe flush every X seconds/frames instead of every update?
	void Update(){
		int error = fflush(file);
		Assert(!error, "logger failed to flush file");
	}
	
	void Cleanup(){
		fclose(file);
		is_logging = false;
	}
}; //namespace Logging