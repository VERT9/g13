//
// Created by vert9 on 11/23/23.
//

#include <fcntl.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <libusb-1.0/libusb.h>
#include <linux/uinput.h>
#include <sstream>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>


#include "container.h"
#include "g13.h"
#include "g13_action.h"
#include "g13_device.h"
#include "G13_DisplayApp.h"
#include "g13_fonts.h"
#include "g13_keys.h"
#include "g13_lcd.h"
#include "g13_log.h"
#include "g13_manager.h"
#include "g13_profile.h"
#include "g13_stick.h"
#include "logo.h"
#include "helper.h"

using Helper::repr;
using std::to_string;

namespace G13 {
	G13_Device::G13_Device(std::shared_ptr<G13_Log> logger, libusb_device_handle* handle, unsigned long _id) :
		_id_within_manager(_id),
		handle(handle),
		ctx(0),
		_uinput_fid(-1),
		_logger(std::move(logger)),
		_lcd(*this, *_logger) {
		_current_profile = std::make_shared<G13_Profile>("default", "default");
		_profiles["default"] = _current_profile;

		for (bool& key : keys)
			key = false;

		lcd().image_clear();

		_init_commands();

		key_buffer = new unsigned char[G13_REPORT_SIZE*2];
		transfer = nullptr;
	}

	G13_Device::~G13_Device() {
		delete[] key_buffer;
		libusb_free_transfer(transfer);
	}

	G13_Device& G13_Device::init() {
		_stick = std::make_shared<G13_Stick>(_logger);
		_init_apps();

		return *this;
	}

	int G13_Device::g13_create_uinput() {
		struct uinput_user_dev uinp;
		const char* dev_uinput_fname =
				access("/dev/input/uinput", F_OK) == 0 ? "/dev/input/uinput" :
				access("/dev/uinput", F_OK) == 0 ? "/dev/uinput" : 0;
		if (!dev_uinput_fname) {
			_logger->error("Could not find an uinput device");
			return -1;
		}
		if (access(dev_uinput_fname, W_OK) != 0) {
			_logger->error(std::to_string(*dev_uinput_fname) + " doesn't grant write permissions");
			return -1;
		}
		int ufile = open(dev_uinput_fname, O_WRONLY | O_NDELAY);
		if (ufile <= 0) {
			_logger->error("Could not open uinput");
			return -1;
		}
		memset(&uinp, 0, sizeof(uinp));
		char name[] = "G13";
		strncpy(uinp.name, name, sizeof(name));
		uinp.id.version = 1;
		uinp.id.bustype = BUS_USB;
		uinp.id.product = G13_PRODUCT_ID;
		uinp.id.vendor = G13_VENDOR_ID;
		uinp.absmin[ABS_X] = 0;
		uinp.absmin[ABS_Y] = 0;
		uinp.absmax[ABS_X] = 0xff;
		uinp.absmax[ABS_Y] = 0xff;
		//  uinp.absfuzz[ABS_X] = 4;
		//  uinp.absfuzz[ABS_Y] = 4;
		//  uinp.absflat[ABS_X] = 0x80;
		//  uinp.absflat[ABS_Y] = 0x80;

		ioctl(ufile, UI_SET_EVBIT, EV_KEY);
		ioctl(ufile, UI_SET_EVBIT, EV_ABS);
		/*  ioctl(ufile, UI_SET_EVBIT, EV_REL);*/
		ioctl(ufile, UI_SET_MSCBIT, MSC_SCAN);
		ioctl(ufile, UI_SET_ABSBIT, ABS_X);
		ioctl(ufile, UI_SET_ABSBIT, ABS_Y);
		/*  ioctl(ufile, UI_SET_RELBIT, REL_X);
		 ioctl(ufile, UI_SET_RELBIT, REL_Y);*/
		for (int i = 0; i < 256; i++)
			ioctl(ufile, UI_SET_KEYBIT, i);
		ioctl(ufile, UI_SET_KEYBIT, BTN_THUMB);

		int retcode = write(ufile, &uinp, sizeof(uinp));
		if (retcode < 0) {
			_logger->error("Could not write to uinput device (" + std::to_string(retcode) + ")");
			return -1;
		}
		retcode = ioctl(ufile, UI_DEV_CREATE);
		if (retcode) {
			_logger->error("Error creating uinput device for G13");
			return -1;
		}
		return ufile;
	}

