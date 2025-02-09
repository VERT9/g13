//
// Created by vert9 on 11/23/23.
//

#include <csignal>
#include <filesystem>
#include <format>
#include <iostream>
#include <set>
#include <sstream>

#include <pugixml.hpp>
#include <wordexp.h>
#include <utility>

#include "g13_device.h"
#include "g13_keys.h"
#include "g13_manager.h"
#include "g13_profile.h"
#include "g13_stick.h"
#include "helper.h"

using namespace std;
using Helper::repr;
using Helper::find_or_throw;

namespace G13 {
	G13_Manager::G13_Manager(std::shared_ptr<G13_Log> logger, std::shared_ptr<G13_KeyMap> keymap) : devs(0), ctx(0), _logger(std::move(logger)), _keymap(std::move(keymap)) {}

	bool G13_Manager::running = true;

	void G13_Manager::set_stop(int) {
		running = false;
	}

	void G13_Manager::discover_g13s(libusb_device** devs, ssize_t count, vector<G13_Device*>& g13s) {
		for (int i = 0; i < count; i++) {
			libusb_device_descriptor desc{};
			int r = libusb_get_device_descriptor(devs[i], &desc);
			if (r < 0) {
				_logger->error("Failed to get device descriptor");
				return;
			}
			if (desc.idVendor == G13_VENDOR_ID && desc.idProduct == G13_PRODUCT_ID) {
				libusb_device_handle* handle;
				int r = libusb_open(devs[i], &handle);
				if (r != 0) {
					_logger->error("Error opening G13 device");
					return;
				}
				if (libusb_kernel_driver_active(handle, 0) == 1)
					if (libusb_detach_kernel_driver(handle, 0) == 0)
						_logger->info("Kernel driver detached");

				r = libusb_claim_interface(handle, 0);
				if (r < 0) {
					_logger->error("Cannot Claim Interface");
					return;
				}

				auto device = new G13_Device(_logger, handle, g13s.size());
				g13s.push_back(device);
				g13s.back()->init();
			}
		}
	}

	void G13_Manager::cleanup() {
		_logger->info("cleaning up");
		for (int i = 0; i < g13s.size(); i++) {
			g13s[i]->cleanup();
			delete g13s[i];
		}
		libusb_exit(ctx);
	}

	std::string G13_Manager::string_config_value(const std::string& name) const {
		try {
			return find_or_throw(_string_config_values, name);
		}
		catch (...) {
			return "";
		}
	}

	void G13_Manager::set_string_config_value(const std::string& name, const std::string& value) {
		_logger->info("set_string_config_value " + name + " = " + repr(value).s);
		_string_config_values[name] = value;
	}

	int G13_Manager::run() {
		display_keys();

		ssize_t cnt;
		int ret;

		const struct libusb_init_option options = {.option = LIBUSB_OPTION_LOG_LEVEL, .value = {.ival = LIBUSB_LOG_LEVEL_INFO}};
		ret = libusb_init_context(&ctx, &options, 1);
		if (ret < 0) {
			_logger->error("Initialization error: " + std::to_string(ret));
			return 1;
		}

		cnt = libusb_get_device_list(ctx, &devs);
		if (cnt < 0) {
			_logger->error("Error while getting device list");
			return 1;
		}

		discover_g13s(devs, cnt, g13s);
		libusb_free_device_list(devs, 1);
		_logger->info("Found " + std::to_string(g13s.size()) + " G13s");
		if (g13s.empty()) {
			return 1;
		}

		for (int i = 0; i < g13s.size(); i++) {
			g13s[i]->register_context(ctx, *this);
		}
		signal(SIGINT, set_stop);
		if (g13s.size() > 0 && logo_filename.size()) {
			g13s[0]->write_lcd_file(logo_filename);
		}

		_logger->info("Active Stick zones ");
		g13s[0]->stick().dump(std::cout);

		std::string config_fn = string_config_value("config");
		if (config_fn.size()) {
			_logger->info("config_fn = " + config_fn);
			g13s[0]->read_config_file(config_fn);
		}

		init_profiles();

		do {
			if (!g13s.empty()) {
				for (auto & g13 : g13s) {
					// TODO allow for other LCD apps to run
					g13->display_app();

					int status = g13->read_keys();
					g13->read_commands();
					int err = libusb_handle_events(ctx);
					if(status < 0 || err != LIBUSB_SUCCESS)
						running = false;
				}
			}
		}
		while (running);
		cleanup();

		return 0;
	}

	void G13_Manager::init_keynames() {
	}

	void G13_Manager::display_keys() {
		_logger->info("Known keys on G13:");
		std::stringstream stream;
		stream << _keymap->map_G13_keys();
		_logger->info(stream.str());

		_logger->info("Known keys to map to:");
		stream << _keymap->map_input_keys();
		_logger->info(stream.str());
	}

