#define W 0xFFFFFFFF
#define B 0xFFFFFFFF

u32 cursor_bitmap_arrow[16*16] = {
	B,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	B,B,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	B,W,B,0,0,0,0,0,0,0,0,0,0,0,0,0,
	B,W,W,B,0,0,0,0,0,0,0,0,0,0,0,0,
	B,W,W,W,B,0,0,0,0,0,0,0,0,0,0,0,
	B,W,W,W,W,B,0,0,0,0,0,0,0,0,0,0,
	B,W,W,W,W,W,B,0,0,0,0,0,0,0,0,0,
	B,W,W,W,W,W,W,B,0,0,0,0,0,0,0,0,
	B,W,W,B,W,B,B,B,B,0,0,0,0,0,0,0,
	B,W,B,B,W,B,0,0,0,0,0,0,0,0,0,0,
	B,B,0,B,W,W,B,0,0,0,0,0,0,0,0,0,
	B,0,0,0,B,W,B,0,0,0,0,0,0,0,0,0,
	0,0,0,0,B,W,W,B,0,0,0,0,0,0,0,0,
	0,0,0,0,0,B,W,B,0,0,0,0,0,0,0,0,
	0,0,0,0,0,B,W,W,B,0,0,0,0,0,0,0,
	0,0,0,0,0,0,B,B,B,0,0,0,0,0,0,0,
};

u32 cursor_bitmap_hresize[16*16] = {
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,B,0,0,0,0,0,0,B,0,0,0,0,
	0,0,0,B,B,0,0,0,0,0,0,B,B,0,0,0,
	0,0,B,W,B,0,0,0,0,0,0,B,W,B,0,0,
	0,B,W,W,B,B,B,B,B,B,B,B,W,W,B,0,
	B,W,W,W,W,W,W,W,W,W,W,W,W,W,W,B,
	B,W,W,W,W,W,W,W,W,W,W,W,W,W,W,B,
	0,B,W,W,B,B,B,B,B,B,B,B,W,W,B,0,
	0,0,B,W,B,0,0,0,0,0,0,B,W,B,0,0,
	0,0,0,B,B,0,0,0,0,0,0,B,B,0,0,0,
	0,0,0,0,B,0,0,0,0,0,0,B,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
};

u32 cursor_bitmap_vresize[16*16] = {
	0,0,0,0,0,0,0,B,B,0,0,0,0,0,0,0,
	0,0,0,0,0,0,B,W,W,B,0,0,0,0,0,0,
	0,0,0,0,0,B,W,W,W,W,B,0,0,0,0,0,
	0,0,0,0,B,W,W,W,W,W,W,B,0,0,0,0,
	0,0,0,B,B,B,B,W,W,B,B,B,B,0,0,0,
	0,0,0,0,0,0,B,W,W,B,0,0,0,0,0,0,
	0,0,0,0,0,0,B,W,W,B,0,0,0,0,0,0,
	0,0,0,0,0,0,B,W,W,B,0,0,0,0,0,0,
	0,0,0,0,0,0,B,W,W,B,0,0,0,0,0,0,
	0,0,0,0,0,0,B,W,W,B,0,0,0,0,0,0,
	0,0,0,0,0,0,B,W,W,B,0,0,0,0,0,0,
	0,0,0,B,B,B,B,W,W,B,B,B,B,0,0,0,
	0,0,0,0,B,W,W,W,W,W,W,B,0,0,0,0,
	0,0,0,0,0,B,W,W,W,W,B,0,0,0,0,0,
	0,0,0,0,0,0,B,W,W,B,0,0,0,0,0,0,
	0,0,0,0,0,0,0,B,B,0,0,0,0,0,0,0,
};

u32 cursor_bitmap_rightdiagresize[16*16] = {
	0,0,0,0,0,0,0,0,0,0,B,B,B,B,B,B,
	0,0,0,0,0,0,0,0,0,0,B,W,W,W,W,B,
	0,0,0,0,0,0,0,0,0,0,0,B,W,W,W,B,
	0,0,0,0,0,0,0,0,0,0,B,W,W,W,W,B,
	0,0,0,0,0,0,0,0,0,B,W,W,W,B,W,B,
	0,0,0,0,0,0,0,0,B,W,W,W,B,0,B,B,
	0,0,0,0,0,0,0,B,W,W,W,B,0,0,0,0,
	0,0,0,0,0,0,B,W,W,W,B,0,0,0,0,0,
	0,0,0,0,0,B,W,W,W,B,0,0,0,0,0,0,
	0,0,0,0,B,W,W,W,B,0,0,0,0,0,0,0,
	B,B,0,B,W,W,W,B,0,0,0,0,0,0,0,0,
	B,W,B,W,W,W,B,0,0,0,0,0,0,0,0,0,
	B,W,W,W,W,B,0,0,0,0,0,0,0,0,0,0,
	B,W,W,W,B,0,0,0,0,0,0,0,0,0,0,0,
	B,W,W,W,W,B,0,0,0,0,0,0,0,0,0,0,
	B,B,B,B,B,B,0,0,0,0,0,0,0,0,0,0,
};

