#define ZPL_IMPLEMENTATION
#include "bloat.hpp"


namespace File
{
	zpl_string        Source        = nullptr;
	zpl_string        Destination   = nullptr;
	
	zpl_file_contents Content {};

	zpl_arena Buffer;

	void cleanup()
	{
		zpl_arena_free( & Buffer );
	}

	void read()
	{
		zpl_file file_src = {};

		Content.allocator = g_allocator;

		zpl_file_error error_src  = zpl_file_open( & file_src, Source );

		if ( error_src == ZPL_FILE_ERROR_NONE ) 
		{
			zpl_isize fsize = cast(zpl_isize) zpl_file_size( & file_src);

			if ( fsize > 0 ) 
			{
				zpl_arena_init_from_allocator( & Buffer, zpl_heap(), (fsize + fsize % 64) * 4 );
				
				Content.data = zpl_alloc( zpl_arena_allocator( & Buffer), fsize);
				Content.size = fsize;

				zpl_file_read_at ( & file_src, Content.data, Content.size, 0);
			}

			zpl_file_close( & file_src);
		}

		if ( Content.data == nullptr )
		{
			zpl_printf( "Unable to open source file: %s\n", Source );
			fatal();
		}
	}

	void write(zpl_string refactored)
	{
		if ( refactored == nullptr)
			return;

		zpl_file       file_dest {};
		zpl_file_error error =  zpl_file_create( & file_dest, Destination );

		if ( error != ZPL_FILE_ERROR_NONE )
		{
			zpl_printf( "Unable to open destination file: %s\n", Destination );
			fatal();
		}

		zpl_file_write( & file_dest, refactored, zpl_string_length(refactored) );
	}
}

namespace Spec
{
	zpl_string File;

	enum Tok 
	{
		Not,
		Namespace,
		Word,

		Num_Tok
	};

	ct 
	char const* str_tok( Tok tok )
	{
		ct 
		char const*	tok_to_str[ Tok::Num_Tok ] = 
		{
			"not",
			"namespace",
			"word",
		};

		return tok_to_str[ tok ];
	}

	ct 
	c8 strlen_tok( Tok tok )
	{
		ct 
		const u8 tok_to_len[ Tok::Num_Tok ] = 
		{
			3,
			9,
			4,
		};

		return tok_to_len[ tok ];
	}

	forceinline
	bool is_tok( Tok tok, zpl_string str, u32 length )
	{
		char const* tok_str = str_tok(tok);
		const u8    tok_len = strlen_tok(tok);

		if ( tok_len != length)
			return false;

		s32 result = zpl_strncmp( tok_str, str, tok_len );

		return result == 0;
	}

	struct Entry
	{
		zpl_string Sig = nullptr; // Signature
		zpl_string Sub = nullptr; // Substitute
	};

	zpl_arena        Buffer {};
	zpl_array(Entry) Word_Ignores;
	zpl_array(Entry) Namespace_Ignores;
	zpl_array(Entry) Words;
	zpl_array(Entry) Namespaces;

	u32 Sig_Smallest = zpl_kilobytes(1);

	forceinline
	void find_next_token( zpl_string& token, char*& line, u32& length )
	{
		zpl_string_clear( token );
		length = 0;
		while ( zpl_char_is_alphanumeric( line[length] ) || line[length] == '_' )
		{
			length++;
		}

		if ( length == 0 )
		{
			zpl_printf("Failed to find valid initial token");
			fatal();
		}

		token  = zpl_string_append_length( token, line, length );
		line  += length;
	}

