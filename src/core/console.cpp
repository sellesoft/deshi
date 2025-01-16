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
				console.total_lines += 1;
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
		console.total_lines += 1;
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
	console.console_dim = Vec2((f32)g_window->width, 0);
	console.scroll_to_bottom = true;
	console.chunk_render_arena = memory_create_arena(512);
	console.state = ConsoleState_Closed;
	
	Font* default_font = assets_font_get_by_name(STR8("baked_gohufont_11_bdf"));
	if(!default_font){
		default_font = assets_font_create_from_memory_bdf(baked_font_gohufont_11_bdf.str, baked_font_gohufont_11_bdf.count, STR8("baked_gohufont_11_bdf"));
	}
	
	//initialize ui elements
	console.ui.main = ui_begin_item(0);{
		console.ui.main->id = STR8("console.main");
		console.ui.main->style.display = display_flex;
		console.ui.main->style.positioning = pos_fixed;
		console.ui.main->style.sizing = size_percent;
		console.ui.main->style.width = 100;
		console.ui.main->style.height = 0;
		console.ui.main->style.padding = {2,2,2,2};
		console.ui.main->style.background_color = color(14,14,14);
		console.ui.main->style.font = default_font;
		console.ui.main->style.font_height = 11;
		console.ui.main->style.text_color = Color_White;
		
		console.ui.buffer = ui_begin_item(0);{
			console.ui.buffer->id = STR8("console.main.buffer");
			console.ui.buffer->style.sizing = size_flex | size_percent_x; //fills the space not occupied by input box
			console.ui.buffer->style.height = 1; //occupy as much vertical space as it can in the container
			console.ui.buffer->style.width = 100; //100% the width of the container
			console.ui.buffer->style.padding = {2,2,2,2};
			console.ui.buffer->style.background_color = Color_Black;
		}ui_end_item();
		
		console.ui.inputbox = ui_begin_item(0);{
			console.ui.inputbox->id = STR8("console.main.inputbox");
			console.ui.inputbox->style.background_color = Color_DarkGray;
			console.ui.inputbox->style.sizing = size_percent_x; //this element's size is static so it doesnt flex
			console.ui.inputbox->style.width = 100;
			console.ui.inputbox->style.height = console.ui.inputbox->style.font->max_height + 2;
			console.ui.inputbox->style.padding = {2,2,2,2};
			console.ui.inputbox->style.content_align = {0.5,0.5};
			console.ui.inputbox->style.overflow = overflow_scroll;
			console.ui.inputbox->style.tab_spaces = 2;
			console.ui.inputbox->style.text_wrap = text_wrap_none;
			
			console.ui.inputtext = ui_make_input_text(str8null, 0);
			console.ui.inputtext->id = STR8("console.main.inputbox.inputtext");
			console.ui.inputtext->style.sizing = size_percent;
			console.ui.inputtext->style.size = {100,100};
		}ui_end_item();
	}ui_end_item();
	
	console.total_lines = 0;
	console.initialized = true;
	DeshiStageInitEnd(DS_CONSOLE);
}

