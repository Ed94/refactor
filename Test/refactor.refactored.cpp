#define ZPL_IMPLEMENTATION
#include "bloat.refactored.hpp"


namespace File
{
	using namespace zpl;

	string        Source        = nullptr;
	string        Destination   = nullptr;
	file_contents Content {};

	arena Buffer;

	void cleanup()
	{
		arena_free( & Buffer );
	}

	void read()
	{
		file file_src = {};

		Content.allocator = g_allocator;

		file_error error_src  = file_open( & file_src, Source );

		if ( error_src == ZPL_FILE_ERROR_NONE ) 
		{
			sw fsize = zpl_cast(sw) file_size( & file_src);

			if ( fsize > 0 ) 
			{
				arena_init_from_allocator( & Buffer, heap(), (fsize + fsize % 64) * 4 );
				
				Content.data = alloc( arena_allocator( & Buffer), fsize);
				Content.size = fsize;

				file_read_at ( & file_src, Content.data, Content.size, 0);
			}

			file_close( & file_src);
		}

		if ( Content.data == nullptr )
		{
			fatal( "Unable to open source file: %s\n", Source );
		}
	}

	void write(string refactored)
	{
		if ( refactored == nullptr)
			return;

		file       file_dest {};
		file_error error =  file_create( & file_dest, Destination );

		if ( error != ZPL_FILE_ERROR_NONE )
		{
			fatal( "Unable to open destination file: %s\n", Destination );
		}

		file_write( & file_dest, refactored, string_length(refactored) );
	}
}

namespace Spec
{
	using namespace zpl;

	string File;

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
	char strlen_tok( Tok tok )
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
	bool is_tok( Tok tok, string str, u32 length )
	{
		char const* tok_str = str_tok(tok);
		const u8    tok_len = strlen_tok(tok);

		if ( tok_len != length)
			return false;

		s32 result = str_compare( tok_str, str, tok_len );

		return result == 0;
	}

	struct Entry
	{
		string Sig = nullptr; // Signature
		string Sub = nullptr; // Substitute
	};

	arena            Buffer {};
	zpl_array(Entry) Word_Ignores;
	zpl_array(Entry) Namespace_Ignores;
	zpl_array(Entry) Words;
	zpl_array(Entry) Namespaces;

	u32 Sig_Smallest = kilobytes(1);

	forceinline
	void find_next_token( string& token, char*& line, u32& length )
	{
		string_clear( token );
		length = 0;
		while ( char_is_alphanumeric( line[length] ) || line[length] == '_' )
		{
			length++;
		}

		if ( length == 0 )
		{
			fatal("Failed to find valid initial token");
		}

		token  = string_append_length( token, line, length );
		line  += length;
	}

	void process()
	{
		char* content;

		zpl_array(char*) lines;

		// Get the contents of the file.
		{
             file   file {};
             file_error error = file_open( & file, File);

			 if ( error != ZPL_FILE_ERROR_NONE )
			 {
				fatal("Could not open the specification file: %s", File);
			 }

             sw fsize = scast( sw, file_size( & file ) );

			 if ( fsize <= 0 )
			 {
				fatal("No content in specificaiton to process");
			 }

			 arena_init_from_allocator( & Buffer, heap(), (fsize + fsize % 64) * 10 + kilobytes(1) );

             char* content = rcast( char*, alloc( arena_allocator( & Buffer), fsize + 1) );

             file_read( & file, content, fsize);

             content[fsize] = 0;

             lines = str_split_lines( arena_allocator( & Buffer ), content, false );

             file_close( & file );
		}
		
		sw left = array_count( lines );

		if ( left == 0 )
		{
			fatal("Spec::process: lines array imporoperly setup");
		}

		// Skip the first line as its the version number and we only support __VERSION 1.
		left--;
		lines++;

		array_init( Word_Ignores,      arena_allocator( & Buffer));
		array_init( Namespace_Ignores, arena_allocator( & Buffer));
		array_init( Words,             arena_allocator( & Buffer));
		array_init( Namespaces,        arena_allocator( & Buffer));

		// Limiting the maximum output of a token to 1 KB
		string token = string_make_reserve( arena_allocator( & Buffer), kilobytes(1));

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
				while ( char_is_space( line[0] ) )
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

				while ( char_is_space( line[0] ) )
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

					while ( char_is_space( line[0] ) )
						line++;

					if ( line[0] == '\0' )
					{
						lines++;
						continue;
					}
				}

