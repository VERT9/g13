#include <chrono>
#include <format>
#include <fstream>
#include <iostream>
#include <map>

#include "g13_log.h"

namespace G13 {
	G13_Log& G13_Log::set_log_level(LogLevel lvl) {
		level = lvl;

		internal = true;
		auto str = "set log level to " + enum_to_str[lvl];
		log(LogLevel::info, str);
		internal = false;

		return *this;
	}

	G13_Log& G13_Log::set_log_level( const std::string &level ) {
		if (str_to_enum.contains(level)) {
			set_log_level(str_to_enum[level]);
			return *this;
		}

		error("unknown log level: " + level);

		return *this;
	}

	void G13_Log::log(LogLevel lvl, std::string& message) {
		if (lvl >= level || internal) {
			const auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(
			std::chrono::system_clock::now().time_since_epoch()).count() % 1000;
			const auto t = std::time(nullptr);
			const auto now = std::localtime(&t);
			char timestamp[sizeof "9999-12-31 29:59:59.9999"];
			sprintf(
				timestamp,
				"%04d-%02d-%02d %02d:%02d:%02d.%lld",
				now->tm_year + 1900,
				now->tm_mon + 1,
				now->tm_mday,
				now->tm_hour,
				now->tm_min,
				now->tm_sec,
				millis);

			std::cout << std::format("[{}] [{}] [{}] {}", timestamp, getpid(), enum_to_str[lvl], message) << std::endl;
		}
	}

	void G13_Log::trace(std::string message) {
		log(LogLevel::trace, message);
	}
	void G13_Log::debug(std::string message) {
		log(LogLevel::debug, message);
	}
	void G13_Log::info(std::string message) {
		log(LogLevel::info, message);
	}
	void G13_Log::warning(std::string message) {
		log(LogLevel::warning, message);
	}
	void G13_Log::error(std::string message) {
		log(LogLevel::error, message);
	}
	void G13_Log::fatal(std::string message) {
		log(LogLevel::fatal, message);
	}
} // namespace G13
