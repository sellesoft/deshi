#pragma once
#ifndef DESHI_TEXTEDIT_H
#define DESHI_TEXTEDIT_H

#include "unicode.h"



struct TextEdit{
    str8b buffer;

    struct{
        s64 pos;   //byte offset into buffer
        s64 count; //selection size in byte count
        s64 line;  //line number the cursor is on
    }cursor;

    u64* wrap_indexes; // an array of byte indexes into the buffer that indicate where it wraps visually
    u64  wrap_indexes_count;

    struct{
        b32 dont_skip_newline_characters;
    }config;
};

TextEdit textedit_init(str8 init, Allocator* allocator = deshi_allocator){
    TextEdit out;
    memset(&out, 0, sizeof(TextEdit));
    str8_builder_init(&out.buffer, init, allocator);
    str8_builder_grow(&out.buffer, Max(Kilobytes(1),init.count)); //give ourselves some room
    return out;
}

void textedit_move_cursor_right(TextEdit* te){
    if(te->cursor.pos == te->buffer.count) return;
    DecodedCodepoint dc = decoded_codepoint_from_utf8(te->buffer.str+te->cursor.pos);

}

void textedit_move_cursor_left(TextEdit* te){
    if(!te->cursor.pos) return;

}

void textedit_move_cursor_up(TextEdit* te){

}

void textedit_move_cursor_down(TextEdit* te){

}

//deletes the cursor's selection
void textedit_delete_selection(TextEdit* te){
    if(!te->cursor.count) return;
    u64 offset = Min(te->cursor.pos, te->cursor.pos + te->cursor.count);
    forI(te->cursor.count){
        str8_builder_remove_codepoint_at_byteoffset(&te->buffer,offset+i);
    }
}

//deletes the character to the right of the cursor
//if there is a selection it will just delete the selection instead
void textedit_delete_right(TextEdit* te){
    if(te->cursor.count) return (void)textedit_delete_selection(te);
}

//deletes the character to the left of the cursor
//if there is a selection it will just delete the selection instead
void textedit_delete_left(TextEdit* te){
    if(te->cursor.count) return (void)textedit_delete_selection(te);

    
}

//inserts the given string 's' at the cursor
//if there is a selection it overwrites it with 's'
void textedit_insert_string(TextEdit* te, str8 s){
    if(te->cursor.count) textedit_delete_selection(te);
    if(te->buffer.count + s.count < te->buffer.space) str8_builder_grow(&te->buffer, s.count);
    str8_builder_insert_byteoffset(&te->buffer,te->cursor.pos,s);
    forI(s.count){
        DecodedCodepoint dc = str8_advance(&s);
        switch(dc.codepoint){
            case '\n':{
                te->cursor.line++;
            }break;
        }
    }
}







#endif