	void process()
	{
		char* content;

		zpl_array(char*) lines;

		// Get the contents of the file.
		{
             zpl_file       file {};
             zpl_file_error error = zpl_file_open( & file, File);

			 if ( error != ZPL_FILE_ERROR_NONE )
			 {
				zpl_printf("Could not open the specification file: %s", File);
				fatal();
			 }

             sw fsize = scast( sw, zpl_file_size( & file ) );

			 if ( fsize <= 0 )
			 {
				zpl_printf("No content in specificaiton to process");
				fatal();
			 }

			 zpl_arena_init_from_allocator( & Buffer, zpl_heap(), (fsize + fsize % 64) * 10 + zpl_kilobytes(1) );

             char* content = rcast( char*, zpl_alloc( zpl_arena_allocator( & Buffer), fsize + 1) );

             zpl_file_read( & file, content, fsize);

             content[fsize] = 0;

             lines = zpl_str_split_lines( zpl_arena_allocator( & Buffer ), content, false );

             zpl_file_close( & file );
		}
		
		sw left = zpl_array_count( lines );

		if ( left == 0 )
		{
			zpl_printf("Spec::process: lines array imporoperly setup");
			fatal();
		}

		// Skip the first line as its the version number and we only support __VERSION 1.
		left--;
		lines++;

		zpl_array_init( Word_Ignores,      zpl_arena_allocator( & Buffer));
		zpl_array_init( Namespace_Ignores, zpl_arena_allocator( & Buffer));
		zpl_array_init( Words,             zpl_arena_allocator( & Buffer));
		zpl_array_init( Namespaces,        zpl_arena_allocator( & Buffer));

		// Limiting the maximum output of a token to 1 KB
		zpl_string token = zpl_string_make_reserve( zpl_arena_allocator( & Buffer), zpl_kilobytes(1));

		while ( left-- )
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

			u32 length = 0;

			// Find a valid token
			find_next_token( token, line, length );

			Tok   type   = Tok::Num_Tok;
			bool  ignore = false;
			Entry entry {};

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
				find_next_token( token, line, length );
			}

			if ( is_tok( Tok::Namespace, token, length ) )
			{
				type = Tok::Namespace;
			}
			else if ( is_tok( Tok::Word, token, length ) )
			{
				type = Tok::Word;
			}

			// Parse line.
			{
				// Find first argument
				{

					while ( zpl_char_is_space( line[0] ) )
						line++;

					if ( line[0] == '\0' )
					{
						lines++;
						continue;
					}
				}

				find_next_token( token, line, length );

				// First argument is signature.
				entry.Sig = zpl_string_make_length( g_allocator, token, length );

				if ( length < Sig_Smallest )
					Sig_Smallest = length;

				if ( line[0] == '\0' || ignore )
				{
					switch ( type )
					{
						case Tok::Namespace:
							if ( ignore)
								zpl_array_append( Namespace_Ignores, entry );

							else
								zpl_array_append( Namespaces, entry );
						break;

						case Tok::Word:
							if ( ignore)
							{
								zpl_array_append( Word_Ignores, entry );
								u32 test = zpl_array_count( Word_Ignores );
							}
								

							else
								zpl_array_append( Words, entry );
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
								case Tok::Namespace:
									zpl_array_append( Namespaces, entry );
								break;

								case Tok::Word:
									zpl_array_append( Words, entry );
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
							case Tok::Namespace:
								zpl_array_append( Namespaces, entry );
							break;

							case Tok::Word:
								zpl_array_append( Words, entry );
							break;
						}

						lines++;
						continue;
					}
				}

				find_next_token( token, line, length );

				// Second argument is substitute.
				entry.Sub = zpl_string_make_length( g_allocator, token, length );

				switch ( type )
				{
					case Tok::Namespace:
						zpl_array_append( Namespaces, entry );
						lines++;
						continue;

					case Tok::Word:
						zpl_array_append( Words, entry );
						lines++;
						continue;
				}
			}

			zpl_printf("Specification Line: %d is missing valid keyword", zpl_array_count(lines) - left);
			lines++;
		}
	}

	void cleanup()
	{
		zpl_arena_free( & Buffer );
	}
}


struct Token
{
	u32 Start;
	u32 End;

	zpl_string Sig;
	zpl_string Sub;
};

