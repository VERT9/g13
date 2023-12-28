#include "g13_fonts.h"

using namespace std;

namespace G13 {
	G13_Font::G13_Font() : _name("default"), _width(8) {}
	G13_Font::G13_Font(const std::string& name, unsigned int width) : _name(name), _width(width) {}

	void G13_FontChar::set_character(unsigned char* data, int width, unsigned flags) {
		unsigned char* dest = bits_regular;
		memset(dest, 0, CHAR_BUF_SIZE);
		if (flags && FF_ROTATE) {
			for (int x = 0; x < width; x++) {
				unsigned char x_mask = 1 << x;
				for (int y = 0; y < 8; y++) {
					unsigned char y_mask = 1 << y;
					if (data[y] & x_mask) {
						dest[x] |= 1 << y;
					}
				}
			}
		} else {
			memcpy(dest, data, width);
		}
		for (int x = 0; x < width; x++) {
			bits_inverted[x] = ~dest[x];
		}
	}
} // namespace G13

