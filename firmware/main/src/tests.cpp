#include "tests.hpp"

#include <stddef.h>

RegisterTests::RegisterTests(void (*tests)()): tests(tests) {
	if (first == NULL) {
		first = this;
		next = this;
		prev = this;
	} else {
		first->prev->next = this;
		prev = first->prev;
		next = first;
		first->prev = this;
	}
}

RegisterTests::~RegisterTests() {
	if (next == this) {
		first = NULL;
	} else {
		next = prev;
		prev = next;
	}
}

void RegisterTests::run_all_tests() {
	RegisterTests *test = first;
	if (!test) {
		return;
	}
	do {
		test->tests();
		test = test->next;
	} while (test != first);
}

RegisterTests *RegisterTests::first = NULL;

