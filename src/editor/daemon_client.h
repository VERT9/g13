#ifndef G13_EDITOR_DAEMON_CLIENT_H
#define G13_EDITOR_DAEMON_CLIENT_H

#include <string>

namespace G13::Editor {
	/**
	 * Sends profile-control commands to the running g13 daemon FIFO.
	 */
	class DaemonClient {
	public:
		/**
		 * Path to the daemon input pipe used for editor commands.
		 */
		std::string pipe_path;

		/**
		 * Creates a client using the default daemon pipe under XDG_RUNTIME_DIR.
		 */
		DaemonClient();

		/**
		 * Requests a full profile directory reload from the daemon.
		 * @param error receives a human-readable failure reason.
		 * @return true when the command was written to the daemon FIFO.
		 */
		bool send_reload_profiles(std::string& error) const;

		/**
		 * Requests a reload of one profile XML by GUID.
		 * @param guid profile GUID to reload.
		 * @param error receives a human-readable failure reason.
		 * @return true when the command was written to the daemon FIFO.
		 */
		bool send_reload_profile(const std::string& guid, std::string& error) const;

		/**
		 * Requests activation of one profile by GUID.
		 * @param guid profile GUID to activate.
		 * @param error receives a human-readable failure reason.
		 * @return true when the command was written to the daemon FIFO.
		 */
		bool send_activate_profile(const std::string& guid, std::string& error) const;

	private:
		/**
		 * Writes a raw daemon command to the configured FIFO.
		 * @param command daemon command text, including its trailing newline.
		 * @param error receives a human-readable failure reason.
		 * @return true when the complete command was written.
		 */
		bool send_command(const std::string& command, std::string& error) const;
	};
}

#endif
