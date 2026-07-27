#include "profile_document.h"

#include <algorithm>
#include <filesystem>
#include <format>
#include <random>
#include <set>
#include <utility>

namespace {
	/**
	 * Returns true when a pugixml node is an element node.
	 * @param node node to inspect.
	 * @return true when node is a valid element node.
	 */
	bool is_element(const pugi::xml_node& node) {
		return node && node.type() == pugi::node_element;
	}

	/**
	 * Generates a Logitech-style GUID string for new profiles and macros.
	 * @return generated GUID string wrapped in braces.
	 */
	std::string make_guid() {
		static std::random_device device;
		static std::mt19937 rng(device());
		static std::uniform_int_distribution<int> hex(0, 15);
		const char* digits = "0123456789ABCDEF";
		std::string guid = "{00000000-0000-4000-8000-000000000000}";
		for (char& c : guid) {
			if (c == '0') {
				c = digits[hex(rng)];
			}
		}
		guid[15] = '4';
		guid[20] = digits[8 + (hex(rng) % 4)];
		return guid;
	}

	/**
	 * Builds the profile XML path for a generated GUID.
	 * @param directory directory that will contain the profile XML.
	 * @param guid profile GUID used as the filename stem.
	 * @return full path to the profile XML file.
	 */
	std::filesystem::path path_for_guid(const std::filesystem::path& directory, const std::string& guid) {
		return directory / (guid + ".xml");
	}
}

namespace G13::Editor {
	bool ProfileDocument::load(const std::filesystem::path& path, std::string& error) {
		_xml.reset();
		_model = {};

		const auto result = _xml.load_file(path.c_str());
		if (!result) {
			error = std::format("Failed to load profile XML '{}': {}", path.string(), result.description());
			return false;
		}

		_profile = _xml.child("profiles").child("profile");
		if (!_profile) {
			error = "Profile XML does not contain /profiles/profile.";
			return false;
		}

		_model.path = path.string();
		parse_model();
		_model.dirty = false;
		return true;
	}

	bool ProfileDocument::create_empty(const std::filesystem::path& directory, const std::string& name, std::string& error) {
		std::error_code error_code;
		std::filesystem::create_directories(directory, error_code);
		if (error_code) {
			error = std::format("Failed to create profile directory '{}': {}", directory.string(), error_code.message());
			return false;
		}

		_xml.reset();
		_model = {};
		const auto guid = make_guid();
		_model.path = path_for_guid(directory, guid).string();
		_model.guid = guid;
		_model.name = name.empty() ? "New Profile" : name;
		_model.backlight_color = "#ffffff";

		auto declaration = _xml.append_child(pugi::node_declaration);
		declaration.append_attribute("version").set_value("1.0");
		declaration.append_attribute("encoding").set_value("utf-8");

		auto profiles = _xml.append_child("profiles");
		profiles.append_attribute("xmlns").set_value("http://www.logitech.com/Cassandra/2010.7/Profile");
		_profile = profiles.append_child("profile");
		_profile.append_attribute("lastplayeddate").set_value("");
		_profile.append_attribute("gameid").set_value("");
		_profile.append_attribute("lock").set_value("0");
		_profile.append_attribute("launchable").set_value("1");
		_profile.append_attribute("name").set_value(_model.name.c_str());
		_profile.append_attribute("gkeysdk").set_value("0");
		_profile.append_attribute("gpasupported").set_value("0");
		_profile.append_attribute("guid").set_value(_model.guid.c_str());
		_profile.append_child("description");
		auto signature = _profile.append_child("signature");
		signature.append_attribute("value").set_value("");
		signature.append_attribute("key").set_value("");
		signature.append_attribute("name").set_value("");
		signature.append_attribute("executable").set_value("");
		_profile.append_child("macros");
		assignments_node();
		auto backlight = _profile.append_child("backlight");
		backlight.append_attribute("devicemodel").set_value("Logitech.Gaming.LeftHandedController.G13");
		for (int shiftstate = 1; shiftstate <= 3; ++shiftstate) {
			auto mode = backlight.append_child("mode");
			mode.append_attribute("shiftstate").set_value(std::to_string(shiftstate).c_str());
			mode.append_attribute("color").set_value(_model.backlight_color.c_str());
		}
		_profile.append_child("script").text().set("");

		_model.dirty = true;
		return save(error);
	}

