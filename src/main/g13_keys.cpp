/*
 * This file contains code for managing keys a profile
 */

#include "g13_device.h"
#include "g13_keys.h"

using namespace std;

namespace G13 {
	void G13_Key::dump(std::ostream& o) const {
		o << manager().find_g13_key_name(index()) << "(" << index() << ") : ";
		if (action()) {
			action()->dump(o);
		} else {
			o << "(no action)";
		}
	}

	void G13_Key::parse_key(unsigned char* byte, G13_Device* g13) {
		const bool key_is_down = byte[_index.offset] & _index.mask;
		if (bool key_state_changed = g13->update(_index.index, key_is_down)) {
			// Output the current button push regardless of attached action
			std::ostringstream out;
			dump(out);
			_logger->debug(std::format("{}[{}]", out.str(), key_is_down?"DOWN":"UP"));
			if (_action) {
 				_action->act(*g13, key_is_down);
			}
		}
	}
} // namespace G13