				find_next_token( token, line, length );

				// First argument is signature.
				entry.Sig = string_make_length( g_allocator, token, length );

				if ( length < Sig_Smallest )
					Sig_Smallest = length;

				if ( line[0] == '\0' || ignore )
				{
					switch ( type )
					{
						case Tok::Namespace:
							if ( ignore)
								array_append( Namespace_Ignores, entry );

							else
								array_append( Namespaces, entry );
						break;

						case Tok::Word:
							if ( ignore)
							{
								array_append( Word_Ignores, entry );
								u32 test = array_count( Word_Ignores );
							}
								

							else
								array_append( Words, entry );
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
									array_append( Namespaces, entry );
								break;

								case Tok::Word:
									array_append( Words, entry );
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

					while ( char_is_space( line[0] ) )
						line++;

					if ( line[0] == '\0' )
					{
						switch ( type )
						{
							case Tok::Namespace:
								array_append( Namespaces, entry );
							break;

							case Tok::Word:
								array_append( Words, entry );
							break;
						}

						lines++;
						continue;
					}
				}

				find_next_token( token, line, length );

				// Second argument is substitute.
				entry.Sub = string_make_length( g_allocator, token, length );

				switch ( type )
				{
					case Tok::Namespace:
						array_append( Namespaces, entry );
						lines++;
						continue;

					case Tok::Word:
						array_append( Words, entry );
						lines++;
						continue;
				}
			}

			log_fmt("Specification Line: %d is missing valid keyword", array_count(lines) - left);
			lines++;
		}
	}

	void cleanup()
	{
		arena_free( & Buffer );
	}
}

using namespace zpl;

struct Token
{
	u32 Start;
	u32 End;

	string Sig;
	string Sub;
};

