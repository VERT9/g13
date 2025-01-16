//
// Created by vert9 on 12/27/23.
//

#ifndef G13_G13_LOG_H
#define G13_G13_LOG_H

#include <map>
#include <memory>
#include <string>

namespace G13 {
	enum LogLevel {
		trace,
		debug,
		info,
		warning,
		error,
		fatal
	};

	class G13_Log {
		public:
			static std::shared_ptr<G13_Log> get() {
				static std::shared_ptr<G13_Log> instance(new G13_Log());
				return instance;
			}

			G13_Log(G13_Log const&) = delete;
			void operator=(G13_Log const&) = delete;

			G13_Log& set_log_level(LogLevel lvl);
			G13_Log& set_log_level(const std::string&);

			void log(LogLevel lvl, std::string& message);
			void trace(std::string message);
			void debug(std::string message);
			void info(std::string message);
			void warning(std::string message);
			void error(std::string message);
			void fatal(std::string message);

		private:
			G13_Log() = default;
			LogLevel level = LogLevel::info;
			bool internal = false;

			std::map<std::string, LogLevel> str_to_enum = {
				{"trace", LogLevel::trace},
				{"debug", LogLevel::debug},
				{"info", LogLevel::info},
				{"warning", LogLevel::warning},
				{"error", LogLevel::error},
				{"fatal", LogLevel::fatal}
			};
			std::map<LogLevel, std::string> enum_to_str = {
				{LogLevel::trace, "trace"},
				{LogLevel::debug, "debug"},
				{LogLevel::info, "info"},
				{LogLevel::warning, "warning"},
				{LogLevel::error, "error"},
				{LogLevel::fatal, "fatal"}
			};
	};
}

#endif //G13_G13_LOG_H
