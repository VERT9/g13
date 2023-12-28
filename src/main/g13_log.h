//
// Created by vert9 on 12/27/23.
//

#ifndef G13_G13_LOG_H
#define G13_G13_LOG_H

#include <boost/log/trivial.hpp>

namespace G13 {
	#define G13_LOG(level, message) BOOST_LOG_TRIVIAL( level ) << message
	#define G13_OUT(message) BOOST_LOG_TRIVIAL( info ) << message

	class G13_Log {
	public:
		void set_log_level(::boost::log::trivial::severity_level lvl);
		void set_log_level(const std::string&);

		void trace(std::string message);
		void debug(std::string message);
		void info(std::string message);
		void warning(std::string message);
		void error(std::string message);
		void fatal(std::string message);
	};
}

#endif //G13_G13_LOG_H
