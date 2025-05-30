//
// Created by vert9 on 11/23/23.
//

#ifndef G13_G13_STICK_H
#define G13_G13_STICK_H

#include <memory>
#include <string>
#include <vector>

#include "helper.h"
#include "g13_action.h"

namespace G13 {
	typedef Helper::Coord<int> G13_StickCoord;
	typedef Helper::Bounds<int> G13_StickBounds;
	typedef Helper::Coord<double> G13_ZoneCoord;
	typedef Helper::Bounds<double> G13_ZoneBounds;

	enum stick_mode_t { STICK_ABSOLUTE, STICK_RELATIVE, STICK_KEYS, STICK_CALCENTER, STICK_CALBOUNDS, STICK_CALNORTH };

	class G13_Stick;
	class G13_Device;

	class G13_StickZone : public G13_Actionable<G13_Stick> {
	public:

		G13_StickZone(G13_Stick&, const std::string& name, const G13_ZoneBounds&, G13_ActionPtr = 0);

		bool operator==(const G13_StickZone& other) const { return _name == other._name; }

		void dump(std::ostream&) const;

		void parse_key(unsigned char* byte, G13_Device* g13);
		void test(G13_Device& keypad, const G13_ZoneCoord& loc);
		void set_bounds(const G13_ZoneBounds& bounds) { _bounds = bounds; }

	protected:

		bool _active;

		G13_ZoneBounds _bounds;
	};

// *************************************************************************

	class G13_Stick {
	public:
		G13_Stick(std::shared_ptr<G13_Log> logger);

		void parse_joystick(G13_Device& keypad, unsigned char* buf);

		void set_mode(stick_mode_t);
		G13_StickZone* zone(const std::string&, bool create = false);
		void remove_zone(const G13_StickZone& zone);

		const std::vector<G13_StickZone>& zones() const { return _zones; }

		void dump(std::ostream&) const;

		G13_StickCoord getCurrentPos();
		double getDX();
		double getDY();

	protected:

		void _recalc_calibrated();

		std::shared_ptr<G13_Log> _logger;
		std::vector<G13_StickZone> _zones;

		G13_StickBounds _bounds;
		G13_StickCoord _center_pos;
		G13_StickCoord _north_pos;

		G13_StickCoord _current_pos;

		stick_mode_t _stick_mode;
	};
}

#endif //G13_G13_STICK_H
