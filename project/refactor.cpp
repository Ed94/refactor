#include "Bloat.cpp"
#include "IO.cpp"
#include "Spec.cpp"


#define Build_Debug 1


void parse_options( int num, char** arguments )
{
	zpl_opts opts;
	zpl_opts_init( & opts, zpl_heap(), "refactor");
	zpl_opts_add(  & opts, "num",  "num" , "Number of files to refactor"  , ZPL_OPTS_INT   );
	zpl_opts_add(  & opts, "src" , "src" , "File/s to refactor"           , ZPL_OPTS_STRING);
	zpl_opts_add(  & opts, "dst" , "dst" , "File/s post refactor"         , ZPL_OPTS_STRING);
	zpl_opts_add(  & opts, "spec", "spec", "Specification for refactoring", ZPL_OPTS_STRING);

#if Build_Debug
	zpl_opts_add( & opts, "debug", "debug", "Allows for wait to attach", ZPL_OPTS_FLAG);
#endif

	if (opts_custom_compile( & opts, num, arguments))
	{
		sw num = 0;

	#if Build_Debug
		if ( zpl_opts_has_arg( & opts, "debug" ) )
		{
			zpl_printf("Will wait (pause available for attachment)");
			char pause = getchar();
		}
	#endif
		
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

			zpl_array_init_reserve( IO::Sources,      g_allocator, 1 );
			zpl_array_init_reserve( IO::Destinations, g_allocator, 1 );	
		}

		zpl_printf("NUM IS: %d", num);
		
		if ( zpl_opts_has_arg( & opts, "src" ) )
		{
			zpl_string opt = zpl_opts_string( & opts, "src", "INVALID SRC ARGUMENT" );
			
			if ( num == 1 )
			{
				zpl_string path = zpl_string_make_length( g_allocator, opt, zpl_string_length( opt ));
				zpl_array_append( IO::Sources, path );
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
					while ( length++, opt++, *opt != ' ' && *opt != '\0' );
									
					zpl_string path_string = zpl_string_make_length( g_allocator, path, length );
					zpl_array_append( IO::Sources, path_string );

					opt++;
				} 
				while ( --left );
			}
		}
		else
		{
			fatal( "-src not provided\n" );
		}

		if ( zpl_opts_has_arg( & opts, "dst" ) )
		{
			zpl_string opt = zpl_opts_string( & opts, "dst", "INVALID DST ARGUMENT" );

 			if ( num == 1 )
			{
				zpl_string path = zpl_string_make_length( g_allocator, opt, zpl_string_length( opt) );
				zpl_array_append( IO::Destinations, path );
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
					while ( length++, opt++, *opt != ' ' && *opt != '\0' );
					
					zpl_string path_string = zpl_string_make_length( g_allocator, path, length );
					zpl_array_append( IO::Destinations, path_string );

					opt++;
				} 
				while ( --left );

				if ( zpl_array_count(IO::Destinations) != zpl_array_count( IO::Sources ) )
				{
					fatal("-dst count must match -src count");
				}
			}
		}
		else
		{
			uw left = num;
			do
			{
				zpl_array_append( IO::Destinations, IO::Sources[num - left] );
			} 
			while ( --left );
		}

		if ( zpl_opts_has_arg( & opts, "spec" ) )
		{
			zpl_string opt = zpl_opts_string( & opts, "spec", "INVALID PATH" );

			IO::Specification = zpl_string_make( g_allocator, "" );
			IO::Specification = zpl_string_append( IO::Specification, opt );
		}
		else
		{
			fatal( "-spec not provided\n" );
		}
	}
	else
	{
		zpl_printf("\nArguments: ");
		for ( int index = 0; index < num; index++)
		{
			zpl_printf("\nArg[%d]: %s", index, arguments[index]);
		}
		fatal( "Failed to parse arguments\n" );
	}

	zpl_opts_free( & opts);
}


zpl_arena Refactor_Buffer;

