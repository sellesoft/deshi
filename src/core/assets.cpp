////////////////////
//// file paths ////
////////////////////

bool Assets::
deleteFile(std::string& filepath, bool logError){
	bool result = std::filesystem::remove(filepath);
	if(logError && !result) ERROR("Failed to find file: ", filepath);
	return result;
}

u64 Assets::
deleteDirectory(std::string& dirpath, bool logError){
	auto result = std::filesystem::remove_all(dirpath);
	if(logError && !result) ERROR("Failed to find directory: ", dirpath);
	return (u64)result;
}

/////////////////////////
//// file read-write ////
/////////////////////////

std::vector<char> Assets::
readFile(const std::string& filepath, u32 chars, bool logError) {
	std::ifstream file(filepath, std::ios::ate);
	if(!file.is_open()){ 
		if(logError) ERROR_LOC("Failed to open file: ", filepath); 
		return {}; 
	}
	defer{ file.close(); };
	
	if(chars == 0){ chars = (u32)file.tellg(); }
	
	std::vector<char> buffer(chars);
	file.seekg(0);
	file.read(buffer.data(), chars);
	
	return buffer;
}

std::vector<char> Assets::
readFileBinary(const std::string& filepath, u32 bytes, bool logError) {
	std::ifstream file(filepath, std::ios::ate | std::ios::binary);
	if(!file.is_open()){ 
		if(logError) ERROR_LOC("Failed to open file: ", filepath); 
		return {}; 
	}
	defer{ file.close(); };
	
	if(bytes == 0){ bytes = (u32)file.tellg(); }
	std::vector<char> buffer(bytes);
	file.seekg(0);
	file.read(buffer.data(), bytes);
	
	return buffer;
}

char* Assets::
readFileAsciiToArray(std::string filepath, u32 chars, bool logError){
	std::ifstream file(filepath, std::ifstream::in);
	if(!file.is_open()){ 
		if(logError) ERROR_LOC("Failed to open file: ", filepath); 
		return 0; 
	}
	defer{ file.close(); };
	
	file.seekg(0, file.end);
	if(chars == 0) chars = file.tellg();
	
	char* buffer = new char[chars];
	file.seekg(0, file.beg);
	file.read(buffer,chars);
	return buffer;
}

char* Assets::
readFileBinaryToArray(std::string filepath, u32 bytes, bool logError){
	std::ifstream file(filepath, std::ifstream::in | std::ios::binary);
	if(!file.is_open()){ 
		if(logError) ERROR_LOC("Failed to open file: ", filepath); 
		return 0; 
	}
	defer{ file.close(); };
	
	file.seekg(0, file.end);
	if(bytes == 0) bytes = file.tellg();
	
	char* buffer = new char[bytes];
	file.seekg(0, file.beg);
	file.read(buffer,bytes);
	return buffer;
}

void Assets::
writeFile(const std::string& filepath, std::vector<char>& data, u32 chars, bool logError){
	std::ofstream file(filepath, std::ios::out | std::ios::trunc);
	if(!file.is_open()){ 
		if(logError) ERROR_LOC("Failed to open file: ", filepath); 
		return; 
	}
	defer{ file.close(); };
	
	if(chars == 0){ chars = data.size(); }
	file.write(reinterpret_cast<const char*>(data.data()), chars);
}

void Assets::
writeFile(const std::string& filepath, const char* data, u32 chars, bool logError){
	std::ofstream file(filepath, std::ios::out | std::ios::trunc);
	if(!file.is_open()){ 
		if(logError) ERROR_LOC("Failed to open file: ", filepath); 
		return; 
	}
	defer{ file.close(); };
	
	file.write(data, chars);
}

void Assets::
appendFile(const std::string& filepath, std::vector<char>& data, u32 chars, bool logError){
	std::ofstream file(filepath, std::ios::out | std::ios::app);
	if(!file.is_open()){ 
		if(logError) ERROR_LOC("Failed to open file: ", filepath); 
		return; 
	}
	defer{ file.close(); };
	
	if(chars == 0){ chars = data.size(); }
	file.write(reinterpret_cast<const char*>(data.data()), chars);
}

void Assets::
appendFile(const std::string& filepath, const char* data, u32 chars, bool logError){
	std::ofstream file(filepath, std::ios::out | std::ios::app);
	if(!file.is_open()){ 
		if(logError) ERROR_LOC("Failed to open file: ", filepath); 
		return; 
	}
	defer{ file.close(); };
	
	file.write(data, chars);
}

