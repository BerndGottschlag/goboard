#ifndef TESTS_HPP_INCLUDED
#define TESTS_HPP_INCLUDED

class RegisterTests {
public:
	RegisterTests(void (*tests)());
	~RegisterTests();

	static void run_all_tests();
private:
	void (*tests)();

	RegisterTests *next;
	RegisterTests *prev;
	static RegisterTests *first;
};

#endif

