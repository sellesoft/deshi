/* deshi Text module
    Represets text and provides an api for manipulating it using a cursor, selections, and copy/pasting.

Notes:
    There is very little error or bounds checking. For example we dont check that a selection range is valid
    when delete_selection is called since a selection made using the api should never be invalid, so its 
    important you be careful with editing anything internal manually.
    

TODOs:
    [!, *, 2022/08/09]
    Make it so that movement by word and word part are customizable, just make it so that we store a list of characters
    to check for to stop, or a list of character to keep moving on.

    [!!!, ***, 2022/08/09]
    Possibly add support for selections working across multiple Texts. This is essencial for UI since it uses Text to make text
    selectable and copyable. It is probably better for UI to implement this itself, but if it can be nicely implemented into the 
    API itself, that would be cool.



*/

#pragma once
#ifndef DESHI_TEXTEDIT_H
#define DESHI_TEXTEDIT_H

#include "kigu/unicode.h"

enum{
    Text_MoveWord     = 1 << 0,
    Text_MoveWordPart = 1 << 1,
    Text_MoveSelect   = 1 << 2,
};

struct Text{
    str8b buffer;

    struct{
        s64 pos;   //byte offset into buffer
        s64 count; //selection size in byte count
    }cursor;
};

//initializes a Text object with an optional initial string
global Text
text_init(str8 init, Allocator* allocator = deshi_allocator){
    Text out = {0};
    str8_builder_init(&out.buffer, init, deshi_allocator);
    str8_builder_grow(&out.buffer, Max(Kilobytes(1),out.buffer.count));
    return out;
}

//moves the cursor right by one codepoint
//returns how many bytes the cursor moved
global u64
text_move_cursor_right(Text* te, b32 select = 0){
    if(te->cursor.pos == te->buffer.count) return 0;
    DecodedCodepoint dc = decoded_codepoint_from_utf8(te->buffer.str+te->cursor.pos, 4);
    te->cursor.pos+=dc.advance;
    return dc.advance;
}

//moves the cursor right by one word
//returns how many bytes the cursor moved 
global u64 
text_move_cursor_right_word(Text* te, b32 select = 0){
    s64 start = te->cursor.pos;
    u32 punc_codepoint = 0;
    DecodedCodepoint dc = decoded_codepoint_from_utf8(te->buffer.str+te->cursor.pos, 4);
    if(!isalnum(dc.codepoint)) punc_codepoint = dc.codepoint;
    while(1){
        if(!text_move_cursor_right(te, select)) break;
        DecodedCodepoint dc = decoded_codepoint_from_utf8(te->buffer.str+te->cursor.pos, 4);
        if     ( punc_codepoint && dc.codepoint != punc_codepoint) break;
        else if(!punc_codepoint && !isalnum(dc.codepoint) && dc.codepoint != U'_') break;
    }
    return abs(start-te->cursor.pos);
}

//moves the cursor right by one wordpart
//returns how many bytes the cursor moved
//TODO(sushi) currently this does not behave like vscode does, which is the behavoir i want but dont feel like spending time on right now
//            vscode will try and stop at actual words instead of blindly looking for differences in case, for instance
//            ApplesOrangesAndBananas - will stop before each capital and
//            aPPLESoRANGESaNDbANANAS - will stop before each capital that is followed by a lowercase letter
//                 ^      ^  ^       it will place the cursor before these characters
global u64
text_move_cursor_right_wordpart(Text* te, b32 select = 0){
    s64 start = te->cursor.pos;
    u32 punc_codepoint = 0;
    b32 caps = 0;
    DecodedCodepoint dc = decoded_codepoint_from_utf8(te->buffer.str+te->cursor.pos, 4);
    if     (!isalnum(dc.codepoint)) punc_codepoint = dc.codepoint;
    else if( isupper(dc.codepoint)) caps = 1; 
    while(1){
        if(!text_move_cursor_right(te, select)) break;
        DecodedCodepoint dc = decoded_codepoint_from_utf8(te->buffer.str+te->cursor.pos, 4);
        if     ( punc_codepoint && dc.codepoint != punc_codepoint) break;
        else if(!punc_codepoint && !isalnum(dc.codepoint) && dc.codepoint != U'_') break;
        else if( caps           && !isupper(dc.codepoint)) break;
        else if(!caps           &&  isupper(dc.codepoint)) break;
    }
    return abs(start-te->cursor.pos);
}

