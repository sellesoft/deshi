local b32 _resized = false;
local int _width, _height, _x, _y;
local int opengl_version;
local b32 block_mouse_pos_change = false;


struct{
	struct{
		// points to the X server
		X11::Display* display;
		// which screen of the display are we using
		int screen;
	}x11;
}linux;




Window*
window_create(str8 title, s32 width, s32 height, s32 x, s32 y, DisplayMode display_mode, Decoration decorations){
	DeshiStageInitStart(DS_WINDOW, DS_PLATFORM, "Called window_create() before initializing Platform module");
	
	linux.x11.display = X11::XOpenDisplay(0);
	linux.x11.screen = X11::XDefaultScreen(linux.x11.display);
	u32 black = X11::XBlackPixel(linux.x11.display, linux.x11.screen);
	u32 white = X11::XWhitePixel(linux.x11.display, linux.x11.screen);

	Window* window = (Window*)memalloc(sizeof(Window));
	window->handle = (void*)X11::XCreateSimpleWindow(linux.x11.display, X11::XDefaultRootWindow(linux.x11.display), 0,0,200,300,5,white,black);
	
	X11::XSetStandardProperties(linux.x11.display, (X11::Window)window->handle, (const char*)title.str, 0,0,0,0,0);
	DeshiStageInitEnd(DS_WINDOW);
	return window;

} // window_create


void window_close(Window* window){
	NotImplemented;
}

void window_display_mode(Window* window, DisplayMode displayMode){
	NotImplemented;
}

void 
window_cursor_mode(Window* window, CursorMode mode){
	NotImplemented;
}

void window_set_cursor_position(Window* window, s32 x, s32 y){
	NotImplemented;
}

void window_set_cursor_type(Window* window, CursorType curtype) {
	NotImplemented;
}