#define BLOAT_IMPL
#include "bloat.hpp"
#include "IO.cpp"
#include "Spec.cpp"


#define Build_Debug 1


void parse_options( int num, char** arguments )
{
	zpl_opts opts;
	zpl_opts_init( & opts, g_allocator, "refactor");
	zpl_opts_add(  & opts, "num",  "num" , "Number of files to refactor"  , ZPL_OPTS_INT   );
	zpl_opts_add(  & opts, "src" , "src" , "File/s to refactor"           , ZPL_OPTS_STRING);
	zpl_opts_add(  & opts, "dst" , "dst" , "File/s post refactor"         , ZPL_OPTS_STRING);
	zpl_opts_add(  & opts, "spec", "spec", "Specification for refactoring", ZPL_OPTS_STRING);

	if (zpl_opts_compile( & opts, num, arguments))
	{
		sw num = 0;
		
		if ( zpl_opts_has_arg( & opts, "num" ) )
		{
			   num            = zpl_opts_integer( & opts, "num", -1 );
			uw global_reserve = num * sizeof(zpl_string) * IO::Path_Size_Largest * 2 + 8;
			
			if ( global_reserve > zpl_megabytes(1) )
			{
				Memory::resize( global_reserve + zpl_megabytes(2) );
			}
			
			zpl_array_init_reserve( IO::Sources,      g_allocator, num );
			zpl_array_init_reserve( IO::Destinations, g_allocator, num );			
		}
		else
		{
			num = 1;
		}
		
		if ( zpl_opts_has_arg( & opts, "src" ) )
		{
			zpl_string opt = zpl_opts_string( & opts, "src", "INVALID SRC ARGUMENT" );
			
			if ( num == 1 )
			{
				IO::Sources[0] = zpl_string_make_length( g_allocator, opt, zpl_string_length( opt) );
			}
			else
			{
				char buffer[ IO::Path_Size_Largest ];

				uw left = num;
 				do
				{
					char* path   = buffer;					
					sw    length = 0;					

					do
					{
						path[length] = *opt;
					} 
					while ( length++, opt++, *opt != ' ' );
									
					IO::Sources[num - left] = zpl_string_make_length( g_allocator, path, length );

					opt++;
				} 
				while ( --left );
			}
		}
		else
		{
			fatal( "-source not provided\n" );
		}

		if ( zpl_opts_has_arg( & opts, "dst" ) )
		{
			zpl_string opt = zpl_opts_string( & opts, "dst", "INVALID DST ARGUMENT" );

 			if ( num == 1 )
			{
				IO::Destinations[0] = zpl_string_make_length( g_allocator, opt, zpl_string_length( opt) );
			}
			else
			{
				char buffer[ IO::Path_Size_Largest ];

				uw left = num;
				do
				{
					char* path   = buffer;
					sw    length = 0;

					do
					{
						path[length] = *opt;
					} 
					while ( length++, opt++, *opt != ' ' );
					
					IO::Destinations[num - left] = zpl_string_make_length( g_allocator, path, length );

					opt++;
				} 
				while ( --left );
			}
		}

		if ( zpl_opts_has_arg( & opts, "spec" ) )
		{
			zpl_string opt = zpl_opts_string( & opts, "spec", "INVALID PATH" );

			IO::Specification = zpl_string_make( g_allocator, "" );
			IO::Specification = zpl_string_append( IO::Specification, opt );
		}
	}
	else
	{
		fatal( "Failed to parse arguments\n" );
	}

	zpl_opts_free( & opts);
}


zpl_arena Refactor_Buffer;

