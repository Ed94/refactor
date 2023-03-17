#define BLOAT_IMPL
#include "bloat.hpp"



namespace Memory
{
	static zpl_arena Global_Arena {};
	
	void setup()
	{
		zpl_arena_init_from_allocator( & Global_Arena, zpl_heap(), zpl_megabytes(2) );

		if ( Global_Arena.total_size == 0 )
		{
			zpl_assert_crash( "Failed to reserve memory for Tests:: Global_Arena" );
		}
	}
	
	void resize( uw new_size )
	{
		void* new_memory = zpl_resize( zpl_heap(), Global_Arena.physical_start, Global_Arena.total_size, new_size );		
		
		if ( new_memory == nullptr )
		{
			fatal("Failed to resize global arena!");
		}
		
		Global_Arena.physical_start = new_memory;
		Global_Arena.total_size     = new_size;
	}

	void cleanup()
	{
		zpl_arena_free( & Global_Arena);
	}	
}