	bool ProfileDocument::copy_loaded_as(const std::filesystem::path& directory, const std::string& name, std::string& error) {
		if (!_profile) {
			error = "No profile is loaded.";
			return false;
		}

		std::error_code error_code;
		std::filesystem::create_directories(directory, error_code);
		if (error_code) {
			error = std::format("Failed to create profile directory '{}': {}", directory.string(), error_code.message());
			return false;
		}

		write_model_to_xml();
		const auto guid = make_guid();
		_model.path = path_for_guid(directory, guid).string();
		_model.guid = guid;
		_model.name = name.empty() ? "Profile Copy" : name;
		_model.dirty = true;
		return save(error);
	}

	bool ProfileDocument::save(std::string& error) {
		if (_model.path.empty()) {
			error = "No profile path is loaded.";
			return false;
		}

		write_model_to_xml();
		if (!_xml.save_file(_model.path.c_str(), "  ", pugi::format_default, pugi::encoding_utf8)) {
			error = "Failed to write profile XML.";
			return false;
		}

		_model.dirty = false;
		return true;
	}

	Macro* ProfileDocument::find_macro(const std::string& guid) {
		const auto found = std::ranges::find_if(_model.macros, [&](const Macro& macro) {
			return macro.guid == guid;
		});
		return found == _model.macros.end() ? nullptr : &*found;
	}

	const Macro* ProfileDocument::find_macro(const std::string& guid) const {
		const auto found = std::ranges::find_if(_model.macros, [&](const Macro& macro) {
			return macro.guid == guid;
		});
		return found == _model.macros.end() ? nullptr : &*found;
	}

	std::string ProfileDocument::macro_name(const std::string& guid) const {
		if (const auto* macro = find_macro(guid)) {
			return macro->name;
		}
		return {};
	}

	void ProfileDocument::set_assignment(const std::string& context_id, const std::string& macro_guid) {
		if (macro_guid.empty()) {
			_model.assignments.erase(context_id);
		} else {
			_model.assignments[context_id] = macro_guid;
		}
		_model.dirty = true;
	}

	Macro& ProfileDocument::add_macro() {
		auto& macro = _model.macros.emplace_back();
		macro.guid = make_guid();
		macro.name.clear();
		macro.keys = {"A"};
		_model.dirty = true;
		return macro;
	}

	void ProfileDocument::delete_macro(const std::string& guid) {
		std::erase_if(_model.macros, [&](const Macro& macro) {
			return macro.guid == guid;
		});
		for (auto it = _model.assignments.begin(); it != _model.assignments.end();) {
			if (it->second == guid) {
				it = _model.assignments.erase(it);
			} else {
				++it;
			}
		}
		_model.dirty = true;
	}

	void ProfileDocument::parse_model() {
		_model.guid = _profile.attribute("guid").value();
		_model.name = _profile.attribute("name").value();
		_model.description = _profile.child("description").child_value();

		for (const auto target : _profile.children("target")) {
			_model.targets.emplace_back(target.attribute("path").value());
		}

		if (const auto mode = _profile.child("backlight").child("mode")) {
			_model.backlight_color = mode.attribute("color").value();
		}

		if (const auto macros = _profile.child("macros")) {
			for (const auto macro_node : macros.children("macro")) {
				Macro macro;
				macro.guid = macro_node.attribute("guid").value();
				macro.name = macro_node.attribute("name").value();
				macro.supported = false;

				for (auto keys_node = macro_node.first_child(); keys_node; keys_node = keys_node.next_sibling()) {
					if (!is_element(keys_node)) {
						continue;
					}

					const std::string node_name = keys_node.name();
					if (node_name != "keystroke" && node_name != "multikey") {
						break;
					}

					macro.supported = true;
					for (const auto key : keys_node.children("key")) {
						macro.keys.emplace_back(key.attribute("value").value());
					}
					break;
				}

				_model.macros.push_back(std::move(macro));
			}
		}

		for (const auto assignments : _profile.children("assignments")) {
			const std::string category = assignments.attribute("devicecategory").value();
			if (category != "Logitech.Gaming.LeftHandedController") {
				continue;
			}
			for (const auto assignment : assignments.children("assignment")) {
				if (std::string(assignment.attribute("backup").value()) != "false") {
					continue;
				}
				_model.assignments[assignment.attribute("contextid").value()] = assignment.attribute("macroguid").value();
			}
		}
	}