	int G13_Device::g13_create_fifo(const char* fifo_name) {
		// Get just the path
		std::string path(fifo_name);
		const size_t pos = path.find_last_of("\\/");
		path = (std::string::npos == pos) ? "" : path.substr(0, pos);
		// Create the path if not empty
		if (!path.empty()) {
			std::filesystem::create_directories(path);
		}

		// mkfifo(g13->fifo_name(), 0777); - didn't work
		mkfifo(fifo_name, 0666);
		chmod(fifo_name, 0777);

		return open(fifo_name, O_RDWR | O_NONBLOCK);
	}

	std::string G13_Device::describe_libusb_error_code(int code) {
		switch (code) {
			case LIBUSB_SUCCESS: return "SUCCESS";
			case LIBUSB_ERROR_IO: return "ERROR_IO";
			case LIBUSB_ERROR_INVALID_PARAM: return "ERROR_INVALID_PARAM";
			case LIBUSB_ERROR_ACCESS: return "ERROR_ACCESS";
			case LIBUSB_ERROR_NO_DEVICE: return "ERROR_NO_DEVICE";
			case LIBUSB_ERROR_NOT_FOUND: return "ERROR_NOT_FOUND";
			case LIBUSB_ERROR_BUSY: return "ERROR_BUSY";
			case LIBUSB_ERROR_TIMEOUT: return "ERROR_TIMEOUT";
			case LIBUSB_ERROR_OVERFLOW: return "ERROR_OVERFLOW";
			case LIBUSB_ERROR_PIPE: return "ERROR_PIPE";
			case LIBUSB_ERROR_INTERRUPTED: return "ERROR_INTERRUPTED";
			case LIBUSB_ERROR_NO_MEM: return "ERROR_NO_MEM";
			case LIBUSB_ERROR_NOT_SUPPORTED: return "ERROR_NOT_SUPPORTED";
			case LIBUSB_ERROR_OTHER: return "ERROR_OTHER";
			default: return "unknown error";
		}
	}

	void G13_Device::init_lcd() {
		int error = libusb_control_transfer(handle, 0, 9, 1, 0, 0, 0, 1000);
		if (error) {
			_logger->error("Error when initializing lcd endpoint");
		}
	}

	void G13_Device::write_lcd(unsigned char* data, size_t size) {
		init_lcd();
		if (size != G13_LCD_BUFFER_SIZE) {
			_logger->error("Invalid LCD data size " + std::to_string(size) + ", should be " + std::to_string(G13_LCD_BUFFER_SIZE));
			return;
		}
		unsigned char buffer[G13_LCD_BUFFER_SIZE + 32];
		memset(buffer, 0, G13_LCD_BUFFER_SIZE + 32);
		buffer[0] = 0x03;
		memcpy(buffer + 32, data, G13_LCD_BUFFER_SIZE);
		int bytes_written;
		int error = libusb_interrupt_transfer(handle, LIBUSB_ENDPOINT_OUT | G13_LCD_ENDPOINT, buffer,
											  G13_LCD_BUFFER_SIZE + 32, &bytes_written, 1000);
		if (error)
			_logger->error("Error when transferring image: " + std::to_string(error) + ", " + std::to_string(bytes_written) + " bytes written");
	}

	void G13_Device::write_lcd_file(const string& filename) {
		filebuf* pbuf;
		ifstream filestr;
		size_t size;

		filestr.open(filename.c_str());
		pbuf = filestr.rdbuf();

		size = pbuf->pubseekoff(0, ios::end, ios::in);
		pbuf->pubseekpos(0, ios::in);

		char buffer[size];

		pbuf->sgetn(buffer, size);

		filestr.close();
		write_lcd((unsigned char*) buffer, size);
	}

