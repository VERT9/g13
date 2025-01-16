#ifndef __G13_H__
#define __G13_H__

#include <exception>
#include <string>

namespace G13 {
	class G13_CommandException : public std::exception {
	public:
		G13_CommandException(const std::string& reason) : _reason(reason) {}
		virtual ~G13_CommandException() throw() {}
		virtual const char* what() const throw() { return _reason.c_str(); }

		std::string _reason;
	};
} // namespace G13

#endif // __G13_H__
