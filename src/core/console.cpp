//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @internal
local Console console;
local CursorMode restore_cursor_mode;

void console_parse_message(str8 message, str8 tag, Type type, b32 from_logger, u32 logger_offset){DPZoneScoped;
	if(!console.initialized) return;
	if(message.count == 0) return;
	
	if(console.automatic_scroll){
		console.scroll_to_bottom = true;
	}
	
	u64 log_offset = (from_logger?logger_offset:console.logger->file->bytes);
	ConsoleChunk chunk = {type,tag,Color_White,Color_Blank};
	chunk.newline = 1;
	u8* chunk_start = message.str;
	str8 stream = message;
	while(stream){
		DecodedCodepoint dc = str8_advance(&stream);
		switch(dc.codepoint){
			case '\n':{
				chunk.start = log_offset;
				chunk.size = stream.str - dc.advance - chunk_start;
				console.dictionary.add(chunk);
				log_offset += chunk.size+1;
				chunk = {type,tag,chunk.fg,chunk.bg};
				chunk.newline = 1;
				chunk_start = stream.str;
			}break;
			
			case '\x1b':{
				u8* escstart = stream.str - 1;
				ConsoleChunk nuchunk = chunk;
				nuchunk.newline = 0;
				DecodedCodepoint dcnext = str8_advance(&stream);
				if(dcnext.codepoint != '[') continue;
				str8 numstr = str8_eat_int(stream);
				s32 num = stoi(numstr);
				str8_increment(&stream, numstr.count);
				switch(num){
					case 0:   nuchunk.fg = Color_White, nuchunk.bg = Color_Blank; break;
					case 1:   NotImplemented; //TODO(sushi) implement bold
					case 22:  NotImplemented; 
					case 4:   NotImplemented; //TODO(sushi) implement underline
					case 24:  NotImplemented;
					//the following two just do the same thing, as 27 is meant to undo 7, but i dont feel like tracking that
					case 7:   {color save=nuchunk.fg;nuchunk.fg=nuchunk.bg;nuchunk.bg=save;} break;
					case 27:  {color save=nuchunk.fg;nuchunk.fg=nuchunk.bg;nuchunk.bg=save;} break;
					case 30:  nuchunk.fg = Color_Black;        break;
					case 31:  nuchunk.fg = Color_Red;          break;
					case 32:  nuchunk.fg = Color_Green;        break;
					case 33:  nuchunk.fg = Color_Yellow;       break;
					case 34:  nuchunk.fg = Color_Blue;         break;
					case 35:  nuchunk.fg = Color_Magenta;      break;
					case 36:  nuchunk.fg = Color_Cyan;         break;
					case 37:  nuchunk.fg = Color_White;        break;
					case 39:  nuchunk.fg = Color_White;        break;
					case 40:  nuchunk.bg = Color_Black;        break;
					case 41:  nuchunk.bg = Color_Red;          break;
					case 42:  nuchunk.bg = Color_Green;        break;
					case 43:  nuchunk.bg = Color_Yellow;       break;
					case 44:  nuchunk.bg = Color_Blue;         break;
					case 45:  nuchunk.bg = Color_Magenta;      break;
					case 46:  nuchunk.bg = Color_Cyan;         break;
					case 47:  nuchunk.bg = Color_White;        break;
					case 49:  nuchunk.bg = Color_White;        break;
					case 90:  nuchunk.fg = Color_LightGrey;    break;
					case 91:  nuchunk.fg = Color_LightRed;     break;
					case 92:  nuchunk.fg = Color_LightGreen;   break;
					case 93:  nuchunk.fg = Color_LightYellow;  break;
					case 94:  nuchunk.fg = Color_LightBlue;    break;
					case 95:  nuchunk.fg = Color_LightMagenta; break;
					case 96:  nuchunk.fg = Color_LightCyan;    break;
					case 97:  nuchunk.fg = Color_White;        break;
					case 100: nuchunk.bg = Color_LightGrey;    break;
					case 101: nuchunk.bg = Color_LightRed;     break;
					case 102: nuchunk.bg = Color_LightGreen;   break;
					case 103: nuchunk.bg = Color_LightYellow;  break;
					case 104: nuchunk.bg = Color_LightBlue;    break;
					case 105: nuchunk.bg = Color_LightMagenta; break;
					case 106: nuchunk.bg = Color_LightCyan;    break;
					case 107: nuchunk.bg = Color_White;        break;
					case 38:
					case 48:{
						DecodedCodepoint dc = str8_advance(&stream);
						if(dc.codepoint != ';') continue; //invalid code, just continue
						dc = str8_advance(&stream);
						if(dc.codepoint == '2'){
							dc = str8_advance(&stream);
							if(dc.codepoint != ';') continue; //invalid code, just continue
							str8 r = str8_eat_int(stream);
							str8_increment(&stream,r.count);
							dc = str8_advance(&stream);
							if(dc.codepoint != ';') continue; //invalid code, just continue
							str8 g = str8_eat_int(stream);
							str8_increment(&stream,g.count);
							dc = str8_advance(&stream);
							if(dc.codepoint != ';') continue; //invalid code, just continue
							str8 b = str8_eat_int(stream);
							str8_increment(&stream,b.count);
							dc = str8_advance(&stream);
							if(dc.codepoint != ';') continue; //invalid code, just continue
							
							if(num == 38) nuchunk.fg = color(stoi(r),stoi(g),stoi(b));
							else          nuchunk.bg = color(stoi(r),stoi(g),stoi(b));
						}else if(dc.codepoint == '5'){
							NotImplemented; //we dont support color tables
						}else continue; //invalid code
					}break; 
				}
				dc = str8_advance(&stream);
				if(dc.codepoint != 'm') continue;
				chunk.start = log_offset;
				chunk.size = escstart - chunk_start;
				log_offset += chunk.size + (stream.str - escstart);
				console.dictionary.add(chunk);
				chunk = nuchunk; 
				chunk_start = stream.str;
			}break;
		}
	}
	if(chunk_start != stream.str){
		//add the last chunk if it exists
		chunk.start = log_offset;
		chunk.size = stream.str - chunk_start;
		console.dictionary.add(chunk);
	}
	if(!from_logger) Log("", message);
}

