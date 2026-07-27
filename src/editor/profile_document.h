#ifndef G13_EDITOR_PROFILE_DOCUMENT_H
#define G13_EDITOR_PROFILE_DOCUMENT_H

#include <filesystem>
#include <string>

#include <pugixml.hpp>

#include "profile_model.h"

namespace G13::Editor {
	/**
	 * Owns a Logitech profile XML document and synchronizes it with ProfileModel.
	 */
	class ProfileDocument {
	public:
		/**
		 * Loads a profile XML from disk into the editable model.
		 * @param path profile XML path to load.
		 * @param error receives a human-readable failure reason.
		 * @return true when the profile was loaded and parsed.
		 */
		bool load(const std::filesystem::path& path, std::string& error);

		/**
		 * Creates and saves a minimal new G13 profile in the target directory.
		 * @param directory directory where the new profile XML should be written.
		 * @param name requested profile display name.
		 * @param error receives a human-readable failure reason.
		 * @return true when the profile was created and saved.
		 */
		bool create_empty(const std::filesystem::path& directory, const std::string& name, std::string& error);

		/**
		 * Saves a copy of the currently loaded profile under a new GUID and name.
		 * @param directory directory where the copied profile XML should be written.
		 * @param name requested copied profile display name.
		 * @param error receives a human-readable failure reason.
		 * @return true when the copied profile was saved.
		 */
		bool copy_loaded_as(const std::filesystem::path& directory, const std::string& name, std::string& error);

		/**
		 * Writes the current model state back to the profile XML path.
		 * @param error receives a human-readable failure reason.
		 * @return true when the XML was written.
		 */
		bool save(std::string& error);

		/**
		 * Returns the editable profile model.
		 * @return mutable profile model.
		 */
		ProfileModel& model() { return _model; }

		/**
		 * Returns the current profile model.
		 * @return immutable profile model.
		 */
		const ProfileModel& model() const { return _model; }

		/**
		 * Finds an editable macro by GUID.
		 * @param guid macro GUID to find.
		 * @return mutable macro pointer, or nullptr when missing.
		 */
		Macro* find_macro(const std::string& guid);

		/**
		 * Finds a macro by GUID.
		 * @param guid macro GUID to find.
		 * @return immutable macro pointer, or nullptr when missing.
		 */
		const Macro* find_macro(const std::string& guid) const;

		/**
		 * Returns a macro display name for a GUID, or an empty string if missing.
		 * @param guid macro GUID to name.
		 * @return macro display name, or an empty string when missing.
		 */
		std::string macro_name(const std::string& guid) const;

		/**
		 * Assigns a macro to a context id, or clears the assignment when macro_guid is empty.
		 * @param context_id G13 context id to update.
		 * @param macro_guid macro GUID to assign, or empty to clear.
		 */
		void set_assignment(const std::string& context_id, const std::string& macro_guid);

		/**
		 * Adds a new editable macro with a generated GUID.
		 * @return newly added macro.
		 */
		Macro& add_macro();

		/**
		 * Deletes a macro and clears any assignments that referenced it.
		 * @param guid macro GUID to delete.
		 */
		void delete_macro(const std::string& guid);

	private:
		pugi::xml_document _xml;
		pugi::xml_node _profile;
		ProfileModel _model;

		/**
		 * Parses the current XML tree into the editable profile model.
		 */
		void parse_model();

		/**
		 * Writes the editable model back into the owned XML tree.
		 */
		void write_model_to_xml();

		/**
		 * Returns an existing profile child node or creates one.
		 * @param name child element name.
		 * @return existing or newly created profile child node.
		 */
		pugi::xml_node profile_child(const char* name);

		/**
		 * Returns the G13 LeftHandedController assignments node, creating it when needed.
		 * @return assignments node for the G13 controller.
		 */
		pugi::xml_node assignments_node();

		/**
		 * Returns the profile macros node, creating it when needed.
		 * @return profile macros node.
		 */
		pugi::xml_node macros_node();
	};
}

#endif
