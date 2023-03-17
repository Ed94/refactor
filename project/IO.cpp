#include "IO.hpp"


namespace IO
{
	using array_string = zpl_array( zpl_string );
	
	namespace StaticData
	{
		array_string Sources       = nullptr;
		array_string Destinations  = nullptr;
		zpl_string   Specification = nullptr;
		
		// Current source and destination index.
		// Used to keep track of which file get_next_source or write refer to.
		sw    Current          = -1;
		char* Current_Content  = nullptr;
		uw    Current_Size     = 0;
		uw    Largest_Src_Size = 0;

		zpl_arena  MemPerist;
	}
	using namespace StaticData;
	
	 
	void prepare()
	{
		const sw num_srcs = zpl_array_count( Sources );
		
		// Determine the largest content size.
		sw          left = num_srcs;
		zpl_string* path = Sources;
		do
		{
			zpl_file       src   = {};
			zpl_file_error error = zpl_file_open( & src, *path );
			                       
			if ( error != ZPL_FILE_ERROR_NONE )
			{
				fatal("IO::Prepare - Could not open source file: %s", *path );
			}
			
			const sw fsize = zpl_file_size( & src );
			                 
			if ( fsize > Largest_Src_Size )
			{
				Largest_Src_Size = fsize;
			}

			zpl_file_close( & src );
		}
		while ( left--, left > 1 );
			
		uw persist_size = ZPL_ARRAY_GROW_FORMULA( Largest_Src_Size );
			
		zpl_arena_init_from_allocator( & MemPerist, zpl_heap(), persist_size );
	}
	
	void cleanup()
	{
		zpl_arena_free( & MemPerist );
	}

	Array_Line get_specification()
	{
		zpl_file       file {};
		zpl_file_error error = zpl_file_open( & file, Specification);

		if ( error != ZPL_FILE_ERROR_NONE )
		{
			fatal("Could not open the specification file: %s", Specification);
		}

		sw fsize = scast( sw, zpl_file_size( & file ) );

		if ( fsize <= 0 )
		{
			fatal("No content in specificaiton to process");
		}

		char* content = rcast( char*, zpl_alloc( zpl_arena_allocator( & MemPerist), fsize + 1) );

		zpl_file_read( & file, content, fsize);
		zpl_file_close( & file );

		content[fsize] = 0;

		Array_Line lines = zpl_str_split_lines( zpl_arena_allocator( & MemPerist ), content, false );
		return lines;
	}

	char* get_next_source()
	{
		zpl_memset( MemPerist.physical_start, 0, MemPerist.total_allocated);
		MemPerist.total_allocated = 0;
		MemPerist.temp_count      = 0;

		Current++;

		zpl_file       file {};
		zpl_file_error error = zpl_file_open( & file, Sources[Current]);

		if ( error != ZPL_FILE_ERROR_NONE )
		{
			fatal("IO::get_next_source - Could not open the source file: %s", Sources[Current]);
		}

		auto size = zpl_file_size( & file );
		Current_Size = scast( sw, size );

		if ( Current_Size <= 0 )
			return nullptr;

		Current_Content = rcast( char* , zpl_alloc( zpl_arena_allocator( & MemPerist), Current_Size + 1) );

		zpl_file_read( & file, Current_Content, Current_Size );
		zpl_file_close( & file );

		Current_Content[Current_Size] = 0;
		Current_Size++;

		return Current_Content;
	}

	void write( zpl_string refacotred )
	{
		if ( refacotred == nullptr)
			return;

		zpl_string dst = Destinations[Current];

		zpl_file       file_dest {};
		zpl_file_error error =  zpl_file_create( & file_dest, dst );

		if ( error != ZPL_FILE_ERROR_NONE )
		{
			fatal( "Unable to open destination file: %s\n", dst );
		}

		zpl_file_write( & file_dest, refacotred, zpl_string_length(refacotred) );

		zpl_file_close( & file_dest );
	}
}