void console_update(){DPZoneScoped;
	Assert(DeshiModuleLoaded(DS_CONSOLE), "console_init() must be called before console_update().");
	
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
	
	//HACK(delle) hacky way of preventing tilde inputs when opening/closing the console
	//  banned inputs should maybe be a feature of the widget
	Text* text = &ui_get_input_text(console.ui.inputtext)->text;
	if(text->buffer.count > 0 && (   text->buffer.str[text->buffer.count-1] == '`'
								  || text->buffer.str[text->buffer.count-1] == '~')){
		text_move_cursor_to_end(text, false);
		text_delete_left(text);
	}
	
	//console openness
	if(console.open_target != console.open_amount){
		f32 t = Min((f32)peek_stopwatch(console.open_timer) / console.open_dt, 1.0f);
		console.ui.main->style.height = Math::lerp(console.open_amount, console.open_target, t);
		if(console.ui.main->style.height / 100 == console.open_target){
			console.open_amount = console.open_target;
		}
	}
	u32 visible_lines = console.ui.buffer->height / (console.ui.buffer->style.font_height + console.ui.buffer->style.content_advance);
	
	if(console_is_open()){
		//ensure that the console is the topmost UI item
		if((&console.ui.main->node != g_ui->base.node.last_child)){
			move_to_parent_last(&console.ui.main->node);
		}
		
		//handle tab completion
		if(g_ui->active == console.ui.inputtext && key_pressed(Key_TAB)){
			Text* text = &ui_get_input_text(console.ui.inputtext)->text;
			str8 input = ui_input_text_peek(console.ui.inputtext);
			
			//only try to complete if there's input and we're at the end of the input
			if(input.count > 0 && text->cursor.pos == input.count){
				//find commands that start with the input
				Command* commands = cmd_list();
				str8* matches = array_create(str8, 16, deshi_temp_allocator);
				
				for_array(commands){
					if(str8_begins_with(it->name, input)){
						array_push_value(matches, it->name);
					}
				}
				
				upt matches_count = array_count(matches);
				if(matches_count == 1){
					//exactly one match - complete it
					text_clear(text);
					text_insert_string(text, matches[0]);
				}else if(matches_count > 1){
					//multiple matches - show them
					dstr8 builder;
					dstr8_init(&builder, STR8("Matching commands:\n"), deshi_temp_allocator);
					for_array(matches){
						dstr8_append(&builder, STR8("  "));
						dstr8_append(&builder, *it);
						dstr8_append(&builder, STR8("\n"));
					}
					Log("console", dstr8_peek(&builder));
					console.scroll_to_bottom = true;
				}
			}
		}
		
		//next/previous input box history
		if(g_ui->active == console.ui.inputtext && console.input_history.count > 0){
			if(key_pressed(Key_UP)){
				console.input_history_index -= 1;
				if(console.input_history_index == -2){
					console.input_history_index = console.input_history.count-1;
				}
				
				if(console.input_history_index >= console.input_history.count){
					console.input_history_index = -1;
					text_clear(&ui_get_input_text(console.ui.inputtext)->text);
				}else if(console.input_history_index != -1){
					text_clear(&ui_get_input_text(console.ui.inputtext)->text);
					text_insert_string(&ui_get_input_text(console.ui.inputtext)->text, console.input_history[console.input_history_index]);
				}
			}else if(key_pressed(Key_DOWN)){
				console.input_history_index += 1;
				
				if(console.input_history_index >= console.input_history.count){
					console.input_history_index = -1;
					text_clear(&ui_get_input_text(console.ui.inputtext)->text);
				}else if(console.input_history_index != -1){
					text_clear(&ui_get_input_text(console.ui.inputtext)->text);
					text_insert_string(&ui_get_input_text(console.ui.inputtext)->text, console.input_history[console.input_history_index]);
				}
			}
		}
		
		//dont allow scrolling if content fits on screen
		if(console.total_lines <= visible_lines){
			console.scroll = 0;
		}else if(key_pressed(Key_PAGEDOWN) && (console.scroll < console.dictionary.count)){
			if(input_ctrl_down()){
				console.scroll = console.dictionary.count;
			}else{
				console.scroll += 1;
				while(console.scroll < console.dictionary.count && !console.dictionary[console.scroll].newline){
					console.scroll += 1;
				}
			}
		}else if(key_pressed(Key_PAGEUP) && (console.scroll > 0)){
			if(input_ctrl_down()){
				console.scroll = 0;
			}else{
				console.scroll -= 1;
				while(console.scroll > 0 && !console.dictionary[console.scroll].newline){
					console.scroll -= 1;
				}
			}
		}else if(ui_item_hovered(console.ui.buffer, hovered_area)){
			if(g_input->scrollY < 0 && console.scroll < console.dictionary.count){
				forI(-g_input->scrollY){
					console.scroll += 1;
					while(console.scroll < console.dictionary.count && !console.dictionary[console.scroll].newline){
						console.scroll += 1;
					}
				}
			}else if(g_input->scrollY > 0 && console.scroll > 0){
				forI(g_input->scrollY){
					console.scroll -= 1;
					while(console.scroll > 0 && !console.dictionary[console.scroll].newline){
						console.scroll -= 1;
					}
				}
			}
		}
	}
	
	ui_begin_immediate_branch(console.ui.buffer);{
		vec2 cursor = vec2::ZERO;
		if(visible_lines > 0 && console.dictionary.count > 0){
			uiItem* line = ui_begin_item(0);
			line->id = STR8("console.line0");
			line->style.display = display_horizontal;
			line->style.sizing = size_percent_x;
			line->style.size = {100, f32(console.ui.buffer->style.font_height)};
			
			if(console.scroll_to_bottom){
				console.scroll_to_bottom = false;
				console.scroll = console.dictionary.count-1;
			}
			
			//calculate max scroll by counting actual newlines to find the last visible line
			u32 max_scroll = console.dictionary.count-1;
			u32 lines_from_bottom = 0;
			for(s32 i = console.dictionary.count-1; i >= 0; --i){
				if(console.dictionary[i].newline){
					lines_from_bottom += 1;
					if(lines_from_bottom >= visible_lines){
						max_scroll = i;
						break;
					}
				}
			}
			console.scroll = ClampMax(console.scroll, max_scroll);
			
			//find the first line that should be visible based on scroll
			u32 line_index = 0;
			u32 dict_index = console.scroll;
			while(line_index < visible_lines && dict_index < console.dictionary.count){
				//if console_chunk_render_arena isn't large enough, double the space for it
				//NOTE(delle) using >= because we need to account for the null terminator
				//TODO(delle) do we really need to have a null terminator?
				if(console.chunk_render_arena->used + console.dictionary[dict_index].size >= console.chunk_render_arena->size){
					console.chunk_render_arena = memory_grow_arena(console.chunk_render_arena, console.chunk_render_arena->size);
				}
				
				if(console.dictionary[dict_index].newline && line_index++ != 0){
					ui_end_item();
					line = ui_begin_item(&line->style);
					line->id = to_dstr8v(deshi_temp_allocator, "console.line",line_index).fin;
				}
				
				//get chunk text from the log file
				file_set_cursor(console.logger->file, console.dictionary[dict_index].start);
				file_read(console.logger->file, console.chunk_render_arena->start, console.dictionary[dict_index].size);
				console.chunk_render_arena->start[console.dictionary[dict_index].size] = '\0';
				
				str8 out = {(u8*)console.chunk_render_arena->start, (s64)console.dictionary[dict_index].size};
				uiItem* text = ui_make_text(out, &line->style);
				text->id = to_dstr8v(deshi_temp_allocator, "console.text",dict_index).fin;
				text->style.text_color = console.dictionary[dict_index].fg;
				line->style.background_color = console.dictionary[dict_index].bg;
				
				dict_index += 1;
			}
			
			ui_end_item();
		}
		
		uiItem* debug = ui_begin_item(0);
		debug->style.positioning = pos_absolute;
		debug->style.anchor = anchor_bottom_right;
		debug->style.sizing = size_auto;
		debug->id = STR8("console.debug");
		ui_make_text(to_dstr8v(deshi_temp_allocator,
							   "      show tags: ", console.tag_show, "\n",
							   " highlight tags: ", console.tag_highlighting, "\n",
							   "   outline tags: ", console.tag_outlines, "\n",
							   "highlight lines: ", console.line_highlighing, "\n",
							   "    auto scroll: ", console.automatic_scroll, "\n",
							   "          state: ", console.state, "\n",
							   "   small open %: ", console.open_small_percent, "\n",
							   "     max open %: ", console.open_max_percent, "\n",
							   "    open amount: ", console.open_amount, "\n",
							   "    open target: ", console.open_target, "\n",
							   "open delta time: ", console.open_dt, "\n",
							   "         scroll: ", console.scroll).fin,
					 0);
		ui_end_item();
	}ui_end_immediate_branch();
	
	if(g_ui->active == console.ui.inputtext && key_pressed(Key_ENTER)){
		str8 command = ui_input_text_peek(console.ui.inputtext);
		
		if(command.count > 0 && (console.input_history.count == 0 || !str8_equal_lazy(command, console.input_history[console.input_history.count-1]))){
			if(console.input_history.count == console.input_history.capacity){
				memory_zfree(console.input_history[console.input_history.start].str);
			}
			console.input_history.add(str8_copy(command));
			console.input_history_index = -1;
		}
		
		Log("", CyanFormat("> "), command);
		cmd_run(command);
		
		text_clear(&ui_get_input_text(console.ui.inputtext)->text);
		console.scroll_to_bottom = true;
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
			console.console_pos = vec2::ZERO;
			console.console_dim.x = (f32)g_window->width;
			ui_set_active_item(console.ui.inputtext);
			window_set_cursor_mode(g_window, CursorMode_Default);
		}break;
		case ConsoleState_OpenBig:{
			console.open_target = 100 * console.open_max_percent;
			console.open_amount = console.ui.main->style.height;
			console.console_pos = vec2::ZERO;
			console.console_dim.x = (f32)g_window->width;
			ui_set_active_item(console.ui.inputtext);
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
