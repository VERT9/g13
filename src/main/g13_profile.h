//
// Created by vert9 on 11/23/23.
//

#ifndef G13_G13_PROFILE_H
#define G13_G13_PROFILE_H

#include <string>
#include <utility>
#include <vector>

#include "g13_key_map.h"

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
		G13_Profile(std::string guid, std::string  name = "") :
				_name(std::move(name)), _guid(std::move(guid)) {
			_keymap = Container::Instance().Resolve<G13_KeyMap>();
			_init_keys();
		}
		G13_Profile(const G13_Profile& other, std::string  guid, std::string  name = "") :
				_keys(other._keys), _name(std::move(name)), _guid(std::move(guid)) {
			_keymap = Container::Instance().Resolve<G13_KeyMap>();
		}

		// search key by G13 keyname
		G13_Key* find_key(const std::string& keyname);

		void dump(std::ostream& o) const;

		void parse_keys(unsigned char* buf, G13_Device& device);
		const std::string& name() const { return _name; }
		const std::string& guid() const { return _guid; }

	protected:
		std::shared_ptr<G13_KeyMap> _keymap;
		std::vector<G13_Key> _keys;
		std::string _name;
		std::string _guid;

		void _init_keys();
	};
}
#endif //G13_G13_PROFILE_H