void Assets::
writeFileBinary(const std::string& filepath, std::vector<char>& data, u32 bytes, bool logError){
	std::ofstream file(filepath, std::ios::out | std::ios::binary | std::ios::trunc);
	if(!file.is_open()){ 
		if(logError) ERROR_LOC("Failed to open file: ", filepath); 
		return; 
	}
	defer{ file.close(); };
	
	if(bytes == 0){ bytes = data.size(); }
	file.write(reinterpret_cast<const char*>(data.data()), bytes);
}

void Assets::
writeFileBinary(const std::string& filepath, void* data, u32 bytes, bool logError){
	std::ofstream file(filepath, std::ios::out | std::ios::binary | std::ios::trunc);
	if(!file.is_open()){ 
		if(logError) ERROR_LOC("Failed to open file: ", filepath); 
		return; 
	}
	defer{ file.close(); };
	
	file.write((char*)data, bytes);
}

void Assets::
appendFileBinary(const std::string& filepath, void* data, u32 bytes, bool logError){
	std::ofstream file(filepath, std::ios::out | std::ios::binary | std::ios::app);
	if(!file.is_open()){ 
		if(logError) ERROR_LOC("Failed to open file: ", filepath); 
		return; 
	}
	defer{ file.close(); };
	
	file.write((char*)data, bytes);
}

std::vector<std::string> Assets::
iterateDirectory_(const std::string& filepath, const char* extension) {
	//TODO std::filesystem::recursive_directory_iterator
	using namespace std::filesystem;
	std::vector<std::string> files;
	for (auto& p : directory_iterator(filepath)) {
		if(extension){
			if(p.path().extension().string() == std::string(extension)){
				files.push_back(p.path().filename().string());
			}
		}else{
			files.push_back(p.path().filename().string());
		}
	}
	return files;
}

array<string> Assets::
iterateDirectory(const std::string& filepath, const char* extension) {
	//TODO std::filesystem::recursive_directory_iterator
	using namespace std::filesystem;
	array<string> files;
	for (auto& p : directory_iterator(filepath)) {
		if(extension){
			if(p.path().extension().string() == std::string(extension)){
				files.add(string(p.path().filename().string()));
			}
		}else{
			files.add(string(p.path().filename().string()));
		}
	}
	return files;
}

void Assets::
enforceDirectories() {
	using namespace std::filesystem;
	if(!is_directory(dirData())){
		create_directory(dirData());
		create_directory(dirConfig());
		create_directory(dirEntities());
		create_directory(dirFonts());
		create_directory(dirLogs());
		create_directory(dirLevels());
		create_directory(dirModels());
		create_directory(dirSaves());
		create_directory(dirShaders());
		create_directory(dirSounds());
		create_directory(dirTextures());
		create_directory(dirTemp());
	}else{
		if(!is_directory(dirConfig()))   create_directory(dirConfig());
		if(!is_directory(dirEntities())) create_directory(dirEntities());
		if(!is_directory(dirLogs()))     create_directory(dirLogs());
		if(!is_directory(dirLevels()))     create_directory(dirLevels());
		if(!is_directory(dirFonts()))    create_directory(dirFonts());
		if(!is_directory(dirModels()))   create_directory(dirModels());
		if(!is_directory(dirSaves()))    create_directory(dirSaves());
		if(!is_directory(dirShaders()))  create_directory(dirShaders());
		if(!is_directory(dirSounds()))   create_directory(dirSounds());
		if(!is_directory(dirTextures())) create_directory(dirTextures());
		if( is_directory(dirTemp()))     remove_all(dirTemp());
		create_directory(dirTemp());
	}
}

///////////////////////////
//// parsing utilities ////
///////////////////////////

pair<std::string, std::string> Assets::
split_keyValue(std::string str){
	size_t idx = str.find_first_of(' ');
	if (idx == -1) return pair<std::string, std::string>(str, std::string(""));
	
	std::string key = str.substr(0, idx);
	std::string val;
	size_t start = str.find_first_not_of(' ', idx);
	size_t end = -1;
	if(str[start] == '\"') {
		end = str.find_first_of('\"', start + 1);
		val = str.substr(start + 1, end - (start + 1));
	}else if(str[start] == '\'') {
		end = str.find_first_of('\'', start + 1);
		val = str.substr(start + 1, end - (start + 1));
	}else if(str[start] == '(') {
		end = str.find_first_of(')', start + 1);
		val = str.substr(start + 1, end - (start + 1));
	}else{
		end = str.find_first_of(" #\n", start + 1);
		val = str.substr(start, end - start);
	}
	
	return pair<std::string, std::string>(key, val);
}

