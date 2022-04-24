//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @internal
local Console deshi__console;
local Logger* deshi__console_logger;
local b32 deshi__console_open_pressed = false;
local UIWindow* deshi__console_window;
local UIWindowFlags deshi__console_window_flags = UIWindowFlags_NoMove | UIWindowFlags_NoResize | UIWindowFlags_NoScrollX;
local b32 deshi__console_scroll_to_bottom = false;
local Stopwatch deshi__console_open_timer;
local Arena* deshi__console_chunk_render_arena;

color console_string_to_color(str8 s){
	u64 s_hash = str8_hash64(s);
	switch(s_hash){
		case str8_static_hash64("red"    ): return Color_Red;
		case str8_static_hash64("dred"   ): return Color_DarkRed;
		case str8_static_hash64("blue"   ): return Color_Blue;
		case str8_static_hash64("dblue"  ): return Color_DarkBlue;
		case str8_static_hash64("cyan"   ): return Color_Cyan;
		case str8_static_hash64("dcyan"  ): return Color_DarkCyan;
		case str8_static_hash64("grey"   ): return Color_Grey;
		case str8_static_hash64("dgrey"  ): return Color_DarkGrey;
		case str8_static_hash64("green"  ): return Color_Green;
		case str8_static_hash64("dgreen" ): return Color_DarkGreen;
		case str8_static_hash64("yellow" ): return Color_Yellow;
		case str8_static_hash64("dyellow"): return Color_DarkYellow;
		case str8_static_hash64("magen"  ): return Color_Magenta;
		case str8_static_hash64("dmegen" ): return Color_DarkMagenta;
		case str8_static_hash64("white"  ): return Color_White;
		case str8_static_hash64("black"  ): return Color_Black;
		default:
		LogW("console","Unknown color in chunk formatting: ", s);
		return Color_White;
	}
}

void console_send_to_logger(str8 message, b32 newline){DPZoneScoped;
	b32 restore_mirror  = deshi__console_logger->mirror_to_console;
	b32 restore_tag     = deshi__console_logger->ignore_tags;
	b32 restore_newline = deshi__console_logger->auto_newline;
	b32 restore_track   = deshi__console_logger->track_caller;
	deshi__console_logger->mirror_to_console = false;
	deshi__console_logger->ignore_tags       = true;
	deshi__console_logger->auto_newline      = newline;
	deshi__console_logger->track_caller      = false;
	logger_format_log(str8_lit(__FILE__),__LINE__,str8{},LogType_Normal,str8_lit("%.*s"),message.count,message.str);
	deshi__console_logger->mirror_to_console = restore_mirror;
	deshi__console_logger->ignore_tags       = restore_tag;
	deshi__console_logger->auto_newline      = restore_newline;
	deshi__console_logger->track_caller      = restore_track;
}

