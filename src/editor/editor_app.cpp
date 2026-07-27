#include "editor_app.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <format>

#include <imgui.h>
#include <imgui_stdlib.h>
#include <pugixml.hpp>

#include "g13_layout.h"

namespace {
	/**
	 * FontAwesome-backed button glyphs used by compact editor action buttons.
	 */
	enum class ActionIcon {
		Add,
		Activate,
		Copy,
		Edit,
		Delete,
		Reload,
		Save,
		Settings,
	};

	/**
	 * Draws a fixed-size icon button with an optional tooltip.
	 * @param id stable ImGui id suffix for the button.
	 * @param icon icon glyph to draw.
	 * @param tooltip tooltip text, or nullptr for no tooltip.
	 * @return true when the button was pressed.
	 */
	bool icon_button(const char* id, ActionIcon icon, const char* tooltip) {
		const char* icon_text = "";
		switch (icon) {
			case ActionIcon::Add:
				icon_text = "\xef\x81\xa7";
				break;
			case ActionIcon::Activate:
				icon_text = "\xef\x81\x8b";
				break;
			case ActionIcon::Copy:
				icon_text = "\xef\x83\x85";
				break;
			case ActionIcon::Edit:
				icon_text = "\xef\x8c\x83";
				break;
			case ActionIcon::Delete:
				icon_text = "\xef\x87\xb8";
				break;
			case ActionIcon::Reload:
				icon_text = "\xef\x80\xa1";
				break;
			case ActionIcon::Save:
				icon_text = "\xef\x83\x87";
				break;
			case ActionIcon::Settings:
				icon_text = "\xef\x80\x93";
				break;
		}
		const std::string label = std::string(icon_text) + "##" + id;
		const bool pressed = ImGui::Button(label.c_str(), ImVec2(26.0f, 24.0f));

		if (ImGui::IsItemHovered() && tooltip && *tooltip) {
			ImGui::SetTooltip("%s", tooltip);
		}
		return pressed;
	}

	/**
	 * Formats Logitech XML key names as a user-editable chord string.
	 * @param keys key names to join.
	 * @return key chord string separated by plus signs.
	 */
	std::string join_keys(const std::vector<std::string>& keys) {
		std::string out;
		for (const auto& key : keys) {
			if (!out.empty()) {
				out += "+";
			}
			out += key;
		}
		return out;
	}

	/**
	 * Parses a user-entered chord string into uppercase Logitech XML key names.
	 * @param text chord text entered by the user.
	 * @return parsed uppercase key names.
	 */
	std::vector<std::string> split_keys(const std::string& text) {
		std::vector<std::string> keys;
		std::string current;
		for (const char c : text) {
			if (c == '+' || c == ',' || c == ' ' || c == '\t' || c == '\n') {
				if (!current.empty()) {
					keys.push_back(current);
					current.clear();
				}
			} else {
				current += static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
			}
		}
		if (!current.empty()) {
			keys.push_back(current);
		}
		return keys;
	}

	/**
	 * Returns a lowercase copy for case-insensitive filtering.
	 * @param text text to convert.
	 * @return lowercase copy of text.
	 */
	std::string lowercase(std::string text) {
		std::ranges::transform(text, text.begin(), [](const unsigned char c) {
			return static_cast<char>(std::tolower(c));
		});
		return text;
	}

	/**
	 * Tests whether a listed profile should be shown for the current search text.
	 * @param profile profile row to test.
	 * @param search search text from the profile filter.
	 * @return true when the profile matches or the search text is empty.
	 */
	bool profile_matches_search(const G13::Editor::ListedProfile& profile, const std::string& search) {
		if (search.empty()) {
			return true;
		}

		const auto query = lowercase(search);
		return lowercase(profile.name).find(query) != std::string::npos
				|| lowercase(profile.guid).find(query) != std::string::npos
				|| lowercase(profile.path).find(query) != std::string::npos;
	}

