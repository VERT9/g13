#include "g13_manager.h"

#include <iostream>
#include <boost/program_options.hpp>

#include "container.h"
#include "G13_DisplayApp.h"

using namespace std;
using namespace G13;
namespace po = boost::program_options;

/**
 * @brief Loads all necessary class factories into the IoC container
 */
void bootFramework() {
	auto& ioc = Container::Instance();

	ioc.RegisterFactory<G13_Log>([&] {
		return std::make_shared<G13_Log>();
	});
	ioc.RegisterFactory<G13_Manager>([&] {
		return std::make_shared<G13_Manager>(ioc.Resolve<G13_Log>());
	});
	ioc.RegisterFactory<G13_CurrentProfileApp>([&] {
		return std::make_shared<G13_CurrentProfileApp>(ioc.Resolve<G13_Log>());
	});
	ioc.RegisterFactory<G13_ProfileSwitcherApp>([&] {
		return std::make_shared<G13_ProfileSwitcherApp>(ioc.Resolve<G13_Log>());
	});
	ioc.RegisterFactory<G13_TesterApp>([&] {
		return std::make_shared<G13_TesterApp>(ioc.Resolve<G13_Log>());
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

