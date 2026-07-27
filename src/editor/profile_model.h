#ifndef G13_EDITOR_PROFILE_MODEL_H
#define G13_EDITOR_PROFILE_MODEL_H

#include <map>
#include <string>
#include <vector>

namespace G13::Editor {
	/**
	 * Editable macro parsed from or written to a Logitech profile XML.
	 */
	struct Macro {
		/**
		 * Logitech macro GUID used by assignments.
		 */
		std::string guid;

		/**
		 * Display name shown in the editor and profile XML.
		 */
		std::string name;

		/**
		 * Logitech key names emitted by the macro.
		 */
		std::vector<std::string> keys;

		/**
		 * False when the source XML macro shape cannot be safely edited.
		 */
		bool supported = true;
	};

	/**
	 * In-memory editable representation of one Logitech G13 profile XML.
	 */
	struct ProfileModel {
		/**
		 * Source XML path for this profile.
		 */
		std::string path;

		/**
		 * Profile GUID used by Logitech XML and daemon activation.
		 */
		std::string guid;

		/**
		 * User-visible profile name.
		 */
		std::string name;

		/**
		 * Profile description text.
		 */
		std::string description;

		/**
		 * Target executable paths associated with the profile.
		 */
		std::vector<std::string> targets;

		/**
		 * Editable macro definitions available for assignment.
		 */
		std::vector<Macro> macros;

		/**
		 * Mapping from G13 context id to macro GUID for active assignments.
		 */
		std::map<std::string, std::string> assignments;

		/**
		 * Hex RGB backlight color string when present in the source profile.
		 */
		std::string backlight_color;

		/**
		 * True when model edits have not yet been saved to XML.
		 */
		bool dirty = false;
	};
}

#endif