std::map<std::string, std::string> Assets::
extractConfig(const std::string& filepath) {
	std::map<std::string, std::string> out;
	
	std::fstream in(Assets::dirConfig() + filepath, std::fstream::in);
	if(!in.is_open()){ ERROR("Failed to open file: ", filepath); out.emplace("FileNotFound", ""); return out; }
	defer{ in.close(); };
	
	std::regex r = std::regex("([A-Za-z]+) += +(.+)");
	int line = 0;
	char* c = (char*)malloc(255);
	while (!in.eof()) {
		in.getline(c, 255);
		std::string s(c);
		
		if (s[0] == '>') { line++; continue; }
		
		std::smatch m;
		std::regex_match(s, m, r);
		if (m.size() == 1) {
			ERROR_LOC(m[1], "\nConfig regex did not find a match for the string above.");
			line++;
			continue;
		}
		out.emplace(m[1], m[2]);
	}
	return out;
}

bool Assets::
parse_bool(std::string& str, const char* filepath, u32 line_number){
	if(str == "true" || str == "1"){
		return true;
	}else if(str == "false" || str == "0"){
		return false;
	}else{
		if(filepath && line_number){
			ERROR("Error parsing '",filepath,"' on line '",line_number,"'! Invalid boolean value: ", str);
		}else{
			ERROR("Failed to parse boolean value: ", str);
		}
		return false;
	}
}

void Assets::
saveConfig(const char* filename, const ConfigMap& configMap){
	std::string filepath = Assets::dirConfig() + filename;
	std::ofstream out(filepath, std::ios::out | std::ios::trunc); //flushes on destruction
	if(!out.is_open()) { ERROR_LOC("Failed to open file: ", filepath); return; }
	
	size_t pad_amount = 1;
	for(auto& config : configMap){
		//print key
		out << config.first;
		
		//print padding
		size_t key_len = strlen(config.first);
		if(pad_amount > key_len){
			out << std::string(pad_amount - key_len, ' ');
		}else{
			out << ' ';
		}
		
		//print value
		switch(config.second){
			case ConfigValueType_NONE:{
				//do nothing
			}break;
			case ConfigValueType_PADSECTION:{
				pad_amount = (config.third) ? (size_t)config.third : 1;
			}break;
			case ConfigValueType_S32:{
				out << *(s32*)config.third;
			}break;
			case ConfigValueType_Bool:{
				out << ((*(bool*)config.third) ? "true" : "false");
			}break;
			case ConfigValueType_U32:{
				out << *(u32*)config.third;
			}break;
			case ConfigValueType_U8:{
				out << (u32)(*(u8*)config.third);
			}break;
			case ConfigValueType_F32:{
				out << std::to_string(*(f32*)config.third);
			}break;
			case ConfigValueType_F64:{
				out << std::to_string(*(f64*)config.third);
			}break;
			case ConfigValueType_FV2:{
				out << ((vec2*)config.third)->str();
			}break;
			case ConfigValueType_FV3:{
				out << ((vec3*)config.third)->str();
			}break;
			case ConfigValueType_FV4:{
				out << ((vec4*)config.third)->str();
			}break;
			case ConfigValueType_StdString:{
				out << '\"' << *(std::string*)config.third << '\"';
			}break;
			case ConfigValueType_Key:{
				u32 keymod = *(Key::Key*)config.third;
				out << KeyStrings[keymod & 0x000000FF];
				switch(keymod & 0xFFFFFF00){
					case(InputMod_NONE):           break; //do nothing
					case(InputMod_Any):            out << ",Any"; break;
					case(InputMod_Lctrl):          out << ",LCTRL"; break;
					case(InputMod_Rctrl):          out << ",RCTRL"; break;
					case(InputMod_Lshift):         out << ",LSHIFT"; break;
					case(InputMod_Rshift):         out << ",RSHIFT"; break;
					case(InputMod_Lalt):           out << ",LALT"; break;
					case(InputMod_Ralt):           out << ",RALT"; break;
					case(InputMod_LctrlLshift):    out << ",LCTRL,LSHIFT"; break;
					case(InputMod_LctrlRshift):    out << ",LCTRL,RSHIFT"; break;
					case(InputMod_RctrlLshift):    out << ",RCTRL,LSHIFT"; break;
					case(InputMod_RctrlRshift):    out << ",RCTRL,RSHIFT"; break;
					case(InputMod_LctrlLalt):      out << ",LCTRL,LALT"; break;
					case(InputMod_LctrlRalt):      out << ",LCTRL,RALT"; break;
					case(InputMod_RctrlLalt):      out << ",RCTRL,LALT"; break;
					case(InputMod_RctrlRalt):      out << ",RCTRL,RALT"; break;
					case(InputMod_LshiftLalt):     out << ",LSHIFT,LALT"; break;
					case(InputMod_LshiftRalt):     out << ",LSHIFT,RALT"; break;
					case(InputMod_RshiftLalt):     out << ",RSHIFT,LALT"; break;
					case(InputMod_RshiftRalt):     out << ",RSHIFT,RALT"; break;
					case(InputMod_LctrlLshiftLalt):out << ",LCTRL,LSHIFT,LALT"; break;
					case(InputMod_LctrlLshiftRalt):out << ",LCTRL,LSHIFT,RALT"; break;
					case(InputMod_LctrlRshiftLalt):out << ",LCTRL,RSHIFT,LALT"; break;
					case(InputMod_LctrlRshiftRalt):out << ",LCTRL,RSHIFT,RALT"; break;
					case(InputMod_RctrlLshiftLalt):out << ",RCTRL,LSHIFT,LALT"; break;
					case(InputMod_RctrlLshiftRalt):out << ",RCTRL,LSHIFT,RALT"; break;
					case(InputMod_RctrlRshiftLalt):out << ",RCTRL,RSHIFT,LALT"; break;
					case(InputMod_RctrlRshiftRalt):out << ",RCTRL,RSHIFT,RALT"; break;
					default: ERROR("Unknown key-modifier combination \'",keymod,"\' when saving config: ", filename);
				} 
			}break;
			default:{
				ERROR("Unknown value type when saving config: ", filename);
			}break;
		}
		out << '\n';
	}
}

