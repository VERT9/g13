#include "g13_manager.h"

#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "container.h"
#include "G13_DisplayApp.h"
#include "g13_action.h"
#include "g13_key_map.h"

using namespace std;
using namespace G13;

namespace {
	struct StringOption {
		std::string name;
		std::string description;
	};

	const std::vector<StringOption> string_options = {
		{"logo", "set logo from file"},
		{"config", "load config commands from file"},
		{"pipe_in", "specify name for input pipe; default is '/tmp/g13-0'"},
		{"pipe_out", "specify name for output pipe; default is '/tmp/g13-0_out'"},
		{"log_level", "logging level; default is 'info'"},
		{"profiles_dir", "profiles directory; default is '~/.g13d/profiles'"},
	};

	/**
	 * @brief Maps supported legacy option names to the internal configuration key.
	 */
	std::string canonical_option_name(const std::string& name) {
		if (name == "profile_dir") {
			return "profiles_dir";
		}
		return name;
	}

	/**
	 * @brief Checks whether an option name is one of the supported string-valued options.
	 */
	bool is_string_option(const std::string& name) {
		const auto canonical_name = canonical_option_name(name);
		for (const auto& option : string_options) {
			if (option.name == canonical_name) {
				return true;
			}
		}
		return false;
	}

	/**
	 * @brief Stores the positional logo argument and rejects duplicates.
	 */
	void set_positional_logo(std::map<std::string, std::string>& options, const std::string& value, std::string& error) {
		if (options.contains("logo")) {
			error = "logo specified more than once";
			return;
		}
		options["logo"] = value;
	}

	/**
	 * @brief Parses supported command-line options into config values for G13_Manager.
	 *
	 * Supports --option value, --option=value, --help, and one positional logo path.
	 * The -- delimiter treats the remaining arguments as positional values.
	 *
	 * @return true if parsing succeeds; false with error populated if an argument is invalid.
	 */
	bool parse_args(int argc, char* argv[], std::map<std::string, std::string>& options, bool& help, std::string& error) {
		for (int i = 1; i < argc; ++i) {
			std::string arg = argv[i];

			if (arg == "--") {
				for (++i; i < argc; ++i) {
					set_positional_logo(options, argv[i], error);
					if (!error.empty()) {
						return false;
					}
				}
				return true;
			}

			if (!arg.starts_with("--")) {
				set_positional_logo(options, arg, error);
				if (!error.empty()) {
					return false;
				}
				continue;
			}

			std::string name_and_value = arg.substr(2);
			const auto separator = name_and_value.find('=');
			std::string name = canonical_option_name(name_and_value.substr(0, separator));
			std::string value;

			if (name == "help") {
				if (separator != std::string::npos) {
					error = "--help does not take a value";
					return false;
				}
				help = true;
				continue;
			}

			if (!is_string_option(name)) {
				error = "unknown option '--" + name + "'";
				return false;
			}

			if (separator == std::string::npos) {
				if (i + 1 >= argc) {
					error = "missing value for '--" + name + "'";
					return false;
				}
				value = argv[++i];
			} else {
				value = name_and_value.substr(separator + 1);
			}

			options[name] = value;
		}

		return true;
	}

	/**
	 * @brief Prints supported command-line options.
	 */
	void print_usage(const char* executable) {
		cout << executable << " : user space G13 driver" << endl;
		cout << "Allowed options:\n";
		cout << "  --help\tproduce help message\n";
		for (const auto& option : string_options) {
			cout << "  --" << option.name << " arg\t" << option.description << "\n";
		}
		cout << "\nA positional argument is treated as --logo.\n";
	}
}

/**
 * @brief Loads all necessary class factories into the IoC container
 */
void bootFramework() {
	auto& ioc = Container::Instance();

	ioc.RegisterFactory<G13_Log>([&](auto arg1, auto arg2, auto arg3) {
		return G13_Log::get();
	});
	ioc.RegisterFactory<G13_Manager>([&](auto arg1, auto arg2, auto arg3) {
		return std::make_shared<G13_Manager>(ioc.Resolve<G13_Log>(), ioc.Resolve<G13_KeyMap>());
	});
	ioc.RegisterFactory<G13_CurrentProfileApp>([&](auto arg1, auto arg2, auto arg3) {
		return std::make_shared<G13_CurrentProfileApp>(ioc.Resolve<G13_Log>(), ioc.Resolve<G13_KeyMap>());
	});
	ioc.RegisterFactory<G13_ProfileSwitcherApp>([&](auto arg1, auto arg2, auto arg3) {
		return std::make_shared<G13_ProfileSwitcherApp>(ioc.Resolve<G13_Log>(), ioc.Resolve<G13_KeyMap>());
	});
	ioc.RegisterFactory<G13_TesterApp>([&](auto arg1, auto arg2, auto arg3) {
		return std::make_shared<G13_TesterApp>(ioc.Resolve<G13_Log>(), ioc.Resolve<G13_KeyMap>());
	});
	ioc.RegisterFactory<G13_KeyMap>([&](auto arg1, auto arg2, auto arg3) {
		return G13_KeyMap::get();
	});
	ioc.RegisterFactory<G13_Action_Keys>([&](auto arg1, auto arg2, auto arg3) {
		return std::make_shared<G13_Action_Keys>(ioc.Resolve<G13_Log>(), ioc.Resolve<G13_KeyMap>(), std::any_cast<std::string>(arg1));
	});
	ioc.RegisterFactory<G13_Action_Command>([&](auto arg1, auto arg2, auto arg3) {
		return std::make_shared<G13_Action_Command>(ioc.Resolve<G13_Log>(), ioc.Resolve<G13_KeyMap>(), std::any_cast<std::string>(arg1));
	});
	ioc.RegisterFactory<G13_Action_PipeOut>([&](auto arg1, auto arg2, auto arg3) {
		return std::make_shared<G13_Action_PipeOut>(ioc.Resolve<G13_Log>(), ioc.Resolve<G13_KeyMap>(), std::any_cast<std::string>(arg1));
	});
	ioc.RegisterFactory<G13_Action_AppChange>([&](auto arg1, auto arg2, auto arg3) {
		return std::make_shared<G13_Action_AppChange>(ioc.Resolve<G13_Log>(), ioc.Resolve<G13_KeyMap>());
	});

	//TODO register logger and other services
}

int main(int argc, char* argv[]) {
	bootFramework();
	Container::Instance().Resolve<G13_Log>()->set_log_level("info");

	const auto manager = Container::Instance().Resolve<G13_Manager>();

	std::map<std::string, std::string> options;
	bool help = false;
	std::string error;
	if (!parse_args(argc, argv, options, help, error)) {
		cerr << argv[0] << ": " << error << "\n\n";
		print_usage(argv[0]);
		return 1;
	}

	if (help) {
		print_usage(argv[0]);
		return 1;
	}

	for (const auto& [name, value] : options) {
		manager->set_string_config_value(name, value);
	}

	if (options.contains("logo")) {
		manager->set_logo(options["logo"]);
	}

	if (options.contains("log_level")) {
		Container::Instance().Resolve<G13_Log>()->set_log_level(manager->string_config_value("log_level"));
	}

	manager->run();
}
