//
// Created by vert9 on 11/23/23.
//

#ifndef G13_G13_DEVICE_H
#define G13_G13_DEVICE_H

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include <libusb-1.0/libusb.h>
#include <linux/uinput.h>

#include "g13_lcd.h"

namespace G13 {
	// Forward declarations
	class G13_Action;
	class G13_DisplayApp;
	class G13_Font;
	class G13_Log;
	class G13_LCD;
	class G13_Manager;
	class G13_Profile;
	class G13_Stick;

	typedef std::shared_ptr<G13_Action> G13_ActionPtr;
	typedef std::shared_ptr<G13_Font> FontPtr;
	typedef std::shared_ptr<G13_Profile> ProfilePtr;

	const size_t G13_INTERFACE = 0;
	const size_t G13_KEY_ENDPOINT = 1;
	const size_t G13_LCD_ENDPOINT = 2;
	const size_t G13_KEY_READ_TIMEOUT = 0;
	const size_t G13_VENDOR_ID = 0x046d;
	const size_t G13_PRODUCT_ID = 0xc21c;
	const size_t G13_REPORT_SIZE = 8;
	const size_t G13_LCD_BUFFER_SIZE = 0x3c0;
	const size_t G13_NUM_KEYS = 40;

	/**
	 * @brief Handles completion of an asynchronous libusb key transfer.
	 * @param transfer completed libusb transfer.
	 */
	void transfer_cb(libusb_transfer* transfer);

	/**
	 * @brief Runtime representation of one connected Logitech G13 device.
	 */
	class G13_Device {
	public:
		/**
		 * @brief Creates a device wrapper around an open libusb handle.
		 * @param logger logger used for device diagnostics.
		 * @param handle open libusb device handle.
		 * @param id device index assigned by the manager.
		 * @param profiles_dir directory containing profile XML files.
		 */
		G13_Device(std::shared_ptr<G13_Log> logger, libusb_device_handle* handle, unsigned long id, std::string profiles_dir);

		/**
		 * @brief Releases device resources owned by this wrapper.
		 */
		~G13_Device();

		/**
		 * @brief Initializes input, LCD, command, app, and profile subsystems.
		 * @return this device after initialization.
		 */
		G13_Device& init();

		/**
		 * @brief Gets the mutable LCD controller.
		 * @return mutable LCD controller.
		 */
		G13_LCD& lcd() { return _lcd; }

		/**
		 * @brief Gets the immutable LCD controller.
		 * @return immutable LCD controller.
		 */
		const G13_LCD& lcd() const { return _lcd; }

		/**
		 * @brief Gets the mutable joystick controller.
		 * @return mutable joystick controller.
		 */
		G13_Stick& stick() { return *_stick; }

		/**
		 * @brief Gets the immutable joystick controller.
		 * @return immutable joystick controller.
		 */
		const G13_Stick& stick() const { return *_stick; }

		/**
		 * @brief Gets the device logger.
		 * @return shared logger instance.
		 */
		std::shared_ptr<G13_Log> logger() { return _logger; }

		/**
		 * @brief Gets the device logger.
		 * @return shared logger instance.
		 */
		const std::shared_ptr<G13_Log> logger() const { return _logger; }

		/**
		 * @brief Switches the LCD renderer to a named font.
		 * @param name font name to activate.
		 * @return selected font, or nullptr when the font is unavailable.
		 */
		FontPtr switch_to_font(const std::string& name);

		/**
		 * @brief Switches the active profile by profile id.
		 * @param name profile id to activate.
		 */
		void switch_to_profile(const std::string& name);

		/**
		 * @brief Finds or creates a profile entry.
		 * @param id profile id.
		 * @param name profile display name to use when creating a new entry.
		 * @return profile shared pointer.
		 */
		ProfilePtr profile(const std::string& id, const std::string& name = "");

		/**
		 * @brief Writes device diagnostic state to a stream.
		 * @param output stream that receives the diagnostic dump.
		 * @param detail verbosity level.
		 */
		void dump(std::ostream& output, int detail = 0);

		/**
		 * @brief Executes one text command.
		 * @param str command string to parse and execute.
		 */
		void command(char const* str);

		/**
		 * @brief Reads pending commands from the input FIFO.
		 */
		void read_commands();

		/**
		 * @brief Reads and applies a device configuration file.
		 * @param filename configuration file path.
		 */
		void read_config_file(const std::string& filename);

		/**
		 * @brief Reads one key report from the device.
		 * @return libusb submission or read status code.
		 */
		int read_keys();

		/**
		 * @brief Parses joystick state from a raw G13 key report.
		 * @param buf raw key report buffer.
		 */
		void parse_joystick(unsigned char* buf);

		/**
		 * @brief Creates an action object from a textual action description.
		 * @param action textual action description.
		 * @return created action, or nullptr when parsing fails.
		 */
		G13_ActionPtr make_action(const std::string& action);

		/**
		 * @brief Sets the G-key backlight color.
		 * @param red red channel value.
		 * @param green green channel value.
		 * @param blue blue channel value.
		 */
		void set_key_color(int red, int green, int blue);

		/**
		 * @brief Sets the hardware mode LEDs.
		 * @param leds bitmask of mode LEDs to enable.
		 */
		void set_mode_leds(int leds);

