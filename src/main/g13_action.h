//
// Created by vert9 on 11/23/23.
//

#ifndef G13_G13_ACTION_H
#define G13_G13_ACTION_H

#include <boost/shared_ptr.hpp>
#include <ostream>
#include <vector>
#include <string>

#include "container.h"
#include "g13_manager.h"

namespace G13 {
	class G13_Device;

	/*!
	 * holds potential actions which can be bound to G13 activity
	 */
	class G13_Action {
	public:
		G13_Action(G13_Device& keypad, G13_Log& logger) : _keypad(keypad), _logger(logger) {}

		virtual ~G13_Action();

		virtual void act(G13_Device&, bool is_down) = 0;
		virtual void dump(std::ostream&) const = 0;

		void act(bool is_down) { act(keypad(), is_down); }

		G13_Device& keypad() { return _keypad; }
		const G13_Device& keypad() const { return _keypad; }

		G13_Manager& manager();
		const G13_Manager& manager() const;

	protected:
		G13_Log _logger;

	private:
		G13_Device& _keypad;
	};

	/*!
	 * action to send one or more keystrokes
	 */
	class G13_Action_Keys : public G13_Action {
	public:
		G13_Action_Keys(G13_Device& keypad, G13_Log& logger, const std::string& keys);
		virtual ~G13_Action_Keys();

		virtual void act(G13_Device&, bool is_down);
		virtual void dump(std::ostream&) const;

		std::vector<LINUX_KEY_VALUE> _keys;
	};

	/*!
	 * action to send a string to the output pipe
	 */
	class G13_Action_PipeOut : public G13_Action {
	public:
		G13_Action_PipeOut(G13_Device& keypad, G13_Log& logger, const std::string& out);
		virtual ~G13_Action_PipeOut();

		virtual void act(G13_Device&, bool is_down);
		virtual void dump(std::ostream&) const;

		std::string _out;
	};

	/*!
	 * action to send a command to the g13
	 */
	class G13_Action_Command : public G13_Action {
		public:
		G13_Action_Command(G13_Device& keypad, G13_Log& logger, const std::string& cmd);
		virtual ~G13_Action_Command();

		virtual void act(G13_Device&, bool is_down);
		virtual void dump(std::ostream&) const;

		std::string _cmd;
	};

	/*!
	 * @brief an action to change the current display app on the g13
	 */
	class G13_Action_AppChange : public G13_Action {
		public:
			G13_Action_AppChange(G13_Device& keypad, G13_Log& logger):G13_Action(keypad, logger) {};
			~G13_Action_AppChange() override = default;

			void act(G13_Device&, bool is_down) override;
			void dump(std::ostream&) const override;
	};

	/**
	 * @brief an action that allows for dynamic setup via a lamda function
	 */
	class G13_Action_Dynamic : public G13_Action {
		public:
			G13_Action_Dynamic(G13_Device& keypad, G13_Log& logger, std::function<void()> action);
			~G13_Action_Dynamic() override = default;

			void act(G13_Device&, bool is_down) override;
			void dump(std::ostream&) const override;
		private:
			std::function<void()> _action;
	};

	typedef boost::shared_ptr<G13_Action> G13_ActionPtr;

	// *************************************************************************
	template<class PARENT_T>
		class G13_Actionable {
		public:
			G13_Actionable(PARENT_T& parent_arg, const std::string& name) : _parent_ptr(&parent_arg), _name(name) {
				_logger = Container::Instance().Resolve<G13_Log>();
			}
			virtual ~G13_Actionable() { _parent_ptr = 0; }

			G13_ActionPtr action() const { return _action; }

			const std::string& name() const { return _name; }

			PARENT_T& parent() { return *_parent_ptr; }
			const PARENT_T& parent() const { return *_parent_ptr; }

			G13_Manager& manager() { return _parent_ptr->manager(); }
			const G13_Manager& manager() const { return _parent_ptr->manager(); }

			virtual void set_action(const G13_ActionPtr& action) {
				_action = action;
			}

		protected:
			std::string _name;
			G13_ActionPtr _action;
			std::shared_ptr<G13_Log> _logger;

		private:
			PARENT_T* _parent_ptr;
		};
}
#endif //G13_G13_ACTION_H
