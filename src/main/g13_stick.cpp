/*
 * This file contains code for managing keys and profiles
 */

#include "g13_device.h"
#include "g13_log.h"
#include "g13_stick.h"

using namespace std;

namespace G13 {
	G13_Stick::G13_Stick(std::shared_ptr<G13_Log> logger) :
			_logger(std::move(logger)),
			_bounds(0, 0, 255, 255),
			_center_pos(127, 127),
			_north_pos(127, 0) {
		_stick_mode = STICK_KEYS;

		auto add_zone = [this](const std::string& name, double x1, double y1, double x2, double y2) {
			auto key = "KEY_" + name;
			std::shared_ptr<G13_Action_Keys> action = Container::Instance().Resolve<G13_Action_Keys>(key);
			_zones.emplace_back(*this, "STICK_" + name,
										   G13_ZoneBounds(x1, y1, x2, y2),
										   action
			);
		};

		add_zone("UP", 0.0, 0.1, 1.0, 0.3);
		add_zone("DOWN", 0.0, 0.7, 1.0, 0.9);
		add_zone("LEFT", 0.0, 0.0, 0.2, 1.0);
		add_zone("RIGHT", 0.8, 0.0, 1.0, 1.0);
		add_zone("PAGEUP", 0.0, 0.0, 1.0, 0.1);
		add_zone("PAGEDOWN", 0.0, 0.9, 1.0, 1.0);
	}

	G13_StickZone* G13_Stick::zone(const std::string& name, bool create) {
		for (auto& zone : _zones) {
			if (zone.name() == name) {
				return &zone;
			}
		}

		if (create) {
			_zones.emplace_back(*this, name, G13_ZoneBounds(0.0, 0.0, 0.0, 0.0));
			return zone(name);
		}
		return 0;
	}

	void G13_Stick::set_mode(stick_mode_t m) {
		if (m == _stick_mode)
			return;
		if (_stick_mode == STICK_CALCENTER || _stick_mode == STICK_CALBOUNDS || _stick_mode == STICK_CALNORTH) {
			_recalc_calibrated();
		}
		_stick_mode = m;
		switch (_stick_mode) {
			case STICK_CALBOUNDS: _bounds.tl = G13_StickCoord(255, 255);
				_bounds.br = G13_StickCoord(0, 0);
				break;
		}
	}

	void G13_Stick::_recalc_calibrated() {
	}

	void G13_Stick::remove_zone(const G13_StickZone& zone) {
		G13_StickZone target(zone);
		_zones.erase(std::remove(_zones.begin(), _zones.end(), target), _zones.end());
	}

	void G13_Stick::dump(std::ostream& out) const {
		for (auto& zone : _zones) {
			zone.dump(out);
			out << endl;
		}
	}

	G13_StickCoord G13_Stick::getCurrentPos() {
		return _current_pos;
	}

	double G13_Stick::getDX() {
		double dx = 0.5;
		if (_current_pos.x <= _center_pos.x) {
			dx = _current_pos.x - _bounds.tl.x;
			dx /= (_center_pos.x - _bounds.tl.x) * 2;
		} else {
			dx = _bounds.br.x - _current_pos.x;
			dx /= (_bounds.br.x - _center_pos.x) * 2;
			dx = 1.0 - dx;
		}

		return dx;
	}

	double G13_Stick::getDY() {
		double dy = 0.5;
		if (_current_pos.y <= _center_pos.y) {
			dy = _current_pos.y - _bounds.tl.y;
			dy /= (_center_pos.y - _bounds.tl.y) * 2;
		} else {
			dy = _bounds.br.y - _current_pos.y;
			dy /= (_bounds.br.y - _center_pos.y) * 2;
			dy = 1.0 - dy;
		}

		return dy;
	}

	void G13_Stick::parse_joystick(G13_Device& keypad, unsigned char* buf) {

		_current_pos.x = buf[1];
		_current_pos.y = buf[2];

		// update targets if we're in calibration mode
		switch (_stick_mode) {

			case STICK_CALCENTER: _center_pos = _current_pos;
				return;

			case STICK_CALNORTH: _north_pos = _current_pos;
				return;

			case STICK_CALBOUNDS: _bounds.expand(_current_pos);
				return;
		};

		// determine our normalized position
		double dx = getDX();
		double dy = getDY();

		_logger->trace(std::format("x={} y={} dx={} dy={}", std::to_string(_current_pos.x), std::to_string(_current_pos.y), std::to_string(dx), std::to_string(dy)));
		G13_ZoneCoord jpos(dx, dy);
		if (_stick_mode == STICK_ABSOLUTE) {
			keypad.send_event(EV_ABS, ABS_X, _current_pos.x);
			keypad.send_event(EV_ABS, ABS_Y, _current_pos.y);
		} else if (_stick_mode == STICK_KEYS) {
			for (auto& zone : _zones) {
				zone.test(keypad, jpos);
			}
		} else {
			/*    send_event(g13->uinput_file, EV_REL, REL_X, stick_x/16 - 8);
			 send_event(g13->uinput_file, EV_REL, REL_Y, stick_y/16 - 8);*/
		}
	}

	//**************************************************************************

	void G13_StickZone::dump(std::ostream& out) const {
		out << "   " << setw(20) << name() << "   " << _bounds << "  ";
		if (action()) {
			action()->dump(out);
		} else {
			out << " (no action)";
		}
	}

	void G13_StickZone::test(G13_Device& keypad, const G13_ZoneCoord& loc) {
		if (!_action) return;
		bool prior_active = _active;
		_active = _bounds.contains(loc);
		if (!_active) {
			if (prior_active) {
				// cout << "exit stick zone " << _name << std::endl;
				_action->act(false, keypad);
			}
		} else {
			// cout << "in stick zone " << _name << std::endl;
			_action->act(true, keypad);
		}
	}

	G13_StickZone::G13_StickZone(G13_Stick& stick, const std::string& name, const G13_ZoneBounds& b,
								 G13_ActionPtr action) :
			G13_Actionable<G13_Stick>(stick, name), _bounds(b), _active(false) {
		set_action(action);
	}
} // namespace G13

