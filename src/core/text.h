/* deshi Text module
    represets text and provides an api for manipulating it using a cursor, selections, and copy/pasting
    the module does not track multiline information and the only multiline thing that it does is provide
    functions for moving the cursor up and down

Notes:
    there is very little error or bounds checking, for example we dont check that a selection range is valid
    when delete_selection is called since a selection made using the api should never be invalid. so its 
    important you be careful with editing anything interal

*/


#pragma once
#ifndef DESHI_TEXTEDIT_H
#define DESHI_TEXTEDIT_H

#include "kigu/unicode.h"

struct Text{
    str8b buffer;

    struct{
        s64 pos;   //byte offset into buffer
        s64 count; //selection size in byte count
    }cursor;
};

global Text
text_init(str8 init, Allocator* allocator = deshi_allocator){
    Text out;
    memset(&out, 0, sizeof(Text));
    str8_builder_init(&out.buffer, init, deshi_allocator);
    str8_builder_grow(&out.buffer, Max(Kilobytes(1),out.buffer.count));
    return out;
}

global void
text_move_cursor_right(Text* te, b32 select = 0){
    if(te->cursor.pos == te->buffer.count) return;
    DecodedCodepoint dc = decoded_codepoint_from_utf8(te->buffer.str+te->cursor.pos, 4);
    te->cursor.pos+=dc.advance;
}

global void
text_move_cursor_left(Text* te){
    if(!te->cursor.pos) return;
    while(utf8_continuation_byte(*(te->buffer.str+te->cursor.pos))) te->cursor.pos--;
    te->cursor.pos--;
}

global void
text_move_cursor_up(Text* te){

}

global void
text_move_cursor_down(Text* te){

}

//deletes the cursor's selection
global void
text_delete_selection(Text* te){
    if(!te->cursor.count) return;
    u64 offset = Min(te->cursor.pos, te->cursor.pos + te->cursor.count);
    forI(te->cursor.count){
        str8_builder_remove_codepoint_at_byteoffset(&te->buffer,offset+i);
    }
    te->cursor.count = 0;
}

//deletes the character to the right of the cursor
//if there is a selection it will just delete the selection instead
global void
text_delete_right(Text* te){
    if(te->cursor.count) return (void)text_delete_selection(te);
}

//deletes the character to the left of the cursor
//if there is a selection it will just delete the selection instead
global void
text_delete_left(Text* te){
    if(te->cursor.count) return (void)text_delete_selection(te);

    
}

//inserts the given string 's' at the cursor
//if there is a selection it overwrites it with 's'
global void
text_insert_string(Text* te, str8 s){
    if(te->cursor.count) text_delete_selection(te);
    if(te->buffer.count + s.count < te->buffer.space) str8_builder_grow(&te->buffer, s.count);
    str8_builder_insert_byteoffset(&te->buffer,te->cursor.pos,s);
    forI(s.count){
        DecodedCodepoint dc = str8_advance(&s);
        switch(dc.codepoint){
            case '\n':{
            }break;
        }
    }
}

global str8
text_get_selection(Text* te){
    u64 offset = Min(te->cursor.pos,te->cursor.pos+te->cursor.count);
    return str8{te->buffer.str + offset, abs(te->cursor.count)};
}

#endif