	void G13_Device::parse_joystick(unsigned char* buf) {
		_stick->parse_joystick(*this, buf);
	}

	void G13_Device::send_event(int type, int code, int val) {
		memset(&_event, 0, sizeof(_event));
		gettimeofday(&_event.time, nullptr);
		_event.type = type;
		_event.code = code;
		_event.value = val;
		write(_uinput_fid, &_event, sizeof(_event));
	}

	void G13_Device::write_output_pipe(const std::string& out) {
		write(_output_pipe_fid, out.c_str(), out.size());
	}

	void G13_Device::set_mode_leds(int leds) {

		unsigned char usb_data[] = {5, 0, 0, 0, 0};
		usb_data[1] = leds;
		int r = libusb_control_transfer(handle,
										(uint8_t) LIBUSB_REQUEST_TYPE_CLASS | (uint8_t) LIBUSB_RECIPIENT_INTERFACE, 9, 0x305, 0,
										usb_data, 5, 1000);
		if (r != 5) {
			_logger->error("Problem sending data");
			return;
		}
	}

	void G13_Device::set_key_color(int red, int green, int blue) {
		int error;
		unsigned char usb_data[] = {5, 0, 0, 0, 0};
		usb_data[1] = red;
		usb_data[2] = green;
		usb_data[3] = blue;

		error = libusb_control_transfer(handle,
										(uint8_t) LIBUSB_REQUEST_TYPE_CLASS | (uint8_t) LIBUSB_RECIPIENT_INTERFACE, 9, 0x307, 0,
										usb_data, 5, 1000);
		if (error != 5) {
			_logger->error("Problem sending data");
			return;
		}
	}

	std::string G13_Device::make_pipe_name(G13_Manager& manager, bool is_input) {
		std::string config_base, direction;
		if (is_input) {
			config_base = manager.string_config_value("pipe_in");
			direction = "in";
		} else {
			config_base = manager.string_config_value("pipe_out");
			direction = "out";
		}

		// Set base directory if one is not supplied
		if (config_base.empty()) {
			return std::format("/tmp/g13/{}/{}", direction, id_within_manager());
		}

		return std::format("{}-{}-{}", config_base, id_within_manager(), direction);
	}

	void G13_Device::register_context(libusb_context* _ctx, G13_Manager& manager) {
		ctx = _ctx;

		int leds = 0;
		int red = 0;
		int green = 0;
		int blue = 255;
		init_lcd();

		set_mode_leds(leds);
		set_key_color(red, green, blue);

		write_lcd(g13_logo, sizeof(g13_logo));

		_uinput_fid = g13_create_uinput();

		_input_pipe_name = make_pipe_name(manager, true);
		_input_pipe_fid = g13_create_fifo(_input_pipe_name.c_str());
		_output_pipe_name = make_pipe_name(manager, false);
		_output_pipe_fid = g13_create_fifo(_output_pipe_name.c_str());

		if (_input_pipe_fid == -1) {
			_logger->error("failed opening input pipe");
		}
		if (_output_pipe_fid == -1) {
			_logger->error("failed opening output pipe");
		}
	}

	void G13_Device::cleanup() {
		remove(_input_pipe_name.c_str());
		remove(_output_pipe_name.c_str());
		ioctl(_uinput_fid, UI_DEV_DESTROY);
		close(_uinput_fid);
		libusb_release_interface(handle, 0);
		libusb_close(handle);
	}

