//
// Created by vert9 on 1/4/25.
//

#ifndef G13_KEY_MAP_H
#define G13_KEY_MAP_H

#include <format>
#include <map>
#include <memory>
#include <libevdev-1.0/libevdev/libevdev.h>
#include <string>

#include "container.h"
#include "helper.h"
#include "g13_log.h"

namespace G13 {
	typedef int G13_KEY_INDEX;
	typedef int LINUX_KEY_VALUE;
	const LINUX_KEY_VALUE BAD_KEY_VALUE = -1;

	/*! G13_KEY_SEQ was a Boost Preprocessor sequence containing the
	 * G13 keys. Now it's an array of strings. The order is very specific,
	 * with the position of each item corresponding to a specific bit in
	 * the G13's USB message format. Do NOT remove or insert items in this list.
	 */
	const std::string G13_KEY_SEQ[] = {
		/* byte 3 */ "G1", "G2", "G3", "G4", "G5", "G6", "G7", "G8",
		/* byte 4 */ "G9", "G10", "G11", "G12", "G13", "G14", "G15", "G16",
		/* byte 5 */ "G17", "G18", "G19", "G20", "G21", "G22", "UNDEF1", "LIGHT_STATE",
		/* byte 6 */ "BD", "L1", "L2", "L3", "L4", "M1", "M2", "M3",
		/* byte 7 */ "MR", "LEFT", "DOWN", "TOP", "UNDEF3", "LIGHT", "LIGHT2", "MISC_TOGGLE"
	};


	/*! G13_NONPARSED_KEY_SEQ was a Boost Preprocessor sequence containing the
	 * G13 keys that shouldn't be tested input. Now it's an array of strings.
	 * These aren't actually keys, but they are in the bitmap defined by G13_KEY_SEQ.
	 */
	const std::string G13_NONPARSED_KEY_SEQ[] = {
		"UNDEF1","LIGHT_STATE","UNDEF3","LIGHT","LIGHT2","UNDEF3","MISC_TOGGLE"
	};

	/*! KB_INPUT_KEY_SEQ was a Boost Preprocessor sequence containing the
	 * names of keyboard keys we can send through binding actions. Now it's
	 * an array of strings. These correspond to KEY_xxx value definitions in
	 * <linux/input.h>, i.e. ESC is KEY_ESC, 1 is KEY_1, etc.
	 */
	const std::string KB_INPUT_KEY_SEQ[] = {
		"ESC", "1", "2", "3", "4", "5", "6", "7", "8", "9", "0",
		"MINUS", "EQUAL", "BACKSPACE", "TAB",
		"Q", "W", "E", "R", "T", "Y", "U", "I", "O", "P",
		"LEFTBRACE", "RIGHTBRACE", "ENTER", "LEFTCTRL", "RIGHTCTRL",
		"A", "S", "D", "F", "G", "H", "J", "K", "L",
		"SEMICOLON", "APOSTROPHE", "GRAVE", "LEFTSHIFT", "BACKSLASH",
		"Z", "X", "C", "V", "B", "N", "M",
		"COMMA", "DOT", "SLASH", "RIGHTSHIFT", "KPASTERISK",
		"LEFTALT", "RIGHTALT", "SPACE", "CAPSLOCK",
		"F1", "F2", "F3", "F4", "F5", "F6", "F7", "F8", "F9", "F10", "F11", "F12",
		"NUMLOCK", "SCROLLLOCK",
		"KP7", "KP8", "KP9", "KPMINUS", "KP4", "KP5", "KP6", "KPPLUS",
		"KP1", "KP2", "KP3", "KP0", "KPDOT",
		"LEFT", "RIGHT", "UP", "DOWN",
		"PAGEUP", "PAGEDOWN", "HOME", "END", "INSERT", "DELETE"
	};

	class G13_KeyMap {
		public:
			static std::shared_ptr<G13_KeyMap> get() {
				static std::shared_ptr<G13_KeyMap> instance{new G13_KeyMap(Container::Instance().Resolve<G13_Log>())};
				return instance;
			}
			G13_KeyMap(const G13_KeyMap&) = delete;
			G13_KeyMap& operator=(const G13_KeyMap&) = delete;
		private:
			explicit G13_KeyMap(const std::shared_ptr<G13_Log>& _logger) {
				// setup maps to let us convert between strings and G13 key names
				int key_index = 0;
				for (const std::string& name: G13_KEY_SEQ) {
					g13_key_to_name[key_index] = name;
					g13_name_to_key[name] = key_index;
					key_index++;
				}

				// setup maps to let us convert between strings and linux key names
				for (const std::string& name: KB_INPUT_KEY_SEQ) {
					int keyval = libevdev_event_code_from_name(EV_KEY, ("KEY_"+name).c_str());
					if (keyval != BAD_KEY_VALUE) {
						input_key_to_name[keyval] = name;
						input_name_to_key[name] = keyval;
					} else {
						_logger->warning(std::format("Unknown key code []", name));
					}
				}
			}
		public:
			LINUX_KEY_VALUE find_g13_key_value(const std::string& keyname) const {
				auto i = g13_name_to_key.find(keyname);
				if (i == g13_name_to_key.end()) {
					return BAD_KEY_VALUE;
				}
				return i->second;
			}

			LINUX_KEY_VALUE find_input_key_value(const std::string& keyname) const {
				// if there is a KEY_ prefix, strip it off
				if (!strncmp(keyname.c_str(), "KEY_", 4)) {
					return find_input_key_value(keyname.c_str() + 4);
				}

				auto i = input_name_to_key.find(keyname);
				if (i == input_name_to_key.end()) {
					return BAD_KEY_VALUE;
				}
				return i->second;
			}

			std::string find_input_key_name(LINUX_KEY_VALUE v) const {
				try {
					return Helper::find_or_throw(input_key_to_name, v);
				}
				catch (...) {
					return "(unknown linux key)";
				}
			}

			std::string find_g13_key_name(G13_KEY_INDEX v) const {
				try {
					return Helper::find_or_throw(g13_key_to_name, v);
				}
				catch (...) {
					return "(unknown G13 key)";
				}
			}

			template <class MAP_T>
			struct _map_keys_out {
				_map_keys_out( const MAP_T&c, const std::string &s ) : container(c), sep(s) {}
				const MAP_T&container;
				std::string sep;
			};

			template <class STREAM_T, class MAP_T>
			friend STREAM_T &operator <<( STREAM_T &o, const _map_keys_out<MAP_T> &_mko ) {
				bool first = true;
				for( auto i = _mko.container.begin(); i != _mko.container.end(); i++ ) {
					if( first ) {
						first = false;
						o << i->first;
					} else {
						o << _mko.sep << i->first;
					}
				}

				return o;
			};

			template <class MAP_T>
			_map_keys_out<MAP_T> map_keys_out( const MAP_T &c, const std::string &sep = " " ) {
				return _map_keys_out<MAP_T>( c, sep );
			};

			auto map_G13_keys() {
				return map_keys_out(g13_name_to_key);
			}

			auto map_input_keys() {
				return map_keys_out(input_name_to_key);
			}

		private:
			std::map<G13_KEY_INDEX, std::string> g13_key_to_name;
			std::map<std::string, G13_KEY_INDEX> g13_name_to_key;
			std::map<LINUX_KEY_VALUE, std::string> input_key_to_name;
			std::map<std::string, LINUX_KEY_VALUE> input_name_to_key;
	};
}

#endif //G13_KEY_MAP_H
