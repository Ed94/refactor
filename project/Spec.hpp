#pragma once
#include "Bloat.hpp"


namespace Spec
{
	enum Tok 
	{
		Not,
		Include,
		Namespace,
		Word,

		Num_Tok
	};

	forceinline
	char const* str_tok( Tok tok )
	{
		static
		char const*	tok_to_str[ Tok::Num_Tok ] = 
		{
			"not",
			"include",
			"namespace",
			"word",
		};

		return tok_to_str[ tok ];
	}

	forceinline
	char strlen_tok( Tok tok )
	{
		static
		const u8 tok_to_len[ Tok::Num_Tok ] = 
		{
			3,
			7,
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

	using Array_Entry = zpl_array( Entry );

	void cleanup();

	// Extract the specificication from the provided file.
	void parse();
}
