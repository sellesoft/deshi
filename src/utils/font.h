#pragma once

#ifndef DESHI_FONT_H
#define DESHI_FONT_H

#include "../core/assets.h"
#include "../defines.h"
#include "string.h"
#include "array.h"
#include "tuple.h"
#include "../core/console.h"
#include "../math/Math.h"
#include "../utils/utils.h"

static string hexCharToBin(char c) {
	switch (toupper(c)) { //TODO(sushi, Cl) make it so we can return a str literal in a function that returns string
		case '0': return string("0000");
		case '1': return string("0001");
		case '2': return string("0010");
		case '3': return string("0011");
		case '4': return string("0100");
		case '5': return string("0101");
		case '6': return string("0110");
		case '7': return string("0111");
		case '8': return string("1000");
		case '9': return string("1001");
		case 'A': return string("1010");
		case 'B': return string("1011");
		case 'C': return string("1100");
		case 'D': return string("1101");
		case 'E': return string("1110");
		case 'F': return string("1111");
	}
	return string("");
}

static u32 four_u8_to_u32(u8 a, u8 b, u8 c, u8 d) {
	return ((u32)d << 24) | ((u32)c << 16) | ((u32)b << 8) | ((u32)a << 0);
}


const u32 white = ((u32)255 << 24) | ((u32)255 << 16) | ((u32)255 << 8) | ((u32)255 << 0);

struct Font {
	u32 width;
	u32 height;
	
	u32 char_count = 0;
	
	string name = "";
	string weight = "";
	
	u32 font_size;
	vec2 dpi;
	
	vec4 bbx;
	
	array<pair<u32, array<u32>>> textures;
	
	u32* texture_sheet;
	
