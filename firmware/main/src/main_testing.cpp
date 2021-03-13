#include <ztest.h>

void test_test(void)
{
        zassert_true(1, "true");
}

void test_main(void)
{
	printk("test\n");
	ztest_test_suite(common,
		ztest_unit_test(test_test)
	);
	ztest_run_test_suite(common);
	// TODO
}

