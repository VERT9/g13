//
// Created by vert9 on 11/23/23.
//

#ifndef G13_G13_FONTS_H
#define G13_G13_FONTS_H

#include <cstring>
#include <string>

namespace G13 {
	class G13_FontChar {
	public:
		static const int CHAR_BUF_SIZE = 8;
		enum FONT_FLAGS { FF_ROTATE = 0x01 };

		G13_FontChar() {
			memset(bits_regular, 0, CHAR_BUF_SIZE);
			memset(bits_inverted, 0, CHAR_BUF_SIZE);
		}
		void set_character(unsigned char* data, int width, unsigned flags);
		unsigned char bits_regular[CHAR_BUF_SIZE];
		unsigned char bits_inverted[CHAR_BUF_SIZE];
	};

	class G13_Font {
	public:
		G13_Font();
		G13_Font(const std::string& name, unsigned int width = 8);

		void set_character(unsigned int c, unsigned char* data);

		template<typename T, int size>
			int GetFontCharacterCount(T(&)[size]) { return size; }
		template<class ARRAY_T, class FLAGST>
			void install_font(ARRAY_T& data, FLAGST flags, int first = 0) {
				for (size_t i = 0; i < GetFontCharacterCount(data); i++) {
					_chars[i + first].set_character(&data[i][0], _width, flags);
				}
			}

		const std::string& name() const { return _name; }
		unsigned int width() const { return _width; }

		const G13_FontChar& char_data(unsigned int x) { return _chars[x]; }
	protected:
		std::string _name;
		unsigned int _width;

		G13_FontChar _chars[256];

		//unsigned char font_basic[256][8];
		//unsigned char font_inverted[256][8];
	};
}

#endif //G13_G13_FONTS_H
