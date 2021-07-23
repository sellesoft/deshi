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

const char* hexCharToBin(char c) {
	switch (toupper(c)) {
		case '0': return "0000";
		case '1': return "0001";
		case '2': return "0010";
		case '3': return "0011";
		case '4': return "0100";
		case '5': return "0101";
		case '6': return "0110";
		case '7': return "0111";
		case '8': return "1000";
		case '9': return "1001";
		case 'A': return "1010";
		case 'B': return "1011";
		case 'C': return "1100";
		case 'D': return "1101";
		case 'E': return "1110";
		case 'F': return "1111";
	}
}


struct Font {
	u32 width;
	u32 height;

	u32 char_count = 0;

	string name = "";
	string weight = "";

	u32 font_size;
	Vector2 dpi;

	Vector4 bbx;

	array<pair<u32, array<u32>>> textures;

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
						while (*currChar != ' ') { if (*currChar != '"') value += *currChar; currChar++; }
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
						Vector4 glyphbbx;
						//get glyph bounding box width
						currChar += Utils::skipSpacesLeading(currChar);
						while (*currChar != ' ') { value += *currChar; currChar++; }
						glyphbbx.x = string::stoi(value);

						value.clear();

						//get glyph bounding box height
						currChar += Utils::skipSpacesLeading(currChar);
						while (*currChar != ' ') { value += *currChar; currChar++; }
						glyphbbx.y = string::stoi(value);

						value.clear();

						//get lower left x
						currChar += Utils::skipSpacesLeading(currChar);
						while (*currChar != ' ') { value += *currChar; currChar++; }
						glyphbbx.z = string::stoi(value);

						value.clear();

						//get lower left y
						currChar += Utils::skipSpacesLeading(currChar);
						while (*currChar != ' ') { value += *currChar; currChar++; }
						glyphbbx.w = string::stoi(value);

						


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

	}	
	

};



#endif