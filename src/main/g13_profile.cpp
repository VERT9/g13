//
// Created by vert9 on 11/23/23.
//

#include <cassert>
#include <ostream>

#include "helper.h"
#include "g13_device.h"
#include "g13_keys.h"
#include "g13_manager.h"
#include "g13_profile.h"

using Helper::repr;

namespace G13 {
	void G13_Profile::_init_keys() {
		int key_index = 0;

		// create a G13_Key entry for every key in G13_KEY_SEQ
		for (const std::string& name: G13_KEY_SEQ) {
			G13_Key key( *this, name, key_index++ );
			_keys.push_back( key );
		}

		assert(_keys.size() == G13_NUM_KEYS);

		// now disable testing for keys in G13_NONPARSED_KEY_SEQ
		for (const std::string& name: G13_NONPARSED_KEY_SEQ) {
			G13_Key *key = find_key(name);
			assert(key);
			key->_should_parse = false;
		}
	}

	void G13_Profile::dump(std::ostream& o) const {
		o << "Profile " << repr(name()) << std::endl;
		for (const auto& key : _keys) {
			if (key.action()) {
				o << "   ";
				key.dump(o);
				o << std::endl;
			}
		}
	}

	void G13_Profile::parse_keys(unsigned char* buf, G13_Device& device) {
		buf += 3;
		for (size_t i = 0; i < _keys.size(); i++) {
			if (_keys[i]._should_parse) {
				_keys[i].parse_key(buf, &device);
			}
		}
	}

	G13_Key* G13_Profile::find_key(const std::string& keyname) {

		auto key = _keymap->find_g13_key_value(keyname);
		if (key >= 0 && key < _keys.size()) {
			return &_keys[key];
		}
		return 0;
	}
}