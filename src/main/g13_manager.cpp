//
// Created by vert9 on 11/23/23.
//

#include "helper.h"
#include <csignal>

#include <linux/input.h>
#include <boost/log/core/core.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/sources/severity_feature.hpp>
#include <boost/log/sources/severity_logger.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/utility/setup.hpp>
#include <boost/log/utility/setup/console.hpp>

#include "g13_manager.h"
#include "g13_device.h"
#include "g13_keys.h"

using namespace std;
using Helper::repr;
using Helper::find_or_throw;

namespace G13 {
	#define CONTROL_DIR std::string("/tmp/")

	G13_Manager::G13_Manager() : ctx(0), devs(0) {}

	bool G13_Manager::running = true;

	void G13_Manager::set_stop(int) {
		running = false;
	}

	void G13_Manager::discover_g13s(libusb_device** devs, ssize_t count, vector<G13_Device*>& g13s) {
		for (int i = 0; i < count; i++) {
			libusb_device_descriptor desc;
			int r = libusb_get_device_descriptor(devs[i], &desc);
			if (r < 0) {
				_logger.error("Failed to get device descriptor");
				return;
			}
			if (desc.idVendor == G13_VENDOR_ID && desc.idProduct == G13_PRODUCT_ID) {
				libusb_device_handle* handle;
				int r = libusb_open(devs[i], &handle);
				if (r != 0) {
					_logger.error("Error opening G13 device");
					return;
				}
				if (libusb_kernel_driver_active(handle, 0) == 1)
					if (libusb_detach_kernel_driver(handle, 0) == 0)
						_logger.info("Kernel driver detached");

				r = libusb_claim_interface(handle, 0);
				if (r < 0) {
					_logger.error("Cannot Claim Interface");
					return;
				}
				g13s.push_back(new G13_Device(*this, _logger, handle, g13s.size()));
			}
		}
	}

	void G13_Manager::cleanup() {
		_logger.info("cleaning up");
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
		_logger.info("set_string_config_value " + name + " = " + repr(value).s);
		_string_config_values[name] = value;
	}

	std::string G13_Manager::make_pipe_name(G13_Device* d, bool is_input) {
		if (is_input) {
			std::string config_base = string_config_value("pipe_in");
			if (config_base.size()) {
				if (d->id_within_manager() == 0) {
					return config_base;
				} else {
					return config_base + "-" + boost::lexical_cast<std::string>(d->id_within_manager());
				}
			}

			return CONTROL_DIR + "g13-" + boost::lexical_cast<std::string>(d->id_within_manager());
		} else {
			std::string config_base = string_config_value("pipe_out");
			if (config_base.size()) {
				if (d->id_within_manager() == 0) {
					return config_base;
				} else {
					return config_base + "-" + boost::lexical_cast<std::string>(d->id_within_manager());
				}
			}

			return CONTROL_DIR + "g13-" + boost::lexical_cast<std::string>(d->id_within_manager()) + "_out";
		}
	}

	int G13_Manager::run() {

		init_keynames();
		display_keys();

		ssize_t cnt;
		int ret;

		ret = libusb_init(&ctx);
		if (ret < 0) {
			_logger.error("Initialization error: " + std::to_string(ret));
			return 1;
		}

		libusb_set_option(ctx, LIBUSB_OPTION_LOG_LEVEL, 3);
		cnt = libusb_get_device_list(ctx, &devs);
		if (cnt < 0) {
			_logger.error("Error while getting device list");
			return 1;
		}

		discover_g13s(devs, cnt, g13s);
		libusb_free_device_list(devs, 1);
		_logger.info("Found " + std::to_string(g13s.size()) + " G13s");
		if (g13s.size() == 0) {
			return 1;
		}

		for (int i = 0; i < g13s.size(); i++) {
			g13s[i]->register_context(ctx);
		}
		signal(SIGINT, set_stop);
		if (g13s.size() > 0 && logo_filename.size()) {
			g13s[0]->write_lcd_file(logo_filename);
		}

		_logger.info("Active Stick zones ");
		g13s[0]->stick().dump(std::cout);

		std::string config_fn = string_config_value("config");
		if (config_fn.size()) {
			_logger.info("config_fn = " + config_fn);
			g13s[0]->read_config_file(config_fn);
		}

		do {
			if (g13s.size() > 0)
				for (int i = 0; i < g13s.size(); i++) {
					int status = g13s[i]->read_keys();
					g13s[i]->read_commands();
					if (status < 0)
						running = false;
				}
		}
		while (running);
		cleanup();

		return 0;
	}

	void G13_Manager::init_keynames() {

		int key_index = 0;

		// setup maps to let us convert between strings and G13 key names
		#define ADD_G13_KEY_MAPPING(r, data, elem)                             \
        {                                                                      \
            std::string name = BOOST_PP_STRINGIZE(elem);                       \
            g13_key_to_name[key_index] = name;                                 \
            g13_name_to_key[name] = key_index;                                 \
            key_index++;                                                       \
        }                                                                      \

		BOOST_PP_SEQ_FOR_EACH(ADD_G13_KEY_MAPPING, _, G13_KEY_SEQ)

		// setup maps to let us convert between strings and linux key names
		#define ADD_KB_KEY_MAPPING(r, data, elem)                              \
        {                                                                      \
            std::string name = BOOST_PP_STRINGIZE(elem);                       \
            int keyval = BOOST_PP_CAT( KEY_, elem );                           \
            input_key_to_name[keyval] = name;                                  \
            input_name_to_key[name] = keyval;                                  \
        }                                                                      \

		BOOST_PP_SEQ_FOR_EACH(ADD_KB_KEY_MAPPING, _, KB_INPUT_KEY_SEQ)
	}

	LINUX_KEY_VALUE G13_Manager::find_g13_key_value(const std::string& keyname) const {
		auto i = g13_name_to_key.find(keyname);
		if (i == g13_name_to_key.end()) {
			return BAD_KEY_VALUE;
		}
		return i->second;
	}

	LINUX_KEY_VALUE G13_Manager::find_input_key_value(const std::string& keyname) const {

		// if there is a KEY_ prefix, strip it off
		if (!strncmp(keyname.c_str(), "KEY_", 4)) {
			return find_input_key_value(keyname.c_str() + 4);
		}

		auto i = input_name_to_key.find(keyname);
		if (i == input_name_to_key.end()) {
			return BAD_KEY_VALUE;
		}
		return i->second;
	}

	std::string G13_Manager::find_input_key_name(LINUX_KEY_VALUE v) const {
		try {
			return find_or_throw(input_key_to_name, v);
		}
		catch (...) {
			return "(unknown linux key)";
		}
	}

	std::string G13_Manager::find_g13_key_name(G13_KEY_INDEX v) const {
		try {
			return find_or_throw(g13_key_to_name, v);
		}
		catch (...) {
			return "(unknown G13 key)";
		}
	}

	void G13_Manager::display_keys() {

		typedef std::map<std::string, int> mapType;
		G13_OUT("Known keys on G13:");
		G13_OUT(Helper::map_keys_out(g13_name_to_key));

		G13_OUT("Known keys to map to:");
		G13_OUT(Helper::map_keys_out(input_name_to_key));
	}

	void G13_Manager::set_log_level(::boost::log::trivial::severity_level lvl) {
		_logger.set_log_level(lvl);
	}

	void G13_Manager::set_log_level(const std::string& level) {
		_logger.set_log_level(level);
	}
}
