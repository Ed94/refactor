#include "Spec.hpp"

#include "IO.hpp"



namespace Spec
{
	ct uw Array_Reserve_Num = zpl_kilobytes(4);
	ct uw Token_Max_Length  = zpl_kilobytes(1); 

	namespace StaticData
	{
		Array_Entry Ignore_Includes;
		Array_Entry Ignore_Words;
		Array_Entry Ignore_Regexes;
		Array_Entry Ignore_Namespaces;

		Array_Entry Includes;
		Array_Entry Words;
		Array_Entry Regexes;
		Array_Entry Namespaces;

		u32 Sig_Smallest = Token_Max_Length;
	}
	using namespace StaticData;

	void cleanup()
	{
		zpl_array_free( Ignore_Includes );
		zpl_array_free( Ignore_Words );
		zpl_array_free( Ignore_Namespaces );
		zpl_array_free( Includes );
		zpl_array_free( Words );
		zpl_array_free( Namespaces );
	}


	// Helper function for process().
	forceinline
	void find_next_token( Tok& type, zpl_string& token, char*& line, u32& length )
	{
		zpl_string_clear( token );
		length = 0;

	#define current line[length]
		if (type == Tok::Include)
		{
			// Allows for '.'
			while ( zpl_char_is_alphanumeric( current ) 
				|| current == '_' 
				|| current == '.' )
			{
				length++;
			}
		}
		else
		{
			while ( zpl_char_is_alphanumeric( current ) || current == '_' )
			{
				length++;
			}
		}
	#undef current

		if ( length == 0 )
		{
			fatal("Failed to find valid initial token");
		}

		token  = zpl_string_append_length( token, line, length );
		line  += length;
	}

	void parse()
	{
		static zpl_string token = zpl_string_make_reserve( g_allocator, zpl_kilobytes(1));

		static bool Done = false;
		if (Done)
		{
			zpl_array_clear( Ignore_Includes );
			zpl_array_clear( Ignore_Words );
			zpl_array_clear( Ignore_Namespaces );
			zpl_array_clear( Includes );
			zpl_array_clear( Words );
			zpl_array_clear( Namespaces );
		}
		else
		{
			Done = true;

			zpl_array_init_reserve( Ignore_Includes,   zpl_heap(), Array_Reserve_Num );
			zpl_array_init_reserve( Ignore_Words,      zpl_heap(), Array_Reserve_Num );
			zpl_array_init_reserve( Ignore_Namespaces, zpl_heap(), Array_Reserve_Num );
			zpl_array_init_reserve( Includes,          zpl_heap(), Array_Reserve_Num );
			zpl_array_init_reserve( Words,             zpl_heap(), Array_Reserve_Num );
			zpl_array_init_reserve( Namespaces,        zpl_heap(), Array_Reserve_Num );
		}

		Array_Line lines = IO::get_specification();

		sw left = zpl_array_count( lines );

		if ( left == 0 )
		{
			fatal("Spec::parse: lines array imporoperly setup");
		}

		// Skip the first line as its the version number and we only support __VERSION 1.
		left--;
		lines++;

		do
		{
			char* line = * lines;

			// Ignore line if its a comment
			if ( line[0] == '/' && line[1] == '/')
			{
				lines++;
				continue;
			}

			// Remove indent
			{
				while ( zpl_char_is_space( line[0] ) )
					line++;

				if ( line[0] == '\0' )
				{
					lines++;
					continue;
				}
			}

			u32   length = 0;
			Tok   type   = Tok::Num_Tok;
			bool  ignore = false;
			Entry entry {};


			// Find a valid token
			find_next_token( type, token, line, length );

			// Will be reguarded as an ignore.
			if ( is_tok( Tok::Not, token, length ))
			{
				ignore = true;

				while ( zpl_char_is_space( line[0] ) )
					line++;

				if ( line[0] == '\0' )
				{
					lines++;
					continue;
				}

				// Find the category token
				find_next_token( type, token, line, length );
			}

			if ( is_tok( Tok::Word, token, length ) )
			{
				type = Tok::Word;
			}
			else if ( is_tok( Tok::Namespace, token, length ) )
			{
				type = Tok::Namespace;
			}
			else if ( is_tok( Tok::Include, token, length ))
			{
				type = Tok::Include;
			}
			else
			{
				log_fmt( "Sec::Parse - Unsupported keyword: %s on line: %d", token, zpl_array_count(lines) - left );
				lines++;
				continue;
			}

			// Find the first argument
			while ( zpl_char_is_space( line[0] ) )
				line++;

			if ( line[0] == '\0' )
			{
				lines++;
				continue;
			}
		
			find_next_token( type, token, line, length );

			// First argument is signature.
			entry.Sig = zpl_string_make_length( g_allocator, token, length );

			if ( length < StaticData::Sig_Smallest )
				StaticData::Sig_Smallest = length;

			if ( line[0] == '\0' || ignore )
			{
				switch ( type )
				{
					case Tok::Word:
						if ( ignore)
							zpl_array_append( Ignore_Words, entry );
							
						else
							zpl_array_append( Words, entry );
					break;

					case Tok::Namespace:
						if ( ignore)
							zpl_array_append( Ignore_Namespaces, entry );

						else
							zpl_array_append( Namespaces, entry );
					break;

					case Tok::Include:
						if ( ignore)
							zpl_array_append( Ignore_Includes, entry );

						else 
							zpl_array_append( Includes, entry );
					break;
				}

				lines++;
				continue;
			}

			// Look for second argument indicator
			{
				bool bSkip = false;

				while ( line[0] != ',' )
				{
					if ( line[0] == '\0' )
					{
						switch ( type )
						{
							case Tok::Word:
								zpl_array_append( Words, entry );
							break;		

							case Tok::Namespace:
								zpl_array_append( Namespaces, entry );
							break;

							case Tok::Include:
								zpl_array_append( Includes, entry );
							break;
						}

						bSkip = true;
						break;
					}

					line++;
				}

				if ( bSkip )
				{
					lines++;
					continue;
				}
			}

			// Eat the argument delimiter.
			line++;

			// Remove spacing
			{
				bool bSkip = true;

				while ( zpl_char_is_space( line[0] ) )
					line++;

				if ( line[0] == '\0' )
				{
					switch ( type )
					{
						case Tok::Word:
							zpl_array_append( Words, entry );
						break;		

						case Tok::Namespace:
							zpl_array_append( Namespaces, entry );
						break;

						case Tok::Include:
							zpl_array_append( Includes, entry );
						break;
					}

					lines++;
					continue;
				}
			}

			find_next_token( type, token, line, length );

			// Second argument is substitute.
			entry.Sub = zpl_string_make_length( g_allocator, token, length );

			switch ( type )
			{
				case Tok::Word:
					zpl_array_append( Words, entry );
					lines++;
					continue;

				case Tok::Namespace:
					zpl_array_append( Namespaces, entry );
					lines++;
					continue;

				case Tok::Include:
					zpl_array_append( Includes, entry );
					lines++;
					continue;
			}
		}
		while ( --left );
	}
}
