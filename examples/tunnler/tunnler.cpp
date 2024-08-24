/* tunnler multiplayer game
Index:
@types
@vars
@board
@game
@main
*/
#include "kigu/common.h"
#include "core/assets.h"
#include "core/baked/fonts.h"
#include "core/commands.h"
#include "core/console.h"
#include "core/graphics.h"
#include "core/input.h"
#include "core/logger.h"
#include "core/memory.h"
#include "core/networking.h"
#include "core/platform.h"
#include "core/render.h"
#include "core/threading.h"
#include "core/ui.h"
#include "core/ui_widgets.h"
#include "core/window.h"


//~////////////////////////////////////////////////////////////////////////////////////////////////
//// @types


//NOTE(delle) the order of these matter!
typedef u32 Message; enum{
	Message_None = 0,
	
	Message_HostGame, //broadcast by client hosting game
	Message_JoinGame, //broadcast in response to HostGam
	Message_AcknowledgeMessage, //broadcast by either client to indiciate that the last message was recieved
	Message_QuitGame, //broadcast by either client to end game
	
	Message_SkipTurn,
	
	Message_MoveUp,
	Message_MoveDown,
	Message_MoveRight,
	Message_MoveLeft,
	
	Message_DigUp,
	Message_DigDown,
	Message_DigRight,
	Message_DigLeft,
	
	Message_BuildLadder,
	Message_BuildPillar,
	
	Message_PlaceBomb,
	Message_DetonateBomb,
	
	Message_COUNT,
	Message_ACTIONS_START = Message_QuitGame,
	Message_ACTIONS_END = Message_DetonateBomb,
};

str8 MessageStrings[Message_COUNT] = {
	STR8("None"),
	
	STR8("HostGame"),
	STR8("JoinGame"),
	STR8("AcknowledgeMessage"),
	STR8("HostGame"),
	
	STR8("SkipTurn"),
	
	STR8("MoveUp"),
	STR8("MoveDown"),
	STR8("MoveRight"),
	STR8("MoveLeft"),
	
	STR8("DigUp"),
	STR8("DigDown"),
	STR8("DigRight"),
	STR8("DigLeft"),
	
	STR8("BuildLadder"),
	STR8("BuildPillar"),
	
	STR8("PlaceBomb"),
	STR8("DetonateBomb"),
};

typedef u32 TileBG; enum{
	TileBG_Dirt,
	TileBG_Tunnel,
	TileBG_BritishBase,
	TileBG_GermanBase,
	TileBG_Sky,
};

typedef u32 TileFG; enum{
	TileFG_None          = 0,
	
	TileFG_Ladder        = (1 << 0),
	TileFG_Pillar        = (1 << 1),
	
	TileFG_BritishBomb   = (1 << 2),
	TileFG_GermanBomb    = (1 << 3),
	TileFG_BombWire      = (1 << 4),
	
	TileFG_BritishPlayer = (1 << 5),
	TileFG_GermanPlayer  = (1 << 6),
};

typedef u32 GameWinner; enum{
	GameWinner_None,
	GameWinner_Player, //this player won
	GameWinner_Other, //other player won
	GameWinner_Tie,
};

typedef u32 MenuState; enum{
	MenuState_Choice,
	MenuState_Joining,
	MenuState_Hosting,
};

struct Tile{
	TileBG bg;
	TileFG fg;
};

struct Player{
	s32 x, y;
	u32 bombs;
	u32 player_flag;
	u32 bomb_flag;
	u32* placed_bombs_array;
};

struct NetInfo{
	u32 magic = PackU32('T', 'U', 'N', 'L');
	Message message;
	s32 x, y;
	u32 uid;
};

#define CheckMagic(var) (var.magic == PackU32('T','U','N','L'))



//~////////////////////////////////////////////////////////////////////////////////////////////////
//// @vars


b32 game_active = false;
u32 turn_count = 0;
MenuState menu_state = MenuState_Choice;
GameWinner game_winner = GameWinner_None;

Player players[2];
u32 player_idx;

Message last_action;
u32 last_action_x;
u32 last_action_y;

Tile* board;
s32 board_width;
s32 board_height;
s32 board_area;

#define LinearRow(pos) ((pos)/board_width)
#define LinearCol(pos) ((pos)%board_width)
#define ToLinear(x,y) ((board_width*(y))+(x))
#define TileAt(x,y) board[ToLinear(x,y)]
#define TileAtLinear(pos) board[pos]

b32 is_server = false;
b32 is_client = false;
u16 net_port = -1;
netSocket client_socket;
netAddress client_address;

//phase of the other player's turn
//0 - waiting for acknowledgement from other player, rebroadcast message if we timeout
//1 - other player has acknoledged our move, so we listen for their next move
u32 other_turn_phase = 0; 
NetInfo turn_info;
Stopwatch acknowledge_timeout;

NetInfo listener_latch;
b32 close_listener = false;

NetInfo tail_message;
b32 tail_available = false;

u32 join_phase = 0;
u32 host_phase = 0;

