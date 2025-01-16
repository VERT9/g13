#include "g13_manager.h"

#include <iostream>
#include <boost/program_options.hpp>

#include "container.h"
#include "G13_DisplayApp.h"
#include "g13_action.h"
#include "g13_key_map.h"

using namespace std;
using namespace G13;
namespace po = boost::program_options;

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

	// Declare the supported options.
	po::options_description desc("Allowed options");
	desc.add_options()("help", "produce help message");
	vector<string> sopt_names;
	auto add_string_option = [&sopt_names, &desc](const char* name, const char* description) {
		desc.add_options()(name, po::value<string>(), description);
		sopt_names.push_back(name);
	};
	add_string_option("logo", "set logo from file");
	add_string_option("config", "load config commands from file");
	add_string_option("pipe_in", "specify name for input pipe; default is '/tmp/g13-0'");
	add_string_option("pipe_out", "specify name for output pipe; default is '/tmp/g13-0_out'");
	add_string_option("log_level", "logging level; default is 'info'");
	add_string_option("profiles_dir", "profiles directory; default is '~/.g13d/profiles'");

	po::positional_options_description p;
	p.add("logo", -1);
	po::variables_map vm;
	po::store(po::command_line_parser(argc, argv).options(desc).positional(p).run(), vm);
	po::notify(vm);

	if (vm.contains("help")) {
		cout << argv[0] << " : user space G13 driver" << endl;
		cout << desc << "\n";
		return 1;
	}

	for (const auto& tag : sopt_names) {
		if (vm.contains(tag)) {
			manager->set_string_config_value(tag, vm[tag].as<string>());
		}
	}

	if (vm.contains("logo")) {
		manager->set_logo(vm["logo"].as<string>());
	}

	if (vm.contains("log_level")) {
		Container::Instance().Resolve<G13_Log>()->set_log_level(manager->string_config_value("log_level"));
	}

	manager->run();
}

