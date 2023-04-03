/*
	BLOAT.
*/

#pragma once	

#ifdef BLOAT_IMPL
#	define ZPL_IMPLEMENTATION
#endif

#pragma region 									ZPL INCLUDE
#if __clang__
#	pragma clang diagnostic push 
#	pragma clang diagnostic ignored "-Wmissing-braces"
#	pragma clang diagnostic ignored "-Wbraced-scalar-init"
#endif

// #   define ZPL_HEAP_ANALYSIS
#	define ZPL_NO_MATH_H
#	define ZPL_CUSTOM_MODULES
#		define ZPL_MODULE_ESSENTIALS
#		define ZPL_MODULE_CORE
#		define ZPL_MODULE_TIMER
// #	define ZPL_MODULE_HASHING
// #	define ZPL_MODULE_REGEX
// #	define ZPL_MODULE_EVENT
// #	define ZPL_MODULE_DLL
#		define ZPL_MODULE_OPTS
// #	define ZPL_MODULE_PROCESS
// #	define ZPL_MODULE_MAT
// #	define ZPL_MODULE_THREADING
// #	define ZPL_MODULE_JOBS
// #	define ZPL_MODULE_PARSER
#include "zpl.h"

#if __clang__
#	pragma clang diagnostic pop
#endif
#pragma endregion 								ZPL INCLUDE



#if __clang__
#	pragma clang diagnostic ignored "-Wunused-const-variable"
#	pragma clang diagnostic ignored "-Wswitch"
#	pragma clang diagnostic ignored "-Wunused-variable"
#   pragma clang diagnostic ignored "-Wunknown-pragmas"
#endif



#define bit( Value_ )                      ( 1 << Value_ )
#define bitfield_is_equal( Field_, Mask_ ) ( ( Mask_ & Field_ ) == Mask_ )
#define ct                                 constexpr
#define gen( ... )                         template< __VA_ARGS__ >
#define forceinline                        ZPL_ALWAYS_INLINE
#define print_nl( _)                       zpl_printf("\n")
#define scast( Type_, Value_ )             static_cast< Type_ >( Value_ )
#define rcast( Type_, Value_ )             reinterpret_cast< Type_ >( Value_ )
#define pcast( Type_, Value_ )             ( * (Type_*)( & Value_ ) )

#define do_once()      \
do                     \
{                      \
	static             \
	bool Done = false; \
	if ( Done )        \
		return;        \
	Done = true;       \
}                      \
while(0)               \


using b32 = zpl_b32;
using s8  = zpl_i8;
using s32 = zpl_i32;
using s64 = zpl_i64;
using u8  = zpl_u8;
using u16 = zpl_u16;
using u32 = zpl_u32;
using f64 = zpl_f64;
using uw  = zpl_usize;
using sw  = zpl_isize;

using Line       = char*;
using Array_Line = zpl_array( Line );


ct char const* Msg_Invalid_Value = "INVALID VALUE PROVIDED";


namespace Global
{
	extern bool ShouldShowDebug;
}

namespace Memory
{
	ct uw Initial_Reserve = zpl_megabytes(2);

	extern zpl_arena Global_Arena;
	#define g_allocator zpl_arena_allocator( & Memory::Global_Arena)

	void setup();
	void resize( uw new_size );
	void cleanup();
}

// Had to be made to support multiple sub-arguments per "opt" argument.
b32 opts_custom_compile(zpl_opts *opts, int argc, char **argv);


inline
sw log_fmt(char const *fmt, ...) 
{
	if ( Global::ShouldShowDebug == false )
		return 0;

	sw res;
	va_list va;
	
	va_start(va, fmt);
	res = zpl_printf_va(fmt, va);
	va_end(va);
	
	return res;
}

inline
void fatal(char const *fmt, ...) 
{
	zpl_local_persist zpl_thread_local 
	char buf[ZPL_PRINTF_MAXLEN] = { 0 };

	va_list va;

#if Build_Debug
	va_start(va, fmt);
	zpl_snprintf_va(buf, ZPL_PRINTF_MAXLEN, fmt, va);
	va_end(va);

	zpl_assert_crash(buf);
#else
	va_start(va, fmt);
	zpl_printf_err_va( fmt, va);
	va_end(va);

	zpl_exit(1);
#endif
}
