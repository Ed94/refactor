/*
	BLOAT.

	ZPL requires ZPL_IMPLEMENTATION whereever this library is included.
	
	This file assumes it will be included in one compilation unit.
*/

#pragma once	

#if __clang__
#	pragma clang diagnostic ignored "-Wunused-const-variable"
#	pragma clang diagnostic ignored "-Wswitch"
#	pragma clang diagnostic ignored "-Wunused-variable"
#endif



#pragma region 									ZPL INCLUDE
#if __clang__
#	pragma clang diagnostic push 
#	pragma clang diagnostic ignored "-Wmissing-braces"
#	pragma clang diagnostic ignored "-Wbraced-scalar-init"
#endif

// #   define ZPL_HEAP_ANALYSIS
#	define ZPL_NO_MATH_H
#	define ZPL_DISABLE_C_DECLS
#   define ZPL_WRAP_IN_NAMESPACE
#	define ZPL_CUSTOM_MODULES
#		define ZPL_MODULE_ESSENTIALS
#		define ZPL_MODULE_CORE
#		define ZPL_MODULE_TIMER
// #	define ZPL_MODULE_HASHING
// #	define ZPL_MODULE_REGEX
// #	define ZPL_MODULE_EVENT
// #	define ZPL_MODULE_DLL
#	define ZPL_MODULE_OPTS
// #	define ZPL_MODULE_PROCESS
// #	define ZPL_MODULE_MATH
// #	define ZPL_MODULE_THREADING
// #	define ZPL_MODULE_JOBS
// #	define ZPL_MODULE_PARSER
// extern "C" {
#include "zpl.refactored.h"
// }

#if __clang__
#	pragma clang diagnostic pop
#endif
#pragma endregion 								ZPL INCLUDE



#define bit( Value_ )                      ( 1 << Value_ )
#define bitfield_is_equal( Field_, Mask_ ) ( ( Mask_ & Field_ ) == Mask_ )
#define ct                                 constexpr
#define gen( ... )                         template< __VA_ARGS__ >
#define forceinline                        ZPL_ALWAYS_INLINE
#define print_nl( _)                       zpl_printf("\n")
#define cast( Type_, Value_ )              ( ( Type_ ), ( Value_ ) )
#define scast( Type_, Value_ )			   static_cast< Type_ >( Value_ )
#define rcast( Type_, Value_ )			   reinterpret_cast< Type_ >( Value_ )
#define pcast( Type_, Value_ )             ( * (Type_*)( & Value_ ) )

#define do_once()     \
do                    \
{                     \
	static            \
	bool Done = true; \
	if ( Done )       \
		return;       \
	Done = false;     \
}                     \
while(0)              \


ct char const* Msg_Invalid_Value = "INVALID VALUE PROVIDED";


namespace Memory
{
	zpl::arena Global_Arena {};
	#define g_allocator arena_allocator( & Memory::Global_Arena)

	void setup()
	{
		arena_init_from_allocator( & Global_Arena, heap(), megabytes(1) );

		if ( Global_Arena.total_size == 0 )
		{
			zpl::assert_crash( "Failed to reserve memory for Tests:: Global_Arena" );
		}
	}

	void cleanup()
	{
		arena_free( & Global_Arena);
	}
}

zpl::sw log_fmt(char const *fmt, ...) 
{
#if Build_Debug
	zpl::sw res;
	va_list va;
	va_start(va, fmt);
	res = zpl::printf_va(fmt, va);
	va_end(va);
	return res;

#else
	return 0;
#endif
}

void fatal(char const *fmt, ...) 
{
	local_persist thread_local 
	char buf[ZPL_PRINTF_MAXLEN] = { 0 };

	va_list va;

#if Build_Debug
	va_start(va, fmt);
	zpl::snprintf_va(buf, ZPL_PRINTF_MAXLEN, fmt, va);
	va_end(va);

	zpl::assert_crash(buf);
#else
	va_start(va, fmt);
	zpl::printf_err_va( fmt, va);
	va_end(va);

	zpl::exit(1);
#endif
}