		/**
		 * @brief Sends one Linux input event through uinput.
		 * @param type input event type.
		 * @param code input event code.
		 * @param val input event value.
		 */
		void send_event(int type, int code, int val);

		/**
		 * @brief Writes text to the device output FIFO.
		 * @param out text to write.
		 */
		void write_output_pipe(const std::string& out);

		/**
		 * @brief Writes an LCD framebuffer to the hardware.
		 * @param data LCD data buffer.
		 * @param size number of bytes to write.
		 */
		void write_lcd(unsigned char* data, size_t size);

		/**
		 * @brief Checks whether a key is currently pressed.
		 * @param key key index to inspect.
		 * @return true when the key is pressed.
		 */
		bool is_set(int key);

		/**
		 * @brief Updates cached key state.
		 * @param key key index to update.
		 * @param v new pressed state.
		 * @return true when the key state changed.
		 */
		bool update(int key, bool v);

		// used by G13_Manager
		/**
		 * @brief Closes device file descriptors and releases libusb resources.
		 */
		void cleanup();

		/**
		 * @brief Builds a FIFO path for this device.
		 * @param manager manager that owns the runtime pipe directory.
		 * @param is_input true for the input FIFO, false for the output FIFO.
		 * @return FIFO path for this device.
		 */
		std::string make_pipe_name(G13_Manager& manager, bool is_input);

		/**
		 * @brief Registers the libusb context and creates manager-owned FIFOs.
		 * @param ctx libusb context used by this device.
		 * @param manager manager that owns runtime paths.
		 */
		void register_context(libusb_context* ctx, G13_Manager& manager);

		/**
		 * @brief Writes an image file to the LCD.
		 * @param filename image file path to render.
		 */
		void write_lcd_file(const std::string& filename);

		/**
		 * @brief Gets the active profile.
		 * @return active profile.
		 */
		G13_Profile& current_profile() { return *_current_profile; }

		/**
		 * @brief Gets the loaded profile map.
		 * @return loaded profiles indexed by profile id.
		 */
		std::map<std::string, ProfilePtr> get_profiles() { return _profiles; }

		/**
		 * @brief Loads all profile XML files in the currently configured profile directory
		 */
		void init_profiles();

		/**
		 * @brief Loads the specified XML profile file. Duplicate profile IDs will be overwritten.
		 * @param filename file path to the XML profile to load
		 */
		void load_profile(const std::string& filename);

		/**
		 * @brief Reloads the specified XML profile file. If no filepath is specified, all profiles will be reloaded.
		 * @param filename file path to the XML profile to load, or null
		 */
		void reload_profile(const std::string& filename = nullptr);

		/**
		 * @brief Gets this device's manager-local id.
		 * @return manager-local device id.
		 */
		int id_within_manager() const { return _id_within_manager; }

		typedef std::function<void(const char*)> COMMAND_FUNCTION;
		typedef std::map<std::string, COMMAND_FUNCTION> CommandFunctionTable;

		/**
		 * @brief Converts a libusb error code to a human-readable string.
		 * @param code libusb status or error code.
		 * @return human-readable libusb result text.
		 */
		std::string describe_libusb_error_code(int code);

		/**
		 * @brief Displays the currently active app on the LCD screen
		 */
		void display_app();

		/**
		 * @brief Gets the index of the currently displayed app
		 * @return the index of the currently displayed app
		 */
		unsigned int get_current_app() const;

		/**
		 * @brief Cycles to the next app in rotation
		 */
		void next_app();

	protected:
		/**
		 * @brief Initializes the LCD subsystem.
		 */
		void init_lcd();

		/**
		 * @brief Registers built-in text commands.
		 */
		void _init_commands();

		/**
		 * @brief Loads and initializes all available apps for the current device
		 */
		void _init_apps();

		CommandFunctionTable _command_table;

		struct input_event _event {};

		unsigned long _id_within_manager;
		libusb_device_handle* handle;
		libusb_context* ctx;

		int _uinput_fid;

		int _input_pipe_fid;
		std::string _input_pipe_name;
		int _output_pipe_fid;
		std::string _output_pipe_name;

		std::map<std::string, ProfilePtr> _profiles;
		ProfilePtr _current_profile;

		std::shared_ptr<G13_Log> _logger;
		G13_LCD _lcd;
		std::shared_ptr<G13_Stick> _stick;
		std::string _profiles_dir;

		bool keys[G13_NUM_KEYS];

		unsigned char* key_buffer;
		libusb_transfer* transfer;

		/**
		 * @brief Tracks the index of the currently active DisplayApp
		 */
		unsigned int current_app = 0;

		/**
		 * @brief Stores the list of available DisplayApps
		 */
		std::vector<std::shared_ptr<G13_DisplayApp>> _apps;
	private:
		/**
		 * @brief Creates and configures the uinput device.
		 * @return file descriptor for the uinput device, or a negative value on failure.
		 */
		int g13_create_uinput();

		/**
		 * @brief Creates a FIFO if needed and opens it.
		 * @param fifo_name FIFO path to create and open.
		 * @return opened FIFO file descriptor, or a negative value on failure.
		 */
		int g13_create_fifo(const char* fifo_name);
	};
}

#endif //G13_G13_DEVICE_H
