//
// Created by vert9 on 11/23/23.
//

#ifndef G13_G13_MANAGER_H
#define G13_G13_MANAGER_H

#include <map>
#include <memory>
#include <string>
#include <vector>

#include <libusb-1.0/libusb.h>

namespace G13 {
	// Forward declarations
	class G13_Device;
	class G13_KeyMap;
	class G13_Log;

	/*!
	 * top level class, holds what would otherwise be in global variables
	 */
	class G13_Manager {
	public:
		G13_Manager(std::shared_ptr<G13_Log> logger, std::shared_ptr<G13_KeyMap> keymap);

		void set_logo(const std::string& fn) { logo_filename = fn; }

		int run();

		std::string string_config_value(const std::string& name) const;
		void set_string_config_value(const std::string& name, const std::string& val);

	protected:
		void init_keynames();
		void display_keys();
		void init_profiles();
		void load_profile(G13_Device* d, const std::string& filename);
		void discover_g13s(libusb_device** devs, ssize_t count, std::vector<G13_Device*>& g13s);
		void cleanup();

		std::shared_ptr<G13_Log> _logger;
		std::shared_ptr<G13_KeyMap> _keymap;

		std::string logo_filename;
		libusb_device** devs;
		libusb_context* ctx;
		std::vector<G13_Device*> g13s;

		std::map<std::string, std::string> _string_config_values;

		static bool running;
		static void set_stop(int);
	};
}
#endif //G13_G13_MANAGER_H