	void ProfileDocument::write_model_to_xml() {
		_profile.attribute("guid").set_value(_model.guid.c_str());
		_profile.attribute("name").set_value(_model.name.c_str());

		auto description = profile_child("description");
		description.text().set(_model.description.c_str());

		for (auto target = _profile.child("target"); target;) {
			const auto next = target.next_sibling("target");
			_profile.remove_child(target);
			target = next;
		}

		auto insert_after = description ? description : pugi::xml_node{};
		for (const auto& target_path : _model.targets) {
			pugi::xml_node target;
			if (insert_after) {
				target = _profile.insert_child_after("target", insert_after);
			} else {
				target = _profile.prepend_child("target");
			}
			target.append_attribute("path").set_value(target_path.c_str());
			insert_after = target;
		}

		auto macros = macros_node();
		std::set<std::string> seen_macros;
		for (auto macro_node = macros.child("macro"); macro_node;) {
			const auto next = macro_node.next_sibling("macro");
			const std::string guid = macro_node.attribute("guid").value();
			if (const auto* macro = find_macro(guid)) {
				macro_node.attribute("name").set_value(macro->name.c_str());
				for (auto child = macro_node.first_child(); child;) {
					const auto next_child = child.next_sibling();
					if (is_element(child)) {
						macro_node.remove_child(child);
					}
					child = next_child;
				}
				auto keys_node = macro_node.append_child(macro->keys.size() <= 1 ? "keystroke" : "multikey");
				keys_node.append_attribute("xmlns").set_value(macro->keys.size() <= 1
						? "http://www.logitech.com/Cassandra/2010.1/Macros/Keystroke"
						: "http://www.logitech.com/Cassandra/2010.1/Macros/MultiKey");
				for (const auto& key_value : macro->keys) {
					keys_node.append_child("key").append_attribute("value").set_value(key_value.c_str());
				}
				seen_macros.insert(guid);
			} else {
				macros.remove_child(macro_node);
			}
			macro_node = next;
		}

		for (const auto& macro : _model.macros) {
			if (seen_macros.contains(macro.guid)) {
				continue;
			}
			auto macro_node = macros.append_child("macro");
			macro_node.append_attribute("name").set_value(macro.name.c_str());
			macro_node.append_attribute("hidden").set_value("false");
			macro_node.append_attribute("guid").set_value(macro.guid.c_str());
			macro_node.append_attribute("color").set_value("4278246655");
			auto keys_node = macro_node.append_child(macro.keys.size() <= 1 ? "keystroke" : "multikey");
			keys_node.append_attribute("xmlns").set_value(macro.keys.size() <= 1
					? "http://www.logitech.com/Cassandra/2010.1/Macros/Keystroke"
					: "http://www.logitech.com/Cassandra/2010.1/Macros/MultiKey");
			for (const auto& key_value : macro.keys) {
				keys_node.append_child("key").append_attribute("value").set_value(key_value.c_str());
			}
		}

		auto assignments = assignments_node();
		std::set<std::string> seen_assignments;
		for (auto assignment = assignments.child("assignment"); assignment;) {
			const auto next = assignment.next_sibling("assignment");
			if (std::string(assignment.attribute("backup").value()) == "false") {
				const std::string context_id = assignment.attribute("contextid").value();
				const auto desired = _model.assignments.find(context_id);
				if (desired == _model.assignments.end()) {
					assignments.remove_child(assignment);
				} else {
					assignment.attribute("macroguid").set_value(desired->second.c_str());
					seen_assignments.insert(context_id);
				}
			}
			assignment = next;
		}

		for (const auto& [context_id, macro_guid] : _model.assignments) {
			if (seen_assignments.contains(context_id)) {
				continue;
			}
			auto assignment = assignments.append_child("assignment");
			assignment.append_attribute("original").set_value("false");
			assignment.append_attribute("backup").set_value("false");
			assignment.append_attribute("shiftstate").set_value("1");
			assignment.append_attribute("contextid").set_value(context_id.c_str());
			assignment.append_attribute("macroguid").set_value(macro_guid.c_str());
		}

		if (!_model.backlight_color.empty()) {
			if (auto mode = _profile.child("backlight").child("mode")) {
				mode.attribute("color").set_value(_model.backlight_color.c_str());
			}
		}
	}

	pugi::xml_node ProfileDocument::profile_child(const char* name) {
		if (auto child = _profile.child(name)) {
			return child;
		}
		return _profile.prepend_child(name);
	}

	pugi::xml_node ProfileDocument::assignments_node() {
		for (const auto assignments : _profile.children("assignments")) {
			if (std::string(assignments.attribute("devicecategory").value()) == "Logitech.Gaming.LeftHandedController") {
				return assignments;
			}
		}
		auto assignments = _profile.append_child("assignments");
		assignments.append_attribute("devicecategory").set_value("Logitech.Gaming.LeftHandedController");
		return assignments;
	}

	pugi::xml_node ProfileDocument::macros_node() {
		if (auto macros = _profile.child("macros")) {
			return macros;
		}
		return _profile.append_child("macros");
	}
}