	/**
	 * Handles the libusb transfer completion event.
	 *
	 * @param transfer
	 */
	void transfer_cb(struct libusb_transfer* transfer) {
		// Fetch the device from user_data
		auto* device = static_cast<G13_Device*>(transfer->user_data);
		switch(transfer->status) {
			case LIBUSB_TRANSFER_COMPLETED: {
				unsigned char* buffer = transfer->buffer;
				device->parse_joystick(buffer);
				device->current_profile().parse_keys(buffer, *device);
				device->send_event(EV_SYN, SYN_REPORT, 0);
				break;
			}
			// Ignoring these for now
			case LIBUSB_TRANSFER_ERROR:
			case LIBUSB_TRANSFER_TIMED_OUT:
			case LIBUSB_TRANSFER_CANCELLED:
			case LIBUSB_TRANSFER_STALL:
			case LIBUSB_TRANSFER_NO_DEVICE:
			case LIBUSB_TRANSFER_OVERFLOW:
				break;
		}

		// Resubmit transfer for next update
		int error = libusb_submit_transfer(transfer);
		if (error && error != LIBUSB_ERROR_TIMEOUT) {
			device->logger()->error("Error while reading keys (during resubmit): " + std::to_string(error) + " ("
								   + device->describe_libusb_error_code(error) + ")");
		}
	}

	/*!
	 * creates, fills and submits the initial data transfer for the device
	 * @see https://libusb.sourceforge.io/api-1.0/group__libusb__asyncio.html#details
	 */
	int G13_Device::read_keys() {
		// Return if transfer has already been allocated
		if (transfer != nullptr)
			return 0;

		transfer = libusb_alloc_transfer(0);
		// pass the current device (this) along as user_data, so we can manipulate it later
		libusb_fill_interrupt_transfer(transfer,
									   handle,
									   LIBUSB_ENDPOINT_IN | G13_KEY_ENDPOINT,
									   this->key_buffer,
									   G13_REPORT_SIZE,
									   transfer_cb,
									   this,
									   100);

		int error = libusb_submit_transfer(transfer);
		if (error && error != LIBUSB_ERROR_TIMEOUT) {
			_logger->error("Error while reading keys: " + std::to_string(error) + " ("
														+ describe_libusb_error_code(error) + ")");
			libusb_free_transfer(transfer);
			return -1;
		}

		return 0;
	}

	void G13_Device::read_config_file(const std::string& filename) {
		std::ifstream s(filename);

		_logger->info("reading configuration from " + filename);
		while (s.good()) {

			// grab a line
			char buf[1024];
			buf[0] = 0;
			buf[sizeof(buf) - 1] = 0;
			s.getline(buf, sizeof(buf) - 1);

			// strip comment
			char* comment = strchr(buf, '#');
			if (comment) {
				comment--;
				while (comment > buf && isspace(*comment)) { comment--; }
				*comment = 0;
			}

			// send it
			if (buf[0]) {
				string cfg(buf);
				_logger->info("  cfg: " + cfg);
				command(buf);
			}
		}
	}

	void G13_Device::read_commands() {
		fd_set set;
		FD_ZERO(&set);
		FD_SET(_input_pipe_fid, &set);
		struct timeval tv;
		tv.tv_sec = 0;
		tv.tv_usec = 0;
		int ret = select(_input_pipe_fid + 1, &set, 0, 0, &tv);
		if (ret > 0) {
			unsigned char buf[1024 * 1024];
			memset(buf, 0, 1024 * 1024);
			ret = read(_input_pipe_fid, buf, 1024 * 1024);
			_logger->trace("read " + std::to_string(ret) + " characters");

			if (ret == 960) { // TODO probably image, for now, don't test, just assume image
				lcd().image(buf, ret);
			} else {
				std::string buffer = reinterpret_cast<const char*>(buf);
				std::stringstream lines(buffer);
				std::string line;
				while (getline(lines, line, '\n')) {
					std::string cmd = line.substr(0, line.find('#'));
					_logger->info("command: " + cmd);
					command(cmd.c_str());
				}
			}
		}
	}

	FontPtr G13_Device::switch_to_font(const std::string& name) {
		return lcd().switch_to_font(name);
	}

	void G13_Device::switch_to_profile(const std::string& name) {
		if (_current_profile->name() != name){
			_current_profile = profile(name);
			_logger->info("Profile switched to: " + name);
		}
	}

