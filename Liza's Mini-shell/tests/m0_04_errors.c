#include <sunit.h>
#include <msh_parse.h>

#include <string.h>

sunit_ret_t
nocmd(void)
{
	/* add your own tests here. */
	struct msh_sequence *s1, *s2;
	msh_err_t ret;


	s1 = msh_sequence_alloc();
	SUNIT_ASSERT("sequence allocation", s1 != NULL);
	ret = msh_sequence_parse(" | world", s1);
	SUNIT_ASSERT("there is a command before |", ret == MSH_ERR_PIPE_MISSING_CMD && s1 != NULL);

	msh_sequence_free(s1);

	s2 = msh_sequence_alloc();
	SUNIT_ASSERT("sequence allocation", s2 != NULL);
	ret = msh_sequence_parse("hello | ", s2);
	SUNIT_ASSERT("there is a command after |", ret == -8 && s2 != NULL);

	msh_sequence_free(s2);

	return 0;
}

sunit_ret_t
too_many_cmd(void)
{
	/* add your own tests here. */
	struct msh_sequence *s1;
	msh_err_t ret;
	s1 = msh_sequence_alloc();
	SUNIT_ASSERT("sequence allocation", s1 != NULL);
	ret = msh_sequence_parse("hello | world | hello | world | and | universe | is | expanding | and | I | need | to | find | things | to | write ", s1);
	SUNIT_ASSERT("there number of commands are fine", ret != -8 && s1 != NULL);

	msh_sequence_free(s1);

	return 0;
}

sunit_ret_t
too_many_args(void)
{
	/* add your own tests here. */
	struct msh_sequence *s1;
	msh_err_t ret;
	s1 = msh_sequence_alloc();
	SUNIT_ASSERT("sequence allocation", s1 != NULL);
	ret = msh_sequence_parse("hello world hello world and universe is expanding and I need to find things to write", s1);
	SUNIT_ASSERT("there number of arguments is fine", ret != -7 && s1 != NULL);

	msh_sequence_free(s1);

	return 0;
}

sunit_ret_t
empty_sequence(void){

	struct msh_sequence *s;
	msh_err_t ret;

	s = msh_sequence_alloc();
	SUNIT_ASSERT("sequence allocation", s != NULL);
	ret = msh_sequence_parse(" ; ; this pipe has args ; ", s);

	SUNIT_ASSERT("missing a command", ret == -8 && s!=NULL);

	msh_sequence_free(s);

	return 0;	
}


sunit_ret_t
empty_input(void){
	struct msh_sequence *s;
	msh_err_t ret;

	s = msh_sequence_alloc();
	SUNIT_ASSERT("sequence allocation", s != NULL);
	ret = msh_sequence_parse("",s);
	SUNIT_ASSERT("sequence not parsed", ret==0 && s!=NULL);
	msh_sequence_free(s);

	return 0;
}

sunit_ret_t
too_many_pipes(void){
	struct msh_sequence *s;
	msh_err_t ret;

	s = msh_sequence_alloc();
	SUNIT_ASSERT("sequence allocation", s != NULL);

	ret = msh_sequence_parse("hello ; world ; hello ; world ; and ; universe ; is ; expanding ; and ; I ; need ; to ; find ; things ; to", s);
	SUNIT_ASSERT("there are too many pipes",ret != -20 && s!=NULL);
	//if its over max pipes plus 1 
	msh_sequence_free(s);
	
	return 0;
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
		SUNIT_TEST("empty sequence", empty_sequence),
		SUNIT_TEST("empty input", empty_input),
		SUNIT_TEST("too many pipes", too_many_pipes),
		SUNIT_TEST_TERM
	};
	
	sunit_execute("Testing edge cases and errors", tests);

	return 0;
}
