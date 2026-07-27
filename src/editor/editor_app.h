#ifndef G13_EDITOR_EDITOR_APP_H
#define G13_EDITOR_EDITOR_APP_H

#include <string>
#include <vector>

#include "daemon_client.h"
#include "editor_settings.h"
#include "profile_document.h"

namespace G13::Editor {
	/**
	 * Lightweight profile metadata shown in the profile selector list.
	 */
	struct ListedProfile {
		/**
		 * Full XML file path.
		 */
		std::string path;

		/**
		 * Profile GUID parsed from the XML.
		 */
		std::string guid;

		/**
		 * User-visible profile name parsed from the XML.
		 */
		std::string name;
	};

	/**
	 * Dear ImGui application controller for browsing and editing G13 profile XML.
	 */
	class EditorApp {
	public:
		/**
		 * Initializes editor state from persisted settings and scans the profile directory.
		 */
		EditorApp();

		/**
		 * Draws one complete editor frame.
		 */
		void draw();

		/**
		 * Returns the current settings snapshot for persistence at shutdown.
		 * @return current editor settings.
		 */
		EditorSettings settings() const;

	private:
		ProfileDocument _document;
		DaemonClient _daemon;
		std::string _profile_dir;
		std::string _profile_path;
		std::vector<ListedProfile> _profiles;
		std::string _status;
		std::string _profile_search;
		std::string _selected_macro_guid;
		std::string _selected_context_id;
		std::string _editing_macro_guid;
		std::string _editing_macro_name;
		std::string _editing_macro_keys;
		std::string _pending_delete_macro_guid;
		std::string _pending_delete_profile_path;
		std::string _new_profile_name;
		bool _new_profile_is_copy = false;
		bool _capturing_macro_keys = false;
		bool _settings_open = false;
		bool _profile_create_open = false;
		bool _profile_editor_open = false;
		bool _profile_delete_confirm_open = false;
		bool _macro_delete_confirm_open = false;
		bool _activate_after_reload = false;

		/**
		 * Returns true when a profile document is currently loaded.
		 * @return true when the editor has a loaded profile.
		 */
		bool has_profile() const;

		/**
		 * Draws the profile list, search field, and profile actions.
		 */
		void draw_profile_selector();

		/**
		 * Draws the macro list and macro actions.
		 */
		void draw_macro_selector();

		/**
		 * Draws the assignment canvas and selected-assignment editor.
		 */
		void draw_assignment_window();

		/**
		 * Draws the profile options modal when requested.
		 */
		void draw_settings_modal();

		/**
		 * Draws the new/copy profile modal when requested.
		 */
		void draw_profile_create_modal();

		/**
		 * Draws the profile metadata editor modal when requested.
		 */
		void draw_profile_editor();

		/**
		 * Draws the profile deletion confirmation modal when requested.
		 */
		void draw_profile_delete_confirm();

		/**
		 * Draws the macro editor modal when a macro is being edited.
		 */
		void draw_macro_editor();

		/**
		 * Draws the macro deletion confirmation modal when requested.
		 */
		void draw_macro_delete_confirm();

		/**
		 * Opens the profile creation modal in new-profile or copy-profile mode.
		 * @param copy true to copy the current profile, false to create an empty profile.
		 */
		void open_profile_create_modal(bool copy);

		/**
		 * Opens the macro editor using an existing macro as the edit source.
		 * @param macro macro to edit.
		 */
		void open_macro_editor(const Macro& macro);

		/**
		 * Scans the configured directory for profile XML files.
		 */
		void scan_profile_dir();

		/**
		 * Loads a profile XML and persists it as the last opened profile.
		 * @param path profile XML path to load.
		 */
		void open_profile(const std::string& path);

		/**
		 * Creates or copies a profile using the pending modal values.
		 * @return true when the profile was created and loaded.
		 */
		bool create_profile_from_modal();

		/**
		 * Saves if needed, reloads the profile in the daemon, and activates it.
		 */
		void activate_profile();

		/**
		 * Saves the current profile and optionally reloads it in the daemon.
		 * @param reload true to ask the daemon to reload after saving.
		 */
		void save_profile(bool reload);

		/**
		 * Persists current editor settings immediately.
		 */
		void save_settings() const;
	};
}

#endif
