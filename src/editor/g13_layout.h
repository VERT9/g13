#ifndef G13_EDITOR_G13_LAYOUT_H
#define G13_EDITOR_G13_LAYOUT_H

#include <string>
#include <vector>

#include <imgui.h>

namespace G13::Editor {
	/**
	 * Normalized rectangle and binding metadata for one visible G13 control.
	 */
	struct ButtonRegion {
		/**
		 * Human-readable label drawn on the assignment button.
		 */
		std::string label;

		/**
		 * Logitech assignment context id, for example G1 or G26.
		 */
		std::string context_id;

		/**
		 * Top-left and bottom-right corners in normalized assignment-canvas coordinates.
		 */
		ImVec2 min;
		ImVec2 max;

		/**
		 * Whether this region can accept profile assignments.
		 */
		bool editable;
	};

	/**
	 * Returns the static assignment-button layout used by the profile editor.
	 * @return immutable list of normalized G13 button regions.
	 */
	const std::vector<ButtonRegion>& g13_button_regions();
}

#endif