global u64
text_move_cursor_left(Text* te, Flags flags = 0){
    if(!te->cursor.pos) return 0;
    s64 save = te->cursor.pos;
    while(utf8_continuation_byte(*(te->buffer.str+te->cursor.pos))) te->cursor.pos--;
    te->cursor.pos--;
    return abs(save - te->cursor.pos);
}

//moves the cursor left by one word
//returns how many bytes the cursor moved 
global u64 
text_move_cursor_left_word(Text* te, b32 select = 0){
    s64 start = te->cursor.pos;
    u32 punc_codepoint = 0;
    DecodedCodepoint dc = decoded_codepoint_from_utf8(te->buffer.str+te->cursor.pos, 4);
    if(!isalnum(dc.codepoint)) punc_codepoint = dc.codepoint;
    while(1){
        if(!text_move_cursor_left(te, select)) break;
        DecodedCodepoint dc = decoded_codepoint_from_utf8(te->buffer.str+te->cursor.pos, 4);
        if     ( punc_codepoint && dc.codepoint != punc_codepoint) break;
        else if(!punc_codepoint && !isalnum(dc.codepoint) && dc.codepoint != U'_') break;
    }
    return abs(start-te->cursor.pos);
}

//moves the cursor left by one wordpart
//returns how many bytes the cursor moved
//TODO(sushi) see text_move_cursor_right_wordpart's TODO
global u64
text_move_cursor_left_wordpart(Text* te, b32 select = 0){
    s64 start = te->cursor.pos;
    u32 punc_codepoint = 0;
    b32 caps = 0;
    DecodedCodepoint dc = decoded_codepoint_from_utf8(te->buffer.str+te->cursor.pos, 4);
    if     (!isalnum(dc.codepoint)) punc_codepoint = dc.codepoint;
    else if( isupper(dc.codepoint)) caps = 1; 
    while(1){
        if(!text_move_cursor_left(te, select)) break;
        DecodedCodepoint dc = decoded_codepoint_from_utf8(te->buffer.str+te->cursor.pos, 4);
        if     ( punc_codepoint && dc.codepoint != punc_codepoint) break;
        else if(!punc_codepoint && !isalnum(dc.codepoint) && dc.codepoint != U'_') break;
        else if( caps           && !isupper(dc.codepoint)) break;
        else if(!caps           &&  isupper(dc.codepoint)) break;
    }
    return abs(start-te->cursor.pos);
}

//deletes the selection
global void
text_delete_selection(Text* te){
    if(!te->cursor.count) return;
    u64 offset = Min(te->cursor.pos, te->cursor.pos + te->cursor.count);
    while(te->cursor.count){
        te->cursor.count -= str8_builder_remove_codepoint_at_byteoffset(&te->buffer,offset);
    }
}

//deletes the character to the right of the cursor
//if there is a selection it will just delete the selection instead
global void
text_delete_right(Text* te){
    if(te->cursor.count) return (void)text_delete_selection(te);
    if(te->cursor.pos == te->buffer.count) return; 
    str8_builder_remove_codepoint_at_byteoffset(&te->buffer,te->cursor.pos);
}

//deletes the character to the left of the cursor
//if there is a selection it will just delete the selection instead
global void
text_delete_left(Text* te){
    if(te->cursor.count) return (void)text_delete_selection(te);
    if(!te->cursor.pos) return;
    text_move_cursor_left(te);
    text_delete_right(te);
}

//inserts the given string 's' at the cursor
//if there is a selection it overwrites it with 's'
global void
text_insert_string(Text* te, str8 s){
    if(te->cursor.count) text_delete_selection(te);
    if(te->buffer.count + s.count < te->buffer.space) str8_builder_grow(&te->buffer, s.count);
    str8_builder_insert_byteoffset(&te->buffer,te->cursor.pos,s);
}

//returns a str8 of the Text's selection
global str8
text_get_selection(Text* te){
    u64 offset = Min(te->cursor.pos,te->cursor.pos+te->cursor.count);
    return str8{te->buffer.str + offset, abs(te->cursor.count)};
}

//completely clears the Text's buffer and resets its information
global void
text_clear(Text* t){
    str8_builder_clear(&t->buffer);
    t->cursor = {0};
}

//completely clears the Text's buffer and replaces it with the given string
global void
text_clear_and_replace(Text* t, str8 str){
    text_clear(t);
    text_insert_string(t,str);
}


#endif