	/**
	 * Captures one supported ImGui key press into a Logitech XML key name list.
	 * @param keys chord string to append to when a key is captured.
	 * @return true when a supported key press was captured.
	 */
	bool capture_key(std::string& keys) {
		/**
		 * ImGui key and matching Logitech XML key name used by capture mode.
		 */
		struct KeyName {
			/**
			 * ImGui key code to test.
			 */
			ImGuiKey key;

			/**
			 * Logitech XML key name appended to the macro chord.
			 */
			const char* name;
		};
		static const KeyName key_names[] = {
				{ImGuiKey_Escape, "ESCAPE"}, {ImGuiKey_Tab, "TAB"}, {ImGuiKey_Space, "SPACEBAR"},
				{ImGuiKey_LeftShift, "LSHIFT"}, {ImGuiKey_RightShift, "RSHIFT"},
				{ImGuiKey_LeftCtrl, "LCTRL"}, {ImGuiKey_RightCtrl, "RCTRL"},
				{ImGuiKey_LeftAlt, "LALT"}, {ImGuiKey_RightAlt, "RALT"},
				{ImGuiKey_Enter, "ENTER"}, {ImGuiKey_Backspace, "BACKSPACE"},
				{ImGuiKey_Delete, "DELETE"}, {ImGuiKey_Insert, "INSERT"},
				{ImGuiKey_Home, "HOME"}, {ImGuiKey_End, "END"},
				{ImGuiKey_PageUp, "PAGEUP"}, {ImGuiKey_PageDown, "PAGEDOWN"},
				{ImGuiKey_UpArrow, "UP"}, {ImGuiKey_RightArrow, "RIGHT"},
				{ImGuiKey_DownArrow, "DOWN"}, {ImGuiKey_LeftArrow, "LEFT"},
				{ImGuiKey_0, "0"}, {ImGuiKey_1, "1"}, {ImGuiKey_2, "2"}, {ImGuiKey_3, "3"},
				{ImGuiKey_4, "4"}, {ImGuiKey_5, "5"}, {ImGuiKey_6, "6"}, {ImGuiKey_7, "7"},
				{ImGuiKey_8, "8"}, {ImGuiKey_9, "9"},
				{ImGuiKey_A, "A"}, {ImGuiKey_B, "B"}, {ImGuiKey_C, "C"}, {ImGuiKey_D, "D"},
				{ImGuiKey_E, "E"}, {ImGuiKey_F, "F"}, {ImGuiKey_G, "G"}, {ImGuiKey_H, "H"},
				{ImGuiKey_I, "I"}, {ImGuiKey_J, "J"}, {ImGuiKey_K, "K"}, {ImGuiKey_L, "L"},
				{ImGuiKey_M, "M"}, {ImGuiKey_N, "N"}, {ImGuiKey_O, "O"}, {ImGuiKey_P, "P"},
				{ImGuiKey_Q, "Q"}, {ImGuiKey_R, "R"}, {ImGuiKey_S, "S"}, {ImGuiKey_T, "T"},
				{ImGuiKey_U, "U"}, {ImGuiKey_V, "V"}, {ImGuiKey_W, "W"}, {ImGuiKey_X, "X"},
				{ImGuiKey_Y, "Y"}, {ImGuiKey_Z, "Z"},
				{ImGuiKey_F1, "F1"}, {ImGuiKey_F2, "F2"}, {ImGuiKey_F3, "F3"}, {ImGuiKey_F4, "F4"},
				{ImGuiKey_F5, "F5"}, {ImGuiKey_F6, "F6"}, {ImGuiKey_F7, "F7"}, {ImGuiKey_F8, "F8"},
				{ImGuiKey_F9, "F9"}, {ImGuiKey_F10, "F10"}, {ImGuiKey_F11, "F11"}, {ImGuiKey_F12, "F12"},
		};

		for (const auto& key_name : key_names) {
			if (!ImGui::IsKeyPressed(key_name.key, false)) {
				continue;
			}
			if (!keys.empty()) {
				keys += "+";
			}
			keys += key_name.name;
			return true;
		}
		return false;
	}

	/**
	 * Adds clipped assignment-button text without changing ImGui layout.
	 * @param draw_list ImGui draw list that receives the text.
	 * @param pos screen-space text position.
	 * @param clip_rect screen-space clipping rectangle.
	 * @param font_size font size in pixels.
	 * @param color packed text color.
	 * @param text text to draw.
	 */
	void add_clipped_text(ImDrawList* draw_list, const ImVec2& pos, const ImVec4& clip_rect,
			const float font_size, const ImU32 color, const std::string& text) {
		if (text.empty()) {
			return;
		}
		draw_list->AddText(ImGui::GetFont(), font_size, pos, color, text.c_str(), nullptr, 0.0f, &clip_rect);
	}
}

