//
// Created by vert9 on 11/23/23.
//

#ifndef G13_G13_DEVICE_H
#define G13_G13_DEVICE_H

#include <ostream>
#include <string>
#include <map>
#include <libusb-1.0/libusb.h>
#include <linux/uinput.h>
#include <boost/shared_ptr.hpp>
#include <boost/function.hpp>
#include "g13_lcd.h"
#include "g13_stick.h"

namespace G13 {
	class G13_Profile;
	class G13_Action;
	class G13_Manager;
	class G13_Font;

	typedef boost::shared_ptr<G13_Action> G13_ActionPtr;
	typedef boost::shared_ptr<G13_Font> FontPtr;
	typedef boost::shared_ptr<G13_Profile> ProfilePtr;

	const size_t G13_INTERFACE = 0;
	const size_t G13_KEY_ENDPOINT = 1;
	const size_t G13_LCD_ENDPOINT = 2;
	const size_t G13_KEY_READ_TIMEOUT = 0;
	const size_t G13_VENDOR_ID = 0x046d;
	const size_t G13_PRODUCT_ID = 0xc21c;
	const size_t G13_REPORT_SIZE = 8;
	const size_t G13_LCD_BUFFER_SIZE = 0x3c0;
	const size_t G13_NUM_KEYS = 40;

	class G13_Device {
	public:
		G13_Device(G13_Manager& manager, G13_Log& logger, libusb_device_handle* handle, int id);
		~G13_Device();

		G13_Manager& manager() { return _manager; }
		const G13_Manager& manager() const { return _manager; }

		G13_LCD& lcd() { return _lcd; }
		const G13_LCD& lcd() const { return _lcd; }

		G13_Stick& stick() { return _stick; }
		const G13_Stick& stick() const { return _stick; }

		FontPtr switch_to_font(const std::string& name);
		void switch_to_profile(const std::string& name);
		ProfilePtr profile(const std::string& id, const std::string& name = "");

		void dump(std::ostream&, int detail = 0);
		void command(char const* str);

		void read_commands();
		void read_config_file(const std::string& filename);

		int read_keys();
		void parse_joystick(unsigned char* buf);

		G13_ActionPtr make_action(const std::string&);

		void set_key_color(int red, int green, int blue);
		void set_mode_leds(int leds);

		void send_event(int type, int code, int val);
		void write_output_pipe(const std::string& out);

		void write_lcd(unsigned char* data, size_t size);

		bool is_set(int key);
		bool update(int key, bool v);

		// used by G13_Manager
		void cleanup();
		void register_context(libusb_context* ctx);
		void write_lcd_file(const std::string& filename);

		G13_Font& current_font() { return *_current_font; }

		G13_Profile& current_profile() { return *_current_profile; }

		int id_within_manager() const { return _id_within_manager; }

		typedef boost::function<void(const char*)> COMMAND_FUNCTION;
		typedef std::map<std::string, COMMAND_FUNCTION> CommandFunctionTable;

	protected:
		void _init_fonts();
		void init_lcd();
		void _init_commands();

		CommandFunctionTable _command_table;

		struct input_event _event {};

		int _id_within_manager;
		libusb_device_handle* handle;
		libusb_context* ctx;

		int _uinput_fid;

		int _input_pipe_fid;
		std::string _input_pipe_name;
		int _output_pipe_fid;
		std::string _output_pipe_name;

		std::map<std::string, FontPtr> _fonts;
		FontPtr _current_font;
		std::map<std::string, ProfilePtr> _profiles;
		ProfilePtr _current_profile;

		G13_Manager& _manager;
		G13_Log& _logger;
		G13_LCD _lcd;
		G13_Stick _stick;

		bool keys[G13_NUM_KEYS];
	private:
		int g13_create_uinput();
		int g13_create_fifo(const char* fifo_name);
		std::string describe_libusb_error_code(int code);
	};
}

#endif //G13_G13_DEVICE_H
