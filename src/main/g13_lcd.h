//
// Created by vert9 on 11/23/23.
//

#ifndef G13_G13_LCD_H
#define G13_G13_LCD_H

#include <cstdlib>
#include <cstring>
#include "g13_log.h"

using namespace std;
namespace G13 {
	class G13_Device;

	const size_t G13_LCD_COLUMNS = 160;
	const size_t G13_LCD_ROWS = 48;
	const size_t G13_LCD_BYTES_PER_ROW = G13_LCD_COLUMNS / 8;
	const size_t G13_LCD_BUF_SIZE = G13_LCD_ROWS * G13_LCD_BYTES_PER_ROW;
	const size_t G13_LCD_TEXT_CHEIGHT = 8;
	const size_t G13_LCD_TEXT_ROWS = 160 / G13_LCD_TEXT_CHEIGHT;

	class G13_LCD {
	public:
		G13_LCD(G13_Device& keypad, G13_Log& logger);

		G13_Device& _keypad;
		G13_Log& _logger;
		unsigned char image_buf[G13_LCD_BUF_SIZE + 8];
		unsigned cursor_row;
		unsigned cursor_col;
		int text_mode;

		void image(unsigned char* data, int size);

		void image_send() {
			image(image_buf, G13_LCD_BUF_SIZE);
		}

		void image_clear() {
			memset(image_buf, 0, G13_LCD_BUF_SIZE);
		}

		unsigned image_byte_offset(unsigned row, unsigned col) {
			return col + (row / 8) * G13_LCD_BYTES_PER_ROW * 8;
		}

		void image_setpixel(unsigned row, unsigned col);
		void image_clearpixel(unsigned row, unsigned col);

		void write_char(char c, int row = -1, int col = -1);
		void write_string(const char* str);
		void write_pos(int row, int col);

		void image_test(int x, int y);
	};
}

#endif //G13_G13_LCD_H
