#include "g13_log.h"
#include <fstream>
#include <map>

#include <boost/log/sources/severity_feature.hpp>
#include <boost/log/sources/severity_logger.hpp>
#include <boost/log/core/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/utility/setup.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/support/date_time.hpp>

namespace G13 {
	void G13_Log::set_log_level( ::boost::log::trivial::severity_level lvl ) {
		boost::log::core::get()->set_filter(::boost::log::trivial::severity >= lvl);
		G13_OUT( "set log level to " << lvl );
	}

	void G13_Log::set_log_level( const std::string &level ) {
		std::map<std::string, boost::log::trivial::severity_level> values = {
				{BOOST_PP_STRINGIZE(trace), boost::log::trivial::severity_level::trace},
				{BOOST_PP_STRINGIZE(debug), boost::log::trivial::severity_level::debug},
				{BOOST_PP_STRINGIZE(info), boost::log::trivial::severity_level::info},
				{BOOST_PP_STRINGIZE(warning), boost::log::trivial::severity_level::warning},
				{BOOST_PP_STRINGIZE(error), boost::log::trivial::severity_level::error},
				{BOOST_PP_STRINGIZE(fatal), boost::log::trivial::severity_level::fatal},
		};
		if (values.contains(level)) {
			set_log_level(values[level]);
			return;
		}

		error("unknown log level: " + level);
	}

	void G13_Log::trace(std::string message) {
		BOOST_LOG_TRIVIAL(trace) << message;
	}

	void G13_Log::debug(std::string message) {
		BOOST_LOG_TRIVIAL(debug) << message;
	}
	void G13_Log::info(std::string message) {
		BOOST_LOG_TRIVIAL(info) << message;
	}
	void G13_Log::warning(std::string message) {
		BOOST_LOG_TRIVIAL(warning) << message;
	}
	void G13_Log::error(std::string message) {
		BOOST_LOG_TRIVIAL(error) << message;
	}
	void G13_Log::fatal(std::string message) {
		BOOST_LOG_TRIVIAL(fatal) << message;
	}
} // namespace G13