str8 dots_strings[4] = { STR8(""), STR8("."), STR8(".."), STR8("...") };


//~////////////////////////////////////////////////////////////////////////////////////////////////
//// @net


void listen_for_message(void* data){
	listener_latch.magic = 0;
	while(!close_listener){
		platform_sleep(500);
		
		NetInfo info;
		if(tail_available){
			tail_available = false;
			info = tail_message;
		}else{
			netAddress sender_address;
			u64 bytes_read = net_socket_recv(&client_socket, &sender_address, &info, sizeof(info));
			if(bytes_read > 0){
				Log("tunnler-net", "Received ",bytes_read," bytes from ",net_address_str(sender_address, true),"; Message: ",MessageStrings[info.message]);
				NetInfo eat;
				while(net_socket_recv(&client_socket, &sender_address, &eat, sizeof(eat)) > 0){
					if(memcmp(&info, &eat, sizeof(info))){
						if(CheckMagic(eat)){
							tail_available = true;
							tail_message = eat;
						}
						break;
					}
				}
			}else{
				info.magic = 0;
			}
		}
		
		if(CheckMagic(info) && info.uid != player_idx){
			listener_latch = info;
		}
	}
	close_listener = false;
}


//~////////////////////////////////////////////////////////////////////////////////////////////////
//// @board


//NOTE(delle) the board's origin is at the bottom left
void init_board(s32 width, s32 height){
	board        = (Tile*)memory_alloc((width*height)*sizeof(Tile));
	board_width  = width;
	board_height = height;
	turn_count   = 0;
	board_area   = width*height;
	
	players[0] = {1,             board_height-5, 3, TileFG_BritishPlayer, TileFG_BritishBomb, 0};
	TileAt(0,board_height-1).bg = TileBG_BritishBase;
	for(int i = 1; i < board_width-1; ++i) TileAt(i,board_height-1).bg = TileBG_Sky;
	TileAt(0,board_height-2).bg = TileBG_Tunnel;
	TileAt(0,board_height-2).fg = TileFG_Ladder;
	TileAt(0,board_height-3).bg = TileBG_Tunnel;
	TileAt(0,board_height-3).fg = TileFG_Ladder;
	TileAt(0,board_height-4).bg = TileBG_Tunnel;
	TileAt(0,board_height-4).fg = TileFG_Ladder;
	TileAt(0,board_height-5).bg = TileBG_Tunnel;
	TileAt(0,board_height-5).fg = TileFG_Ladder;
	TileAt(players[0].x,players[0].y).bg = TileBG_Tunnel;
	TileAt(players[0].x,players[0].y).fg = TileFG_BritishPlayer;
	
	players[1] = {board_width-2, board_height-5, 3, TileFG_GermanPlayer,  TileFG_GermanBomb, 0};
	TileAt(board_width-1,board_height-1).bg = TileBG_GermanBase;
	TileAt(board_width-1,board_height-2).bg = TileBG_Tunnel;
	TileAt(board_width-1,board_height-2).fg = TileFG_Ladder;
	TileAt(board_width-1,board_height-3).bg = TileBG_Tunnel;
	TileAt(board_width-1,board_height-3).fg = TileFG_Ladder;
	TileAt(board_width-1,board_height-4).bg = TileBG_Tunnel;
	TileAt(board_width-1,board_height-4).fg = TileFG_Ladder;
	TileAt(board_width-1,board_height-5).bg = TileBG_Tunnel;
	TileAt(board_width-1,board_height-5).fg = TileFG_Ladder;
	TileAt(players[1].x,players[1].y).bg = TileBG_Tunnel;
	TileAt(players[1].x,players[1].y).fg = TileFG_GermanPlayer;
}

void deinit_board(){
	memzfree(board);
}

//Returns true if the tile is in line of sight of the target
b32 line_of_sight(u32 tile_x, u32 tile_y, u32 target_x, u32 target_y){
	u32 x = tile_x, y = tile_y;
	while(x != target_x || y != target_y){
		vec2 tile_towards_target = vec2_normalized(Vec2(target_x - x, target_y - y));
		
		//not in line of sight if nearest tile along line is opaque 
		if(abs(tile_towards_target.x) > abs(tile_towards_target.y)){
			if(tile_towards_target.x > 0){
				Assert(x+1 < board_width);
				if(TileAt(x+1,y).bg == TileBG_Dirt) return false;
				x += 1;
			}else{
				Assert(x-1 >= 0);
				if(TileAt(x-1,y).bg == TileBG_Dirt) return false;
				x -= 1;
			}
		}else{
			if(tile_towards_target.y > 0){
				Assert(y+1 < board_height);
				if(TileAt(x,y+1).bg == TileBG_Dirt) return false;
				y += 1;
			}else{
				Assert(y-1 >= 0);
				if(TileAt(x,y-1).bg == TileBG_Dirt) return false;
				y -= 1;
			}
		}
		
		if(x == target_x && y == target_y) break;
	}
	return true;
}