	ProfilePtr G13_Device::profile(const std::string& id, const std::string& name) {
		ProfilePtr rv = _profiles[id];
		if (!rv) {
			rv = std::make_shared<G13_Profile>(*_current_profile, id, name.empty() ? id : name);
			_profiles[id] = rv;
		}
		return rv;
	}

	void G13_Device::dump(std::ostream& o, int detail) {
		o << "G13 id=" << id_within_manager() << endl;
		o << "   input_pipe_name=" << repr(_input_pipe_name) << endl;
		o << "   output_pipe_name=" << repr(_output_pipe_name) << endl;
		o << "   current_profile=" << _current_profile->name() << endl;
		o << "   current_font=" << lcd().current_font().name() << std::endl;

		if (detail > 0) {
			o << "STICK" << std::endl;
			stick().dump(o);
			if (detail == 1) {
				_current_profile->dump(o);
			} else {
				for (auto i = _profiles.begin(); i != _profiles.end(); i++) {
					i->second->dump(o);
				}
			}
		}
	}

	void G13_Device::_init_commands() {
		using Helper::advance_ws;

		_command_table["out"] = [this](const char* remainder) {
			lcd().write_string(remainder);
		};

		_command_table["pos"] = [this](const char* remainder) {
			int row, col;
			if (sscanf(remainder, "%i %i", &row, &col) == 2) {
				lcd().write_pos(row, col);
			} else {
				return _logger->error("bad pos : " + std::string(remainder));
			}
		};

		_command_table["bind"] = [this](const char* remainder) {
			std::string keyname;
			advance_ws(remainder, keyname);
			std::string action = remainder;
			try {
				if (auto key = _current_profile->find_key(keyname)) {
					vector<std::string> excluded{"BD", "L1", "L2", "L3", "L4"};
					if (ranges::find(excluded, keyname) == excluded.end())
						key->set_action(make_action(action));
				} else if (auto stick_key = _stick->zone(keyname)) {
					stick_key->set_action(make_action(action));
				} else {
					return _logger->error("bind key " + keyname + " unknown");
				}
				_logger->debug("bind " + keyname + " [" + action + "]");
			} catch (const std::exception& ex) {
				return _logger->error("bind " + keyname + " " + action + " failed: " + ex.what());
			}
		};

		_command_table["profile"] = [this](const char* remainder) {
			switch_to_profile(remainder);
		};

		_command_table["font"] = [this](const char* remainder) {
			switch_to_font(remainder);
		};

		_command_table["mod"] = [this](const char* remainder) {
			set_mode_leds(atoi(remainder));
		};

		_command_table["textmode"] = [this](const char* remainder) {
			lcd().text_mode = atoi(remainder);
		};

		_command_table["rgb"] = [this](const char* remainder) {
			int red, green, blue;
			if (sscanf(remainder, "%i %i %i", &red, &green, &blue) == 3) {
				set_key_color(red, green, blue);
			} else {
				return _logger->error("rgb bad format: <" + std::string(remainder) + ">");
			}
		};

		_command_table["stickmode"] = [this](const char* remainder) {
			std::string mode = remainder;

			if (mode == "ABSOLUTE") { return _stick->set_mode(STICK_ABSOLUTE); }
			if (mode == "RELATIVE") { return _stick->set_mode(STICK_RELATIVE); }
			if (mode == "KEYS") { return _stick->set_mode(STICK_KEYS); }
			if (mode == "CALCENTER") { return _stick->set_mode(STICK_CALCENTER); }
			if (mode == "CALBOUNDS") { return _stick->set_mode(STICK_CALBOUNDS); }
			if (mode == "CALNORTH") { return _stick->set_mode(STICK_CALNORTH); }

			return _logger->error("unknown stick mode : <" + mode + ">");
		};

		_command_table["stickzone"] = [this](const char* remainder) {
			std::string operation, zonename;
			advance_ws(remainder, operation);
			advance_ws(remainder, zonename);
			if (operation == "add") {
				G13_StickZone* zone = _stick->zone(zonename, true);
			} else {
				G13_StickZone* zone = _stick->zone(zonename);
				if (!zone) {
					throw G13_CommandException("unknown stick zone");
				}
				if (operation == "action") {
					zone->set_action(make_action(remainder));
				} else if (operation == "bounds") {
					double x1, y1, x2, y2;
					if (sscanf(remainder, "%lf %lf %lf %lf", &x1, &y1, &x2, &y2) != 4) {
						throw G13_CommandException("bad bounds format");
					}
					zone->set_bounds(G13_ZoneBounds(x1, y1, x2, y2));
				} else if (operation == "del") {
					_stick->remove_zone(*zone);
				} else {
					return _logger->error("unknown stickzone operation: <" + operation + ">");
				}
			}
		};

		_command_table["dump"] = [this](const char* remainder) {
			std::string target;
			advance_ws(remainder, target);
			if (target == "all") {
				dump(std::cout, 3);
			} else if (target == "current") {
				dump(std::cout, 1);
			} else if (target == "summary") {
				dump(std::cout, 0);
			} else {
				return _logger->error("unknown dump target: <" + target + ">");
			}
		};

		_command_table["log_level"] = [this](const char* remainder) {
			std::string level;
			advance_ws(remainder, level);
			_logger->set_log_level(level);
		};

		_command_table["refresh"] = [this](const char* remainder) {
			lcd().image_send();
		};

		_command_table["clear"] = [this](const char* remainder) {
			lcd().image_clear();
			lcd().image_send();
		};

		/* TODO add more commands
		 * New command template:
			_command_table[""] = [this](const char *remainder) {
			};
		 */
	}