	//this is currently set up to only support monospaced fonts
	void load_bdf_font(const char* fontname) {
		char* file = Assets::readFileAsciiToArray(Assets::dirFonts() + fontname);
		if (!file) { return; }
		defer{ delete[] file; };
		
		string buff = "";
		char* currChar = file;
		
		u32 line_number = 0;
		
		while (*currChar != 0) {
			while (*currChar != '\n') {
				buff += *currChar;
				currChar++;
				
				//key has stopped and we're getting values
				if (*currChar == ' ') {
					if (!line_number) {
						Assert(buff == "STARTFONT", "Not a bdf file or is invalid");
						while(*++currChar != '\n'){}
						currChar++;
						line_number++;
					}
					else if (buff == "SIZE") {
						string value = "";
						
						//get the font pixel size
						currChar += Utils::skipSpacesLeading(currChar);
						while (*currChar != ' ' && *currChar != '\n') { value += *currChar; currChar++; }
						font_size = string::stoi(value);
						
						value.clear();
						
						//get x dpi
						currChar += Utils::skipSpacesLeading(currChar);
						while (*currChar != ' ' && *currChar != '\n') { value += *currChar; currChar++; }
						dpi.x = string::stoi(value);
						
						value.clear();
						
						//get y dpi
						currChar += Utils::skipSpacesLeading(currChar);
						while (*currChar != ' ' && *currChar != '\n') { value += *currChar; currChar++; }
						dpi.y = string::stoi(value);
						
					}
					else if (buff == "FONTBOUNDINGBOX") {
						string value = "";
						
						//get font bounding box width
						currChar += Utils::skipSpacesLeading(currChar);
						while (*currChar != ' ') { value += *currChar; currChar++; }
						bbx.x = string::stoi(value);
						width = bbx.x;
						
						value.clear();
						
						//get font bounding box height
						currChar += Utils::skipSpacesLeading(currChar);
						while (*currChar != ' ') { value += *currChar; currChar++; }
						bbx.y = string::stoi(value);
						height = bbx.y;
						
						value.clear();
						
						//get lower left x
						currChar += Utils::skipSpacesLeading(currChar);
						while (*currChar != ' ') { value += *currChar; currChar++; }
						bbx.z = string::stoi(value);
						
						value.clear();
						
						//get lower left y
						currChar += Utils::skipSpacesLeading(currChar);
						while (*currChar != ' ') { value += *currChar; currChar++; }
						bbx.w = string::stoi(value);
						
					}
					else if (buff == "FONT_NAME") {
						currChar += Utils::skipSpacesLeading(currChar);
						while (*currChar != ' ') { if(*currChar != '"') name += *currChar; currChar++; }
					}
					else if (buff == "WEIGHT_NAME") {
						currChar += Utils::skipSpacesLeading(currChar);
						while (*currChar != ' ') { if (*currChar != '"') weight += *currChar; currChar++; }
					}
					else if (buff == "CHARS") {
						string value = "";
						
						currChar += Utils::skipSpacesLeading(currChar);
						while (*currChar != ' ' && *currChar != '\n') { if (*currChar != '"') value += *currChar; currChar++; }
						char_count = string::stoi(value);
					}
					else if (buff == "STARTCHAR") {
						//begin creating the character's texture array 
						while (*++currChar != '\n') {}
						currChar++;
						line_number++;
						
						string value = "";
						
						//get encoding
						u32 encoding = 0;
						while (*currChar++ != ' ') {} //skip ENCODING key
						
						while (*currChar != ' ') { if (*currChar != '"') value += *currChar; currChar++; }
						encoding = string::stoi(value);
						
						value.clear();
						
						//move onto SWIDTH, which we dont use so we skip it
						while (*++currChar != '\n') {}
						currChar++;
						line_number++;
						
						//move onto DWIDTH
						//since this only supports monospace fonts this should always be the same
						//and sets the width variable for the font
						while (*currChar++ != ' ') {} //skip DWIDTH key
						
						while (*currChar != ' ') { if (*currChar != '"') value += *currChar; currChar++; }
						
						//we ignore the second value of DWIDTH for now
						while (*++currChar != '\n') {}
						currChar++;
						line_number++;
						
						value.clear();
						
						//move onto BBX
						//this determines the smallest box that can enclose the glyph and it's offset from the origin
						vec4 gbbx;
						
						while (*currChar++ != ' ') {} //skip BBX key
						
						//get glyph bounding box width
						currChar += Utils::skipSpacesLeading(currChar);
						while (*currChar != ' ') { value += *currChar; currChar++; }
						gbbx.x = string::stoi(value);
						
						value.clear();
						
						//get glyph bounding box height
						currChar += Utils::skipSpacesLeading(currChar);
						while (*currChar != ' ') { value += *currChar; currChar++; }
						gbbx.y = string::stoi(value);
						
						value.clear();
						
						//get lower left x
						currChar += Utils::skipSpacesLeading(currChar);
						while (*currChar != ' ') { value += *currChar; currChar++; }
						gbbx.z = string::stoi(value);
						
						value.clear();
						
						//get lower left y
						currChar += Utils::skipSpacesLeading(currChar);
						while (*currChar != '\n') { value += *currChar; currChar++; }
						gbbx.w = string::stoi(value);
						
						currChar++;
						
						//begin building the texture array for the character
						array<u32> tex;
						
						for (int i = 0; i < bbx.x * bbx.y; i++) {
							tex.add(0);
						}
						
						//skip over BITMAP key
						while (*++currChar != '\n') {}
						currChar++;
						line_number++;
						
						
						//decend down rows of pixels and turn their hex representation
						//into binary, then into u32 representation for texture
						for (int row = gbbx.y; row != 0; row--) {
							//calculate where in the tex array we will be drawing this row of pixels
							//NOTE: this may fail if a font has a positive y offset so be wary of that
							int bbxrow = bbx.y - (gbbx.w + row - bbx.w);
							if (*currChar == '\n') currChar++;
							string binary = hexCharToBin(*currChar++) + hexCharToBin(*currChar++);
							
							//turn binary string into array of u8s
							//TODO(sushi, OpCl) do this better
							u8 binArr[8];
							for (int stoa = 0; stoa < 8; stoa++) {
								binArr[stoa] = (u8)string::stoi(binary[stoa]);
							}
							
							for (int col = gbbx.z, idx = 0; col < gbbx.z + gbbx.x; col++, idx++) {
								if (binArr[idx]) {
									tex[bbxrow * bbx.x + col] = white;
								}
							}
							
							//PRINTLN(binary);
						}
						
						/*
						PRINTLN("");
						for (int i = 1; i < tex.size() + 1; i++) {
							std::cout << tex[i - 1];
							if (i % (int)bbx.x == 0) std::cout << "\n";
						}
						*/
						
						textures.add(pair<u32, array<u32>>(encoding, tex));
					}
					else {
						//if we dont find any of the keys we need proceed to next line
						while (*++currChar != '\n') {}
						currChar++;
						line_number++;
					}
					
					buff.clear();
				}
				
			}
			buff.clear();
			currChar++;
			line_number++;
		}
		
		//set up texture_sheet
		//i try to align the characters in rows here and account for the remaining white space
		
		//attempt to align into 4 rows
		
		
		
		texture_sheet = (u32*)calloc(char_count, width * height * sizeof(u32));
		
		for (int i = 0; i < textures.size(); i++) {
			array<u32> tex = textures[i].second;
			memcpy(texture_sheet + i * width * height, tex.data, width * height * sizeof(u32));
		}
	}	
	
	
	
	
};



#endif