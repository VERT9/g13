//
// Created by vert9 on 11/23/23.
//

#ifndef G13_G13_KEYS_H
#define G13_G13_KEYS_H

#include "g13_action.h"

namespace G13 {
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