void draw_board(Window* window){
	ui_begin_immediate_branch(0);
	
	//draw board
	uiItem* ui_board = ui_begin_item(0);{ ui_board->id = STR8("tunnler.board");
		ui_board->style.positioning = pos_fixed;
		ui_board->style.sizing = size_percent;
		ui_board->style.size = {100.0f, 100.0f};
		ui_board->style.background_color = Color_LightBlue;
		
		f32 tile_width  = (f32)window->width / (f32)board_width;
		f32 tile_height = tile_width;
		vec2 tile_dims = {tile_width, tile_height};
		forX(y, board_height){
			forX(x, board_width){
				vec2 tile_pos = {x * tile_width, window->height - (y+1)*tile_height};
				
				//if in line of sight of player or base, show true tile, else show dirt
				if(   line_of_sight(x, y, players[player_idx].x,              players[player_idx].y)
				   || line_of_sight(x, y, ((player_idx) ? board_width-1 : 0), board_height-1)){
					//draw tile background
					switch(TileAt(x,y).bg){
						case TileBG_Dirt:{
							uiItem* rect = ui_make_item(0);
							rect->style.positioning = pos_fixed;
							rect->style.pos = tile_pos;
							rect->style.size = tile_dims;
							rect->style.background_color = color(70,37,33);
							
							uiItem* text = ui_make_text(STR8("Dirt"), 0);
							text->style.positioning = pos_fixed;
							text->style.pos = tile_pos + vec2_ONE();
						}break;
						case TileBG_Tunnel:{
							uiItem* rect = ui_make_item(0);
							rect->style.positioning = pos_fixed;
							rect->style.pos = tile_pos;
							rect->style.size = tile_dims;
							rect->style.background_color = color(150,112,91);
							
							uiItem* text = ui_make_text(STR8("Tunnel"), 0);
							text->style.positioning = pos_fixed;
							text->style.pos = tile_pos + vec2_ONE();
						}break;
						case TileBG_BritishBase:{
							uiItem* rect1 = ui_make_item(0);
							rect1->style.positioning = pos_fixed;
							rect1->style.pos = tile_pos;
							rect1->style.size = tile_dims;
							rect1->style.background_color = Color_Grey;
							
							uiItem* rect2 = ui_make_item(0);
							rect2->style.positioning = pos_fixed;
							rect2->style.pos = tile_pos;
							rect2->style.size = tile_dims - Vec2(0, tile_dims.y * 0.75f);
							rect2->style.background_color = Color_DarkBlue;
							
							uiItem* text = ui_make_text(STR8("Base (B)"), 0);
							text->style.positioning = pos_fixed;
							text->style.pos = tile_pos + vec2_ONE();
						}break;
						case TileBG_GermanBase:{
							uiItem* rect1 = ui_make_item(0);
							rect1->style.positioning = pos_fixed;
							rect1->style.pos = tile_pos;
							rect1->style.size = tile_dims;
							rect1->style.background_color = Color_Grey;
							
							uiItem* rect2 = ui_make_item(0);
							rect2->style.positioning = pos_fixed;
							rect2->style.pos = tile_pos;
							rect2->style.size = tile_dims - Vec2(0, tile_dims.y * 0.75f);
							rect2->style.background_color = Color_DarkRed;
							
							uiItem* text = ui_make_text(STR8("Base (G)"), 0);
							text->style.positioning = pos_fixed;
							text->style.pos = tile_pos + vec2_ONE();
						}break;
						case TileBG_Sky:{
							uiItem* text = ui_make_text(STR8("Sky"), 0);
							text->style.positioning = pos_fixed;
							text->style.pos = tile_pos + vec2_ONE();
						}break;
					}
					
					//draw foreground structures
					if      (HasFlag(TileAt(x,y).fg, TileFG_Ladder)){
						uiItem* text = ui_make_text(STR8("Ladder"), 0);
						text->style.positioning = pos_fixed;
						text->style.pos = tile_pos + Vec2(1.0f, 1.0f + text->style.font_height);
					}else if(HasFlag(TileAt(x,y).fg, TileFG_Pillar)){
						uiItem* text = ui_make_text(STR8("Pillar"), 0);
						text->style.positioning = pos_fixed;
						text->style.pos = tile_pos + Vec2(1.0f, 1.0f + text->style.font_height);
					}
					
					//draw foreground bombs
					if      (HasFlag(TileAt(x,y).fg, TileFG_BritishBomb)){
						uiItem* text = ui_make_text(STR8("Bomb (B)"), 0);
						text->style.positioning = pos_fixed;
						text->style.pos = tile_pos + Vec2(1.0f, 1.0f + 2.0f*text->style.font_height);
					}else if(HasFlag(TileAt(x,y).fg, TileFG_GermanBomb)){
						uiItem* text = ui_make_text(STR8("Bomb (G)"), 0);
						text->style.positioning = pos_fixed;
						text->style.pos = tile_pos + Vec2(1.0f, 1.0f + 2.0f*text->style.font_height);
					}else if(HasFlag(TileAt(x,y).fg, TileFG_BombWire)){
						uiItem* text = ui_make_text(STR8("Wire"), 0);
						text->style.positioning = pos_fixed;
						text->style.pos = tile_pos + Vec2(1.0f, 1.0f + 2.0f*text->style.font_height);
					}
					
					//draw foreground players
					if      (HasFlag(TileAt(x,y).fg, TileFG_BritishPlayer)){
						uiItem* text = ui_make_text(STR8("Player (B)"), 0);
						text->style.positioning = pos_fixed;
						text->style.pos = tile_pos + Vec2(1.0f, 1.0f + 3.0f*text->style.font_height);
					}else if(HasFlag(TileAt(x,y).fg, TileFG_GermanPlayer)){
						uiItem* text = ui_make_text(STR8("Player (G)"), 0);
						text->style.positioning = pos_fixed;
						text->style.pos = tile_pos + Vec2(1.0f, 1.0f + 3.0f*text->style.font_height);
					}
				}else{
					uiItem* rect = ui_make_item(0);
					rect->style.positioning = pos_fixed;
					rect->style.pos = tile_pos;
					rect->style.size = tile_dims;
					rect->style.background_color = color(70,37,33);
					
					uiItem* text = ui_make_text(STR8("Dirt"), 0);
					text->style.positioning = pos_fixed;
					text->style.pos = tile_pos + vec2_ONE();
				}
				
				//draw tile border
				uiItem* border = ui_make_item(0);
				border->style.positioning = pos_fixed;
				border->style.pos = tile_pos + vec2_ONE();
				border->style.size = tile_dims - vec2_ONE();
				border->style.background_color = Color_Clear;
				border->style.border_width = 1.0f;
				border->style.border_color = Color_Black;
			}
		}
	}ui_end_item();
	
	//draw menu
	uiItem* menu = ui_begin_item(0);{ menu->id = STR8("tunnler.menu");
		menu->style.sizing = size_auto;
		menu->style.padding = {1.0f, 1.0f, 1.0f, 1.0f};
		menu->style.display = display_horizontal;
		menu->style.background_color = Color_DarkBlue;
		
		persist str8 controls1 = STR8("(Escape) Exit Game\n(Enter)  Skip Turn");
		persist str8 controls2 = STR8("(Up)     Move Up\n(Down)   Move Down\n(Right)  Move Right\n(Left)   Move Left");
		persist str8 controls3 = STR8("(Shift Up)     Dig Up\n(Shift Down)   Dig Down\n(Shift Right)  Dig Right\n(Shift Left)   Dig Left");
		persist str8 controls4 = STR8("(b) Place Bomb\n(B) Detonate Bomb\n(l) Build Ladder\n(p) Build Pillar");
		str8 controls5 = to_dstr8v(deshi_temp_allocator,
								   "FPS: ", (int)(1000.0f / DeshTime->deltaTime),
								   "\nTurn Count: ", turn_count,
								   "\nPlayer Turn: ", (turn_count % 2 == 0) ? "British" : "German",
								   "\nLast Action: ", MessageStrings[last_action],
								   " at (",last_action_x,",",last_action_y,")").fin;
		ui_make_text(controls1, 0);
		ui_make_text(controls2, 0);
		ui_make_text(controls3, 0);
		ui_make_text(controls4, 0);
		ui_make_text(controls5, 0);
	}ui_end_item();
	
	ui_end_immediate_branch();
}



