//
// Created by vert9 on 11/23/23.
//

#include <csignal>
#include <filesystem>
#include <format>
#include <iostream>
#include <sstream>

#include <wordexp.h>
#include <utility>

#include "g13_device.h"
#include "g13_manager.h"
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

				auto profile_dir = string_config_value("profile_dir", "~/.g13d/profiles");
				auto device = new G13_Device(_logger, handle, g13s.size(), profile_dir);
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

	std::string G13_Manager::string_config_value(const std::string& name, std::string default_val) const {
		try {
			auto found = find_or_throw(_string_config_values, name);
			if (found.empty()) {
				return default_val;
			}
			return found;
		}
		catch (...) {
			return default_val;
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

	void G13_Manager::init_profiles() {
		// Load all profiles in directory into all devices
		for (const auto& g13 : g13s) {
			g13->init_profiles();
		}
	}
}