namespace G13::Editor {
	EditorApp::EditorApp() {
		constexpr const char* test_profile = "{F954F39F-63B4-44B5-B947-137848537CEB}.xml";
		const auto settings = load_editor_settings();
		if (!settings.daemon_pipe.empty()) {
			_daemon.pipe_path = settings.daemon_pipe;
		}
		_activate_after_reload = settings.activate_after_reload;
		if (!settings.profile_dir.empty()) {
			_profile_dir = settings.profile_dir;
			scan_profile_dir();
			if (!settings.last_profile_guid.empty()) {
				const auto found = std::ranges::find_if(_profiles, [&](const ListedProfile& profile) {
					return profile.guid == settings.last_profile_guid;
				});
				if (found != _profiles.end()) {
					open_profile(found->path);
				}
			}
			return;
		}

#ifdef G13_PROJECT_SOURCE_DIR
		_profile_dir = G13_PROJECT_SOURCE_DIR;
		const auto source_candidate = std::filesystem::path(G13_PROJECT_SOURCE_DIR) / test_profile;
		if (std::filesystem::exists(source_candidate)) {
			_profile_path = source_candidate.string();
			scan_profile_dir();
			return;
		}
#endif
		_profile_dir = std::filesystem::current_path().string();
		_profile_path = test_profile;
		scan_profile_dir();
	}

	EditorSettings EditorApp::settings() const {
		EditorSettings settings;
		settings.profile_dir = _profile_dir;
		settings.daemon_pipe = _daemon.pipe_path;
		settings.activate_after_reload = _activate_after_reload;
		settings.last_profile_guid = _document.model().guid;
		return settings;
	}

	bool EditorApp::has_profile() const {
		return !_document.model().guid.empty();
	}

	void EditorApp::draw() {
		ImGuiViewport* viewport = ImGui::GetMainViewport();
		ImGui::SetNextWindowPos(viewport->WorkPos);
		ImGui::SetNextWindowSize(viewport->WorkSize);

		const ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
									  ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus;
		ImGui::Begin("G13 Profile Editor", nullptr, flags);

		if (!_status.empty()) {
			ImGui::TextWrapped("%s", _status.c_str());
			ImGui::Separator();
		}

		const float profile_width = 330.0f;
		const float macro_width = 320.0f;
		ImGui::BeginChild("Profile Selector", ImVec2(profile_width, 0), true);
		draw_profile_selector();
		ImGui::EndChild();

		ImGui::SameLine();
		ImGui::BeginChild("Macro Selector", ImVec2(macro_width, 0), true);
		draw_macro_selector();
		ImGui::EndChild();

		ImGui::SameLine();
		ImGui::BeginChild("Assignment Window", ImVec2(0, 0), true);
		draw_assignment_window();
		ImGui::EndChild();

		draw_settings_modal();
		draw_profile_create_modal();
		draw_profile_editor();
		draw_profile_delete_confirm();
		draw_macro_editor();
		draw_macro_delete_confirm();
		ImGui::End();
	}

	void EditorApp::draw_profile_selector() {
		ImGui::TextUnformatted("Profiles");
		const float icon_width = 26.0f;
		const float spacing = ImGui::GetStyle().ItemSpacing.x;
		const float actions_width = icon_width * 2.0f + spacing;
		ImGui::SameLine(std::max(ImGui::GetCursorPosX(), ImGui::GetWindowContentRegionMax().x - actions_width));
		if (icon_button("new-profile", ActionIcon::Add, "New profile")) {
			open_profile_create_modal(false);
		}
		ImGui::SameLine();
		if (icon_button("profile-settings", ActionIcon::Settings, "Profile options")) {
			_settings_open = true;
		}
		ImGui::Separator();
		const float clear_width = 26.0f;
		const float search_width = std::max(80.0f, ImGui::GetContentRegionAvail().x - clear_width - spacing);
		ImGui::SetNextItemWidth(search_width);
		ImGui::InputTextWithHint("##profile-search", "Search profiles", &_profile_search);
		ImGui::SameLine();
		ImGui::BeginDisabled(_profile_search.empty());
		if (icon_button("clear-profile-search", ActionIcon::Delete, "Clear search")) {
			_profile_search.clear();
		}
		ImGui::EndDisabled();
		ImGui::Separator();

		if (_profiles.empty()) {
			ImGui::TextDisabled("No XML profiles found.");
		}
		for (const auto& profile : _profiles) {
			if (!profile_matches_search(profile, _profile_search)) {
				continue;
			}

			ImGui::PushID(profile.path.c_str());
			const bool selected = has_profile() && _document.model().path == profile.path;
			const std::string label = profile.name.empty() ? profile.path : profile.name;
			const float edit_width = 26.0f;
			const float copy_width = 26.0f;
			const float delete_width = 26.0f;
			const float selectable_width = std::max(80.0f,
					ImGui::GetContentRegionAvail().x - edit_width - copy_width - delete_width - spacing * 3.0f);
			if (ImGui::Selectable(label.c_str(), selected, 0, ImVec2(selectable_width, 0.0f))) {
				open_profile(profile.path);
			}
			ImGui::SameLine();
			if (icon_button("edit-profile", ActionIcon::Edit, "Edit profile")) {
				open_profile(profile.path);
				if (has_profile()) {
					_profile_editor_open = true;
				}
			}
			ImGui::SameLine();
			if (icon_button("copy-profile", ActionIcon::Copy, "Copy profile")) {
				open_profile(profile.path);
				if (has_profile()) {
					open_profile_create_modal(true);
				}
			}
			ImGui::SameLine();
			if (icon_button("delete-profile", ActionIcon::Delete, "Delete profile")) {
				_pending_delete_profile_path = profile.path;
				_profile_delete_confirm_open = true;
			}
			if (!profile.guid.empty()) {
				ImGui::TextDisabled("%s", profile.guid.c_str());
			}
			ImGui::Separator();
			ImGui::PopID();
		}
	}

