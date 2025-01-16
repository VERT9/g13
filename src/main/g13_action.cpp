//
// Created by vert9 on 11/23/23.
//

#include <utility>

#include "g13.h"
#include "g13_log.h"
#include "g13_device.h"
#include "g13_action.h"
#include "helper.h"

using Helper::repr;

namespace G13 {
	G13_Action_Keys::G13_Action_Keys(std::shared_ptr<G13_Log> logger, std::shared_ptr<G13_KeyMap> keymap, std::string keys) : G13_Action(std::move(logger), std::move(keymap)) {
		std::stringstream buffer(keys);
		std::string key;
		while (getline(buffer, key, '+')) {
			auto kval = _keymap->find_input_key_value(key);
			if (kval == BAD_KEY_VALUE) {
				throw G13_CommandException("create action unknown key : " + key);
			}
			_keys.push_back(kval);
		}
	}

	void G13_Action_Keys::act(const bool is_down, G13_Device& device) {
		for (int _key : _keys) {
			device.send_event(EV_KEY, _key, is_down);
			if (is_down) {
				_logger->trace("sending KEY DOWN " + std::to_string(_key));
			} else {
				_logger->trace("sending KEY UP " + std::to_string(_key));
			}
		}
	}

	void G13_Action_Keys::dump(std::ostream& out) const {
		out << " SEND KEYS: ";

		for (size_t i = 0; i < _keys.size(); i++) {
			if (i) out << " + ";
			out << _keymap->find_input_key_name(_keys[i]);
		}
	}

	G13_Action_PipeOut::G13_Action_PipeOut(std::shared_ptr<G13_Log> logger, std::shared_ptr<G13_KeyMap> keymap, std::string out) :
			G13_Action(std::move(logger), std::move(keymap)), _out(out + "\n") {}

	void G13_Action_PipeOut::act(const bool is_down, G13_Device& device) {
		if (is_down) {
			device.write_output_pipe(_out);
		}
	}

	void G13_Action_PipeOut::dump(std::ostream& o) const {
		o << "WRITE PIPE : " << repr(_out);
	}

	G13_Action_Command::G13_Action_Command(std::shared_ptr<G13_Log> logger, std::shared_ptr<G13_KeyMap> keymap, std::string cmd) :
			G13_Action(std::move(logger), std::move(keymap)), _cmd(std::move(cmd)) {}

	void G13_Action_Command::act(const bool is_down, G13_Device& device) {
		if (is_down) {
			device.command(_cmd.c_str());
		}
	}

	void G13_Action_Command::dump(std::ostream& o) const {
		o << "COMMAND : " << repr(_cmd);
	}

	void G13_Action_AppChange::act(const bool is_down, G13_Device& device) {
		if (is_down) {
			device.next_app();
		}

		_current_app = device.get_current_app();
	}

	void G13_Action_AppChange::dump(std::ostream& o) const {
		o << "APP INDEX : " << _current_app;
	}

	G13_Action_Dynamic::G13_Action_Dynamic(std::shared_ptr<G13_Log> logger,
											std::shared_ptr<G13_KeyMap> keymap, std::function<void()> action) :
		G13_Action(std::move(logger), std::move(keymap)), _action(std::move(action)) {
	}

	void G13_Action_Dynamic::act(const bool is_down, G13_Device& device) {
		if (is_down) {
			_action();
		}
	}

	void G13_Action_Dynamic::dump(std::ostream& o) const {
		o << "Dynamic Action!";
	}
}