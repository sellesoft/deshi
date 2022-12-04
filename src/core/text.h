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
    str8_builder_init(&out.buffer, init, allocator);
    str8_builder_grow(&out.buffer, Max(Kilobytes(1),out.buffer.count));
    return out;
}

//moves the cursor right by one codepoint
//returns how many bytes the cursor moved
global u64
text_move_cursor_right(Text* t, b32 select = 0){
    if(t->cursor.pos == t->buffer.count) return 0;
    DecodedCodepoint dc = decoded_codepoint_from_utf8(t->buffer.str+t->cursor.pos, 4);
    t->cursor.pos+=dc.advance;
    if(select) t->cursor.count -= dc.advance;
    else       t->cursor.count = 0;
    return dc.advance;
}

//moves the cursor right by one word
//returns how many bytes the cursor moved 
global u64 
text_move_cursor_right_word(Text* t, b32 select = 0){
    s64 start = t->cursor.pos;
    u32 punc_codepoint = 0;
    DecodedCodepoint dc = decoded_codepoint_from_utf8(t->buffer.str+t->cursor.pos, 4);
    if(!isalnum(dc.codepoint)) punc_codepoint = dc.codepoint;
    while(1){
        if(!text_move_cursor_right(t, select)) break;
        DecodedCodepoint dc = decoded_codepoint_from_utf8(t->buffer.str+t->cursor.pos, 4);
        if     ( punc_codepoint && dc.codepoint != punc_codepoint) break;
        else if(!punc_codepoint && !isalnum(dc.codepoint) && dc.codepoint != U'_') break;
    }
    return abs(start-t->cursor.pos);
}

//moves the cursor right by one wordpart
//returns how many bytes the cursor moved
//TODO(sushi) currently this does not behave like vscode does, which is the behavoir i want but dont feel like spending time on right now
//            vscode will try and stop at actual words instead of blindly looking for differences in case, for instance
//            ApplesOrangesAndBananas - will stop before each capital and
//            aPPLESoRANGESaNDbANANAS - will stop before each capital that is followed by a lowercase letter
//                 ^      ^  ^       it will place the cursor before these characters
global u64
text_move_cursor_right_wordpart(Text* t, b32 select = 0){
    s64 start = t->cursor.pos;
    u32 punc_codepoint = 0;
    b32 caps = 0;
    DecodedCodepoint dc = decoded_codepoint_from_utf8(t->buffer.str+t->cursor.pos, 4);
    if     (!isalnum(dc.codepoint)) punc_codepoint = dc.codepoint;
    else if( isupper(dc.codepoint)) caps = 1; 
    while(1){
        if(!text_move_cursor_right(t, select)) break;
        DecodedCodepoint dc = decoded_codepoint_from_utf8(t->buffer.str+t->cursor.pos, 4);
        if     ( punc_codepoint && dc.codepoint != punc_codepoint) break;
        else if(!punc_codepoint && !isalnum(dc.codepoint) && dc.codepoint != U'_') break;
        else if( caps           && !isupper(dc.codepoint)) break;
        else if(!caps           &&  isupper(dc.codepoint)) break;
    }
    return abs(start-t->cursor.pos);
}

global u64
text_move_cursor_left(Text* t, b32 select = 0){
    if(!t->cursor.pos) return 0;
    s64 start = t->cursor.pos;
    while(utf8_continuation_byte(*(t->buffer.str+t->cursor.pos))) t->cursor.pos--;
    t->cursor.pos--;
    if(select) t->cursor.count += abs(start - t->cursor.pos);
    else       t->cursor.count = 0;
    return abs(start - t->cursor.pos);
}

//moves the cursor left by one word
//returns how many bytes the cursor moved 
global u64 
text_move_cursor_left_word(Text* t, b32 select = 0){
    s64 start = t->cursor.pos;
    u32 punc_codepoint = 0;
    DecodedCodepoint dc = decoded_codepoint_from_utf8(t->buffer.str+t->cursor.pos, 4);
    if(!isalnum(dc.codepoint)) punc_codepoint = dc.codepoint;
    while(1){
        if(!text_move_cursor_left(t, select)) break;
        DecodedCodepoint dc = decoded_codepoint_from_utf8(t->buffer.str+t->cursor.pos, 4);
        if     ( punc_codepoint && dc.codepoint != punc_codepoint) break;
        else if(!punc_codepoint && !isalnum(dc.codepoint) && dc.codepoint != U'_') break;
    }
    return abs(start-t->cursor.pos);
}

