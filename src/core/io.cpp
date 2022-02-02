local b32   io_crash_on_error = false;
local Arena file_data_arena;

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
	return true;
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





