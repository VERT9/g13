//
// Created by vert9 on 11/23/23.
//

#include <boost/preprocessor/seq.hpp>
#include <ostream>
#include "helper.h"
#include "g13_manager.h"
#include "g13_device.h"
#include "g13_profile.h"

using Helper::repr;

namespace G13 {
	/*inline*/ const G13_Manager& G13_Profile::manager() const {
		return _keypad.manager();
	}

	void G13_Profile::_init_keys() {
		int key_index = 0;

		// create a G13_Key entry for every key in G13_KEY_SEQ
		#define INIT_KEY(r, data, elem)                                        \
        {                                                                      \
            G13_Key key( *this, BOOST_PP_STRINGIZE(elem), key_index++ );       \
            _keys.push_back( key );                                            \
        }                                                                      \

		BOOST_PP_SEQ_FOR_EACH(INIT_KEY, _, G13_KEY_SEQ)

		assert(_keys.size() == G13_NUM_KEYS);

		// now disable testing for keys in G13_NONPARSED_KEY_SEQ
		#define MARK_NON_PARSED_KEY(r, data, elem)                             \
        {                                                                      \
            G13_Key *key = find_key( BOOST_PP_STRINGIZE(elem) );               \
            assert(key);                                                       \
            key->_should_parse = false;                                        \
        }                                                                      \

		BOOST_PP_SEQ_FOR_EACH(MARK_NON_PARSED_KEY, _, G13_NONPARSED_KEY_SEQ)
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

	void G13_Profile::parse_keys(unsigned char* buf) {
		buf += 3;
		for (size_t i = 0; i < _keys.size(); i++) {
			if (_keys[i]._should_parse) {
				_keys[i].parse_key(buf, &_keypad);
			}
		}
	}

	G13_Key* G13_Profile::find_key(const std::string& keyname) {

		auto key = _keypad.manager().find_g13_key_value(keyname);
		if (key >= 0 && key < _keys.size()) {
			return &_keys[key];
		}
		return 0;
	}
}