void refactor()
{
	sw buffer_size = File::Content.size;

	zpl_array(Token) tokens;
	zpl_array_init( tokens, g_allocator);

	char* content = rcast( char*, File::Content.data );

	zpl_string current = zpl_string_make( g_allocator, "");
	zpl_string preview = zpl_string_make( g_allocator, "");

	sw left = File::Content.size;
	sw line = 0;

	while ( left )
	{
		if ( content[0] == '\n' )
		{
			line++;
		}

		// Word Ignores
		{
			Spec::Entry* ignore = Spec::Word_Ignores;

			sw ignores_left = zpl_array_count( Spec::Word_Ignores);

			do
			{
				if ( ignore->Sig[0] != content[0] )
				{
					continue;
				}

				zpl_string_clear( current );

				u32 sig_length = zpl_string_length( ignore->Sig );
				    current    = zpl_string_append_length( current, content, sig_length );

				// bool match = false;
				// if ( zpl_strncmp( "zpl_printf", current, sig_length ) == 0 )
				// {
 				// 	match = true;
				// }

				if ( zpl_string_are_equal( ignore->Sig, current ) )
				{
					char before = content[-1];
					char after  = content[sig_length];

					if (   zpl_char_is_alphanumeric( before ) || before == '_'
						|| zpl_char_is_alphanumeric( after  ) || after  == '_' )
					{
						continue;
					}

					zpl_printf("\nIgnored   %-81s line %d", current, line );

					content += sig_length;
					left    -= sig_length;
					goto Skip;
				}
			}
			while ( ignore++, --ignores_left );
		}

		// Namespace Ignores
		{
			Spec::Entry* ignore = Spec::Namespace_Ignores;

			sw ignores_left = zpl_array_count( Spec::Namespace_Ignores);

			do
			{
				if ( ignore->Sig[0] != content[0] )
				{
					continue;
				}

				zpl_string_clear( current );

				u32 sig_length = zpl_string_length( ignore->Sig );
				    current    = zpl_string_append_length( current, content, sig_length );
				
				if ( zpl_string_are_equal( ignore->Sig, current ) )
				{
					u32   length     = sig_length;
					char* ns_content = content + sig_length;

					while ( zpl_char_is_alphanumeric( ns_content[0] ) || ns_content[0] == '_' )
					{
						length++;
						ns_content++;
					}

					zpl_string_clear( preview );
					preview = zpl_string_append_length( preview, content, length );
					zpl_printf("\nIgnored   %-40s %-40s line %d", preview, ignore->Sig, line);

					content += length;
					left    -= length;
					goto Skip;
				}
			}
			while ( ignore++, --ignores_left );
		}

		// Words to match
		{
			Spec::Entry* word = Spec::Words;

			sw words_left = zpl_array_count ( Spec::Words);

			do
			{
				if ( word->Sig[0] != content[0] )
				{
					continue;
				}

				zpl_string_clear( current );

				sw sig_length = zpl_string_length( word->Sig);
				   current    = zpl_string_append_length( current, content, sig_length );

				if ( zpl_string_are_equal( word->Sig, current ) )
				{
					char before = content[-1];
					char after  = content[sig_length];

					if (   zpl_char_is_alphanumeric( before ) || before == '_'
						|| zpl_char_is_alphanumeric( after  ) || after  == '_' )
					{
						continue;
					}

					Token entry {};

					entry.Start = File::Content.size - left;
					entry.End   = entry.Start + sig_length;
					entry.Sig   = word->Sig;

					if ( word->Sub != nullptr )
					{
						entry.Sub    = word->Sub;
						buffer_size += zpl_string_length( entry.Sub) - sig_length;
					}

					zpl_array_append( tokens, entry );

					zpl_printf("\nFound     %-81s line %d", current, line);

					content += sig_length;
					left    -= sig_length;
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
				if ( nspace->Sig[0] != content[0] )
				{
					continue;
				}

				zpl_string_clear( current );

				u32 sig_length = zpl_string_length( nspace->Sig );
				    current    = zpl_string_append_length( current, content, sig_length );

				if ( zpl_string_are_equal( nspace->Sig, current ) )
				{
					u32   length     = sig_length;
					char* ns_content = content + sig_length;

					while ( zpl_char_is_alphanumeric( ns_content[0] ) || ns_content[0] == '_' )
					{
						length++;
						ns_content++;
					}

					Token entry {};

					entry.Start = File::Content.size - left;
					entry.End   = entry.Start + length;
					entry.Sig   = nspace->Sig;

					buffer_size += sig_length;

					if ( nspace->Sub != nullptr )
					{
						entry.Sub    = nspace->Sub;
						buffer_size += zpl_string_length( entry.Sub ) - length;
					}

					zpl_array_append( tokens, entry );

					zpl_string_clear( preview );
					preview = zpl_string_append_length( preview, content, length);
					zpl_printf("\nFound     %-40s %-40s line %d", preview, nspace->Sig, line);

					content += length;
					left    -= length;
				}
			}
			while ( nspace++, --nspaces_left );
		}

		content++;
		left--;
	Skip:
		continue;
	}

	left    = zpl_array_count( tokens);
	content = rcast( char*, File::Content.data);
	
	// Generate the refactored file content.
	zpl_arena  buffer;
	zpl_string refactored = nullptr;
	{
		Token* entry = tokens;

		if ( entry == nullptr)
			return;

		zpl_arena_init_from_allocator( & buffer, zpl_heap(), buffer_size * 2 );

		zpl_string 
		new_string = zpl_string_make_reserve( zpl_arena_allocator( & buffer), zpl_kilobytes(1) );
		refactored = zpl_string_make_reserve( zpl_arena_allocator( & buffer), buffer_size );

		sw previous_end = 0;

		while ( left-- )
		{
			sw segment_length = entry->Start - previous_end;

			sw sig_length = zpl_string_length( entry->Sig );

			// Append between tokens
			refactored  = zpl_string_append_length( refactored, content, segment_length );
			content    += segment_length + sig_length;

			segment_length = entry->End - entry->Start - sig_length;

			// Append token
			if ( entry->Sub )
				refactored  = zpl_string_append( refactored, entry->Sub );

			refactored  = zpl_string_append_length( refactored, content, segment_length );
			content    += segment_length;

			previous_end = entry->End;
			entry++;
		}

		entry--;

		if ( entry->End < File::Content.size ) 
		{
			refactored = zpl_string_append_length( refactored, content, File::Content.size - entry->End );
		}
	}
	
	// Write refactored content to destination.
	File::write( refactored );

	zpl_arena_free( & buffer );
}


inline
void parse_options( int num, char** arguments )
{
	zpl_opts opts;
	zpl_opts_init( & opts, g_allocator, "refactor");
	zpl_opts_add(  & opts, "source"       , "src" , "File to refactor"             , ZPL_OPTS_STRING);
	zpl_opts_add(  & opts, "destination"  , "dst" , "File post refactor"           , ZPL_OPTS_STRING);
	zpl_opts_add(  & opts, "specification", "spec", "Specification for refactoring", ZPL_OPTS_STRING);

	if (zpl_opts_compile( & opts, num, arguments))
	{
		if ( zpl_opts_has_arg( & opts, "src" ) )
		{
			zpl_string opt = zpl_opts_string( & opts, "src", "INVALID PATH" );

			File::Source = zpl_string_make( g_allocator, "" );
			File::Source = zpl_string_append( File::Source, opt );
		}
		else
		{
			zpl_printf( "-source not provided\n" );
			fatal();
		}

		if ( zpl_opts_has_arg( & opts, "dst" ) )
		{
			zpl_string opt = zpl_opts_string( & opts, "dst", "INVALID PATH" );

			File::Destination = zpl_string_make( g_allocator, "" );
			File::Destination = zpl_string_append( File::Destination, opt );
		}
		else if ( File::Source )
		{
			File::Destination = zpl_string_make( g_allocator, "" );
			File::Destination = zpl_string_append( File::Destination, File::Source );
		}

		if ( zpl_opts_has_arg( & opts, "spec" ) )
		{
			zpl_string opt = zpl_opts_string( & opts, "spec", "INVALID PATH" );

			Spec::File = zpl_string_make( g_allocator, "" );
			Spec::File = zpl_string_append( Spec::File, opt );
		}
	}
	else
	{
		zpl_printf( "Failed to parse arguments\n" );
		fatal();
	}

	zpl_opts_free( & opts);
}

int main( int num, char** arguments)
{
	Memory::setup();

	parse_options( num, arguments );

	if ( Spec::File )
		Spec::process();

	File::read();

	refactor();

	Spec::  cleanup();
	File::  cleanup();
	Memory::cleanup();
}