void refactor()
{
	sw buffer_size = File::Content.size;

	zpl_array(Token) tokens;
	array_init( tokens, g_allocator);

	char* content = rcast( char*, File::Content.data );

	string current = string_make( g_allocator, "");
	string preview = string_make( g_allocator, "");

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

			sw ignores_left = array_count( Spec::Word_Ignores);

			do
			{
				if ( ignore->Sig[0] != content[0] )
				{
					continue;
				}

				string_clear( current );

				u32 sig_length = string_length( ignore->Sig );
				    current    = string_append_length( current, content, sig_length );

				if ( string_are_equal( ignore->Sig, current ) )
				{
					char before = content[-1];
					char after  = content[sig_length];

					if (   char_is_alphanumeric( before ) || before == '_'
						|| char_is_alphanumeric( after  ) || after  == '_' )
					{
						continue;
					}

					log_fmt("\nIgnored   %-81s line %d", current, line );

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

			sw ignores_left = array_count( Spec::Namespace_Ignores);

			do
			{
				if ( ignore->Sig[0] != content[0] )
				{
					continue;
				}

				string_clear( current );

				u32 sig_length = string_length( ignore->Sig );
				    current    = string_append_length( current, content, sig_length );
				
				if ( string_are_equal( ignore->Sig, current ) )
				{
					u32   length     = sig_length;
					char* ns_content = content + sig_length;

					while ( char_is_alphanumeric( ns_content[0] ) || ns_content[0] == '_' )
					{
						length++;
						ns_content++;
					}

					string_clear( preview );
					preview = string_append_length( preview, content, length );
					log_fmt("\nIgnored   %-40s %-40s line %d", preview, ignore->Sig, line);

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

			sw words_left = array_count ( Spec::Words);

			do
			{
				if ( word->Sig[0] != content[0] )
				{
					continue;
				}

				string_clear( current );

				sw sig_length = string_length( word->Sig);
				   current    = string_append_length( current, content, sig_length );

				if ( string_are_equal( word->Sig, current ) )
				{
					char before = content[-1];
					char after  = content[sig_length];

					if (   char_is_alphanumeric( before ) || before == '_'
						|| char_is_alphanumeric( after  ) || after  == '_' )
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
						buffer_size += string_length( entry.Sub) - sig_length;
					}

					array_append( tokens, entry );

					log_fmt("\nFound     %-81s line %d", current, line);

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

			sw nspaces_left = array_count( Spec::Namespaces);

			do
			{
				if ( nspace->Sig[0] != content[0] )
				{
					continue;
				}

				string_clear( current );

				u32 sig_length = string_length( nspace->Sig );
				    current    = string_append_length( current, content, sig_length );

				if ( string_are_equal( nspace->Sig, current ) )
				{
					u32   length     = sig_length;
					char* ns_content = content + sig_length;

					while ( char_is_alphanumeric( ns_content[0] ) || ns_content[0] == '_' )
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
						buffer_size += string_length( entry.Sub ) - length;
					}

					array_append( tokens, entry );

					string_clear( preview );
					preview = string_append_length( preview, content, length);
					log_fmt("\nFound     %-40s %-40s line %d", preview, nspace->Sig, line);

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

	left    = array_count( tokens);
	content = rcast( char*, File::Content.data);
	
	// Generate the refactored file content.
	arena  buffer;
	string refactored = nullptr;
	{
		Token* entry = tokens;

		if ( entry == nullptr)
			return;

		arena_init_from_allocator( & buffer, heap(), buffer_size * 2 );

		string 
		new_string = string_make_reserve( arena_allocator( & buffer), kilobytes(1) );
		refactored = string_make_reserve( arena_allocator( & buffer), buffer_size );

		sw previous_end = 0;

		while ( left-- )
		{
			sw segment_length = entry->Start - previous_end;

			sw sig_length = string_length( entry->Sig );

			// Append between tokens
			refactored  = string_append_length( refactored, content, segment_length );
			content    += segment_length + sig_length;

			segment_length = entry->End - entry->Start - sig_length;

			// Append token
			if ( entry->Sub )
				refactored  = string_append( refactored, entry->Sub );

			refactored  = string_append_length( refactored, content, segment_length );
			content    += segment_length;

			previous_end = entry->End;
			entry++;
		}

		entry--;

		if ( entry->End < File::Content.size ) 
		{
			refactored = string_append_length( refactored, content, File::Content.size - entry->End );
		}
	}
	
	// Write refactored content to destination.
	File::write( refactored );

	arena_free( & buffer );
}


inline
void parse_options( int num, char** arguments )
{
	opts opts;
	opts_init( & opts, g_allocator, "refactor");
	opts_add(  & opts, "source"       , "src" , "File to refactor"             , ZPL_OPTS_STRING);
	opts_add(  & opts, "destination"  , "dst" , "File post refactor"           , ZPL_OPTS_STRING);
	opts_add(  & opts, "specification", "spec", "Specification for refactoring", ZPL_OPTS_STRING);

	if (opts_compile( & opts, num, arguments))
	{
		if ( opts_has_arg( & opts, "src" ) )
		{
			string opt = opts_string( & opts, "src", "INVALID PATH" );

			File::Source = string_make( g_allocator, "" );
			File::Source = string_append( File::Source, opt );
		}
		else
		{
			fatal( "-source not provided\n" );
		}

		if ( opts_has_arg( & opts, "dst" ) )
		{
			string opt = opts_string( & opts, "dst", "INVALID PATH" );

			File::Destination = string_make( g_allocator, "" );
			File::Destination = string_append( File::Destination, opt );
		}
		else if ( File::Source )
		{
			File::Destination = string_make( g_allocator, "" );
			File::Destination = string_append( File::Destination, File::Source );
		}

		if ( opts_has_arg( & opts, "spec" ) )
		{
			string opt = opts_string( & opts, "spec", "INVALID PATH" );

			Spec::File = string_make( g_allocator, "" );
			Spec::File = string_append( Spec::File, opt );
		}
	}
	else
	{
		fatal( "Failed to parse arguments\n" );
	}

	opts_free( & opts);
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
