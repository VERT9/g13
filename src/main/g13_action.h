//
// Created by vert9 on 11/23/23.
//

#ifndef G13_G13_ACTION_H
#define G13_G13_ACTION_H

#include <string>
#include <utility>
#include <vector>

#include "container.h"
#include "g13_key_map.h"

namespace G13 {
	class G13_Device;

	/*!
	 * holds potential actions which can be bound to G13 activity
	 */
	class G13_Action {
	public:
		G13_Action(std::shared_ptr<G13_Log> logger, std::shared_ptr<G13_KeyMap> keymap) : _logger(std::move(logger)), _keymap(std::move(keymap)) {}

		virtual ~G13_Action() = default;

		virtual void act(bool is_down, G13_Device& device) = 0;
		virtual void dump(std::ostream&) const = 0;

	protected:
		std::shared_ptr<G13_Log> _logger;
		std::shared_ptr<G13_KeyMap> _keymap;
	};

	/*!
	 * action to send one or more keystrokes
	 */
	class G13_Action_Keys : public G13_Action {
	public:
		G13_Action_Keys(std::shared_ptr<G13_Log> logger, std::shared_ptr<G13_KeyMap> keymap, std::string keys);

		void act(bool is_down, G13_Device& device) override;
		virtual void dump(std::ostream&) const;

		std::vector<LINUX_KEY_VALUE> _keys;
	};

	/*!
	 * action to send a string to the output pipe
	 */
	class G13_Action_PipeOut : public G13_Action {
	public:
		G13_Action_PipeOut(std::shared_ptr<G13_Log> logger, std::shared_ptr<G13_KeyMap> keymap, std::string out);

		void act(bool is_down, G13_Device& device) override;
		virtual void dump(std::ostream&) const;

		std::string _out;
	};

	/*!
	 * action to send a command to the g13
	 */
	class G13_Action_Command : public G13_Action {
		public:
		G13_Action_Command(std::shared_ptr<G13_Log> logger, std::shared_ptr<G13_KeyMap> keymap, std::string cmd);

		void act(bool is_down, G13_Device& device) override;
		virtual void dump(std::ostream&) const;

		std::string _cmd;
	};

	/*!
	 * @brief an action to change the current display app on the g13
	 */
	class G13_Action_AppChange : public G13_Action {
		public:
			G13_Action_AppChange(std::shared_ptr<G13_Log> logger, std::shared_ptr<G13_KeyMap> keymap):G13_Action(std::move(logger), std::move(keymap)) {};

			void act(bool is_down, G13_Device& device) override;
			void dump(std::ostream&) const override;

		protected:
			unsigned int _current_app = 0;
	};

	/**
	 * @brief an action that allows for dynamic setup via a lamda function
	 */
	class G13_Action_Dynamic : public G13_Action {
		public:
			G13_Action_Dynamic(std::shared_ptr<G13_Log> logger, std::shared_ptr<G13_KeyMap> keymap, std::function<void()> action);

			void act(bool is_down, G13_Device& device) override;
			void dump(std::ostream&) const override;
		private:
			std::function<void()> _action;
	};

	typedef std::shared_ptr<G13_Action> G13_ActionPtr;

	// *************************************************************************
	template<class PARENT_T>
		class G13_Actionable {
		public:
			G13_Actionable(PARENT_T& parent_arg, const std::string& name) : _parent_ptr(&parent_arg), _name(name) {
				_logger = Container::Instance().Resolve<G13_Log>();
				_keymap = Container::Instance().Resolve<G13_KeyMap>();
			}
			virtual ~G13_Actionable() { _parent_ptr = 0; }

			G13_ActionPtr action() const { return _action; }

			const std::string& name() const { return _name; }

			PARENT_T& parent() { return *_parent_ptr; }
			const PARENT_T& parent() const { return *_parent_ptr; }

			virtual void set_action(const G13_ActionPtr& action) {
				_action = action;
			}

		protected:
			std::string _name;
			G13_ActionPtr _action;
			std::shared_ptr<G13_Log> _logger;
			std::shared_ptr<G13_KeyMap> _keymap;

		private:
			PARENT_T* _parent_ptr;
		};
}
#endif //G13_G13_ACTION_H
