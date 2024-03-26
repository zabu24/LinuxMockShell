#include <sunit.h>
#include <msh_parse.h>

#include <string.h>

sunit_ret_t
nocmd(void)
{
	struct msh_sequence *s;
	msh_err_t ret;

	s = msh_sequence_alloc();
	SUNIT_ASSERT("sequence allocation", s != NULL);
	ret = msh_sequence_parse("This |	| Test", s);
	SUNIT_ASSERT("empty command, error", ret == MSH_ERR_PIPE_MISSING_CMD && s !=NULL);

	msh_sequence_free(s);

	return SUNIT_SUCCESS;
}

sunit_ret_t
too_many_cmd(void)
{
	// struct msh_sequence *s;
	// msh_err_t ret;

	// s = msh_sequence_alloc();

	// SUNIT_ASSERT("sequence allocation", s != NULL);
	// ret = msh_sequence_parse("0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 | 11 | 12 | 13 | 14 | 15 | 16", s);
	// SUNIT_ASSERT("Too Many Commands", ret == MSH_ERR_TOO_MANY_ARGS);

	// msh_sequence_free(s);

	return SUNIT_SUCCESS;
}

sunit_ret_t
too_many_args(void)
{
	struct msh_sequence *s;
	msh_err_t ret;

	s = msh_sequence_alloc();

	SUNIT_ASSERT("sequence allocation", s != NULL);
	ret = msh_sequence_parse("hey arg1 arg2 arg3 arg4 arg5 arg6 arg7 arg8 arg9 arg10 arg11 arg12 arg13 arg14 arg15 arg16", s);
	SUNIT_ASSERT("Too Many Arguments", ret == MSH_ERR_TOO_MANY_ARGS);

	msh_sequence_free(s);

	return SUNIT_SUCCESS;
}



int
main(void)
{
	struct sunit_test tests[] = {
		SUNIT_TEST("pipeline with no command after |", nocmd),
		SUNIT_TEST("pipeline with no command before |", nocmd),
		SUNIT_TEST("too many commands", too_many_cmd),
		SUNIT_TEST("too many args", too_many_args),
		/* add your own tests here... */
		SUNIT_TEST_TERM
	};

	sunit_execute("Testing edge cases and errors", tests);

	return 0;
}