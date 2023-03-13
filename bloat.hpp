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
#include "zpl.h"
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


using c8  = char;
using s8  = zpl_i8;
using s32 = zpl_i32;
using s64 = zpl_i64;
using u8  = zpl_u8;
using u16 = zpl_u16;
using u32 = zpl_u32;
using f64 = zpl_f64;
using uw  = zpl_usize;
using sw  = zpl_isize;


ct c8 const* Msg_Invalid_Value = "INVALID VALUE PROVIDED";


namespace Memory
{
	zpl_arena Global_Arena {};
	#define g_allocator zpl_arena_allocator( & Memory::Global_Arena)

	void setup()
	{
		zpl_arena_init_from_allocator( & Global_Arena, zpl_heap(), zpl_megabytes(10) );

		if ( Global_Arena.total_size == 0 )
		{
			zpl_assert_crash( "Failed to reserve memory for Tests:: Global_Arena" );
		}
	}

	void cleanup()
	{
		zpl_arena_free( & Global_Arena);
	}
}

void fatal()
{
	Memory::cleanup();
	zpl_assert_crash("FATAL");
}
