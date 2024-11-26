//
// Created by vert9 on 11/23/23.
//

#include "helper.h"
#include "g13_log.h"
#include "g13_device.h"
#include "g13_action.h"

#include <linux/uinput.h>

#include <utility>

using Helper::repr;

namespace G13 {
	inline G13_Manager& G13_Action::manager() {
		return _keypad.manager();
	}

	inline const G13_Manager& G13_Action::manager() const {
		return _keypad.manager();
	}

	G13_Action::~G13_Action() {}

	G13_Action_Keys::G13_Action_Keys(G13_Device& keypad, G13_Log& logger, const std::string& keys_string) : G13_Action(keypad, logger) {
		std::vector<std::string> keys;
		boost::split(keys, keys_string, boost::is_any_of("+"));

		BOOST_FOREACH(std::string const& key, keys) {
			auto kval = manager().find_input_key_value(key);
			if (kval == BAD_KEY_VALUE) {
				throw G13_CommandException("create action unknown key : " + key);
			}
			_keys.push_back(kval);
		}
	}

	G13_Action_Keys::~G13_Action_Keys() {}

	void G13_Action_Keys::act(G13_Device& g13, bool is_down) {
		for (int _key : _keys) {
			g13.send_event(EV_KEY, _key, is_down);
			if (is_down) {
				_logger.trace("sending KEY DOWN " + std::to_string(_key));
			} else {
				_logger.trace("sending KEY UP " + std::to_string(_key));
			}
		}
	}

	void G13_Action_Keys::dump(std::ostream& out) const {
		out << " SEND KEYS: ";

		for (size_t i = 0; i < _keys.size(); i++) {
			if (i) out << " + ";
			out << manager().find_input_key_name(_keys[i]);
		}
	}

	G13_Action_PipeOut::G13_Action_PipeOut(G13_Device& keypad, G13_Log& logger, const std::string& out) :
			G13_Action(keypad, logger), _out(out + "\n") {}

	G13_Action_PipeOut::~G13_Action_PipeOut() {}

	void G13_Action_PipeOut::act(G13_Device& kp, bool is_down) {
		if (is_down) {
			kp.write_output_pipe(_out);
		}
	}

	void G13_Action_PipeOut::dump(std::ostream& o) const {
		o << "WRITE PIPE : " << repr(_out);
	}

	G13_Action_Command::G13_Action_Command(G13_Device& keypad, G13_Log& logger, const std::string& cmd) :
			G13_Action(keypad, logger), _cmd(cmd) {}

	G13_Action_Command::~G13_Action_Command() {}

	void G13_Action_Command::act(G13_Device& kp, bool is_down) {
		if (is_down) {
			keypad().command(_cmd.c_str());
		}
	}

	void G13_Action_Command::dump(std::ostream& o) const {
		o << "COMMAND : " << repr(_cmd);
	}

	void G13_Action_AppChange::act(G13_Device& kp, const bool is_down) {
		if (is_down) {
			kp.next_app();
		}
	}

	void G13_Action_AppChange::dump(std::ostream& o) const {
		o << "APP INDEX : " << keypad().get_current_app();
	}

	G13_Action_Dynamic::G13_Action_Dynamic(G13_Device& keypad, G13_Log& logger, std::function<void()> action) :
			G13_Action(keypad, logger), _action(std::move(action)) {}

	void G13_Action_Dynamic::act(G13_Device& kp, const bool is_down) {
		if (is_down) {
			_action();
		}
	}

	void G13_Action_Dynamic::dump(std::ostream& o) const {
		o << "Dynamic Action!";
	}
}