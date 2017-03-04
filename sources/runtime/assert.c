#include <runtime/lib.h>
#include <kernel/internal.h>
#include <stdarg.h>

#ifndef UOS_STRICT_STACK
#define UOS_STRICT_STACK        0
#define UOS_STRICTS             0
#endif

#ifdef UOS_EXCEPTION_STACK

#ifdef MIPS32
#define VPRINTF_FRAME (256+16)
#define ARCH_BREAK(x)    { asm volatile("break"); __builtin_unreachable ();}
#else
#define VPRINTF_FRAME 0
#define ARCH_BREAK(x)
#endif

#else //UOS_EXCEPTION_STACK
#define VPRINTF_FRAME 0
#define ARCH_BREAK(x)
#endif //UOS_EXCEPTION_STACK

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
    arch_intr_off ();
#ifdef UOS_EXCEPTION_STACK
    //* if have another stack, and current stack is broken or small,
    //      just go to exception as inevitable
    UOS_STRICT(STACK,)
        if(!(task_stack_enough( VPRINTF_FRAME )) ){
            debug_puts("\nAssertion failed in function `");
            debug_puts(func);
            debug_puts("':");
            debug_puts(cond);
            debug_puts("\n");
            debug_puts(file);
            ARCH_BREAK();
        }
#endif//UOS_EXCEPTION_STACK
    if (0 ==
	debug_printf ("\nAssertion failed in function `%S':\n%S, %u: %S\n\n",
		func, file, line, cond)
		)
        //если неудалась печать, обрушусь в стек исключений
        ARCH_BREAK();
	uos_halt (1);
}

void __assert_msg(const char *msg, ...)
{
    arch_intr_off ();
#ifdef UOS_EXCEPTION_STACK
    //* if have another stack, and current stack is broken or small,
    //      just go to exception as inevitable
    UOS_STRICT(STACK,)
        if(!(task_stack_enough( VPRINTF_FRAME )) ){
            debug_puts(msg);
            debug_puts("\nbreak on exception stack\n");
            return;
        }
#endif//UOS_EXCEPTION_STACK

    va_list args;

    va_start (args, msg);
    if (0 == debug_vprintf (msg, args) )
        //если неудалась печать, обрушусь в стек исключений
        ARCH_BREAK();
    va_end (args);
}

const char uos_assert_task_name_msg[] = "asserted task:%S\n";
const char uos_assert_mutex_task_name_msg[] = "asserted mutex:%p task:%S\n";

#endif