	void G13_Manager::load_profile(G13_Device* device, const std::string& filename) {
		pugi::xml_document doc;
		pugi::xml_parse_result result = doc.load_file(filename.c_str());
		if (!result) {
			_logger->warning(std::string ("Profile can not be read: ").append(filename));
			return;
		}

		std::string name = doc.select_node("/profiles/profile").node().attribute("name").value();
		std::string guid = doc.select_node("/profiles/profile").node().attribute("guid").value();
		_logger->info(std::format("{}: {}", guid, name));
		ProfilePtr profile = device->profile(guid, name);

		pugi::xpath_node_set assignments = doc.select_nodes("/profiles/profile/assignments[@devicecategory='Logitech.Gaming.LeftHandedController']/assignment[@backup='false']");

		for (auto node : assignments) {
			// Find G Key
			std::string keyname = node.node().attribute("contextid").value();
			// TODO stick zones are dynamic and troublesome for this
			// keymap for converting from Logitech -> g13/key
			std::map<std::string, std::string> gkey_convert_map = {
					{"G23", "LEFT"},
					{"G24", "DOWN"},
					{"G25", "TOP"},
					{"G26", "STICK_UP"},
					{"G27", "STICK_RIGHT"},
					{"G28", "STICK_DOWN"},
					{"G29", "STICK_LEFT"}
			};
			if (gkey_convert_map.count(keyname) > 0)
				keyname = gkey_convert_map.at(keyname);

			// Find Keystroke
			// keymap for converting from Logitech -> linux/input
			std::map<std::string, std::string> key_convert_map = {
					{"SPACEBAR", "SPACE"},
					{"LSHIFT", "LEFTSHIFT"},
					{"RSHIFT", "RIGHTSHIFT"},
					{"LCTRL", "LEFTCTRL"},
					{"RCTRL", "RIGHTCTRL"},
					{"LALT", "LEFTALT"},
					{"RALT", "RIGHTALT"},
					{"LBRACKET", "LEFTBRACE"},
					{"RBRACKET", "RIGHTBRACE"},
					{"ESCAPE", "ESC"}
			};

			// TODO handle key direction and timings
			// Get macro from XML
			std::string macroguid = node.node().attribute("macroguid").value();
			std::string macro_query = "/profiles/profile/macros/macro[@guid='"+macroguid+"']";
			pugi::xpath_node_set macro = doc.select_nodes(macro_query.c_str());

			std::string action;
			for(auto keyset : macro) {
				// Check for support: only multikey and keystroke elements for now
				auto keys = keyset.node().first_child();
				if (strcmp(keys.name(), "multikey") != 0 and strcmp(keys.name(), "keystroke") != 0) {
					_logger->warning(std::format("Macro not supported: {}", keys.name()));
					continue;
				}

				// Gather unique keys only
				std::set<std::string> unique_keys;
				for (auto keystroke : keys.children("key")) {
					// Extract value and add to set
					unique_keys.emplace(keystroke.attribute("value").value());
				}

				// Build action
				for (auto key : unique_keys) {
					// Convert if necessary
					if (key_convert_map.count(key) > 0)
						key = key_convert_map.at(key);

					// Add prefix
					key.insert(0, "KEY_");

					// Add operator
					if (!action.empty())
						action += "+";

					// Add keystroke
					action += key;
				}
			}

			// Bind keys to actions
			try {
				if (auto gkey = profile->find_key(keyname)) {
					vector<std::string> excluded {"BD", "L1", "L2", "L3", "L4"};
					if (ranges::find(excluded, keyname) == excluded.end())
						gkey->set_action(device->make_action(action));
				} else if (auto stick_key = device->stick().zone(keyname)) {
					stick_key->set_action(device->make_action(action));
				} else {
					_logger->warning("bind key " + keyname + " unknown");
				}
				_logger->debug(std::format("bind {} [{}]", keyname, action));
			} catch (const std::exception& ex) {
				_logger->error(std::format("bind {} [{}] failed : {}", keyname, action, ex.what()));
			}
		}
	}

	void G13_Manager::init_profiles() {
		// Resolve profiles directory
		std::string profile_dir = string_config_value("profiles_dir");
		if (profile_dir.empty()) {
			profile_dir = std::string("~/.g13d/profiles");
		}

		// Expand possible '~'
		wordexp_t exp_result;
		wordexp(profile_dir.c_str(), &exp_result, 0);
		profile_dir = std::string(*(exp_result.we_wordv));
		wordfree(&exp_result);

		// Make the directory if it doesn't exist
		std::filesystem::create_directories(profile_dir);

		// Load all profiles in directory into all devices
		for (const auto& g13 : g13s) {
			for (const auto& entry: std::filesystem::directory_iterator(profile_dir)) {
				load_profile(g13, entry.path());
			}
		}
	}
}
