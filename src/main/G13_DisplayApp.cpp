//
// Created by vert9 on 11/23/24.
//

#include "G13_DisplayApp.h"
#include "g13_profile.h"

namespace G13 {
	void G13_DisplayApp::init(G13_Device& device) {
		// Disable LIGHT keys in case they've been assigned in other apps
		for (int i = 1; i <= 4; i++) {
			const auto gkey = device.current_profile().find_key("L"+std::to_string(i));
			gkey->set_action(std::make_shared<G13_Action_Dynamic>(device, *_logger, [&] {
				// Do Nothing
			}));
		}
	}

	void G13_CurrentProfileApp::display(G13_Device& device) {
		// Clear screen first
		device.lcd().image_clear();

		// Write current profile name to screen
		device.lcd().write_pos(0, 0);
		std::string profile_name = device.current_profile().name();
		unsigned int profile_length = profile_name.length();
		if (profile_length > 32) {
			// Output 32 chars from a start point
			device.lcd().write_string(profile_name.substr(name_start, 32).c_str(), false);

			// Handle direction change
			if (name_start == 0) {
				direction = 1; // forward
			} else if (name_start + 32 == profile_length - 1) {
				direction = -1; // reverse
			}

			// Only update every 500ms
			const auto elapsed = std::chrono::high_resolution_clock::now() - last_update;
			auto elapsed_in_ms = std::chrono::duration_cast<chrono::milliseconds>(elapsed).count();
			if (elapsed_in_ms > 500) {
				name_start += direction;
				last_update = std::chrono::high_resolution_clock::now();
			}
		} else {
			device.lcd().write_string(profile_name.c_str(), false);
		}

		// Write current date/time to screen
		std::time_t t = std::time(nullptr);
		char mbstr[32];
		std::strftime(mbstr, sizeof(mbstr), "%F %I:%M:%S", std::localtime(&t));
		device.lcd().write_pos(3, 0);
		device.lcd().write_string(mbstr, false);

		// Stick position debug
		char stick[32];
		snprintf(stick, 32, "x:%3d, y:%3d, dx:%.2f, dy:%.2f", device.stick().getCurrentPos().x, device.stick().getCurrentPos().y, device.stick().getDX(), device.stick().getDY());
		device.lcd().write_pos(4, 0);
		device.lcd().write_string(stick, false);

		// Send image to screen
		device.lcd().image_send();
	}

	void G13_ProfileSwitcherApp::init(G13_Device& device) {
		// L1: Decrease index, move up the list
		if (auto gkey = device.current_profile().find_key("L1")) {
			gkey->set_action(std::make_shared<G13_Action_Dynamic>(device, *_logger, [&] {
				if (selected_profile > 0) {
					selected_profile--;
					if (selected_profile - profile_display_start == 0 && profile_display_start != 0)
						profile_display_start--;
				} else {
					selected_profile = device.get_profiles().size() - 1;
					profile_display_start = device.get_profiles().size() - 4;
				}
			}));
		}

		// L2: Increase index, move down the list
		if (auto gkey = device.current_profile().find_key("L2")) {
			gkey->set_action(std::make_shared<G13_Action_Dynamic>(device, *_logger, [&] {
				if (const unsigned long size = device.get_profiles().size(); selected_profile < size - 1) {
					selected_profile++;
					if (selected_profile - profile_display_start > 3)
						profile_display_start++;
				} else {
					selected_profile = 0;
					profile_display_start = 0;
				}
			}));
		}

		// Do nothing for L3
		if (auto gkey = device.current_profile().find_key("L3")) {
			gkey->set_action(std::make_shared<G13_Action_Dynamic>(device, *_logger, [&] {
				// Do Nothing
			}));
		}

		// L4: Select profile, make active
		if (auto gkey = device.current_profile().find_key("L4")) {
			gkey->set_action(std::make_shared<G13_Action_Dynamic>(device, *_logger, [&] {
				// Get list of profiles, formatted in indexable vector
				std::map<std::string, ProfilePtr> profiles = device.get_profiles();
				std::vector<std::pair<std::string, ProfilePtr>> p(profiles.begin(), profiles.end());
				// Switch profile
				device.switch_to_profile(p[selected_profile].first);
				// Reinit app to reapply LIGHT keys
				init(device);
			}));
		}
	}

	void G13_ProfileSwitcherApp::display(G13_Device& device) {
		// Clear screen first
		device.lcd().image_clear();

		// Save text_mode so it can be reset later
		int text_mode = device.lcd().text_mode;

		// List profile names on screen (only 4 rows available)
		device.lcd().text_mode = 0;
		std::map<std::string, ProfilePtr> profiles = device.get_profiles();
		std::vector<std::pair<std::string, ProfilePtr>> p(profiles.begin(), profiles.end());
		for (int i = 0; i < 4; i++) {
			device.lcd().write_pos(i, 0);
			char profile_name[32];
			const bool active = device.current_profile().guid() == p[profile_display_start + i].first;
			device.lcd().text_mode = static_cast<int>(i == selected_profile-profile_display_start);
			snprintf(profile_name, 32, "%s %-32s", active ? "*" : " ", p[profile_display_start + i].second->name().c_str());
			device.lcd().write_string(profile_name, false);
		}

		// Show button prompts
		device.lcd().text_mode = 1;
		device.lcd().write_pos(4, 0);
		device.lcd().write_string("   UP   |  DOWN  |      |   *   ", false);

		// Send image to screen
		device.lcd().image_send();

		// Reset text mode
		device.lcd().text_mode = text_mode;
	}

	void G13_TesterApp::display(G13_Device& device) {
		// Clear screen first
		device.lcd().image_clear();

		// Screen is 5 rows x 32 col with small font
		device.lcd().write_pos(0, 0);
		device.lcd().write_string("01234567890123456789012345678901", false);
		device.lcd().write_pos(1, 0);
		device.lcd().write_string("abcdefghijklmnopqrstuvwxyzabcdef", false);
		device.lcd().write_pos(2, 0);
		device.lcd().write_string("01234567890123456789012345678901", false);
		device.lcd().write_pos(3, 0);
		device.lcd().write_string("abcdefghijklmnopqrstuvwxyzabcdef", false);
		device.lcd().write_pos(4, 0);
		device.lcd().write_string("01234567890123456789012345678901");
	}
}