void console_parse_message(str8 message){DPZoneScoped;
	if(message.count == 0) return;
	
	if(deshi__console.automatic_scroll){
		deshi__console_scroll_to_bottom = true;
	}
	
	ConsoleChunk chunk{ConsoleChunkType_Normal, {}, Color_White};
	fseek(deshi__console_logger->file, 0, SEEK_END);
	u64 log_offset = ftell(deshi__console_logger->file);
	u8* chunk_start = message.str;
	str8 remaining = message;
	while(remaining){
		DecodedCodepoint current = str8_advance(&remaining);
		if(current.codepoint == '\n'){
			chunk.start   = log_offset;
			chunk.size    = remaining.str - current.advance - chunk_start;
			chunk.newline = true;
			deshi__console.dictionary.add(chunk);
			console_send_to_logger(str8{chunk_start,(s64)chunk.size}, true);
			log_offset += chunk.size + 1;
			chunk_start = remaining.str;
		}
		
		if(current.codepoint == '{'){
			//add current chunk to dictionary and start a new one
			if(remaining.str - current.advance - chunk_start > 0){
				chunk.type    = ConsoleChunkType_Normal;
				chunk.tag     = str8{};
				chunk.color   = Color_White;
				chunk.start   = log_offset;
				chunk.size    = remaining.str - current.advance - chunk_start;
				chunk.newline = false;
				deshi__console.dictionary.add(chunk); //TODO(delle) maybe append to prev chunk if no difference in formatting
				console_send_to_logger(str8{chunk_start,(s64)chunk.size}, false);
				log_offset += chunk.size;
				chunk_start = remaining.str;
			}
			
			//NOTE(delle) we use decoded_codepoint_from_utf8() instead of advance() so that
			//we don't have to rollback remaining if the codepoint doesn't match
			DecodedCodepoint next = decoded_codepoint_from_utf8(remaining.str, 4);
			if(next.codepoint == '{'){
				remaining.str += next.advance; remaining.count -= next.advance;
				
				another_specifier:;
				if(!remaining){ //the message ended before finishing specifiers, so treat as a normal chunk
					chunk.type    = ConsoleChunkType_Normal;
					chunk.tag     = str8{};
					chunk.color   = Color_White;
					chunk.start   = log_offset;
					chunk.size    = remaining.str - chunk_start;
					chunk.newline = true;
					deshi__console.dictionary.add(chunk);
					console_send_to_logger(str8{chunk_start,(s64)chunk.size}, chunk.newline);
					return;
				}
				
				next = decoded_codepoint_from_utf8(remaining.str, 4);
				u32 specifier = next.codepoint;
				switch(specifier){
					case 0:{ //error when decoding utf8 in specifiers, so stop parsing and treat as normal chunk
						chunk.type    = ConsoleChunkType_Normal;
						chunk.tag     = str8{};
						chunk.color   = Color_White;
						chunk.start   = log_offset;
						chunk.size    = remaining.str - chunk_start;
						chunk.newline = true;
						deshi__console.dictionary.add(chunk);
						console_send_to_logger(str8{chunk_start,(s64)chunk.size}, chunk.newline);
						return;
					}break;
					case ' ':{ //skip spaces in the specifier
						remaining.str += next.advance; remaining.count -= next.advance;
						goto another_specifier;
					}break;
					
					case 'a':{
						remaining.str += next.advance; remaining.count -= next.advance;
						chunk.type |= ConsoleChunkType_Alert;
					}break;
					case 'e':{
						remaining.str += next.advance; remaining.count -= next.advance;
						chunk.type = HasFlag(chunk.type, ConsoleChunkType_Alert) ? ConsoleChunkType_Error|ConsoleChunkType_Alert : ConsoleChunkType_Error;
						chunk.color = Color_Red;
					}break;
					case 'w':{
						remaining.str += next.advance; remaining.count -= next.advance;
						chunk.type = HasFlag(chunk.type, ConsoleChunkType_Alert) ? ConsoleChunkType_Warning|ConsoleChunkType_Alert : ConsoleChunkType_Warning;
						chunk.color = Color_Yellow;
					}break;
					case 's':{
						remaining.str += next.advance; remaining.count -= next.advance;
						chunk.type = HasFlag(chunk.type, ConsoleChunkType_Alert) ? ConsoleChunkType_Success|ConsoleChunkType_Alert : ConsoleChunkType_Success;
						chunk.color = Color_Green;
					}break;
					
					case 'c':
					case 't':{
						remaining.str += next.advance; remaining.count -= next.advance;
						next = decoded_codepoint_from_utf8(remaining.str, 4);
						if(next.codepoint == '='){
							remaining.str += next.advance; remaining.count -= next.advance;
							
							//loop until } or ,
							b32 end_formatting = false;
							str8 arg{remaining.str, 0};
							while(remaining){
								DecodedCodepoint decoded = decoded_codepoint_from_utf8(remaining.str, 4);
								if(decoded.codepoint == '}'){ end_formatting = true; break; }
								if(decoded.codepoint == ',') break;
								remaining.str += decoded.advance; remaining.count -= decoded.advance;
								arg.count += decoded.advance;
							}
							
							//if remaining is empty, there was no } or , in the rest of the message
							if(remaining){
								remaining.str += 1; remaining.count -= 1; //NOTE(delle) +1 because we know } or , only take 1 byte
								if(specifier == 'c'){
									chunk.color = console_string_to_color(arg);
								}else{
									chunk.tag = arg;
								}
								
								if(end_formatting){
									//loop until } or \n
									another_chunk:;
									end_formatting = false; //NOTE(delle) reusing end_formatting to check whether text_chunk ends with } or \n
									arg.str = remaining.str; arg.count = 0; //same with arg
									while(remaining){
										DecodedCodepoint decoded = decoded_codepoint_from_utf8(remaining.str, 4);
										if(decoded.codepoint == '}'){ end_formatting = true; break; }
										if(decoded.codepoint == '\n') break;
										remaining.str += decoded.advance; remaining.count -= decoded.advance;
										arg.count += decoded.advance;
									}
									
									if(remaining){
										//add formatted chunk
										chunk.start   = log_offset;
										chunk.size    = remaining.str - arg.str;
										chunk.newline = (end_formatting == false || remaining.count <= 0);
										deshi__console.dictionary.add(chunk);
										console_send_to_logger(arg, chunk.newline);
										log_offset += arg.count + ((chunk.newline) ? 1 : 0);
										
										remaining.str += 1; remaining.count -= 1; //NOTE(delle) +1 because we know } or \n only takes 1 byte
										chunk_start = remaining.str;
										
										//reset to normal chunk if formatting ended, else keep parsing with current formatting
										if(end_formatting){
											chunk.type    = ConsoleChunkType_Normal;
											chunk.tag     = str8{};
											chunk.color   = Color_White;
										}else{
											goto another_chunk;
										}
									}else{
										//we reached the end of the message without finding a } so stop parsing
										if(remaining.str - chunk_start){
											chunk.start   = log_offset;
											chunk.size    = remaining.str - chunk_start;
											chunk.newline = true;
											deshi__console.dictionary.add(chunk);
											console_send_to_logger(str8{chunk_start,(s64)chunk.size}, chunk.newline);
										}
										return;
									}
								}else{
									goto another_specifier;
								}
							}else{
								//we reached the end of the message without finding a } so stop parsing
								if(remaining.str - chunk_start){
									chunk.start   = log_offset;
									chunk.size    = remaining.str - chunk_start;
									chunk.newline = true;
									deshi__console.dictionary.add(chunk);
									console_send_to_logger(str8{chunk_start,(s64)chunk.size}, chunk.newline);
								}
								return;
							}
						}
					}break;
				}
			}
		}
	}
}


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @interface
void console_init(){
	DeshiStageInitStart(DS_CONSOLE, DS_MEMORY | DS_LOGGER, "Attempted to initialize Console module before initializing Memory and Logger modules");
	
	deshi__console_logger = logger_expose();
	deshi__console.dictionary.init(512, deshi_allocator);
	deshi__console.input_history.init(256, deshi_allocator);
	deshi__console.console_pos = vec2::ZERO;
	deshi__console.console_dim = vec2(f32(DeshWindow->width), f32(DeshWindow->height) * deshi__console.open_max_percent);
	deshi__console_scroll_to_bottom = true;
	deshi__console_chunk_render_arena = memory_create_arena(512);
	
	deshi__console_logger->mirror_to_console = true;
	
	DeshiStageInitEnd(DS_CONSOLE);
}