//~////////////////////////////////////////////////////////////////////////////////////////////////
//// @game


void game_init(){
	init_board(20, 10);
	turn_count = 0;
	
	last_action = Message_None;
	last_action_x = 0;
	last_action_y = 0;
	
	//init ui
	{
		Font* default_font = assets_font_get_by_name(STR8("baked_gohufont_11_bdf"));
		if(!default_font){
			default_font = assets_font_create_from_memory_bdf(baked_font_gohufont_11_bdf.str, baked_font_gohufont_11_bdf.count, STR8("baked_gohufont_11_bdf"));
		}
		
		g_ui->base.style.font        = default_font;
		g_ui->base.style.font_height = 11;
		g_ui->base.style.text_color  = Color_White;
	}
}

void game_update(Window* window){
	Player* player;
	Player* other_player;
#define TileAtPlayer() TileAt(player->x,player->y)
#define TileNearPlayer(_x,_y) TileAt(player->x+_x,player->y+_y)
#define TileAtOtherPlayer() TileAt(player->x,player->y)
#define TileNearOtherPlayer(_x,_y) TileAt(player->x+_x,player->y+_y)
	
	//debug
	if(key_pressed(Key_ENTER|InputMod_AnyShift)){
		player_idx = (player_idx) ? 0 : 1;
	}
	
	u32 action_performed = Message_None;
	if(turn_count % 2 == player_idx){
		player = &players[player_idx];
		other_player = &players[(player_idx) ? 0 : 1];
		
		//// moving input ////
		if(key_pressed(Key_UP|InputMod_None) && (player->y < board_height-1) && (TileNearPlayer(0,1).bg == TileBG_Tunnel) && HasFlag(TileAtPlayer().fg, TileFG_Ladder)){
			action_performed = Message_MoveUp;
		}else if(key_pressed(Key_DOWN|InputMod_None) && (player->y > 0) && (TileNearPlayer(0,-1).bg == TileBG_Tunnel) && HasFlag(TileAtPlayer().fg, TileFG_Ladder)){
			action_performed = Message_MoveDown;
		}else if(key_pressed(Key_RIGHT|InputMod_None) && (player->x < board_width-1) && (TileNearPlayer(1,0).bg == TileBG_Tunnel)){
			action_performed = Message_MoveRight;
		}else if(key_pressed(Key_LEFT|InputMod_None) && (player->x > 0) && (TileNearPlayer(-1,0).bg == TileBG_Tunnel)){
			action_performed = Message_MoveLeft;
		}
		
		//// digging input ////
		if(key_pressed(Key_UP|InputMod_AnyShift) && (player->y < board_height-1) && (TileNearPlayer(0,1).bg == TileBG_Dirt)){
			action_performed = Message_DigUp;
		}else if(key_pressed(Key_DOWN|InputMod_AnyShift) && (player->y > 0) && (TileNearPlayer(0,-1).bg == TileBG_Dirt)){
			action_performed = Message_DigDown;
		}else if(key_pressed(Key_RIGHT|InputMod_AnyShift) && (player->x < board_width-1) && (TileNearPlayer( 1,0).bg == TileBG_Dirt)){
			action_performed = Message_DigRight;
		}else if(key_pressed(Key_LEFT|InputMod_AnyShift) && (player->x > 0) && (TileNearPlayer(-1,0).bg == TileBG_Dirt)){
			action_performed = Message_DigLeft;
		}
		
		//// building input ////
		if(key_pressed(Key_L|InputMod_None) && (TileAtPlayer().bg == TileBG_Tunnel) && !HasFlag(TileAtPlayer().fg, TileFG_Pillar)){
			action_performed = Message_BuildLadder;
		}else if(key_pressed(Key_P|InputMod_None) && (TileAtPlayer().bg == TileBG_Tunnel) && !HasFlag(TileAtPlayer().fg, TileFG_Ladder)){
			action_performed = Message_BuildPillar;
		}
		
		//// bombing input ////
		if(key_pressed(Key_B|InputMod_None) && !HasFlag(TileAtPlayer().fg, TileFG_BritishBomb|TileFG_GermanBomb) && (player->bombs > 0)){
			action_performed = Message_PlaceBomb;
		}else if(key_pressed(Key_B|InputMod_AnyShift) && (array_count(player->placed_bombs_array) > 0)){
			action_performed = Message_DetonateBomb;
		}
		
		//// game state input ////
		if(key_pressed(Key_ENTER|InputMod_None)){
			action_performed = Message_SkipTurn;
		}else if(key_pressed(Key_ESCAPE|InputMod_None)){
			action_performed = Message_QuitGame;
		}
		
		if(action_performed <= Message_ACTIONS_END && action_performed >= Message_ACTIONS_START){
			last_action = action_performed;
			last_action_x = player->x;
			last_action_y = player->y;
			turn_count += 1;
			
			turn_info.uid     = player_idx;
			turn_info.x       = player->x;
			turn_info.y       = player->y;
			turn_info.message = action_performed;
			net_socket_send(&client_socket, client_address, &turn_info, sizeof(turn_info));
			acknowledge_timeout = start_stopwatch();
		}
	}else{
		player = &players[(player_idx) ? 0 : 1];
		other_player = &players[player_idx];
		
		NetInfo info = listener_latch;
		if(info.magic && info.uid != player_idx){
			if(!other_turn_phase){//wait for acknowledgement from other player before expecting their turn, rebroadcast after timeout
				if(info.message == Message_AcknowledgeMessage){
					other_turn_phase = 1;
				}else if(peek_stopwatch(acknowledge_timeout) > 1000) {
					net_socket_send(&client_socket, client_address, &turn_info, sizeof(turn_info));
					reset_stopwatch(&acknowledge_timeout);
				}
			}
			else{ //other player has acknowledged our turn, so now we check for them to make their's
				action_performed = info.message;
				if(info.message <= Message_ACTIONS_END && info.message >= Message_ACTIONS_START){
					last_action = action_performed;
					last_action_x = player->x;
					last_action_y = player->y;
					turn_count += 1;
					
					NetInfo info;
					info.message = Message_AcknowledgeMessage;
					info.uid = player_idx;
					net_socket_send(&client_socket, client_address, &info, sizeof(info));
				}
				
				threader_add_job({{}, listen_for_message, 0});
				threader_wake_threads();
			}
		}
		listener_latch = {};
	}
	
	
	//perform action for current player
	switch(action_performed){
		//// moving actions ////
		case Message_MoveUp:{
			RemoveFlag(TileAtPlayer().fg, player->player_flag);
			player->y += 1;
			AddFlag(TileAtPlayer().fg, player->player_flag);
		}break;
		case Message_MoveDown:{
			RemoveFlag(TileAtPlayer().fg, player->player_flag);
			player->y -= 1;
			AddFlag(TileAtPlayer().fg, player->player_flag);
		}break;
		case Message_MoveRight:{
			RemoveFlag(TileAtPlayer().fg, player->player_flag);
			player->x += 1;
			while(TileNearPlayer(0,-1).bg == TileBG_Tunnel && !HasFlag(TileAtPlayer().fg, TileFG_Ladder)){
				player->y -= 1;
			}
			AddFlag(TileAtPlayer().fg, player->player_flag);
		}break;
		case Message_MoveLeft:{
			RemoveFlag(TileAtPlayer().fg, player->player_flag);
			player->x -= 1;
			while(TileNearPlayer(0,-1).bg == TileBG_Tunnel && !HasFlag(TileAtPlayer().fg, TileFG_Ladder)){
				player->y -= 1;
			}
			AddFlag(TileAtPlayer().fg, player->player_flag);
		}break;
		
		//// digging actions ////
		case Message_DigUp:{
			TileNearPlayer(0, 1).bg = TileBG_Tunnel;
		}break;
		case Message_DigDown:{
			TileNearPlayer(0,-1).bg = TileBG_Tunnel;
			RemoveFlag(TileAtPlayer().fg, player->player_flag);
			while(TileNearPlayer(0,-1).bg == TileBG_Tunnel && !HasFlag(TileAtPlayer().fg, TileFG_Ladder)){
				player->y -= 1;
			}
			AddFlag(TileAtPlayer().fg, player->player_flag);
		}break;
		case Message_DigRight:{
			TileNearPlayer( 1,0).bg = TileBG_Tunnel;
		}break;
		case Message_DigLeft:{
			TileNearPlayer(-1,0).bg = TileBG_Tunnel;
		}break;
		
		//// building actions ////
		case Message_BuildLadder:{
			AddFlag(TileAtPlayer().fg, TileFG_Ladder);
		}break;
		case Message_BuildPillar:{
			AddFlag(TileAtPlayer().fg, TileFG_Pillar);
		}break;
		
		//// bombing actions ////
		case Message_PlaceBomb:{
			array_push_value(player->placed_bombs_array, ToLinear(player->x,player->y));
			player->bombs -= 1;
			AddFlag(TileAtPlayer().fg, player->bomb_flag);
		}break;
		case Message_DetonateBomb:{
			b32 player_died = false, other_died = false;
			while(array_count(player->placed_bombs_array)){
				//explode up
				if(LinearRow(player->placed_bombs_array[0]) < board_height-1){
					s32 up_tile = player->placed_bombs_array[0] - board_width;
					TileAtLinear(up_tile).bg = TileBG_Tunnel;
					TileAtLinear(up_tile).fg = TileFG_None;
					if(ToLinear(player->x,      player->y      ) == up_tile) player_died = true;
					if(ToLinear(other_player->x,other_player->y) == up_tile) other_died  = true;
				}
				
				//explode down
				if(LinearRow(player->placed_bombs_array[0]) >= 0){
					s32 down_tile = player->placed_bombs_array[0] + board_width;
					TileAtLinear(down_tile).bg = TileBG_Tunnel;
					TileAtLinear(down_tile).fg = TileFG_None;
					if(ToLinear(player->x,      player->y      ) == down_tile) player_died = true;
					if(ToLinear(other_player->x,other_player->y) == down_tile) other_died  = true;
				}
				
				//explode right
				if(LinearCol(player->placed_bombs_array[0]) < board_width-1){
					s32 right_tile = player->placed_bombs_array[0] + 1;
					TileAtLinear(right_tile).bg = TileBG_Tunnel;
					TileAtLinear(right_tile).fg = TileFG_None;
					if(ToLinear(player->x,      player->y      ) == right_tile) player_died = true;
					if(ToLinear(other_player->x,other_player->y) == right_tile) other_died  = true;
				}
				
				//explode left
				if(LinearCol(player->placed_bombs_array[0]) >= 0){
					s32 left_tile = player->placed_bombs_array[0] - 1;
					TileAtLinear(left_tile).bg = TileBG_Tunnel;
					TileAtLinear(left_tile).fg = TileFG_None;
					if(ToLinear(player->x,      player->y      ) == left_tile) player_died = true;
					if(ToLinear(other_player->x,other_player->y) == left_tile) other_died  = true;
				}
				
				if(ToLinear(player->x,      player->y      ) == player->placed_bombs_array[0]) player_died = true;
				if(ToLinear(other_player->x,other_player->y) == player->placed_bombs_array[0]) other_died  = true;
				TileAtLinear(player->placed_bombs_array[0]).fg = TileFG_None;
				array_remove_unordered(player->placed_bombs_array, 0);
			}
			
			//check if the game should end
			if(player_died){
				if(other_died){
					game_winner = GameWinner_Tie;
					game_active = 0;
					menu_state  = 0;
					deinit_board();
				}else{
					game_winner = GameWinner_Other;
					game_active = 0;
					menu_state  = 0;
					deinit_board();
				}
			}else{
				if(other_died){
					game_winner = GameWinner_Player;
					game_active = 0;
					menu_state  = 0;
					deinit_board();
				}
			}
		}break;
		
		//// game state ////
		case Message_SkipTurn:{
			//do nothing
		}break;
		case Message_QuitGame:{
			game_winner = (player == &players[player_idx]) ? GameWinner_Other : GameWinner_Player;
			game_active = 0;
			menu_state  = 0;
			deinit_board();
		}break;
	}
	
	//draw things
	draw_board(window);
}