void refactor()
{
	ct char const* include_sig = "#include \"";
	
	struct Token
	{
		u32 Start;
		u32 End;

		zpl_string Sig;
		zpl_string Sub;
	};

	static zpl_array(Token) tokens  = nullptr;
	static zpl_string       current = zpl_string_make( g_allocator, "");

#if Build_Debug
	static zpl_string preview = zpl_string_make( g_allocator, "");
#endif

	static bool Done = false;
	if (! Done)
	{
		zpl_array_init( tokens, g_allocator );
		Done = true;
	}
	else
	{
		zpl_array_clear( tokens );
	}

	// Prepare data and trackers.
	Array_Line src   = IO::get_next_source();
	Array_Line lines = src;

	if ( src == nullptr )
		return;

	const sw num_lines = zpl_array_count( lines);

	sw buffer_size = IO::Current_Size;

	sw   left = num_lines;
	Line line = *lines;
	uw   pos  = 0;

	do
	{
	Continue_Line:

		// Includes to ignore
		{
			Spec::Entry* ignore       = Spec::Ignore_Words;
			sw           ignores_left = zpl_array_count( Spec::Ignore_Words);

			do
			{
				if ( include_sig[0] != line[0] )
					continue;

				u32 sig_length = zpl_string_length( ignore->Sig );

				current = zpl_string_set( current, include_sig );
				current = zpl_string_append_length( current, line, sig_length );
				current = zpl_string_append_length( current, "\"", 2 );
				// Formats current into: #include "<ignore->Sig>"

				if ( zpl_string_are_equal( ignore->Sig, current ) )
				{
					log_fmt("\nIgnored   %-81s line %d", current, num_lines - left );

					const sw length = zpl_string_length( current );

					line += length;
					pos  += length;

					// Force end of line.
					while ( line != '\0' )
					{
						line++;
						pos++;
					}

					goto Skip;
				}
			}
			while ( ignore++, --ignores_left );
		}

		// Word Ignores
		{
			Spec::Entry* ignore       = Spec::Ignore_Words;
			sw           ignores_left = zpl_array_count( Spec::Ignore_Words);

			do
			{
				if ( ignore->Sig[0] != line[0] )
					continue;

				zpl_string_clear( current );

				u32 sig_length = zpl_string_length( ignore->Sig );
				    current    = zpl_string_append_length( current, line, sig_length );

				if ( zpl_string_are_equal( ignore->Sig, current ) )
				{
					char before = line[-1];
					char after  = line[sig_length];

					if (   zpl_char_is_alphanumeric( before ) || before == '_'
						|| zpl_char_is_alphanumeric( after  ) || after  == '_' )
					{
						continue;
					}

					log_fmt("\nIgnored   %-81s line %d", current, num_lines - left );

					line += sig_length;
					pos  += sig_length;
					goto Skip;
				}
			}
			while ( ignore++, --ignores_left );
		}

		// Namespace Ignores
		{
			Spec::Entry* ignore       = Spec::Ignore_Namespaces;
			sw           ignores_left = zpl_array_count( Spec::Ignore_Namespaces);

			do
			{
				if ( ignore->Sig[0] != line[0] )
					continue;

				zpl_string_clear( current );

				u32 sig_length = zpl_string_length( ignore->Sig );
				    current    = zpl_string_append_length( current, line, sig_length );
				
				if ( zpl_string_are_equal( ignore->Sig, current ) )
				{
					u32   length     = sig_length;
					char* ns_content = line + sig_length;

					while ( zpl_char_is_alphanumeric( ns_content[0] ) || ns_content[0] == '_' )
					{
						length++;
						ns_content++;
					}

				#if Build_Debug
					zpl_string_clear( preview );
					preview = zpl_string_append_length( preview, line, length );
					log_fmt("\nIgnored   %-40s %-40s line %d", preview, ignore->Sig,  - left);
				#endif

					line += length;
					pos  += length;
					goto Skip;
				}
			}
			while ( ignore++, --ignores_left );
		}

		// Includes to match
		{
			Spec::Entry* include = Spec::Includes;

			sw includes_left = zpl_array_count ( Spec::Includes);

			do
			{
				if ( include_sig[0] != line[0] )
					continue;

				u32 sig_length = zpl_string_length( include->Sig );

				current = zpl_string_set( current, include_sig );
				current = zpl_string_append_length( current, line, sig_length );
				current = zpl_string_append_length( current, "\"", 2 );
				// Formats current into: #include "<ignore->Sig>"

				if ( zpl_string_are_equal( include->Sig, current ) )
				{
					Token entry {};

					const sw length = zpl_string_length( current );

					entry.Start = pos;
					entry.End   = pos + length;
					entry.Sig   = include->Sig;

					if ( include->Sub != nullptr )
					{
						entry.Sub    = include->Sub;
						buffer_size += zpl_string_length( entry.Sub) - sig_length;
					}

					zpl_array_append( tokens, entry );

					log_fmt("\nFound     %-81s line %d", current, num_lines - left);

					line += length;
					pos  += length;

					// Force end of line.
					while ( line != '\0' )
					{
						line++;
						pos++;
					}

					goto Skip;
				}
			}
			while ( include++, --includes_left );
		}

		// Words to match
		{
			Spec::Entry* word       = Spec::Words;
			sw           words_left = zpl_array_count ( Spec::Words);

			do
			{
				if ( word->Sig[0] != line[0] )
					continue;

				zpl_string_clear( current );

				sw sig_length = zpl_string_length( word->Sig);
				   current    = zpl_string_append_length( current, line, sig_length );

				if ( zpl_string_are_equal( word->Sig, current ) )
				{
					char before = line[-1];
					char after  = line[sig_length];

					if (   zpl_char_is_alphanumeric( before ) || before == '_'
						|| zpl_char_is_alphanumeric( after  ) || after  == '_' )
					{
						continue;
					}

					Token entry {};

					entry.Start = pos;
					entry.End   = pos + sig_length;
					entry.Sig   = word->Sig;

					if ( word->Sub != nullptr )
					{
						entry.Sub    = word->Sub;
						buffer_size += zpl_string_length( entry.Sub) - sig_length;
					}

					zpl_array_append( tokens, entry );

					log_fmt("\nFound     %-81s line %d", current, num_lines - left);

					line += sig_length;
					pos  += sig_length;
					goto Skip;
				}
			}
			while ( word++, --words_left );
		}

		// Namespaces to match
		{
			Spec::Entry* nspace = Spec::Namespaces;

			sw nspaces_left = zpl_array_count( Spec::Namespaces);

			do
			{
				if ( nspace->Sig[0] != line[0] )
					continue;

				zpl_string_clear( current );

				u32 sig_length = zpl_string_length( nspace->Sig );
				    current    = zpl_string_append_length( current, line, sig_length );

				if ( zpl_string_are_equal( nspace->Sig, current ) )
				{
					u32   length     = sig_length;
					char* ns_content = line + sig_length;

					while ( zpl_char_is_alphanumeric( ns_content[0] ) || ns_content[0] == '_' )
					{
						length++;
						ns_content++;
					}

					Token entry {};

					entry.Start = pos;
					entry.End   = pos + length;
					entry.Sig   = nspace->Sig;

					buffer_size += sig_length;

					if ( nspace->Sub != nullptr )
					{
						entry.Sub    = nspace->Sub;
						buffer_size += zpl_string_length( entry.Sub ) - length;
					}

					zpl_array_append( tokens, entry );

				#if Build_Debug
					zpl_string_clear( preview );
					preview = zpl_string_append_length( preview, line, length);
					log_fmt("\nFound     %-40s %-40s line %d", preview, nspace->Sig, num_lines - left);
				#endif

					line += length;
					pos  += length;
					goto Skip;
				}
			}
			while ( nspace++, --nspaces_left );
		}

	Skip:
		if ( line != '\0' )
			goto Continue_Line;
	} 
	while ( lines++, line = *lines, left );

	// Prep data for building the content
	left = IO::Current_Size;

	char* content = IO::Current_Content; 

	// Generate the refactored file content.
	static zpl_string 
	refactored = zpl_string_make_reserve( zpl_arena_allocator( & Refactor_Buffer ), buffer_size );
	{
		Token* entry        = tokens;
		sw     previous_end = 0;

		do
		{
			sw segment_length = entry->Start - previous_end;
			sw sig_length     = zpl_string_length( entry->Sig );

			// Append between tokens
			refactored  = zpl_string_append_length( refactored, line, segment_length );
			line       += segment_length + sig_length;

			segment_length = entry->End - entry->Start - sig_length;

			// Append token
			if ( entry->Sub )
				refactored  = zpl_string_append( refactored, entry->Sub );

			refactored  = zpl_string_append_length( refactored, line, segment_length );
			line       += segment_length;

			previous_end = entry->End;
			entry++;
		}
		while ( --left );

		entry--;

		if ( entry->End < IO::Current_Size ) 
		{
			refactored = zpl_string_append_length( refactored, line, IO::Current_Size - entry->End );
		}
	}

	IO::write( refactored );
}

int main( int num, char** arguments )
{
	Memory::setup();
	
	parse_options( num, arguments);

	IO::prepare();

	// Just reserving more than we'll ever problably need.
	zpl_arena_init_from_allocator( & Refactor_Buffer, zpl_heap(), IO::Largest_Src_Size * 4 + 8);

	Spec::parse();

	sw left = zpl_array_count( IO::Sources );
	do
	{
		refactor();
	} 
	while ( --left );

	zpl_arena_free( & Refactor_Buffer );

	Spec::  cleanup();
	IO::    cleanup();
	Memory::cleanup();
}