	void EditorApp::draw_macro_selector() {
		ImGui::TextUnformatted("Macros");
		ImGui::Separator();
		if (!has_profile()) {
			ImGui::TextWrapped("Open a profile XML file.");
			return;
		}

		if (ImGui::Button("Add Macro")) {
			auto& macro = _document.add_macro();
			_selected_macro_guid = macro.guid;
			open_macro_editor(macro);
		}

		for (const auto& macro : _document.model().macros) {
			ImGui::PushID(macro.guid.c_str());
			const bool selected = _selected_macro_guid == macro.guid;
			const std::string label = macro.name + "  [" + join_keys(macro.keys) + "]";
			const float edit_width = 26.0f;
			const float remove_width = 26.0f;
			const float spacing = ImGui::GetStyle().ItemSpacing.x;
			const float selectable_width = std::max(80.0f,
					ImGui::GetContentRegionAvail().x - edit_width - remove_width - spacing * 2.0f);
			if (ImGui::Selectable(label.c_str(), selected, 0, ImVec2(selectable_width, 0.0f))) {
				_selected_macro_guid = macro.guid;
			}
			if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
				ImGui::SetDragDropPayload("G13_MACRO_GUID", macro.guid.c_str(), macro.guid.size() + 1);
				ImGui::TextUnformatted(macro.name.c_str());
				ImGui::EndDragDropSource();
			}
			ImGui::SameLine();
			if (icon_button("edit-macro", ActionIcon::Edit, "Edit macro")) {
				open_macro_editor(macro);
			}
			ImGui::SameLine();
			if (icon_button("remove-macro", ActionIcon::Delete, "Remove macro")) {
				_pending_delete_macro_guid = macro.guid;
				_macro_delete_confirm_open = true;
			}
			if (!macro.supported) {
				ImGui::TextDisabled("unsupported");
			}
			ImGui::PopID();
			ImGui::Separator();
		}
	}

	void EditorApp::draw_assignment_window() {
		ImGui::TextUnformatted("Assignments");
		const float icon_width = 26.0f;
		const float actions_width = icon_width * 3.0f + ImGui::GetStyle().ItemSpacing.x * 2.0f;
		ImGui::SameLine(std::max(ImGui::GetCursorPosX(), ImGui::GetWindowContentRegionMax().x - actions_width));
		ImGui::BeginDisabled(!has_profile() || !_document.model().dirty);
		if (icon_button("save-profile", ActionIcon::Save, "Save profile")) {
			save_profile(false);
		}
		ImGui::SameLine();
		if (icon_button("save-reload-profile", ActionIcon::Reload, "Save and reload profile")) {
			save_profile(true);
		}
		ImGui::EndDisabled();
		ImGui::SameLine();
		ImGui::BeginDisabled(!has_profile());
		if (icon_button("activate-profile", ActionIcon::Activate, "Activate profile")) {
			activate_profile();
		}
		ImGui::EndDisabled();
		ImGui::Separator();
		if (!has_profile()) {
			ImGui::TextWrapped("Open a profile XML file.");
			return;
		}

		auto& model = _document.model();
		const ImVec2 available = ImGui::GetContentRegionAvail();
		const float canvas_width = std::max(available.x, 420.0f);
		const float canvas_height = std::min(std::max(available.y - 115.0f, 360.0f), 620.0f);
		const ImVec2 origin = ImGui::GetCursorScreenPos();
		const ImVec2 canvas_size(canvas_width, canvas_height);
		ImDrawList* draw_list = ImGui::GetWindowDrawList();

		draw_list->AddRectFilled(origin, ImVec2(origin.x + canvas_size.x, origin.y + canvas_size.y),
								 IM_COL32(31, 34, 38, 255), 10.0f);
		draw_list->AddRect(origin, ImVec2(origin.x + canvas_size.x, origin.y + canvas_size.y),
						   IM_COL32(95, 108, 125, 255), 10.0f, 0, 2.0f);
		draw_list->AddText(ImVec2(origin.x + 16, origin.y + 14), IM_COL32(220, 225, 232, 255), "G13");

		for (const auto& region : g13_button_regions()) {
			const ImVec2 min(origin.x + region.min.x * canvas_size.x, origin.y + region.min.y * canvas_size.y);
			const ImVec2 max(origin.x + region.max.x * canvas_size.x, origin.y + region.max.y * canvas_size.y);
			const bool selected = _selected_context_id == region.context_id;
			const auto assignment = model.assignments.find(region.context_id);
			const Macro* macro = assignment == model.assignments.end() ? nullptr : _document.find_macro(assignment->second);
			const std::string macro_keys = macro ? join_keys(macro->keys) : "";
			const std::string macro_name = macro ? macro->name : "";
			const ImU32 fill = !region.editable ? IM_COL32(72, 76, 82, 255)
							 : selected ? IM_COL32(74, 123, 190, 255)
							 : assignment == model.assignments.end() ? IM_COL32(56, 66, 79, 255)
							 : IM_COL32(57, 104, 85, 255);
			draw_list->AddRectFilled(min, max, fill, 6.0f);
			draw_list->AddRect(min, max, IM_COL32(180, 188, 198, 180), 6.0f);
			const float text_padding_x = 6.0f;
			const float text_padding_y = 4.0f;
			const float region_height = max.y - min.y;
			const float font_size = std::min(ImGui::GetFontSize(), std::max(9.0f, (region_height - text_padding_y * 2.0f) / 3.0f));
			const float line_step = font_size + 1.0f;
			const ImVec4 clip_rect(min.x + 3.0f, min.y + 3.0f, max.x - 3.0f, max.y - 3.0f);
			const ImVec2 text_pos(min.x + text_padding_x, min.y + text_padding_y);
			add_clipped_text(draw_list, text_pos, clip_rect, font_size, IM_COL32(245, 247, 250, 255), region.label);
			add_clipped_text(draw_list, ImVec2(text_pos.x, text_pos.y + line_step), clip_rect,
					font_size, IM_COL32(226, 231, 238, 255), macro_keys);
			add_clipped_text(draw_list, ImVec2(text_pos.x, text_pos.y + line_step * 2.0f), clip_rect,
					font_size, IM_COL32(196, 205, 216, 255), macro_name);

			ImGui::SetCursorScreenPos(min);
			ImGui::PushID(region.context_id.c_str());
			ImGui::InvisibleButton("button-region", ImVec2(max.x - min.x, max.y - min.y));
			if (ImGui::IsItemClicked()) {
				_selected_context_id = region.context_id;
			}
			if (region.editable && ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
				ImGui::SetDragDropPayload("G13_ASSIGNMENT_CONTEXT_ID",
						region.context_id.c_str(), region.context_id.size() + 1);
				if (macro) {
					ImGui::Text("%s -> %s", region.label.c_str(), macro->name.c_str());
				} else {
					ImGui::Text("%s -> <unassigned>", region.label.c_str());
				}
				ImGui::EndDragDropSource();
			}
			if (region.editable && ImGui::BeginDragDropTarget()) {
				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("G13_MACRO_GUID")) {
					const auto macro_guid = std::string(static_cast<const char*>(payload->Data), payload->DataSize - 1);
					_document.set_assignment(region.context_id, macro_guid);
					_selected_context_id = region.context_id;
				}
				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("G13_ASSIGNMENT_CONTEXT_ID")) {
					const auto source_context_id = std::string(static_cast<const char*>(payload->Data), payload->DataSize - 1);
					if (source_context_id != region.context_id) {
						std::string source_macro_guid;
						if (const auto source_assignment = model.assignments.find(source_context_id);
								source_assignment != model.assignments.end()) {
							source_macro_guid = source_assignment->second;
						}
						_document.set_assignment(region.context_id, source_macro_guid);
						if (!source_macro_guid.empty()) {
							_document.set_assignment(source_context_id, "");
						}
						_selected_context_id = region.context_id;
					}
				}
				ImGui::EndDragDropTarget();
			}
			ImGui::PopID();
		}

		ImGui::SetCursorScreenPos(ImVec2(origin.x, origin.y + canvas_size.y + 10.0f));
		if (_selected_context_id.empty()) {
			ImGui::TextUnformatted("Select a button or drag a macro onto it.");
			return;
		}

		ImGui::Text("Selected: %s", _selected_context_id.c_str());
		std::string current_guid;
		if (const auto found = model.assignments.find(_selected_context_id); found != model.assignments.end()) {
			current_guid = found->second;
		}
		const std::string preview = current_guid.empty() ? "<unassigned>" : _document.macro_name(current_guid);
		if (ImGui::BeginCombo("Macro", preview.c_str())) {
			if (ImGui::Selectable("<unassigned>", current_guid.empty())) {
				_document.set_assignment(_selected_context_id, "");
			}
			for (const auto& macro : model.macros) {
				const bool selected = current_guid == macro.guid;
				if (ImGui::Selectable(macro.name.c_str(), selected)) {
					_document.set_assignment(_selected_context_id, macro.guid);
				}
			}
			ImGui::EndCombo();
		}
	}

	void EditorApp::draw_settings_modal() {
		if (!_settings_open) {
			return;
		}

		ImGui::OpenPopup("Profile Options");
		ImGui::SetNextWindowSize(ImVec2(560, 220), ImGuiCond_Appearing);
		if (ImGui::BeginPopupModal("Profile Options", nullptr, ImGuiWindowFlags_NoResize)) {
			ImGui::TextUnformatted("Profile Directory");
			ImGui::InputText("##settings-profile-dir", &_profile_dir);
			if (ImGui::Button("Scan")) {
				scan_profile_dir();
			}
			ImGui::Separator();
			ImGui::TextUnformatted("Daemon Pipe");
			ImGui::InputText("##settings-daemon-pipe", &_daemon.pipe_path);
			ImGui::Checkbox("Activate after reload", &_activate_after_reload);
			ImGui::Separator();
			if (ImGui::Button("Done")) {
				save_settings();
				_settings_open = false;
				ImGui::CloseCurrentPopup();
			}
			ImGui::EndPopup();
		}
	}

	void EditorApp::draw_profile_create_modal() {
		if (!_profile_create_open) {
			return;
		}

		ImGui::OpenPopup(_new_profile_is_copy ? "Copy Profile" : "New Profile");
		ImGui::SetNextWindowSize(ImVec2(460, 170), ImGuiCond_Appearing);
		if (ImGui::BeginPopupModal(_new_profile_is_copy ? "Copy Profile" : "New Profile", nullptr, ImGuiWindowFlags_NoResize)) {
			ImGui::TextUnformatted("Name");
			ImGui::InputText("##new-profile-name", &_new_profile_name);
			ImGui::Separator();
			if (ImGui::Button(_new_profile_is_copy ? "Copy" : "Create")) {
				if (create_profile_from_modal()) {
					_profile_create_open = false;
					ImGui::CloseCurrentPopup();
				}
			}
			ImGui::SameLine();
			if (ImGui::Button("Cancel")) {
				_profile_create_open = false;
				ImGui::CloseCurrentPopup();
			}
			ImGui::EndPopup();
		}
	}

	void EditorApp::draw_profile_editor() {
		if (!_profile_editor_open || !has_profile()) {
			return;
		}

		ImGui::OpenPopup("Edit Profile");
		ImGui::SetNextWindowSize(ImVec2(560, 420), ImGuiCond_Appearing);
		if (ImGui::BeginPopupModal("Edit Profile", nullptr, ImGuiWindowFlags_NoResize)) {
			auto& model = _document.model();
			ImGui::TextWrapped("%s", model.path.c_str());
			ImGui::Separator();

			ImGui::TextUnformatted("Name");
			if (ImGui::InputText("##profile-name", &model.name)) {
				model.dirty = true;
			}
			ImGui::Separator();

			ImGui::TextUnformatted("Description");
			if (ImGui::InputTextMultiline("##profile-description", &model.description, ImVec2(-1, 80))) {
				model.dirty = true;
			}
			ImGui::Separator();

			ImGui::TextUnformatted("Targets");
			for (int i = 0; i < static_cast<int>(model.targets.size()); ++i) {
				ImGui::PushID(i);
				ImGui::TextUnformatted("Path");
				if (ImGui::InputText("##target-path", &model.targets[i])) {
					model.dirty = true;
				}
				ImGui::SameLine();
				if (ImGui::SmallButton("Remove")) {
					model.targets.erase(model.targets.begin() + i);
					model.dirty = true;
					ImGui::PopID();
					break;
				}
				ImGui::Separator();
				ImGui::PopID();
			}
			if (ImGui::Button("Add Target")) {
				model.targets.emplace_back();
				model.dirty = true;
			}
			ImGui::Separator();

			if (!model.backlight_color.empty()) {
				ImGui::TextUnformatted("Backlight Color");
				if (ImGui::InputText("##backlight-color", &model.backlight_color)) {
					model.dirty = true;
				}
				ImGui::Separator();
			}

			if (ImGui::Button("Done")) {
				_profile_editor_open = false;
				ImGui::CloseCurrentPopup();
			}
			ImGui::SameLine();
			if (ImGui::Button("Save")) {
				save_profile(false);
				_profile_editor_open = false;
				ImGui::CloseCurrentPopup();
			}
			ImGui::EndPopup();
		}
	}

	void EditorApp::draw_profile_delete_confirm() {
		if (!_profile_delete_confirm_open) {
			return;
		}

		ImGui::OpenPopup("Delete Profile");
		ImGui::SetNextWindowSize(ImVec2(520, 180), ImGuiCond_Appearing);
		if (ImGui::BeginPopupModal("Delete Profile", nullptr, ImGuiWindowFlags_NoResize)) {
			ImGui::TextWrapped("Delete this profile XML file?");
			ImGui::TextWrapped("%s", _pending_delete_profile_path.c_str());
			ImGui::Separator();
			if (ImGui::Button("Delete")) {
				std::string error;
				std::error_code remove_error;
				if (!std::filesystem::remove(_pending_delete_profile_path, remove_error)) {
					_status = "Failed to delete profile: " + remove_error.message();
				} else {
					_status = "Deleted profile " + _pending_delete_profile_path;
					if (!_daemon.send_reload_profiles(error)) {
						_status += "; reload-all failed: " + error;
					} else {
						_status += "; reload-all command sent";
					}
					_document = {};
					_profile_path.clear();
					scan_profile_dir();
				}
				_pending_delete_profile_path.clear();
				_profile_delete_confirm_open = false;
				ImGui::CloseCurrentPopup();
			}
			ImGui::SameLine();
			if (ImGui::Button("Cancel")) {
				_pending_delete_profile_path.clear();
				_profile_delete_confirm_open = false;
				ImGui::CloseCurrentPopup();
			}
			ImGui::EndPopup();
		}
	}

	void EditorApp::draw_macro_editor() {
		if (_editing_macro_guid.empty()) {
			return;
		}

		ImGui::OpenPopup("Edit Macro");
		ImGui::SetNextWindowSize(ImVec2(440, 220), ImGuiCond_Appearing);
		if (ImGui::BeginPopupModal("Edit Macro", nullptr, ImGuiWindowFlags_NoResize)) {
			ImGui::InputText("Name", &_editing_macro_name);
			ImGui::InputText("Keys", &_editing_macro_keys);
			ImGui::SameLine();
			if (ImGui::Button("Capture")) {
				_editing_macro_keys.clear();
				_capturing_macro_keys = true;
				ImGui::SetKeyboardFocusHere(-1);
			}
			if (_capturing_macro_keys) {
				ImGui::SameLine();
				ImGui::TextDisabled("press a key");
				if (capture_key(_editing_macro_keys)) {
					if (_editing_macro_name.empty()) {
						_editing_macro_name = _editing_macro_keys;
					}
					_capturing_macro_keys = false;
				}
			}

			if (ImGui::Button("Save")) {
				if (auto* macro = _document.find_macro(_editing_macro_guid)) {
					macro->name = _editing_macro_name.empty() ? "Unnamed Macro" : _editing_macro_name;
					macro->keys = split_keys(_editing_macro_keys);
					if (macro->keys.empty()) {
						macro->keys.push_back("A");
					}
					macro->supported = true;
					_document.model().dirty = true;
				}
				_editing_macro_guid.clear();
				_capturing_macro_keys = false;
				ImGui::CloseCurrentPopup();
			}
			ImGui::SameLine();
			if (ImGui::Button("Delete")) {
				_document.delete_macro(_editing_macro_guid);
				_editing_macro_guid.clear();
				_capturing_macro_keys = false;
				ImGui::CloseCurrentPopup();
			}
			ImGui::SameLine();
			if (ImGui::Button("Cancel")) {
				_editing_macro_guid.clear();
				_capturing_macro_keys = false;
				ImGui::CloseCurrentPopup();
			}
			ImGui::TextWrapped("Keys use Logitech XML names, for example LSHIFT+F1 or SPACEBAR.");
			ImGui::EndPopup();
		}
	}

	void EditorApp::draw_macro_delete_confirm() {
		if (!_macro_delete_confirm_open) {
			return;
		}

		ImGui::OpenPopup("Remove Macro");
		ImGui::SetNextWindowSize(ImVec2(420, 160), ImGuiCond_Appearing);
		if (ImGui::BeginPopupModal("Remove Macro", nullptr, ImGuiWindowFlags_NoResize)) {
			const auto name = _document.macro_name(_pending_delete_macro_guid);
			ImGui::TextWrapped("Remove macro '%s'?", name.empty() ? _pending_delete_macro_guid.c_str() : name.c_str());
			ImGui::TextWrapped("Assignments using this macro will be cleared.");
			ImGui::Separator();
			if (ImGui::Button("Remove")) {
				_document.delete_macro(_pending_delete_macro_guid);
				if (_selected_macro_guid == _pending_delete_macro_guid) {
					_selected_macro_guid.clear();
				}
				_pending_delete_macro_guid.clear();
				_macro_delete_confirm_open = false;
				ImGui::CloseCurrentPopup();
			}
			ImGui::SameLine();
			if (ImGui::Button("Cancel")) {
				_pending_delete_macro_guid.clear();
				_macro_delete_confirm_open = false;
				ImGui::CloseCurrentPopup();
			}
			ImGui::EndPopup();
		}
	}

	void EditorApp::open_macro_editor(const Macro& macro) {
		_editing_macro_guid = macro.guid;
		_editing_macro_name = macro.name;
		_editing_macro_keys = join_keys(macro.keys);
		_capturing_macro_keys = false;
	}

	void EditorApp::open_profile_create_modal(const bool copy) {
		_new_profile_is_copy = copy;
		_new_profile_name = copy && has_profile() ? _document.model().name + " Copy" : "New Profile";
		_profile_create_open = true;
	}

	void EditorApp::scan_profile_dir() {
		_profiles.clear();
		std::error_code error_code;
		if (!std::filesystem::exists(_profile_dir, error_code) || !std::filesystem::is_directory(_profile_dir, error_code)) {
			_status = "Profile directory does not exist: " + _profile_dir;
			return;
		}

		for (const auto& entry : std::filesystem::directory_iterator(_profile_dir, error_code)) {
			if (error_code || !entry.is_regular_file() || entry.path().extension() != ".xml") {
				continue;
			}

			pugi::xml_document doc;
			if (!doc.load_file(entry.path().c_str())) {
				continue;
			}
			const auto profile = doc.child("profiles").child("profile");
			if (!profile) {
				continue;
			}

			_profiles.push_back({
					entry.path().string(),
					profile.attribute("guid").value(),
					profile.attribute("name").value(),
			});
		}

		std::ranges::sort(_profiles, {}, &ListedProfile::name);
		_status = std::format("Found {} profile(s) in {}", _profiles.size(), _profile_dir);
	}

	void EditorApp::open_profile(const std::string& path) {
		std::string error;
		if (_document.load(path, error)) {
			_profile_path = _document.model().path;
			_profile_dir = std::filesystem::path(_profile_path).parent_path().string();
			_status = "Loaded " + _document.model().name;
			save_settings();
		} else {
			_status = error;
		}
	}

	bool EditorApp::create_profile_from_modal() {
		std::string error;
		const auto directory = std::filesystem::path(_profile_dir);
		const bool created = _new_profile_is_copy
				? _document.copy_loaded_as(directory, _new_profile_name, error)
				: _document.create_empty(directory, _new_profile_name, error);
		if (!created) {
			_status = error;
			return false;
		}

		_profile_path = _document.model().path;
		_selected_macro_guid.clear();
		_selected_context_id.clear();
		const auto created_status = (_new_profile_is_copy ? "Copied profile to " : "Created profile ") + _profile_path;
		scan_profile_dir();
		_status = created_status;
		save_settings();
		return true;
	}

	void EditorApp::activate_profile() {
		std::string error;
		if (_document.model().dirty && !_document.save(error)) {
			_status = "Activation save failed: " + error;
			return;
		}

		if (!_daemon.send_reload_profile(_document.model().guid, error)) {
			_status = "Activation reload failed: " + error;
			return;
		}

		if (!_daemon.send_activate_profile(_document.model().guid, error)) {
			_status = "Activation failed: " + error;
		} else {
			scan_profile_dir();
			_status = "Reloaded and activated " + _document.model().name;
		}
	}

	void EditorApp::save_profile(const bool reload) {
		std::string error;
		if (!_document.save(error)) {
			_status = error;
			return;
		}

		const auto saved_status = "Saved " + _document.model().path;
		scan_profile_dir();
		_status = saved_status;
		if (!reload) {
			return;
		}

		if (!_daemon.send_reload_profile(_document.model().guid, error)) {
			_status += "; reload failed: " + error;
			return;
		}
		_status += "; reload command sent";

		if (_activate_after_reload) {
			if (!_daemon.send_activate_profile(_document.model().guid, error)) {
				_status += "; activation failed: " + error;
			} else {
				_status += "; activation command sent";
			}
		}
	}

	void EditorApp::save_settings() const {
		auto merged = load_editor_settings();
		merged.profile_dir = _profile_dir;
		merged.daemon_pipe = _daemon.pipe_path;
		merged.activate_after_reload = _activate_after_reload;
		merged.last_profile_guid = _document.model().guid;
		save_editor_settings(merged);
	}
}
