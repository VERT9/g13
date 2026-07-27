#include "daemon_client.h"

#include <cerrno>
#include <cstring>
#include <cstdlib>
#include <filesystem>

#include <fcntl.h>
#include <unistd.h>

namespace G13::Editor {
	DaemonClient::DaemonClient() {
		if (const char* runtime_dir = std::getenv("XDG_RUNTIME_DIR")) {
			pipe_path = std::string(runtime_dir) + "/g13/in/0";
		} else {
			pipe_path = "/tmp/g13/in/0";
		}
	}

	bool DaemonClient::send_reload_profiles(std::string& error) const {
		return send_command("reload_profile\n", error);
	}

	bool DaemonClient::send_reload_profile(const std::string& guid, std::string& error) const {
		return send_command("reload_profile " + guid + "\n", error);
	}

	bool DaemonClient::send_activate_profile(const std::string& guid, std::string& error) const {
		return send_command("profile " + guid + "\n", error);
	}

	bool DaemonClient::send_command(const std::string& command, std::string& error) const {
		if (!std::filesystem::exists(pipe_path)) {
			error = "Daemon input pipe does not exist: " + pipe_path;
			return false;
		}

		const int fd = open(pipe_path.c_str(), O_WRONLY | O_NONBLOCK);
		if (fd == -1) {
			error = "Failed to open daemon input pipe: " + pipe_path + " (" + std::strerror(errno) + ")";
			return false;
		}

		const auto written = write(fd, command.c_str(), command.size());
		close(fd);
		if (written < 0 || static_cast<size_t>(written) != command.size()) {
			error = "Failed to write daemon command";
			return false;
		}
		return true;
	}
}
