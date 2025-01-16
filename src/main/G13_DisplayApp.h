//
// Created by vert9 on 11/23/24.
//

#ifndef G13_DISPLAYAPP_H
#define G13_DISPLAYAPP_H

#include <chrono>
#include <utility>

namespace G13 {
	class G13_Device;
	class G13_KeyMap;
	class G13_Log;

	/**
	 * @brief Base interface for a DisplayApp
	 */
	class G13_DisplayApp {
		public:
			explicit G13_DisplayApp(std::shared_ptr<G13_Log> logger, std::shared_ptr<G13_KeyMap> keymap) : _logger(std::move(logger)), _keymap(std::move(keymap)){}
			virtual ~G13_DisplayApp() = default;

			/**
			 * @brief Renders the next frame on the display
			 * @param device the device to send display updates to
			 */
			virtual void display(G13_Device& device) = 0;

			/**
			 * @brief Initializes the app allowing for setup such as actions for LIGHT keys. By default, all keys will do nothing when pressed, so this must be overridden to assign an action.
			 * @param device the device to send display updates to
			 */
			virtual void init(G13_Device& device);

		protected:
			std::shared_ptr<G13_Log> _logger;
			std::shared_ptr<G13_KeyMap> _keymap;
	};

	/**
	 * @brief An app that displays the name of the current as well as the current time and stick stats
	 */
	class G13_CurrentProfileApp final : public G13_DisplayApp {
		public:
			explicit G13_CurrentProfileApp(std::shared_ptr<G13_Log> logger, std::shared_ptr<G13_KeyMap> keymap) : G13_DisplayApp(std::move(logger), std::move(keymap)){}
			~G13_CurrentProfileApp() override = default;

			void display(G13_Device& device) override;
		private:
			unsigned int name_start = 0;
			int direction = 1;
			std::chrono::time_point<std::chrono::system_clock> last_update = std::chrono::high_resolution_clock::now();
	};

	/**
	 * @brief An app that displays a list of loaded profiles allowing to easily switch between them
	 */
	class G13_ProfileSwitcherApp final : public G13_DisplayApp {
		public:
			explicit G13_ProfileSwitcherApp(std::shared_ptr<G13_Log> logger, std::shared_ptr<G13_KeyMap> keymap) : G13_DisplayApp(std::move(logger), std::move(keymap)) {}
			~G13_ProfileSwitcherApp() override = default;

			void display(G13_Device& device) override;
			void init(G13_Device& device) override;
		private:
			unsigned int profile_display_start = 0;
			unsigned int selected_profile = 0;
	};

	/**
	 * @brief A simple test app that shows some of the limits of the display
	 */
	class G13_TesterApp final : public G13_DisplayApp {
		public:
			explicit G13_TesterApp(std::shared_ptr<G13_Log> logger, std::shared_ptr<G13_KeyMap> keymap) : G13_DisplayApp(std::move(logger), std::move(keymap)){}
			~G13_TesterApp() override = default;

			void display(G13_Device& device) override;
	};
}

#endif //G13_DISPLAYAPP_H