	void G13_Device::command(char const* str) {
		const char* remainder = str;

		try {
			using Helper::advance_ws;

			std::string cmd;
			advance_ws(remainder, cmd);

			// Ignore comments
			if (cmd.starts_with("#") || cmd.starts_with("//"))
				return;

			auto i = _command_table.find(cmd);
			if (i == _command_table.end()) {
				return _logger->error("unknown command : " + cmd);
			}
			COMMAND_FUNCTION f = i->second;
			f(remainder);
		} catch (const std::exception& ex) {
			return _logger->error("command failed : " + std::string(ex.what()));
		}
	}

	G13_ActionPtr G13_Device::make_action(const std::string& action) {
		if (!action.size()) {
			throw G13_CommandException("empty action string");
		}

		if (action[0] == '>') {
			return Container::Instance().Resolve<G13_Action_PipeOut>(action.substr(1));
		}

		if (action[0] == '!') {
			return Container::Instance().Resolve<G13_Action_Command>(action.substr(1));
		}

		return Container::Instance().Resolve<G13_Action_Keys>(action);
	}

	inline bool G13_Device::is_set(int key) {
		return keys[key];
	}

	bool G13_Device::update(int key, bool v) {
		bool old = keys[key];
		keys[key] = v;
		return old != v;
	}

	void G13_Device::_init_apps() {
		// Bind BD key to switch apps
		if (auto gkey = current_profile().find_key("BD")) {
			gkey->set_action(Container::Instance().Resolve<G13_Action_AppChange>(this));
		}

		// Default time/profile display
		this->_apps.emplace_back(Container::Instance().Resolve<G13_CurrentProfileApp>());
		// Scrollable list allowing for selecting active profile
		this->_apps.emplace_back(Container::Instance().Resolve<G13_ProfileSwitcherApp>());
		// Just a test app to highlight limits of the display
		//this->_apps.emplace_back(Container::Instance().Resolve<G13_TesterApp>());

		// Call init of the app
		this->_apps[this->current_app]->init(*this);
	}

	unsigned int G13_Device::get_current_app() const {
		return this->current_app;
	}

	void G13_Device::next_app() {
		this->current_app = ++this->current_app % this->_apps.size();
		this->_apps[this->current_app]->init(*this);
	}

	void G13_Device::display_app() {
		this->_apps[this->current_app]->display(*this);
	}
}