void menu_update(Window* window){
	ui_begin_immediate_branch(&g_ui->base);
	uiItem* menu = ui_begin_item(0);{ menu->id = STR8("tunnler.main_menu");
		menu->style.positioning = pos_fixed;
		menu->style.sizing = size_percent;
		menu->style.size = {100.0f, 100.0f};
		menu->style.background_color = Color_Clear;
		menu->style.padding_bottom = 10;
		
		uiItem* title = ui_make_text(STR8("TUNNLER"), 0);
		title->style.font_height = 50;
		
		switch(menu_state){
			case MenuState_Choice:{
				uiItem* port_input_row = ui_begin_item(0);{
					port_input_row->id = STR8("tunnler.main_menu.port_input_row");
					port_input_row->style.display = display_horizontal;
					
					uiInputText* port_input = (uiInputText*)ui_make_input_text(STR8("port"), 0);
					port_input->item.id = STR8("tunnler.main_menu.port_input");
					port_input->item.style.background_color = Color_DarkGray;
					port_input->style.filter = input_filter_numbers;
					
					net_port = -1;
					if(port_input->text.buffer.count > 0){
						char* str_end;
						long value = strtol((const char*)port_input->text.buffer.str, &str_end, 10);
						if(value < 0 || value > 65535){
							uiItem* error_text = ui_make_text(STR8("Port must be between 0 and 65535"), 0);
							error_text->style.margin_left = 5;
							error_text->style.text_color = Color_Red;
						}else{
							net_port = (u16)value;
						}
					}
				}ui_end_item(/*port_input_row*/);
				
				if(net_port != (u16)-1){
					uiItem* button_row = ui_begin_item(0);{
						button_row->style.display = display_horizontal;
						
						uiItem* join_button = ui_begin_item(0);{ join_button->id = STR8("tunnler.main_menu.join_button");
							join_button->style.background_color = Color_DarkGray;
							join_button->style.margin = {1, 1, 1, 1};
							join_button->style.content_align = {0.5f, 0.5f};
							join_button->action = [](uiItem* item){
								menu_state = MenuState_Joining;
								is_client = true;
								net_socket_open(&client_socket, net_port, true);
								net_address_init(&client_address, STR8("192.168.0.255"), to_dstr8(net_port, deshi_temp_allocator).fin);
							};
							ui_make_text(STR8("Join"), 0);
						}ui_end_item();
						
						uiItem* host_button = ui_begin_item(0);{ host_button->id = STR8("tunnler.main_menu.host_button");
							host_button->style.background_color = Color_DarkGray;
							host_button->style.margin = {1, 1, 1, 1};
							host_button->style.content_align = {0.5f, 0.5f};
							host_button->action = [](uiItem* item){
								menu_state = MenuState_Hosting;
								is_client = true;
								net_socket_open(&client_socket, net_port, true);
								net_address_init(&client_address, STR8("192.168.0.255"), to_dstr8(net_port, deshi_temp_allocator).fin);
							};
							ui_make_text(STR8("Host"), 0);
						}ui_end_item();
					}ui_end_item();
				}
				
				switch(game_winner){
					case GameWinner_Player: ui_make_text(STR8("You Won."), 0); break;
					case GameWinner_Other: ui_make_text(STR8("You Lost."), 0); break;
					case GameWinner_Tie: ui_make_text(STR8("You Tied."), 0); break;
				}
			}break;
			
			case MenuState_Joining:{
				b32 connected = false;
				switch(join_phase){
					case 0:{ //set player index; start listener thread
						player_idx = 1; //NOTE must be set before the job is added since the job checks it
						threader_add_job({{}, listen_for_message, 0});
						threader_wake_threads();
						join_phase = 1;
					}break;
					case 1:{ //searching for HostGame message; request join if found
						NetInfo info = listener_latch;
						if(CheckMagic(info) && info.message == Message_HostGame){
							info.uid = 1;
							info.message = Message_JoinGame;
							net_socket_send(&client_socket, client_address, &info, sizeof(info));
							threader_add_job({{}, listen_for_message, 0});
							threader_wake_threads();
							join_phase = 2;
						}
						listener_latch = {};
					}break;
					case 2:{ //requested to join; wait for server to acknowledge; connect if acknowledged
						NetInfo info = listener_latch;
						if(CheckMagic(info) && info.message == Message_AcknowledgeMessage){
							threader_add_job({{}, listen_for_message, 0});
							threader_wake_threads();
							join_phase = 0;
							connected = true;
						}
						listener_latch = {};
					}break;
				}
				
				if(connected){
					game_active = true;
				}else{
					uiItem* loading_text = ui_make_text(to_dstr8v(deshi_temp_allocator, STR8("searching for player"), dots_strings[((u32)g_time->totalTime % 3)]).fin, 0);
					loading_text->style.positioning = pos_fixed;
					loading_text->style.pos = vec2i_multiply_constant(window->dimensions, 0.5f).toVec2();
					uiItem* cancel_button = ui_begin_item(0);{ cancel_button->id = STR8("tunnler.main_menu.cancel_button");
						cancel_button->style.positioning = pos_fixed;
						cancel_button->style.pos = vec2i_add(vec2i_multiply_constant(window->dimensions, 0.5f), Vec2i(0, g_ui->base.style.font_height)).toVec2();
						cancel_button->style.background_color = Color_DarkGray;
						cancel_button->style.margin = {1, 1, 1, 1};
						cancel_button->style.content_align = {0.5f, 0.5f};
						cancel_button->action = [](uiItem* item){
							menu_state = MenuState_Choice;
							close_listener = true;
							join_phase = 0;
							is_client = false;
							is_server = false;
							net_socket_close(&client_socket);
						};
						ui_make_text(STR8("Cancel"), 0);
					}ui_end_item();
				}
			}break;
			
			case MenuState_Hosting:{
				persist Stopwatch host_watch = start_stopwatch();
				b32 connected = false;
				switch(host_phase){
					case 0:{ //set player index; send HostGame message; start listener thread
						player_idx = 0; //NOTE must be set before the job is added since the job checks it
						NetInfo info;
						info.uid = 0;
						info.message = Message_HostGame;
						net_socket_send(&client_socket, client_address, &info, sizeof(info));
						threader_add_job({{}, listen_for_message, 0});
						threader_wake_threads();
						host_phase = 1;
					}break;
					case 1:{ //searching for JoinGame message (resend HostGame message every second); send AcknowledgeMessage and connect if found
						NetInfo info = listener_latch;
						if(CheckMagic(info) && info.message == Message_JoinGame){
							info.uid = 0;
							info.message = Message_AcknowledgeMessage;
							net_socket_send(&client_socket, client_address, &info, sizeof(info));
							threader_add_job({{}, listen_for_message, 0});
							threader_wake_threads();
							host_phase = 0;
							connected = true;
						}else if(peek_stopwatch(host_watch) > 1000){
							reset_stopwatch(&host_watch);
							NetInfo info;
							info.uid = 0;
							info.message = Message_HostGame;
							net_socket_send(&client_socket, client_address, &info, sizeof(info));
						}
						listener_latch = {};
					}break;
				}
				
				if(connected){
					game_active = true;
				}else{
					uiItem* loading_text = ui_make_text(to_dstr8v(deshi_temp_allocator, STR8("searching for player"), dots_strings[((u32)g_time->totalTime % 3)]).fin, 0);
					loading_text->style.positioning = pos_fixed;
					loading_text->style.pos = vec2i_multiply_constant(window->dimensions, 0.5f).toVec2();
					uiItem* cancel_button = ui_begin_item(0);{ cancel_button->id = STR8("tunnler.main_menu.cancel_button");
						cancel_button->style.positioning = pos_fixed;
						cancel_button->style.pos = vec2i_add(vec2i_multiply_constant(window->dimensions, 0.5f), Vec2i(0, g_ui->base.style.font_height)).toVec2();
						cancel_button->style.background_color = Color_DarkGray;
						cancel_button->style.margin = {1, 1, 1, 1};
						cancel_button->style.content_align = {0.5f, 0.5f};
						cancel_button->action = [](uiItem* item){
							menu_state = MenuState_Choice;
							close_listener = true;
							host_phase = 0;
							is_client = false;
							is_server = false;
							net_socket_close(&client_socket);
						};
						ui_make_text(STR8("Cancel"), 0);
					}ui_end_item();
				}
			}break;
		}
	}ui_end_item();
	ui_end_immediate_branch();
}


//~////////////////////////////////////////////////////////////////////////////////////////////////
//// @main


int main(){
	memory_init(Megabytes(50), Megabytes(256));
	platform_init();
	logger_init();
	Window* window = window_create(STR8("TUNNLER"));
	window_show(window);
	graphics_init(window);
	assets_init(window);
	ui_init(window);
	threader_init(1, 1, 8);
	net_init();
	game_init();
	
	while(platform_update()){
		if(game_active){
			game_update(window);
		}else{
			menu_update(window);
		}
		ui_debug();
		ui_update(window);
		graphics_update(window);
		logger_update();
		memory_clear_temp();
	}
	
	//net_deinit();
	logger_cleanup();
	memory_cleanup();
}