void console_update(){
	//check for console state changing inputs
	if(key_pressed(Key_TILDE)){
		if      (input_shift_down()){
			console_change_state(ConsoleState_OpenBig);
		}else if(input_ctrl_down()){
			console_change_state(ConsoleState_Popout);
		}else{
			console_change_state(ConsoleState_OpenSmall);
		}
	}
	
	//check for history inputs
	b32 change_input = false;
	if(key_pressed(Key_UP)){
		change_input = true;
		deshi__console.input_history_index--;
		if(deshi__console.input_history_index == -2) deshi__console.input_history_index = deshi__console.input_history.count-1;
	}
	if(key_pressed(Key_DOWN)){
		change_input = true;
		deshi__console.input_history_index++;
		if(deshi__console.input_history_index >= deshi__console.input_history.count) deshi__console.input_history_index = -1;
	}
	if(change_input){
		simulate_key_press(Key_END);
		ZeroMemory(deshi__console.input_buffer, CONSOLE_INPUT_BUFFER_SIZE);
		if(deshi__console.input_history_index != -1){
			u32 cursor = 0;
			u32 chunk_idx = deshi__console.input_history[deshi__console.input_history_index].first;
			ConsoleChunk& chunk = deshi__console.dictionary[chunk_idx];
			
			fseek(deshi__console_logger->file, chunk.start, SEEK_SET);
			for(;;){
				u32 characters = (cursor + chunk.size > CONSOLE_INPUT_BUFFER_SIZE)
					? CONSOLE_INPUT_BUFFER_SIZE - (cursor + chunk.size) : chunk.size;
				fread(deshi__console.input_buffer, sizeof(char), characters, deshi__console_logger->file);
				cursor += characters;
				
				if(   chunk_idx >= deshi__console.input_history[deshi__console.input_history_index].second 
				   || cursor >= CONSOLE_INPUT_BUFFER_SIZE){
					break;
				}else{
					chunk = deshi__console.dictionary[++chunk_idx];
				}
			}
		}
	}
	
	//console openness
	if(deshi__console.open_target != deshi__console.open_amount){
		deshi__console.console_dim.y = Math::lerp(deshi__console.open_amount, deshi__console.open_target, //TODO(sushi) change this to Nudge
												  Min((f32)peek_stopwatch(deshi__console_open_timer) / deshi__console.open_dt, 1.f));
		if(deshi__console.console_dim.y == deshi__console.open_target){
			deshi__console.open_amount = deshi__console.open_target;
		}
	}
	
	UIStyle& style = UI::GetStyle();
	if(deshi__console.console_dim.y > 0){
		if(deshi__console.open_target != deshi__console.open_amount || deshi__console.state != ConsoleState_Popout){
			UI::SetNextWindowPos(deshi__console.console_pos);
			UI::SetNextWindowSize(deshi__console.console_dim);
		}
		
		UI::Begin("deshiConsole", vec2::ZERO, vec2::ZERO, deshi__console_window_flags);
		deshi__console_window = UI::GetWindow();
		
		//-//////////////////////////////////////////////////////////////////////////////////////////////
		//text input panel
		//NOTE(delle) doing this above the terminal window so inputs are registered managed before visuals
		if(deshi__console_open_pressed) UI::SetNextItemActive();
		f32 input_box_height = style.inputTextHeightRelToFont * style.fontHeight;
		UI::SetNextItemSize(vec2(MAX_F32, input_box_height));
		if(UI::InputText("deshiConsoleInput", (char*)deshi__console.input_buffer, CONSOLE_INPUT_BUFFER_SIZE-2,
						 {UI::GetMarginedLeft(), UI::GetMarginedBottom() - input_box_height},
						 "Enter a command...", UIInputTextFlags_AnyChangeReturnsTrue)){
			if(deshi__console.input_buffer[deshi__console.input_length] != 0){
				DecodedCodepoint decoded = decoded_codepoint_from_utf8(deshi__console.input_buffer+deshi__console.input_length, 4);
				if(decoded.codepoint == '~' || decoded.codepoint == '`'){
					deshi__console.input_buffer[deshi__console.input_length] = '\0'; //NOTE(delle) filter out console open/close inputs
				}else{
					deshi__console.input_length += decoded.advance;
				}
			}else{
				while(   deshi__console.input_length
					  && deshi__console.input_buffer[deshi__console.input_length-1] == 0){
					deshi__console.input_length -= 1;
				}
			}
		}
		if(key_pressed(Key_ENTER) && deshi__console.input_length){
			//append a \n to the input
			deshi__console.input_buffer[deshi__console.input_length  ] = '\n';
			deshi__console.input_buffer[deshi__console.input_length+1] = '\0';
			
			//log and chunk the input symbol
			fseek(deshi__console_logger->file, 0, SEEK_END);
			u64 log_offset = ftell(deshi__console_logger->file);
			console_send_to_logger(str8_lit("/\\ "), false);
			deshi__console.dictionary.add({ConsoleChunkType_Normal, str8{}, Color_Cyan,     log_offset,   1, false});
			deshi__console.dictionary.add({ConsoleChunkType_Normal, str8{}, Color_DarkCyan, log_offset+1, 2, false});
			
			//send the input to the message parser
			//console_parse_message(str8_lit("{{c=cyan}/}{{c=dcyan}\\ }"));
			u32 start_chunk = deshi__console.dictionary.count; //NOTE(delle) checking this here b/c we don't know how many chunks input will create
			console_parse_message(str8{deshi__console.input_buffer, deshi__console.input_length+1});
			
			//record the input into the history
			if(strncmp((char*)deshi__console.input_buffer, (char*)deshi__console.prev_input, CONSOLE_INPUT_BUFFER_SIZE) != 0){
				CopyMemory(deshi__console.prev_input, deshi__console.input_buffer, CONSOLE_INPUT_BUFFER_SIZE);
				deshi__console.input_history.add({start_chunk,deshi__console.dictionary.count-1});
			}
			
			cmd_run(str8{deshi__console.input_buffer, deshi__console.input_length});
			ZeroMemory(deshi__console.input_buffer, deshi__console.input_length+1);
			deshi__console_scroll_to_bottom = true;
			deshi__console.input_history_index = -1;
		}
		
		//-//////////////////////////////////////////////////////////////////////////////////////////////
		//terminal output panel
		UI::SetNextWindowSize(vec2(MAX_F32, UI::GetMarginedBottom() - (style.fontHeight * style.inputTextHeightRelToFont + style.itemSpacing.y) * 3));
		UI::BeginChild("deshiConsoleTerminal", (deshi__console_window->dimensions - 2*style.windowMargins).yAdd(-(1.3*style.fontHeight + style.itemSpacing.y)), deshi__console_window_flags);{
			//draw text for the dictionary
			UI::PushVar(UIStyleVar_WindowMargins, vec2(5, 0));
			UI::PushVar(UIStyleVar_ItemSpacing,   vec2(0, 0));
			
			forI(deshi__console.dictionary.count){
				//if deshi__console_chunk_render_arena isn't large enough, double the space for it
				if(deshi__console.dictionary[i].size >= deshi__console_chunk_render_arena->size){
					deshi__console_chunk_render_arena = memory_grow_arena(deshi__console_chunk_render_arena, deshi__console_chunk_render_arena->size);
				}
				
				//get chunk text from the log file
				fseek(deshi__console_logger->file, deshi__console.dictionary[i].start, SEEK_SET);
				fread(deshi__console_chunk_render_arena->start, sizeof(u8), deshi__console.dictionary[i].size, deshi__console_logger->file);
				deshi__console_chunk_render_arena->start[deshi__console.dictionary[i].size] = '\0';
				
				//adjust what we're going to print based on formatting
					if(HasFlag(deshi__console.dictionary[i].type, ConsoleChunkType_Alert)){
						//TODO handle alert flashing
					}
					
					if(deshi__console.tag_show && deshi__console.dictionary[i].tag){
						if(i == 0 || !str8_equal(deshi__console.dictionary[i-1].tag, deshi__console.dictionary[i].tag)){
							f32 top_edge    = UI::GetMarginedTop();
							f32 right_edge  = UI::GetMarginedRight();
							f32 tag_start_y = UI::GetWinCursor().y;
							vec2 tag_size   = UI::CalcTextSize((char*)deshi__console.dictionary[i].tag.str);
							
							//look ahead for the end of the tag range
							//dont draw if its not visible
							f32 tag_end_y = tag_start_y;
							for(int o = i; o < deshi__console.dictionary.count; o++){
								if(str8_equal(deshi__console.dictionary[o].tag, deshi__console.dictionary[i].tag)){
									tag_end_y += tag_size.y;
								}else{
									break;
								}
							}
							if(tag_end_y > top_edge){
								if(tag_end_y < tag_size.y){
									tag_start_y = top_edge + (tag_end_y - tag_size.y);
								}else{
									tag_start_y = Max(tag_start_y, top_edge);
								}
								
								//draw tag text
								UI::PushColor(UIStyleCol_Text, color(150,150,150, 150));
								UI::Text((char*)deshi__console.dictionary[i].tag.str, vec2(right_edge - tag_size.x, tag_start_y));
								UI::PopColor();
								
								if(deshi__console.tag_outlines && tag_end_y - (tag_start_y + tag_size.y) > 4){
									//draw right line
									UI::Line(vec2(right_edge-1, tag_start_y+tag_size.y+3),
											 vec2(right_edge-1, tag_end_y-3),
											 2, color(150,150,150, 150));
									//draw top line
									UI::Line(vec2(right_edge-tag_size.x, tag_start_y+tag_size.y+2),
											 vec2(right_edge,            tag_start_y+tag_size.y+2),
											 2, color(150,150,150, 150));
									//draw bottom line
									UI::Line(vec2(right_edge-tag_size.x, tag_end_y-2),
											 vec2(right_edge,            tag_end_y-2),
											 2, color(150,150,150, 150));
								}
								
								if(deshi__console.tag_highlighting){
									UI::PushLayer(UI::GetCenterLayer()-1); //NOTE(delle) put the highlight on a lower layer than the text
									UI::PushColor(UIStyleCol_SelectableBg,        Color_Clear);
									UI::PushColor(UIStyleCol_SelectableBgHovered, color(155, 155, 155, 10));
									UI::PushColor(UIStyleCol_SelectableBgActive,  color(155, 155, 155, 10));
									UI::SetNextItemSize(vec2(right_edge, tag_end_y - tag_start_y));
									UI::SetNextItemMinSizeIgnored();
									
									UI::Selectable("", vec2(0,tag_start_y), 0);
									
									UI::PopColor(3);
									UI::PopLayer();
								}
								
								UI::SetMarginSizeOffset(vec2(-tag_size.x, 0));
							}
						}
					}
				
				UI::PushColor(UIStyleCol_Text, deshi__console.dictionary[i].color);
				UI::Text((char*)deshi__console_chunk_render_arena->start);
				UI::PopColor();
				
				if(deshi__console.dictionary[i].newline == false && i != (deshi__console.dictionary.count-1)){
					UI::SameLine();
				}
				
					if(deshi__console.line_highlighing){
						UIItem* last_text = UI::GetLastItem();
						UI::PushLayer(UI::GetCenterLayer()-1); //NOTE(delle) put the highlight on a lower layer than the text
						UI::PushColor(UIStyleCol_SelectableBg,        Color_Clear);
						UI::PushColor(UIStyleCol_SelectableBgHovered, color(155,155,155, 10));
						UI::PushColor(UIStyleCol_SelectableBgActive,  color(155,155,155, 10));
						UI::SetNextItemSize(vec2(UI::GetMarginedRight(), last_text->size.y));
						UI::SetNextItemMinSizeIgnored();
						
						UI::Selectable("", vec2(0, last_text->position.y), 0);
						
						UI::PopColor(3);
						UI::PopLayer();
					}
			}
			UI::PopVar(2);
			
			if(deshi__console_scroll_to_bottom){
				UI::SetScroll(vec2(0, MAX_F32));
				deshi__console_scroll_to_bottom = false;
			}
		}UI::EndChild();
		
		UI::PushColor(UIStyleCol_WindowBg, color(0,25,18));
		UI::End();
		UI::PopColor();
	}
	
	deshi__console_open_pressed = false;
}

