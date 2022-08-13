//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @internal
local Console console;

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
		case str8_static_hash64("dmagen" ): return Color_DarkMagenta;
		case str8_static_hash64("white"  ): return Color_White;
		case str8_static_hash64("black"  ): return Color_Black;
		default:
		LogW("console","Unknown color in chunk formatting: ", s);
		return Color_White;
	}
}

void console_send_to_logger(str8 message, b32 newline){DPZoneScoped;
	b32 restore_mirror  = console.logger->mirror_to_console;
	b32 restore_tag     = console.logger->ignore_tags;
	b32 restore_newline = console.logger->auto_newline;
	b32 restore_track   = console.logger->track_caller;
	console.logger->mirror_to_console = false;
	console.logger->ignore_tags       = true;
	console.logger->auto_newline      = newline;
	console.logger->track_caller      = false;
	logger_format_log(str8_lit(__FILE__),__LINE__,str8{},LogType_Normal,str8_lit("%.*s"),message.count,message.str);
	console.logger->mirror_to_console = restore_mirror;
	console.logger->ignore_tags       = restore_tag;
	console.logger->auto_newline      = restore_newline;
	console.logger->track_caller      = restore_track;
}

void console_parse_message(str8 message){DPZoneScoped;
	if(message.count == 0) return;
	
	if(console.automatic_scroll){
		console.scroll_to_bottom = true;
	}
	
	ConsoleChunk chunk{ConsoleChunkType_Normal, {}, Color_White};
	u64 log_offset = console.logger->file->bytes;
	u8* chunk_start = message.str;
	str8 remaining = message;
	while(remaining){
		DecodedCodepoint current = str8_advance(&remaining);
		if(current.codepoint == '\n'){
			chunk.start   = log_offset;
			chunk.size    = remaining.str - current.advance - chunk_start;
			chunk.newline = true;
			console.dictionary.add(chunk);
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
				console.dictionary.add(chunk); //TODO(delle) maybe append to prev chunk if no difference in formatting
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
					console.dictionary.add(chunk);
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
						console.dictionary.add(chunk);
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
										console.dictionary.add(chunk);
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
											console.dictionary.add(chunk);
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
									console.dictionary.add(chunk);
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
	DeshiStageInitStart(DS_CONSOLE, DS_LOGGER, "Attempted to initialize Console module before initializing the Logger module");
	Assert(window_windows.count > 0, "Attempted to initialize Console module before a window was created");
	
	console.logger = logger_expose();
	console.logger->mirror_to_console = true;
	console.dictionary.init(512, deshi_allocator);
	console.input_history.init(256, deshi_allocator);
	console.console_pos = vec2::ZERO;
	console.console_dim = Vec2(f32(DeshWindow->width), 0);
	console.scroll_to_bottom = true;
	console.chunk_render_arena = memory_create_arena(512);
	
	uiStyle base = {0};
	base.font = Storage::CreateFontFromFileBDF(STR8("gohufont-11.bdf")).second;
	base.font_height = 11;
	base.text_color = color(255,255,255);
	base.overflow = overflow_scroll;
	base.text_wrap = text_wrap_none;
	base.sizing = size_normal;

	//initialize ui elements
	console.ui.main = uiItemBS(&base);
		uiItem*  main  = console.ui.main;
		uiStyle* mains = &main->style;
		main->id = STR8("console.main");
		mains-> background_color = color(14,14,14);
		mains->     border_style = border_solid;
		mains->     border_width = 1;
		mains->          display = display_flex;
		mains->           sizing = size_percent;
		mains->            width = 100;
		mains->           height = 100 * console.open_max_percent;
		mains->          padding = {2,2,2,2};
		mains->      positioning = pos_fixed;

		console.ui.buffer = uiItemBS(&base);
			uiItem*  buffer  = console.ui.buffer;
			uiStyle* buffers = &buffer->style;
			buffer->id = STR8("console.buffer");
			buffers->           sizing = size_flex | size_percent_x; //fills the space not occupied by input box
			buffers->           height = 1; //occupy as much vertical space as it can in the container
			buffers->            width = 100; //100% the width of the container
			buffers-> background_color = Color_Black;
			buffers->          padding = {2,2,2,2};
		uiItemE();

		console.ui.inputbox = uiItemBS(&base);
			uiItem*  inputb  = console.ui.inputbox;
			uiStyle* inputbs = &inputb->style;
			inputb->id = STR8("console.inputbox");
			inputbs->background_color = Color_DarkGray;
			inputbs->sizing = size_percent_x; //this element's size is static so it doesnt flex
			inputbs->width = 100;
			inputbs->height = inputbs->font->max_height + 2;
			inputbs->content_align = {0.5,0.5};
			inputbs->tab_spaces = 2;
			inputbs->padding = {2,2,2,2};
			console.ui.inputtext = uiInputTextM();
				uiItem* inputt = console.ui.inputtext;
				uiStyle* inputts = &inputt->style;
				inputt->id = STR8("console.inputtext");
				inputts->sizing = size_percent;
				inputts->size = {100,100};
				uiInputText* it = uiGetInputText(inputt);	
		uiItemE();
	uiItemE();


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
		console.input_history_index--;
		if(console.input_history_index == -2) console.input_history_index = console.input_history.count-1;
	}
	if(key_pressed(Key_DOWN)){
		change_input = true;
		console.input_history_index++;
		if(console.input_history_index >= console.input_history.count) console.input_history_index = -1;
	}
	if(change_input){
		simulate_key_press(Key_END);
		ZeroMemory(console.input_buffer, CONSOLE_INPUT_BUFFER_SIZE);
		console.input_length = 0;
		
		if(console.input_history_index != -1){
			u32 cursor = 0;
			u32 chunk_idx = console.input_history[console.input_history_index].first;
			ConsoleChunk& chunk = console.dictionary[chunk_idx];
			
			u64 restore = console.logger->file->cursor;
			for(;;){
				u32 characters = (cursor + chunk.size > CONSOLE_INPUT_BUFFER_SIZE)
					? CONSOLE_INPUT_BUFFER_SIZE - (cursor + chunk.size) : chunk.size;
				file_cursor(console.logger->file, chunk.start);
				file_read(console.logger->file, console.input_buffer, characters);
				console.input_length += characters;
				cursor += characters;
				
				if(chunk_idx >= console.input_history[console.input_history_index].second || cursor >= CONSOLE_INPUT_BUFFER_SIZE){
					break;
				}else{
					chunk = console.dictionary[++chunk_idx];
				}
			}
			file_cursor(console.logger->file, restore);
		}
	}

	if(ui_item_hovered(console.ui.buffer, 0)){
		console.scroll += DeshInput->scrollY;
		console.scroll = Max(0,console.scroll);
	Log("", DeshInput->scrollY);
	}

	
	//console openness
	if(console.open_target != console.open_amount){
		console.ui.main->style.height = Math::lerp(console.open_amount, console.open_target, //TODO(sushi) change this to Nudge
										   Min((f32)peek_stopwatch(console.open_timer) / console.open_dt, 1.f));
		if(console.ui.main->style.height/100 == console.open_target){
			console.open_amount = console.open_target;
		}
	}

	u32 linestofit = console.ui.buffer->height / console.ui.buffer->style.font_height;

	uiImmediateBP(console.ui.buffer);{
		vec2 cursor = vec2::ZERO;
		uiItem* line = uiItemB();
		line->style.display = display_row;
		line->style.sizing = size_percent_x;
		line->style.size = {100, f32(console.ui.buffer->style.font_height)};
		line->id = STR8("console.line0");
		u64 nlines = 0;
		u64 i = console.scroll;
		while(nlines < linestofit){
			if(i>=console.dictionary.count) break;
			//if console_chunk_render_arena isn't large enough, double the space for it
			if(console.dictionary[i].size >= console.chunk_render_arena->size){
				console.chunk_render_arena = memory_grow_arena(console.chunk_render_arena, console.chunk_render_arena->size);
			}
			
			//get chunk text from the log file
			file_cursor(console.logger->file, console.dictionary[i].start);
			file_read(console.logger->file, console.chunk_render_arena->start, console.dictionary[i].size);
			console.chunk_render_arena->start[console.dictionary[i].size] = '\0';
			
			str8 out = {(u8*)console.chunk_render_arena->start, (s64)console.dictionary[i].size};
			uiTextMS(&line->style, out)->id = toStr8("console.text",i);
			

			if(console.dictionary[i].newline == 1 && i != (console.dictionary.count-1)){
				uiItemE(); 
				line = uiItemBS(&line->style);
				nlines++;
				line->id = toStr8("console.line",nlines);
			}

			//adjust what we're going to print based on formatting
			if(HasFlag(console.dictionary[i].type, ConsoleChunkType_Alert)){
				//TODO handle alert flashing
			}



			i++;
			
			// if(console.tag_show && console.dictionary[i].tag){
			// 	if(i == 0 || !str8_equal(console.dictionary[i-1].tag, console.dictionary[i].tag)){
			// 		f32 top_edge    = UI::GetMarginedTop();
			// 		f32 right_edge  = UI::GetMarginedRight();
			// 		f32 tag_start_y = UI::GetWinCursor().y;
			// 		vec2 tag_size   = UI::CalcTextSize(console.dictionary[i].tag);
					
			// 		//look ahead for the end of the tag range
			// 		//dont draw if its not visible
			// 		f32 tag_end_y = tag_start_y;
			// 		for(int o = i; o < console.dictionary.count; o++){
			// 			if(str8_equal_lazy(console.dictionary[o].tag, console.dictionary[i].tag)){
			// 				tag_end_y += tag_size.y;
			// 			}else{
			// 				break;
			// 			}
			// 		}
			// 		if(tag_end_y > top_edge){
			// 			if(tag_end_y < tag_size.y){
			// 				tag_start_y = top_edge + (tag_end_y - tag_size.y);
			// 			}else{
			// 				tag_start_y = Max(tag_start_y, top_edge);
			// 			}
						
			// 			//draw tag text
			// 			vec2 restore_cursor = UI::GetWinCursor();
			// 			UI::PushColor(UIStyleCol_Text, color(150,150,150, 150));
			// 			UI::TextOld(console.dictionary[i].tag, Vec2(right_edge - tag_size.x - 1, tag_start_y));
			// 			UI::PopColor();
			// 			UI::SetWinCursor(restore_cursor);
						
			// 			if(console.tag_outlines && tag_end_y - (tag_start_y + tag_size.y) > 4){
			// 				//draw right line
			// 				UI::Line(Vec2(right_edge-1, tag_start_y+tag_size.y+3),
			// 							Vec2(right_edge-1, tag_end_y-3),
			// 							2, color(150,150,150, 150));
			// 				//draw top line
			// 				UI::Line(Vec2(right_edge-tag_size.x, tag_start_y+tag_size.y+2),
			// 							Vec2(right_edge,            tag_start_y+tag_size.y+2),
			// 							2, color(150,150,150, 150));
			// 				//draw bottom line
			// 				UI::Line(Vec2(right_edge-tag_size.x, tag_end_y-2),
			// 							Vec2(right_edge,            tag_end_y-2),
			// 							2, color(150,150,150, 150));
			// 			}
						
			// 			if(console.tag_highlighting){
			// 				UI::PushLayer(UI::GetCenterLayer()-1); //NOTE(delle) put the highlight on a lower layer than the text
			// 				UI::PushColor(UIStyleCol_SelectableBg,        Color_Clear);
			// 				UI::PushColor(UIStyleCol_SelectableBgHovered, color(155, 155, 155, 10));
			// 				UI::PushColor(UIStyleCol_SelectableBgActive,  color(155, 155, 155, 10));
			// 				UI::SetNextItemSize(Vec2(right_edge, tag_end_y - tag_start_y));
			// 				UI::SetNextItemMinSizeIgnored();
							
			// 				UI::Selectable(str8_lit(""), Vec2(0,tag_start_y), 0);
							
			// 				UI::PopColor(3);
			// 				UI::PopLayer();
			// 			}
						
			// 			UI::SetMarginSizeOffset(Vec2(-tag_size.x, 0));
			// 		}
			// 	}
			// }
			
			// UI::PushColor(UIStyleCol_Text, console.dictionary[i].color);
			// UI::TextOld(str8{(u8*)console_chunk_render_arena->start, (s64)console.dictionary[i].size});
			// UI::PopColor();
			
			
			
			// if(console.line_highlighing){
			// 	UIItem* last_text = UI::GetLastItem();
			// 	UI::PushLayer(UI::GetCenterLayer()-1); //NOTE(delle) put the highlight on a lower layer than the text
			// 	UI::PushColor(UIStyleCol_SelectableBg,        Color_Clear);
			// 	UI::PushColor(UIStyleCol_SelectableBgHovered, color(155,155,155, 10));
			// 	UI::PushColor(UIStyleCol_SelectableBgActive,  color(155,155,155, 10));
			// 	UI::SetNextItemSize(Vec2(UI::GetMarginedRight(), last_text->size.y));
			// 	UI::SetNextItemMinSizeIgnored();
				
			// 	UI::Selectable(str8_lit(""), Vec2(0, last_text->position.y), 0);
				
			// 	UI::PopColor(3);
			// 	UI::PopLayer();
			// }
		}
		uiItemE();
	}uiImmediateE();
	
	// UIStyle_old& style = UI::GetStyle();
	// if(console.console_dim.y > 0){
	// 	UI::SetNextWindowPos(console.console_pos);
	// 	UI::SetNextWindowSize(Vec2(DeshWindow->width, console.console_dim.y));
	// 	UI::Begin(str8_lit("deshiConsole"), vec2::ZERO, vec2::ZERO, console.window_flags | UIWindowFlags_NoScroll);
	// 	console.window = UI::GetWindow();
		
	// 	//-//////////////////////////////////////////////////////////////////////////////////////////////
	// 	//text input panel
	// 	//NOTE(delle) doing this above the terminal window so inputs are registered managed before visuals
	// 	if(console_open_pressed) UI::SetNextItemActive();
	// 	f32 input_box_height = style.inputTextHeightRelToFont * style.fontHeight;
	// 	UI::SetNextItemSize(Vec2(MAX_F32, input_box_height));
	// 	if(UI::InputText(str8_lit("deshiConsoleInput"), (u8*)console.input_buffer, CONSOLE_INPUT_BUFFER_SIZE-2,
	// 					 {UI::GetMarginedLeft(), UI::GetMarginedBottom() - input_box_height},
	// 					 str8_lit("Enter a command..."), UIInputTextFlags_AnyChangeReturnsTrue)){
	// 		if(console.input_buffer[console.input_length] != 0){
	// 			DecodedCodepoint decoded = decoded_codepoint_from_utf8(console.input_buffer+console.input_length, 4);
	// 			if(decoded.codepoint == '~' || decoded.codepoint == '`'){
	// 				console.input_buffer[console.input_length] = '\0'; //NOTE(delle) filter out console open/close inputs
	// 			}else{
	// 				console.input_length += decoded.advance;
	// 			}
	// 		}else{
	// 			while(   console.input_length
	// 				  && console.input_buffer[console.input_length-1] == 0){
	// 				console.input_length -= 1;
	// 			}
	// 		}
	// 	}
	// 	if(key_pressed(Key_ENTER) && console.input_length){
	// 		//append a \n to the input
	// 		console.input_buffer[console.input_length  ] = '\n';
	// 		console.input_buffer[console.input_length+1] = '\0';
			
	// 		//log and chunk the input symbol
	// 		u64 log_offset = console.logger->file->bytes;
	// 		console_send_to_logger(str8_lit("/\\ "), false);
	// 		console.dictionary.add({ConsoleChunkType_Normal, str8{}, Color_Cyan,     log_offset,   1, false});
	// 		console.dictionary.add({ConsoleChunkType_Normal, str8{}, Color_DarkCyan, log_offset+1, 2, false});
			
	// 		//send the input to the message parser
	// 		u32 start_chunk = console.dictionary.count; //NOTE(delle) checking this here b/c we don't know how many chunks input will create
	// 		console_parse_message(str8{console.input_buffer, console.input_length+1});
			
	// 		//record the input into the history
	// 		if(strncmp((char*)console.input_buffer, (char*)console.prev_input, CONSOLE_INPUT_BUFFER_SIZE) != 0){
	// 			CopyMemory(console.prev_input, console.input_buffer, CONSOLE_INPUT_BUFFER_SIZE);
	// 			console.input_history.add({start_chunk,console.dictionary.count-1});
	// 		}
			
	// 		cmd_run(str8{console.input_buffer, console.input_length});
	// 		ZeroMemory(console.input_buffer, console.input_length+1);
	// 		console_scroll_to_bottom = true;
	// 		console.input_history_index = -1;
	// 	}


		
	// 	//-//////////////////////////////////////////////////////////////////////////////////////////////
	// 	//terminal output panel
	// 	UI::SetNextWindowSize(Vec2(MAX_F32, UI::GetMarginedBottom() - (style.fontHeight * style.inputTextHeightRelToFont + style.itemSpacing.y) * 3));
	// 	UI::BeginChild(str8_lit("deshiConsoleTerminal"), (console.window->dimensions - 2*style.windowMargins).yAdd(-(1.3*style.fontHeight + style.itemSpacing.y)), console.window_flags);{
	// 		//draw text for the dictionary
	// 		UI::PushVar(UIStyleVar_WindowMargins, Vec2(5, 0));
	// 		UI::PushVar(UIStyleVar_ItemSpacing,   Vec2(0, 0));
			
	// 		forI(console.dictionary.count){
	// 			//if console_chunk_render_arena isn't large enough, double the space for it
	// 			if(console.dictionary[i].size >= console_chunk_render_arena->size){
	// 				console_chunk_render_arena = memory_grow_arena(console_chunk_render_arena, console_chunk_render_arena->size);
	// 			}
				
	// 			//get chunk text from the log file
	// 			file_cursor(console.logger->file, console.dictionary[i].start);
	// 			file_read(console.logger->file, console_chunk_render_arena->start, console.dictionary[i].size);
	// 			console_chunk_render_arena->start[console.dictionary[i].size] = '\0';
				
	// 			//adjust what we're going to print based on formatting
	// 			if(HasFlag(console.dictionary[i].type, ConsoleChunkType_Alert)){
	// 				//TODO handle alert flashing
	// 			}
				
	// 			if(console.tag_show && console.dictionary[i].tag){
	// 				if(i == 0 || !str8_equal(console.dictionary[i-1].tag, console.dictionary[i].tag)){
	// 					f32 top_edge    = UI::GetMarginedTop();
	// 					f32 right_edge  = UI::GetMarginedRight();
	// 					f32 tag_start_y = UI::GetWinCursor().y;
	// 					vec2 tag_size   = UI::CalcTextSize(console.dictionary[i].tag);
						
	// 					//look ahead for the end of the tag range
	// 					//dont draw if its not visible
	// 					f32 tag_end_y = tag_start_y;
	// 					for(int o = i; o < console.dictionary.count; o++){
	// 						if(str8_equal_lazy(console.dictionary[o].tag, console.dictionary[i].tag)){
	// 							tag_end_y += tag_size.y;
	// 						}else{
	// 							break;
	// 						}
	// 					}
	// 					if(tag_end_y > top_edge){
	// 						if(tag_end_y < tag_size.y){
	// 							tag_start_y = top_edge + (tag_end_y - tag_size.y);
	// 						}else{
	// 							tag_start_y = Max(tag_start_y, top_edge);
	// 						}
							
	// 						//draw tag text
	// 						vec2 restore_cursor = UI::GetWinCursor();
	// 						UI::PushColor(UIStyleCol_Text, color(150,150,150, 150));
	// 						UI::TextOld(console.dictionary[i].tag, Vec2(right_edge - tag_size.x - 1, tag_start_y));
	// 						UI::PopColor();
	// 						UI::SetWinCursor(restore_cursor);
							
	// 						if(console.tag_outlines && tag_end_y - (tag_start_y + tag_size.y) > 4){
	// 							//draw right line
	// 							UI::Line(Vec2(right_edge-1, tag_start_y+tag_size.y+3),
	// 									 Vec2(right_edge-1, tag_end_y-3),
	// 									 2, color(150,150,150, 150));
	// 							//draw top line
	// 							UI::Line(Vec2(right_edge-tag_size.x, tag_start_y+tag_size.y+2),
	// 									 Vec2(right_edge,            tag_start_y+tag_size.y+2),
	// 									 2, color(150,150,150, 150));
	// 							//draw bottom line
	// 							UI::Line(Vec2(right_edge-tag_size.x, tag_end_y-2),
	// 									 Vec2(right_edge,            tag_end_y-2),
	// 									 2, color(150,150,150, 150));
	// 						}
							
	// 						if(console.tag_highlighting){
	// 							UI::PushLayer(UI::GetCenterLayer()-1); //NOTE(delle) put the highlight on a lower layer than the text
	// 							UI::PushColor(UIStyleCol_SelectableBg,        Color_Clear);
	// 							UI::PushColor(UIStyleCol_SelectableBgHovered, color(155, 155, 155, 10));
	// 							UI::PushColor(UIStyleCol_SelectableBgActive,  color(155, 155, 155, 10));
	// 							UI::SetNextItemSize(Vec2(right_edge, tag_end_y - tag_start_y));
	// 							UI::SetNextItemMinSizeIgnored();
								
	// 							UI::Selectable(str8_lit(""), Vec2(0,tag_start_y), 0);
								
	// 							UI::PopColor(3);
	// 							UI::PopLayer();
	// 						}
							
	// 						UI::SetMarginSizeOffset(Vec2(-tag_size.x, 0));
	// 					}
	// 				}
	// 			}
				
	// 			UI::PushColor(UIStyleCol_Text, console.dictionary[i].color);
	// 			UI::TextOld(str8{(u8*)console_chunk_render_arena->start, (s64)console.dictionary[i].size});
	// 			UI::PopColor();
				
	// 			if(console.dictionary[i].newline == false && i != (console.dictionary.count-1)){
	// 				UI::SameLine();
	// 			}
				
	// 			if(console.line_highlighing){
	// 				UIItem* last_text = UI::GetLastItem();
	// 				UI::PushLayer(UI::GetCenterLayer()-1); //NOTE(delle) put the highlight on a lower layer than the text
	// 				UI::PushColor(UIStyleCol_SelectableBg,        Color_Clear);
	// 				UI::PushColor(UIStyleCol_SelectableBgHovered, color(155,155,155, 10));
	// 				UI::PushColor(UIStyleCol_SelectableBgActive,  color(155,155,155, 10));
	// 				UI::SetNextItemSize(Vec2(UI::GetMarginedRight(), last_text->size.y));
	// 				UI::SetNextItemMinSizeIgnored();
					
	// 				UI::Selectable(str8_lit(""), Vec2(0, last_text->position.y), 0);
					
	// 				UI::PopColor(3);
	// 				UI::PopLayer();
	// 			}
	// 		}
	// 		UI::PopVar(2);
			
	// 		if(console_scroll_to_bottom){
	// 			UI::SetScroll(Vec2(0, MAX_F32));
	// 			console_scroll_to_bottom = false;
	// 		}
	// 	}UI::EndChild();
		
	// 	UI::PushColor(UIStyleCol_WindowBg, color(0,25,18));
	// 	UI::End();
	// 	UI::PopColor();
	// }
	
	// console_open_pressed = false;
}

Console* console_expose(){
	return &console;
}

void console_change_state(ConsoleState new_state){
	if(console.state == new_state) new_state = ConsoleState_Closed;
	
	switch(new_state){
		case ConsoleState_Closed:{
			console.open_target = 0;
			console.open_amount = console.console_dim.y;
		}break;
		case ConsoleState_OpenSmall:{
			console.open_target = 100 * console.open_small_percent;
			console.open_amount = console.console_dim.y;
			console.console_pos = Vec2(0, -1);
			console.console_dim.x = (f32)DeshWindow->width;
		}break;
		case ConsoleState_OpenBig:{
			console.open_target = 100 * console.open_max_percent;
			console.open_amount = console.console_dim.y;
			console.console_pos = Vec2(0, -1);
			console.console_dim.x = (f32)DeshWindow->width;
		}break;
		// case ConsoleState_Popout:{
		// 	console.open_target = console.console_dim.y;
		// 	console.open_amount = 0;
		// }break;
	}
	console.open_timer = start_stopwatch();
	console.state = new_state;
}