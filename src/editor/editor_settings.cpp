#include "editor_settings.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <fstream>

namespace {
	/**
	 * Trims whitespace from both ends of a settings-file token.
	 * @param text token text to trim.
	 * @return trimmed token text.
	 */
	std::string trim(const std::string& text) {
		const auto first = text.find_first_not_of(" \t\r\n");
		if (first == std::string::npos) {
			return {};
		}
		const auto last = text.find_last_not_of(" \t\r\n");
		return text.substr(first, last - first + 1);
	}

	/**
	 * Parses permissive boolean text used in the editor settings ini.
	 * @param value setting value to parse.
	 * @return true for enabled values, false otherwise.
	 */
	bool parse_bool(std::string value) {
		std::ranges::transform(value, value.begin(), [](const unsigned char c) {
			return static_cast<char>(std::tolower(c));
		});
		return value == "1" || value == "true" || value == "yes" || value == "on";
	}

	/**
	 * Parses an integer setting, returning fallback for malformed values.
	 * @param value setting value to parse.
	 * @param fallback value returned when parsing fails.
	 * @return parsed integer or fallback.
	 */
	int parse_int(const std::string& value, const int fallback) {
		try {
			size_t parsed = 0;
			const int result = std::stoi(value, &parsed);
			return parsed == value.size() ? result : fallback;
		} catch (...) {
			return fallback;
		}
	}
}

namespace G13::Editor {
	std::filesystem::path editor_config_dir() {
		if (const char* config_home = std::getenv("XDG_CONFIG_HOME"); config_home && config_home[0] != '\0') {
			return std::filesystem::path(config_home) / "g13";
		}
		if (const char* home = std::getenv("HOME"); home && home[0] != '\0') {
			return std::filesystem::path(home) / ".config" / "g13";
		}
		return std::filesystem::current_path() / ".g13";
	}

	std::filesystem::path editor_imgui_ini_path() {
		return editor_config_dir() / "profile_editor.imgui.ini";
	}

	std::filesystem::path editor_settings_path() {
		return editor_config_dir() / "profile_editor.ini";
	}

	EditorSettings load_editor_settings() {
		EditorSettings settings;
		std::ifstream input(editor_settings_path());
		if (!input) {
			return settings;
		}

		std::string line;
		while (std::getline(input, line)) {
			const auto comment = line.find('#');
			if (comment != std::string::npos) {
				line = line.substr(0, comment);
			}

			const auto separator = line.find('=');
			if (separator == std::string::npos) {
				continue;
			}

			const auto key = trim(line.substr(0, separator));
			const auto value = trim(line.substr(separator + 1));
			if (key == "profile_dir") {
				settings.profile_dir = value;
			} else if (key == "daemon_pipe") {
				settings.daemon_pipe = value;
			} else if (key == "activate_after_reload") {
				settings.activate_after_reload = parse_bool(value);
			} else if (key == "last_profile_guid") {
				settings.last_profile_guid = value;
			} else if (key == "window_x") {
				settings.window_x = parse_int(value, settings.window_x);
				settings.has_window_placement = true;
			} else if (key == "window_y") {
				settings.window_y = parse_int(value, settings.window_y);
				settings.has_window_placement = true;
			} else if (key == "window_width") {
				settings.window_width = parse_int(value, settings.window_width);
				settings.has_window_placement = true;
			} else if (key == "window_height") {
				settings.window_height = parse_int(value, settings.window_height);
				settings.has_window_placement = true;
			} else if (key == "monitor_name") {
				settings.monitor_name = value;
			}
		}
		return settings;
	}

	void save_editor_settings(const EditorSettings& settings) {
		std::filesystem::create_directories(editor_config_dir());
		std::ofstream output(editor_settings_path(), std::ios::trunc);
		if (!output) {
			return;
		}

		output << "profile_dir=" << settings.profile_dir << '\n';
		output << "daemon_pipe=" << settings.daemon_pipe << '\n';
		output << "activate_after_reload=" << (settings.activate_after_reload ? "true" : "false") << '\n';
		output << "last_profile_guid=" << settings.last_profile_guid << '\n';
		output << "window_x=" << settings.window_x << '\n';
		output << "window_y=" << settings.window_y << '\n';
		output << "window_width=" << settings.window_width << '\n';
		output << "window_height=" << settings.window_height << '\n';
		output << "monitor_name=" << settings.monitor_name << '\n';
	}
}
