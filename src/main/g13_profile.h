//
// Created by vert9 on 11/23/23.
//

#ifndef G13_G13_PROFILE_H
#define G13_G13_PROFILE_H

#include <string>
#include <utility>
#include <vector>
#include "g13_keys.h"

namespace G13 {
	class G13_Key;
	class G13_Device;
	class G13_Manager;

	/*!
	 * Represents a set of configured key mappings
	 *
	 * This allows a keypad to have multiple configured
	 * profiles and switch between them easily
	 */
	class G13_Profile {
	public:
		G13_Profile(G13_Device& keypad, std::string  guid, std::string  name = "") :
				_keypad(keypad), _guid(std::move(guid)), _name(std::move(name)) { _init_keys(); }
		G13_Profile(const G13_Profile& other, std::string  guid, std::string  name = "") :
				_keypad(other._keypad), _guid(std::move(guid)), _name(std::move(name)), _keys(other._keys) {}

		// search key by G13 keyname
		G13_Key* find_key(const std::string& keyname);

		void dump(std::ostream& o) const;

		void parse_keys(unsigned char* buf);
		const std::string& name() const { return _name; }
		const std::string& guid() const { return _guid; }

		const G13_Manager& manager() const;

	protected:
		G13_Device& _keypad;
		std::vector<G13_Key> _keys;
		std::string _name;
		std::string _guid;

		void _init_keys();
	};
}
#endif //G13_G13_PROFILE_H
