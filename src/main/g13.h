#ifndef __G13_H__
#define __G13_H__

namespace G13 {
// TODO????
	typedef int G13_KEY_INDEX;
	typedef int LINUX_KEY_VALUE;
	const LINUX_KEY_VALUE BAD_KEY_VALUE = -1;

	class G13_CommandException : public std::exception {
	public:
		G13_CommandException(const std::string& reason) : _reason(reason) {}
		virtual ~G13_CommandException() throw() {}
		virtual const char* what() const throw() { return _reason.c_str(); }

		std::string _reason;
	};
} // namespace G13

#endif // __G13_H__