void refactor()
{
	ct static char const* include_sig = "include";
	
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
	char const* src = IO::get_next_source();

	if ( src == nullptr )
		return;

	log_fmt("\n\nRefactoring: %s", IO::Sources[IO::Current]);

	sw buffer_size = IO::Current_Size;

	sw   left = buffer_size;
	uw   col  = 0;
	uw   line = 0;

	#define pos (IO::Current_Size - left)

	#define move_forward( Amount_ ) \
		left -= Amount_; \
		col  += Amount_; \
		src  += Amount_  \

	do
	{
		// Includes to ignore
		{
			Spec::Entry* ignore       = Spec::Ignore_Includes;
			sw           ignores_left = zpl_array_count( Spec::Ignore_Includes);

			for ( ; ignores_left; ignores_left--, ignore++ )
			{
				if ( include_sig[0] != src[0] )
					continue;

				if ( zpl_strncmp( include_sig, src, sizeof(include_sig) - 1 ) != 0 )
				{
					break;
				}

				src += sizeof(include_sig) - 1;

				// Ignore whitespace
				while ( zpl_char_is_space( src[0] ) || src[0] == '\"' || src[0] == '<' )
				{
					src++;
				}

				u32 sig_length = zpl_string_length( ignore->Sig );

				zpl_string_clear( current );
				current = zpl_string_append_length( current, ignore->Sig, sig_length );

				if ( zpl_string_are_equal( ignore->Sig, current ) )
				{
					log_fmt("\nIgnored   %-81s line %d, col %d", current, line, col );

					const sw length = zpl_string_length( current );

					// The + 1 is for the closing " or > of the include
					move_forward( length + 1 );

					// Force end of line.
					while ( src[0] != '\n' )
					{
						move_forward( 1 );
					}

					goto Skip;
				}
			}
		}

		// Word Ignores
		{
			Spec::Entry* ignore       = Spec::Ignore_Words;
			sw           ignores_left = zpl_array_count( Spec::Ignore_Words);

			for ( ; ignores_left; ignores_left--, ignore++ )
			{
				if ( ignore->Sig[0] != src[0] )
					continue;

				zpl_string_clear( current );

				u32 sig_length = zpl_string_length( ignore->Sig );
				    current    = zpl_string_append_length( current, src, sig_length );

				if ( zpl_string_are_equal( ignore->Sig, current ) )
				{
					char before = src[-1];
					char after  = src[sig_length];

					if (   zpl_char_is_alphanumeric( before ) || before == '_'
						|| zpl_char_is_alphanumeric( after  ) || after  == '_' )
					{
						continue;
					}

					log_fmt("\nIgnored   %-81s line %d, col %d", current, line, col );

					move_forward( sig_length );
					goto Skip;
				}
			}
		}

		// Namespace Ignores
		{
			Spec::Entry* ignore       = Spec::Ignore_Namespaces;
			sw           ignores_left = zpl_array_count( Spec::Ignore_Namespaces);

			for ( ; ignores_left; ignores_left--, ignore++ )
			{
				if ( ignore->Sig[0] != src[0] )
				{
					ignore++;
					continue;
				}

				zpl_string_clear( current );

				u32 sig_length = zpl_string_length( ignore->Sig );
				    current    = zpl_string_append_length( current, src, sig_length );
				
				if ( zpl_string_are_equal( ignore->Sig, current ) )
				{
					u32         length     = sig_length;
					char const* ns_content = src + sig_length;

					while ( zpl_char_is_alphanumeric( ns_content[0] ) || ns_content[0] == '_' )
					{
						length++;
						ns_content++;
					}

				#if Build_Debug
					zpl_string_clear( preview );
					preview = zpl_string_append_length( preview, src, length );
					log_fmt("\nIgnored   %-40s %-40s line %d, column %d", preview, ignore->Sig, line, col );
				#endif

					move_forward( length );
					goto Skip;
				}
			}
		}

		// Includes to match
		{
			Spec::Entry* include = Spec::Includes;

			sw includes_left = zpl_array_count ( Spec::Includes);

			for ( ; includes_left; includes_left--, include++ )
			{
				if ( include_sig[0] != src[0] )
					continue;

				if ( zpl_strncmp( include_sig, src, sizeof(include_sig) - 1 ) != 0 )
				{
					break;
				}

				src += sizeof(include_sig) - 1;

				// Ignore whitespace
				while ( zpl_char_is_space( src[0] ) || src[0] == '\"' || src[0] == '<' )
				{
					src++;
				}

				u32 sig_length = zpl_string_length( include->Sig );

				zpl_string_clear( current );
				current = zpl_string_append_length( current, include->Sig, sig_length );

				if ( zpl_string_are_equal( include->Sig, current ) )
				{
					Token entry {};

					entry.Start = pos;
					entry.End   = pos + sig_length;
					entry.Sig   = include->Sig;

					if ( include->Sub != nullptr )
					{
						entry.Sub    = include->Sub;
						buffer_size += zpl_string_length( entry.Sub) - sig_length;
					}

					zpl_array_append( tokens, entry );

					log_fmt("\nFound     %-81s line %d, column %d", current, line, col );

					move_forward( sig_length );

					// Force end of line.
					while ( src[0] != '\n' )
					{
						move_forward( 1 );
					}

					goto Skip;
				}
			}
		}

		// Words to match
		{
			Spec::Entry* word       = Spec::Words;
			sw           words_left = zpl_array_count ( Spec::Words);

			for ( ; words_left; words_left--, word++ )
			{
				if ( word->Sig[0] != src[0] )
					continue;

				zpl_string_clear( current );

				sw sig_length = zpl_string_length( word->Sig);
				   current    = zpl_string_append_length( current, src, sig_length );

				if ( zpl_string_are_equal( word->Sig, current ) )
				{
					char before = src[-1];
					char after  = src[sig_length];

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

					log_fmt("\nFound     %-81s line %d, column %d", current, line, col );

					move_forward( sig_length );
					goto Skip;
				}
			}
		}

		// Namespaces to match
		{
			Spec::Entry* nspace = Spec::Namespaces;

			sw nspaces_left = zpl_array_count( Spec::Namespaces);

			for ( ; nspaces_left; nspaces_left--, nspace++ )
			{
				if ( nspace->Sig[0] != src[0] )
					continue;

				zpl_string_clear( current );

				u32 sig_length = zpl_string_length( nspace->Sig );
				    current    = zpl_string_append_length( current, src, sig_length );

				if ( zpl_string_are_equal( nspace->Sig, current ) )
				{
					u32         length     = sig_length;
					char const* ns_content = src + sig_length;

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
					preview = zpl_string_append_length( preview, src, length);
					log_fmt("\nFound     %-40s %-40s line %d, column %d", preview, nspace->Sig, line, col );
				#endif

					move_forward( length );
					goto Skip;
				}
			}
		}

	Skip:
		if ( src[0] == '\n' )
		{
			line++;
			col = 0;
		}

		src++;
	} 
	while ( --left );

	if (zpl_array_count( tokens ) == 0)
	{
		return;
	}

	// Prep data for building the content
	left = zpl_array_count( tokens);

	char* content = IO::Current_Content; 

	zpl_string refactored = zpl_string_make_reserve( zpl_arena_allocator( & Refactor_Buffer ), buffer_size );

	// Generate the refactored file content.
	{
		Token* entry        = tokens;
		sw     previous_end = 0;

		do
		{
			sw segment_length = entry->Start - previous_end;
			sw sig_length     = zpl_string_length( entry->Sig );

			// Append between tokens
			refactored  = zpl_string_append_length( refactored, content, segment_length );
			content    += segment_length + sig_length;

			segment_length = entry->End - entry->Start - sig_length;

			// Append token
			if ( entry->Sub )
			{
				refactored = zpl_string_append( refactored, entry->Sub );
			}

			refactored  = zpl_string_append_length( refactored, content, segment_length );
			content    += segment_length;

			previous_end = entry->End;
			entry++;
		}
		while ( --left > 0 );

		entry--;

		if ( entry->End < IO::Current_Size ) 
		{
			refactored = zpl_string_append_length( refactored, content, IO::Current_Size - 1 - entry->End );
		}
	}

	IO::write( refactored );

	zpl_free_all( zpl_arena_allocator( & Refactor_Buffer ));
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

		zpl_printf("\nRefactored: %s", IO::Sources[IO::Current]);
	} 
	while ( --left );

	zpl_arena_free( & Refactor_Buffer );

	Spec::  cleanup();
	IO::    cleanup();
	Memory::cleanup();
}
