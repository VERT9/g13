//
// Created by vert9 on 11/23/23.
//

#ifndef G13_G13_KEYS_H
#define G13_G13_KEYS_H

#include "g13_action.h"
#include "g13_profile.h"

namespace G13 {

	/*! G13_KEY_SEQ is a Boost Preprocessor sequence containing the
	 * G13 keys.  The order is very specific, with the position of each
	 * item corresponding to a specific bit in the G13's USB message
	 * format.  Do NOT remove or insert items in this list.
	 */
	#define G13_KEY_SEQ                                                        \
    /* byte 3 */ (G1)(G2)(G3)(G4)(G5)(G6)(G7)(G8)                              \
    /* byte 4 */ (G9)(G10)(G11)(G12)(G13)(G14)(G15)(G16)                       \
    /* byte 5 */ (G17)(G18)(G19)(G20)(G21)(G22)(UNDEF1)(LIGHT_STATE)           \
    /* byte 6 */ (BD)(L1)(L2)(L3)(L4)(M1)(M2)(M3)                              \
    /* byte 7 */ (MR)(LEFT)(DOWN)(TOP)(UNDEF3)(LIGHT)(LIGHT2)(MISC_TOGGLE)     \


	/*! G13_NONPARSED_KEY_SEQ is a Boost Preprocessor sequence containing the
	 * G13 keys that shouldn't be tested input.  These aren't actually keys,
	 * but they are in the bitmap defined by G13_KEY_SEQ.
	 */
	#define G13_NONPARSED_KEY_SEQ                                              \
        (UNDEF1)(LIGHT_STATE)(UNDEF3)(LIGHT)(LIGHT2)(UNDEF3)(MISC_TOGGLE)      \

	/*! KB_INPUT_KEY_SEQ is a Boost Preprocessor sequence containing the
	 * names of keyboard keys we can send through binding actions.
	 * These correspond to KEY_xxx value definitions in <linux/input.h>,
	 * i.e. ESC is KEY_ESC, 1 is KEY_1, etc.
	 */
	#define KB_INPUT_KEY_SEQ                                                   \
        (ESC)(1)(2)(3)(4)(5)(6)(7)(8)(9)(0)                                    \
        (MINUS)(EQUAL)(BACKSPACE)(TAB)                                         \
        (Q)(W)(E)(R)(T)(Y)(U)(I)(O)(P)                                         \
        (LEFTBRACE)(RIGHTBRACE)(ENTER)(LEFTCTRL)(RIGHTCTRL)                    \
        (A)(S)(D)(F)(G)(H)(J)(K)(L)                                            \
        (SEMICOLON)(APOSTROPHE)(GRAVE)(LEFTSHIFT)(BACKSLASH)                   \
        (Z)(X)(C)(V)(B)(N)(M)                                                  \
        (COMMA)(DOT)(SLASH)(RIGHTSHIFT)(KPASTERISK)                            \
        (LEFTALT)(RIGHTALT)(SPACE)(CAPSLOCK)                                   \
        (F1)(F2)(F3)(F4)(F5)(F6)(F7)(F8)(F9)(F10)(F11)(F12)                    \
        (NUMLOCK)(SCROLLLOCK)                                                  \
        (KP7)(KP8)(KP9)(KPMINUS)(KP4)(KP5)(KP6)(KPPLUS)                        \
        (KP1)(KP2)(KP3)(KP0)(KPDOT)                                            \
        (LEFT)(RIGHT)(UP)(DOWN)                                                \
        (PAGEUP)(PAGEDOWN)(HOME)(END)(INSERT)(DELETE)                          \

	class G13_Device;
	class G13_Profile;

	/*!
	 * manages the bindings for a G13 key
	 */
	class G13_Key : public G13_Actionable<G13_Profile> {
	public:

		void dump(std::ostream& o) const;

		G13_KEY_INDEX index() const { return _index.index; }

		void parse_key(unsigned char* byte, G13_Device* g13);

	protected:

		struct KeyIndex {
			KeyIndex(int key) :
					index(key),
					offset(key / 8),
					mask(1 << (key % 8)) {}

			int index;
			unsigned char offset;
			unsigned char mask;
		};

		// G13_Profile is the only class able to instantiate G13_Keys
		friend class G13_Profile;

		G13_Key(G13_Profile& mode, const std::string& name, int index) :
				G13_Actionable<G13_Profile>(mode, name), _index(index), _should_parse(true) {}
		G13_Key(G13_Profile& mode, const G13_Key& key) :
				G13_Actionable<G13_Profile>(mode, key.name()), _index(key._index),
				_should_parse(key._should_parse) { set_action(key.action()); }

		KeyIndex _index;
		bool _should_parse;
	};
}

#endif //G13_G13_KEYS_H
