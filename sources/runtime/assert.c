#include <runtime/lib.h>
#include <stdarg.h>

/*
 * Called from assert() macro.
 * Print debugging information and halt the system.
 */
#ifdef NDEBUG
void
__assert_fail_ndebug ()
{
	debug_printf ("\nAssertion failed at address %p\n\n",
		__builtin_return_address (0));
	uos_halt (1);
}
#else
//__NORETURN
void
__assert_fail (const char *cond, const char *file, unsigned int line,
	const char *func)
{
    int x = 0;
    mips_intr_disable (&x);
	debug_printf ("\nAssertion failed in function `%S':\n%S, %u: %S\n\n",
		func, file, line, cond);
	uos_halt (1);
}

void __assert_msg(const char *msg, ...)
{
    int x = 0;
    mips_intr_disable (&x);
    va_list args;

    va_start (args, msg);
    debug_vprintf (msg, args);
    va_end (args);
}

const char uos_assert_task_name_msg[] = "asserted task:%S\n";

#endif