u32 cursor_bitmap_leftdiagresize[16*16] = {
	B,B,B,B,B,B,0,0,0,0,0,0,0,0,0,0,
	B,W,W,W,W,B,0,0,0,0,0,0,0,0,0,0,
	B,W,W,W,B,0,0,0,0,0,0,0,0,0,0,0,
	B,W,W,W,W,B,0,0,0,0,0,0,0,0,0,0,
	B,W,B,W,W,W,B,0,0,0,0,0,0,0,0,0,
	B,B,0,B,W,W,W,B,0,0,0,0,0,0,0,0,
	0,0,0,0,B,W,W,W,B,0,0,0,0,0,0,0,
	0,0,0,0,0,B,W,W,W,B,0,0,0,0,0,0,
	0,0,0,0,0,0,B,W,W,W,B,0,0,0,0,0,
	0,0,0,0,0,0,0,B,W,W,W,B,0,0,0,0,
	0,0,0,0,0,0,0,0,B,W,W,W,B,0,B,B,
	0,0,0,0,0,0,0,0,0,B,W,W,W,B,W,B,
	0,0,0,0,0,0,0,0,0,0,B,W,W,W,W,B,
	0,0,0,0,0,0,0,0,0,0,0,B,W,W,W,B,
	0,0,0,0,0,0,0,0,0,0,B,W,W,W,W,B,
	0,0,0,0,0,0,0,0,0,0,B,B,B,B,B,B,
};

u32 cursor_bitmap_hand[16*16] = {
	0,0,0,0,0,0,0,0,0,B,B,0,0,0,0,0,
	0,0,0,0,0,0,B,B,B,W,W,B,B,0,0,0,
	0,0,0,0,0,B,W,W,B,W,W,B,W,B,0,0,
	0,0,0,0,0,B,W,W,B,W,W,B,W,B,B,0,
	0,0,0,0,0,B,W,W,B,W,W,B,W,B,W,B,
	0,0,0,0,0,B,W,W,B,W,W,B,W,B,W,B,
	0,B,B,B,0,B,W,W,B,W,W,B,W,B,W,B,
	0,B,W,W,B,B,W,W,B,W,W,B,W,B,W,B,
	0,B,W,W,W,B,W,W,W,W,W,W,W,W,W,B,
	0,0,B,W,W,W,W,W,W,W,W,W,W,W,W,B,
	0,0,B,W,W,W,W,W,W,W,W,W,W,W,B,0,
	0,0,0,B,W,W,W,W,W,W,W,W,W,W,B,0,
	0,0,0,B,W,W,W,W,W,W,W,W,W,B,0,0,
	0,0,0,0,B,W,W,W,W,W,W,W,W,B,0,0,
	0,0,0,0,B,W,W,W,W,W,W,W,B,0,0,0,
	0,0,0,0,0,B,B,B,B,B,B,B,0,0,0,0,
};

u32 cursor_bitmap_ibeam[16*16] = {
	0,0,0,B,B,B,B,0,B,B,B,B,0,0,0,0,
	0,0,B,W,W,W,W,B,W,W,W,W,B,0,0,0,
	0,0,0,B,B,B,W,W,W,B,B,B,0,0,0,0,
	0,0,0,0,0,0,B,W,B,0,0,0,0,0,0,0,
	0,0,0,0,0,0,B,W,B,0,0,0,0,0,0,0,
	0,0,0,0,0,0,B,W,B,0,0,0,0,0,0,0,
	0,0,0,0,0,0,B,W,B,0,0,0,0,0,0,0,
	0,0,0,0,0,0,B,W,B,0,0,0,0,0,0,0,
	0,0,0,0,0,0,B,W,B,0,0,0,0,0,0,0,
	0,0,0,0,0,0,B,W,B,0,0,0,0,0,0,0,
	0,0,0,0,0,0,B,W,B,0,0,0,0,0,0,0,
	0,0,0,0,0,0,B,W,B,0,0,0,0,0,0,0,
	0,0,0,0,0,0,B,W,B,0,0,0,0,0,0,0,
	0,0,0,B,B,B,W,W,W,B,B,B,0,0,0,0,
	0,0,B,W,W,W,W,B,W,W,W,W,B,0,0,0,
	0,0,0,B,B,B,B,0,B,B,B,B,0,0,0,0,
};

#undef W
#undef B