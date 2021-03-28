#ifndef EXCEPTION_HPP_INCLUDED
#define EXCEPTION_HPP_INCLUDED

class Exception {
public:
	virtual ~Exception() {}

	virtual const char *what() = 0;
};

class InvalidState: Exception {
public:
	InvalidState(const char *message): _message(message) {}

	virtual const char *what() {
		return _message;
	}
private:
	const char *_message;
};

class InitializationFailed: Exception {
public:
	InitializationFailed(const char *message): _message(message) {}

	virtual const char *what() {
		return _message;
	}
private:
	const char *_message;
};

class Unimplemented: Exception {
public:
	Unimplemented(const char *message): _message(message) {}

	virtual const char *what() {
		return _message;
	}
private:
	const char *_message;
};

class HardwareError: Exception {
public:
	HardwareError(const char *message): _message(message) {}

	virtual const char *what() {
		return _message;
	}
private:
	const char *_message;
};

#endif