//moves the cursor left by one wordpart
//returns how many bytes the cursor moved
//TODO(sushi) see text_move_cursor_right_wordpart's TODO
global u64
text_move_cursor_left_wordpart(Text* t, b32 select = 0){
    s64 start = t->cursor.pos;
    u32 punc_codepoint = 0;
    b32 caps = 0;
    DecodedCodepoint dc = decoded_codepoint_from_utf8(t->buffer.str+t->cursor.pos, 4);
    if     (!isalnum(dc.codepoint)) punc_codepoint = dc.codepoint;
    else if( isupper(dc.codepoint)) caps = 1; 
    while(1){
        if(!text_move_cursor_left(t, select)) break;
        DecodedCodepoint dc = decoded_codepoint_from_utf8(t->buffer.str+t->cursor.pos, 4);
        if     ( punc_codepoint && dc.codepoint != punc_codepoint) break;
        else if(!punc_codepoint && !isalnum(dc.codepoint) && dc.codepoint != U'_') break;
        else if( caps           && !isupper(dc.codepoint)) break;
        else if(!caps           &&  isupper(dc.codepoint)) break;
    }
    return abs(start-t->cursor.pos);
}

//deletes the selection
global void
text_delete_selection(Text* t){
    if(!t->cursor.count) return;
    if(t->cursor.count < 0){
        t->cursor.pos += t->cursor.count;
        t->cursor.count *= -1;
    }
    //u64 offset = Min(t->cursor.pos, t->cursor.pos + t->cursor.count);
    while(t->cursor.count){
        t->cursor.count -= str8_builder_remove_codepoint_at_byteoffset(&t->buffer,t->cursor.pos);
    }
}

//deletes the character to the right of the cursor
//if there is a selection it will just delete the selection instead
global void
text_delete_right(Text* t){
    if(t->cursor.count) return (void)text_delete_selection(t);
    if(t->cursor.pos == t->buffer.count) return; 
    str8_builder_remove_codepoint_at_byteoffset(&t->buffer,t->cursor.pos);
}

//deletes the word to the right of the cursor
//if there is a selection it will just delete the selection instead
global void
text_delete_right_word(Text* t){
    if(t->cursor.count) return (void)text_delete_selection(t);
    if(t->cursor.pos == t->buffer.count) return; 
    u64 save = t->cursor.pos;
    u64 ndel = text_move_cursor_right_word(t);
    forI(ndel){
        str8_builder_remove_codepoint_at_byteoffset(&t->buffer,save);
    }
    t->cursor.pos = save;
}

//deletes the wordpart to the right of the cursor
//if there is a selection it will just delete the selection instead
global void
text_delete_right_wordpart(Text* t){
    if(t->cursor.count) return (void)text_delete_selection(t);
    if(t->cursor.pos == t->buffer.count) return; 
    u64 save = t->cursor.pos;
    u64 ndel = text_move_cursor_right_wordpart(t);
    forI(ndel){
        str8_builder_remove_codepoint_at_byteoffset(&t->buffer,save);
    }
    t->cursor.pos = save;
}

//deletes the character to the left of the cursor
//if there is a selection it will just delete the selection instead
global void
text_delete_left(Text* t){
    if(t->cursor.count) return (void)text_delete_selection(t);
    if(!t->cursor.pos) return;
    text_move_cursor_left(t);
    text_delete_right(t);
}

//deletes the character to the left of the cursor
//if there is a selection it will just delete the selection instead
global void
text_delete_left_word(Text* t){
    if(t->cursor.count) return (void)text_delete_selection(t);
    if(!t->cursor.pos) return;
    u64 ndel = text_move_cursor_left_word(t);
    forI(ndel){
        str8_builder_remove_codepoint_at_byteoffset(&t->buffer,t->cursor.pos);
    }
}

//deletes the character to the left of the cursor
//if there is a selection it will just delete the selection instead
global void
text_delete_left_wordpart(Text* t){
    if(t->cursor.count) return (void)text_delete_selection(t);
    if(!t->cursor.pos) return;
    u64 ndel = text_move_cursor_left_wordpart(t);
    forI(ndel){
        str8_builder_remove_codepoint_at_byteoffset(&t->buffer,t->cursor.pos);
    }
}


//inserts the given string 's' at the cursor
//if there is a selection it overwrites it with 's'
global void
text_insert_string(Text* t, str8 s){
    if(t->cursor.count) text_delete_selection(t);
    if(t->buffer.count + s.count >= t->buffer.space) str8_builder_grow(&t->buffer, s.count);
    str8_builder_insert_byteoffset(&t->buffer,t->cursor.pos,s);
    t->cursor.pos += s.count;
}

//returns a str8 of the Text's selection
global str8
text_get_selection(Text* t){
    u64 offset = Min(t->cursor.pos,t->cursor.pos+t->cursor.count);
    return str8{t->buffer.str + offset, abs(t->cursor.count)};
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