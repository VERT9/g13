#ifndef G13_EDITOR_EDITOR_SETTINGS_H
#define G13_EDITOR_EDITOR_SETTINGS_H

#include <filesystem>
#include <string>

namespace G13::Editor {
	/**
	 * User-editable and window-placement settings persisted by the profile editor.
	 */
	struct EditorSettings {
		/**
		 * Directory scanned for Logitech profile XML files.
		 */
		std::string profile_dir;

		/**
		 * Daemon input FIFO path used for reload and activation commands.
		 */
		std::string daemon_pipe;

		/**
		 * Last profile GUID opened by the editor.
		 */
		std::string last_profile_guid;

		/**
		 * Whether saving with reload should also activate the profile.
		 */
		bool activate_after_reload = false;

		/**
		 * True when the remaining window placement fields were loaded from settings.
		 */
		bool has_window_placement = false;

		/**
		 * Last saved window position and size.
		 */
		int window_x = 0;
		int window_y = 0;
		int window_width = 1440;
		int window_height = 900;

		/**
		 * Monitor name used to prefer the same display on the next launch.
		 */
		std::string monitor_name;
	};

	/**
	 * Returns the editor configuration directory, creating no files by itself.
	 * @return user configuration directory used by the editor.
	 */
	std::filesystem::path editor_config_dir();

	/**
	 * Returns the path used by Dear ImGui for its per-window layout ini.
	 * @return Dear ImGui ini file path.
	 */
	std::filesystem::path editor_imgui_ini_path();

	/**
	 * Returns the path to the editor's persisted settings ini.
	 * @return editor settings file path.
	 */
	std::filesystem::path editor_settings_path();

	/**
	 * Loads editor settings, returning defaults when no settings file exists.
	 * @return loaded settings or default values.
	 */
	EditorSettings load_editor_settings();

	/**
	 * Saves editor settings to the user configuration directory.
	 * @param settings settings to persist.
	 */
	void save_editor_settings(const EditorSettings& settings);
}

#endif