Console* console_expose(){
	return &deshi__console;
}

void console_change_state(ConsoleState new_state){
	if(deshi__console.state == new_state) new_state = ConsoleState_Closed;
	
	switch(new_state){
		case ConsoleState_Closed:{
			if(deshi__console.state == ConsoleState_Popout){
				deshi__console.console_pos = deshi__console_window->position;
				deshi__console.console_dim = deshi__console_window->dimensions;
			}
			deshi__console.open_target = 0;
			deshi__console.open_amount = deshi__console.console_dim.y;
		}break;
		case ConsoleState_OpenSmall:{
			deshi__console.open_target = (f32)DeshWindow->height * deshi__console.open_small_percent;
			deshi__console.open_amount =deshi__console. console_dim.y;
			deshi__console.console_pos = vec2(0, -1);
			deshi__console.console_dim.x = (f32)DeshWindow->width;
			deshi__console_window_flags = UIWindowFlags_NoMove | UIWindowFlags_NoResize | UIWindowFlags_NoScrollX;
			deshi__console_open_pressed = 1;
		}break;
		case ConsoleState_OpenBig:{
			deshi__console.open_target = (f32)DeshWindow->height * deshi__console.open_max_percent;
			deshi__console.open_amount = deshi__console.console_dim.y;
			deshi__console.console_pos = vec2(0, -1);
			deshi__console.console_dim.x = (f32)DeshWindow->width;
			deshi__console_window_flags = UIWindowFlags_NoMove | UIWindowFlags_NoResize | UIWindowFlags_NoScrollX;
			deshi__console_open_pressed = 1;
		}break;
		case ConsoleState_Popout:{
			deshi__console.open_target = deshi__console.console_dim.y;
			deshi__console.open_amount = 0;
			deshi__console_window_flags = 0;
			deshi__console_open_pressed = 1;
		}break;
	}
	deshi__console_open_timer = start_stopwatch();
	deshi__console.state = new_state;
}