//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @interface
void console_init(){DPZoneScoped;
	DeshiStageInitStart(DS_CONSOLE, DS_UI|DS_ASSETS, "Attempted to initialize Console module before initializing the UI or Assets module");
	Assert(window_windows.count > 0, "Attempted to initialize Console module before a window was created");
	
	console.logger = logger_expose();
	console.logger->mirror_to_console = true;
	console.dictionary.init(512, deshi_allocator);
	console.input_history.init(256, deshi_allocator);
	console.console_pos = vec2::ZERO;
	console.console_dim = Vec2(f32(DeshWindow->width), 0);
	console.scroll_to_bottom = true;
	console.chunk_render_arena = memory_create_arena(512);
	console.state = ConsoleState_Closed;
	
	uiStyle base = {0};
	base.font = assets_font_create_from_file_bdf(STR8("gohufont-11.bdf"));
	base.font_height = 11;
	base.text_color = color(255,255,255);
	base.overflow = overflow_scroll;
	base.text_wrap = text_wrap_none;
	base.tab_spaces = 4;
	base.sizing = size_normal;
	
	//initialize ui elements
	console.ui.main = ui_begin_item(&base);
	uiItem*  main  = console.ui.main;
	uiStyle* mains = &main->style;
	main->id = STR8("console.main");
	mains-> background_color = color(14,14,14);
	mains->     border_style = border_solid;
	mains->     border_width = 1;
	mains->          display = display_flex;
	mains->           sizing = size_percent;
	mains->            width = 100;
	mains->           height = 0;
	mains->        paddingtl = {2,2};
	mains->        paddingbr = {2,2};
	mains->      positioning = pos_fixed;
	
	console.ui.buffer = ui_begin_item(&base);
	uiItem*  buffer  = console.ui.buffer;
	uiStyle* buffers = &buffer->style;
	buffer->id = STR8("console.buffer");
	buffers->           sizing = size_flex | size_percent_x; //fills the space not occupied by input box
	buffers->           height = 1; //occupy as much vertical space as it can in the container
	buffers->            width = 100; //100% the width of the container
	buffers-> background_color = Color_Black;
	buffers->        paddingtl = {2,2};
	buffers->        paddingbr = {2,2};
	ui_end_item();
	
	console.ui.inputbox = ui_begin_item(&base);
	uiItem*  inputb  = console.ui.inputbox;
	uiStyle* inputbs = &inputb->style;
	inputb->id = STR8("console.inputbox");
	inputbs->background_color = Color_DarkGray;
	inputbs->sizing = size_percent_x; //this element's size is static so it doesnt flex
	inputbs->width = 100;
	inputbs->height = inputbs->font->max_height + 2;
	inputbs->content_align = {0.5,0.5};
	inputbs->tab_spaces = 2;
	inputbs->paddingtl = {2,2};
	inputbs->paddingbr = {2,2};
	console.ui.inputtext = ui_make_input_text(str8null, 0);
	uiItem* inputt = console.ui.inputtext;
	uiStyle* inputts = &inputt->style;
	inputt->id = STR8("console.inputtext");
	inputts->sizing = size_percent;
	inputts->size = {100,100};
	uiInputText* it = (uiInputText*)inputt;	
	ui_end_item();
	ui_end_item();
	
	console.initialized = true;
	DeshiStageInitEnd(DS_CONSOLE);
}

