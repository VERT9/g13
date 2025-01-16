/*
 	 pixels are mapped rather strangely for G13 buffer...

	  byte 0 contains column 0 / row 0 - 7
	  byte 1 contains column 1 / row 0 - 7

	 so the masks for each pixel are laid out as below (ByteOffset.PixelMask)

	 00.01 01.01 02.01 ...
	 00.02 01.02 02.02 ...
	 00.04 01.04 02.04 ...
	 00.08 01.08 02.08 ...
	 00.10 01.10 02.10 ...
	 00.20 01.20 02.20 ...
	 00.40 01.40 02.40 ...
	 00.80 01.80 02.80 ...
	 A0.01 A1.01 A2.01 ...
 */

#include "g13_font_constants.h"
#include "g13_fonts.h"
#include "g13_device.h"
#include "g13_lcd.h"
#include "g13_log.h"

using namespace std;

namespace G13 {
	G13_LCD::G13_LCD(G13_Device& keypad, G13_Log& logger) : _keypad(keypad), _logger(logger) {
		cursor_col = 0;
		cursor_row = 0;
		text_mode = 0;

		_init_fonts();
	}

	void G13_LCD::_init_fonts() {
		_current_font = std::make_shared<G13_Font>("8x8", 8);
		_fonts[_current_font->name()] = _current_font;

		_current_font->install_font(font8x8_basic, G13_FontChar::FF_ROTATE, 0);

		const auto fiveXeight = std::make_shared<G13_Font>("5x8", 5);
		fiveXeight->install_font(font5x8, 0, 32);
		_fonts[fiveXeight->name()] = fiveXeight;
	}

	void G13_LCD::image(unsigned char* data, int size) {
		// TODO remove circular referencing
		_keypad.write_lcd(data, size);
	}

	void G13_LCD::image_setpixel(unsigned row, unsigned col) {
		unsigned offset = image_byte_offset(row, col); // col + (row /8 ) * BYTES_PER_ROW * 8;
		unsigned char mask = 1 << ((row) & 7);

		if (offset >= G13_LCD_BUF_SIZE) {
			_logger.error("bad offset " + std::to_string(offset) + " for " + std::to_string(row) + " x " + std::to_string(col));
			return;
		}

		image_buf[offset] |= mask;
	}

	void G13_LCD::image_clearpixel(unsigned row, unsigned col) {
		unsigned offset = image_byte_offset(row, col); // col + (row /8 ) * BYTES_PER_ROW * 8;
		unsigned char mask = 1 << ((row) & 7);

		if (offset >= G13_LCD_BUF_SIZE) {
			_logger.error("bad offset " + std::to_string(offset) + " for " + std::to_string(row) + " x " + std::to_string(col));
			return;
		}
		image_buf[offset] &= ~mask;
	}

	void G13_LCD::write_char(char c, int row, int col) {
		if (row == -1) {
			row = cursor_row;
			col = cursor_col;
			cursor_col += current_font().width();
			if (cursor_col >= G13_LCD_COLUMNS) {
				cursor_col = 0;
				if (++cursor_row >= G13_LCD_TEXT_ROWS) {
					cursor_row = 0;
				}
			}
		}

		unsigned offset = image_byte_offset(row * G13_LCD_TEXT_CHEIGHT, col); //*_keypad._current_font->_width );
		if (text_mode) {
			memcpy(&image_buf[offset], &current_font().char_data(c).bits_inverted,
				   current_font().width());
		} else {
			memcpy(&image_buf[offset], &current_font().char_data(c).bits_regular,
				   current_font().width());
		}
	}

	/**
	 * Writes a string to the LCD at the current position. By default, the string
	 * is sent directly to the LCD. The flush parameter can be used to disabled
	 * this allowing for multiple strings to be queued in the buffer before
	 * sending to the LCD. Helps with fading images.
	 *
	 * @param char* str
	 * @param bool flush
	 */
	void G13_LCD::write_string(const char* str, bool flush) {
		_logger.trace("writing \"" + std::string(str) + "\"");
		while (*str) {
			if (*str == '\n') {
				cursor_col = 0;
				if (++cursor_row >= G13_LCD_TEXT_ROWS) {
					cursor_row = 0;
				}
			} else if (*str == '\t') {
				cursor_col += 4 - (cursor_col % 4);
				if (++cursor_col >= G13_LCD_COLUMNS) {
					cursor_col = 0;
					if (++cursor_row >= G13_LCD_TEXT_ROWS) {
						cursor_row = 0;
					}
				}
			} else {
				write_char(*str);
			}
			++str;
		}

		if (flush)
			image_send();
	}

	void G13_LCD::write_pos(int row, int col) {
		cursor_row = row;
		cursor_col = col;
		if (cursor_col >= G13_LCD_COLUMNS) {
			cursor_col = 0;
		}
		if (cursor_row >= G13_LCD_TEXT_ROWS) {
			cursor_row = 0;
		}
	}

	void G13_LCD::image_test(int x, int y) {
		int row = 0, col = 0;
		if (y >= 0) {
			image_setpixel(x, y);
		} else {
			image_clear();
			switch (x) {
				case 1:
					for (row = 0; row < G13_LCD_ROWS; ++row) {
						col = row;
						image_setpixel(row, col);
						image_setpixel(row, G13_LCD_COLUMNS - col);
					}
					break;

				case 2:
				default:
					for (row = 0; row < G13_LCD_ROWS; ++row) {
						col = row;
						image_setpixel(row, 8);
						image_setpixel(row, G13_LCD_COLUMNS - 8);
						image_setpixel(row, G13_LCD_COLUMNS / 2);
						image_setpixel(row, col);
						image_setpixel(row, G13_LCD_COLUMNS - col);
					}
					break;
			}
		}
		image_send();
	}

	FontPtr G13_LCD::switch_to_font(const std::string& name) {
		FontPtr rv = _fonts[name];
		if (rv) {
			_current_font = rv;
		}
		return rv;
	}
} // namespace G13