//NOTE this copies the config map so it can remove keys when found
void Assets::
loadConfig(const char* filename, ConfigMap configMap){
	std::string filepath = Assets::dirConfig() + filename;
	char* buffer = Assets::readFileAsciiToArray(filepath, 0, false);
	if(!buffer){ saveConfig(filename, configMap); return; }
	defer{ delete[] buffer; };
	
	char* line_start;  char* line_end = buffer - 1;
	char* info_start;  char* info_end;
	char* key_start;   char* key_end;
	char* value_start; char* value_end;
	bool has_cr = false;
	for(u32 line_number = 1; ;line_number++){
		//get the next line
		line_start = (has_cr) ? line_end+2 : line_end+1;
		if((line_end = strchr(line_start, '\n')) == 0) break; //EOF if no '\n'
		if(has_cr || *(line_end-1) == '\r'){ has_cr = true; line_end -= 1; }
		if(line_start == line_end) continue;
		
		//format the line
		info_start = line_start + Utils::skipSpacesLeading(line_start, line_end-line_start);  if(info_start == line_end) continue;
		info_end   = info_start + Utils::skipComments(info_start, "#", line_end-info_start);  if(info_start == info_end) continue;
		info_end   = info_start + Utils::skipSpacesTrailing(info_start, info_end-info_start); if(info_start == info_end) continue;
		
		{//split the key-value pair
			key_start = info_start;
			key_end   = key_start;
			while(key_end != info_end && *key_end++ != ' '){}
			if(key_end == info_end) { ERROR("Error parsing '",filepath,"' on line '",line_number,"'! No value passed"); break; }
			key_end--;
			
			value_end   = info_end;
			value_start = value_end-1;
			while(*value_start-- != ' '){}
			value_start += 2;
			if(value_end == value_start) { ERROR("Error parsing '",filepath,"' on line '",line_number,"'! No value passed"); break; }
		}
		
		//parse the key-value pair
		for(auto& config : configMap){
			//check if type is valid
			if(config.second == ConfigValueType_NONE || config.second == ConfigValueType_PADSECTION) continue;
			
			if(strncmp(config.first, key_start, key_end-key_start) == 0){
				switch(config.second){
					case ConfigValueType_S32:{
						*(s32*)config.third = atoi(value_start);
					}break;
					case ConfigValueType_Bool:{
						bool* b = (bool*)config.third;
						if     (strncmp("true",  value_start, 4) == 0) *b = true;
						else if(strncmp("1",     value_start, 1) == 0) *b = true;
						else if(strncmp("false", value_start, 5) == 0) *b = false;
						else if(strncmp("0",     value_start, 1) == 0) *b = false;
						else ERROR("Error parsing '",filepath,"' on line '",line_number,"'! Invalid boolean value: ", value_start);
					}break;
					case ConfigValueType_U32:{
						*(u32*)config.third = (u32)atoi(value_start);
					}break;
					case ConfigValueType_U8:{
						*(u8*)config.third = (u8)atoi(value_start);
					}break;
					case ConfigValueType_F32:{
						*(f32*)config.third = (f32)atof(value_start);
					}break;
					case ConfigValueType_F64:{
						*(f64*)config.third = atof(value_start);
					}break;
					case ConfigValueType_FV2:{
						vec2* vec = (vec2*)config.third;
						char* cursor;
						vec->x = strtof(value_start+1, &cursor);
						vec->y = strtof(cursor+1, 0);
					}break;
					case ConfigValueType_FV3:{
						vec3* vec = (vec3*)config.third;
						char* cursor;
						vec->x = strtof(value_start+1, &cursor);
						vec->y = strtof(cursor+1, &cursor);
						vec->z = strtof(cursor+1, 0);
					}break;
					case ConfigValueType_FV4:{
						vec4* vec = (vec4*)config.third;
						char* cursor;
						vec->x = strtof(value_start+1, &cursor);
						vec->y = strtof(cursor+1, &cursor);
						vec->z = strtof(cursor+1, &cursor);
						vec->w = strtof(cursor+1, 0);
					}break;
					case ConfigValueType_StdString:{
						*(std::string*)config.third = std::string(value_start+1, value_end-value_start-2);
					}break;
					case ConfigValueType_Key:{ 
						//TODO(delle,OpAs) improve perf by not using std::string
						//    just do a pseudo binary split against all mod cases at once after
						//    extracting the key, kinda like how im doing already but without a loop
						Key::Key* keybind = (Key::Key*)config.third;
						
						//parse key
						auto keys = Utils::characterDelimit(std::string(value_start, value_end-value_start), ',');
						u32 key_number = -1; //stored for Any edge-case where we want to override other mods
						forI(ArrayCount(KeyStrings)){
							if(strcmp(KeyStrings[i], keys[0].c_str()) == 0){
								*keybind = (Key::Key)i;
								key_number = i;
								break;
							}
						}
						if(key_number == -1) { 
							ERROR("Error parsing '",filepath,"' on line '",line_number,"'! Invalid keybind: ",keys[0]);
							continue;
						}
						
						//parse mods
						if(keys.size() > 1){
							for(int i = 1; i < keys.size(); ++i){
								if(keys[i][0] == 'L'){
									if      (keys[i][1] == 'C'){
										*keybind |= InputMod_Lctrl;
									}else if(keys[i][1] == 'S'){
										*keybind |= InputMod_Lshift;
									}else if(keys[i][1] == 'A'){
										*keybind |= InputMod_Lalt;
									}else{
										ERROR("Error parsing '",filepath,"' on line '",line_number,"'! Unknown keybind mod: ",keys[i]);
									}
								}else if(keys[i][0] == 'R'){
									if      (keys[i][1] == 'C'){
										*keybind |= InputMod_Rctrl;
									}else if(keys[i][1] == 'S'){
										*keybind |= InputMod_Rshift;
									}else if(keys[i][1] == 'A'){
										*keybind |= InputMod_Ralt;
									}else{
										ERROR("Error parsing '",filepath,"' on line '",line_number,"'! Unknown keybind mod: ",keys[i]);
									}
								}else if(keys[i] == "Any"){
									//overwrite and ignore other mods with Any
									*keybind = (Key::Key)(key_number | InputMod_Any);
									break;
								}else{
									ERROR("Error parsing '",filepath,"' on line '",line_number,"'! Unknown keybind mod: ",keys[i]);
								}
							}
						}
					}break;
					default:{
						ERROR("Error parsing '",filepath,"' on line '",line_number,"'! Invalid key: ", key_start);
					}break;
				}
				
				//set type to NONE so key wont get checked again
				config.second = ConfigValueType_NONE;
			}
		}
	}
}