void console_update(){DPZoneScoped;
	//check for console state changing inputs
	if(key_pressed(Key_BACKQUOTE)){
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
	if(g_ui->active == console.ui.inputtext && key_pressed(Key_UP)){
		LogE("console","input history isnt implemented yet!");
		change_input = true;
		console.input_history_index--;
		if(console.input_history_index == -2) console.input_history_index = console.input_history.count-1;
	}
	if(g_ui->active == console.ui.inputtext && key_pressed(Key_DOWN)){
		LogE("console","input history isnt implemented yet!");
		change_input = true;
		console.input_history_index++;
		if(console.input_history_index >= console.input_history.count) console.input_history_index = -1;
	}
	if(change_input){
		simulate_key_press(Key_END);
		text_clear(&ui_get_input_text(console.ui.inputtext)->text);
		
		// if(console.input_history_index != -1){
		// 	u32 cursor = 0;
		// 	u32 chunk_idx = console.input_history[console.input_history_index].first;
		// 	ConsoleChunk& chunk = console.dictionary[chunk_idx];
		
		// 	u64 restore = console.logger->file->cursor;
		// 	for(;;){
		// 		u32 characters = (cursor + chunk.size > CONSOLE_INPUT_BUFFER_SIZE)
		// 			? CONSOLE_INPUT_BUFFER_SIZE - (cursor + chunk.size) : chunk.size;
		// 		file_set_cursor(console.logger->file, chunk.start);
		// 		file_read(console.logger->file, console.input_buffer, characters);
		// 		console.input_length += characters;
		// 		cursor += characters;
		
		// 		if(chunk_idx >= console.input_history[console.input_history_index].second || cursor >= CONSOLE_INPUT_BUFFER_SIZE){
		// 			break;
		// 		}else{
		// 			chunk = console.dictionary[++chunk_idx];
		// 		}
		// 	}
		// 	file_set_cursor(console.logger->file, restore);
		// }
	}
	
	if(ui_item_hovered(console.ui.buffer, 0)){
		if(DeshInput->scrollY < 0 && console.scroll < console.dictionary.count){
			forI(-DeshInput->scrollY){console.scroll++;
				while(console.scroll < console.dictionary.count && !console.dictionary[console.scroll].newline) console.scroll++;
			}
		}else if(DeshInput->scrollY > 0 && console.scroll){
			forI(DeshInput->scrollY){console.scroll--;
				while(console.scroll > 0 && !console.dictionary[console.scroll].newline) console.scroll--;
			}
		}
		console.scroll = Max(0,console.scroll);
	}
	
	u32 linestofit = console.ui.buffer->height / console.ui.buffer->style.font_height;
	
	if(console.scroll_to_bottom){
		console.scroll_to_bottom = 0;
		u32 rem = linestofit;
		u32 idx = console.dictionary.count-1;
		while(rem){
			if(console.dictionary[idx].newline) rem--;
			idx--;
			if(!idx) break;
		}
		console.scroll = (idx?idx+1:0);
	}
	
	//console openness
	if(console.open_target != console.open_amount){
		console.ui.main->style.height = Math::lerp(console.open_amount, console.open_target, //TODO(sushi) change this to Nudge
												   Min((f32)peek_stopwatch(console.open_timer) / console.open_dt, 1.f));
		if(console.ui.main->style.height/100 == console.open_target){
			console.open_amount = console.open_target;
		}
	}
	
	
	// uiImmediateBP(console.ui.buffer);{
	// 	vec2 cursor = vec2::ZERO;
	// 	uiItem* line = uiItemB();
	// 	line->style.display = display_horizontal;
	// 	line->style.sizing = size_percent_x;
	// 	line->style.size = {100, f32(console.ui.buffer->style.font_height)};
	// 	line->id = STR8("console.line0");
	// 	u64 nlines = 0;
	// 	u64 i = console.scroll;
	// 	while(nlines < linestofit){
	// 		if(i>=console.dictionary.count) break;
	// 		//if console_chunk_render_arena isn't large enough, double the space for it
	// 		if(console.dictionary[i].size >= console.chunk_render_arena->size){
	// 			console.chunk_render_arena = memory_grow_arena(console.chunk_render_arena, console.chunk_render_arena->size);
	// 		}
	
	// 		if(console.dictionary[i].newline == 1 && nlines++){
	// 			uiItemE(); 
	// 			line = uiItemBS(&line->style);
	// 			line->id = to_dstr8v(deshi_temp_allocator, "console.line",nlines);
	// 		}
	
	// 		//get chunk text from the log file
	// 		file_set_cursor(console.logger->file, console.dictionary[i].start);
	// 		file_read(console.logger->file, console.chunk_render_arena->start, console.dictionary[i].size);
	// 		console.chunk_render_arena->start[console.dictionary[i].size] = '\0';
	
	// 		str8 out = {(u8*)console.chunk_render_arena->start, (s64)console.dictionary[i].size};
	// 		uiItem* text = uiTextMS(&line->style, out);
	// 		text->id = to_dstr8v(deshi_temp_allocator, "console.text",i);
	// 		text->style.text_color = console.dictionary[i].fg;
	
	
	// 		i++;
	// 	}
	// 	uiItemE();
	
	// 	uiItem* debug = uiItemB();
	// 	debug->style.positioning = pos_absolute;
	// 	debug->style.anchor = anchor_bottom_right;
	// 	debug->style.sizing = size_auto;
	// 	debug->id = STR8("console.debug");
	// 	uiTextM(to_dstr8v(deshi_temp_allocator,
	// 					  "      show tags: ", console.tag_show, "\n",
	// 					  " highlight tags: ", console.tag_highlighting, "\n",
	// 					  "   outline tags: ", console.tag_outlines, "\n",
	// 					  "highlight lines: ", console.line_highlighing, "\n",
	// 					  "    auto scroll: ", console.automatic_scroll, "\n",
	// 					  "          state: ", console.state, "\n",
	// 					  "   small open %: ", console.open_small_percent, "\n",
	// 					  "     max open %: ", console.open_max_percent, "\n",
	// 					  "    open amount: ", console.open_amount, "\n",
	// 					  "    open target: ", console.open_target, "\n",
	// 					  "open delta time: ", console.open_dt, "\n",
	// 					  "         scroll: ", console.scroll
	// 					  ));
	// 	uiItemE();
	// }uiImmediateE();
	
	if(g_ui->active == console.ui.inputtext && key_pressed(Key_ENTER)){
		str8 command = ui_get_input_text(console.ui.inputtext)->text.buffer.fin;
		Log("", CyanFormat("> "), command);
		cmd_run(command);
		text_clear(&ui_get_input_text(console.ui.inputtext)->text);
		console.scroll_to_bottom = 1;
	}
}

Console* console_expose(){DPZoneScoped;
	return &console;
}

void console_change_state(ConsoleState new_state){DPZoneScoped;
	if(console.state == new_state) new_state = ConsoleState_Closed;
	
	if(console.state == ConsoleState_Closed){
		restore_cursor_mode = g_window->cursor_mode;
	}
	
	switch(new_state){
		case ConsoleState_Closed:{
			console.open_target = 0;
			console.open_amount = console.ui.main->style.height;
			window_set_cursor_mode(g_window, restore_cursor_mode);
		}break;
		case ConsoleState_OpenSmall:{
			console.open_target = 100 * console.open_small_percent;
			console.open_amount = console.ui.main->style.height;
			console.console_pos = Vec2(0, -1);
			console.console_dim.x = (f32)DeshWindow->width;
			g_ui->active = console.ui.inputtext;
			window_set_cursor_mode(g_window, CursorMode_Default);
		}break;
		case ConsoleState_OpenBig:{
			console.open_target = 100 * console.open_max_percent;
			console.open_amount = console.ui.main->style.height;
			console.console_pos = Vec2(0, -1);
			console.console_dim.x = (f32)DeshWindow->width;
			g_ui->active = console.ui.inputtext;
			window_set_cursor_mode(g_window, CursorMode_Default);
		}break;
		// case ConsoleState_Popout:{
		// 	console.open_target = console.console_dim.y;
		// 	console.open_amount = 0;
		// }break;
	}
	console.open_timer = start_stopwatch();
	console.state = new_state;
}

b32
console_is_open()
{
	return console.state != ConsoleState_Closed;
}
