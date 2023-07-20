#pragma once

#include "Bloat.hpp"


namespace IO
{
	ct uw Path_Size_Largest = zpl_kilobytes(1);

	// Preps the IO by loading all the files and checking to see what the largest size is.
	// The file with the largest size is used to determine the size of the persistent memory.
	void prepare();

	// Frees the persistent and transient memory arenas.
	void cleanup();

	// Provides the content of the specification.
	Array_Line get_specification();

	// Provides the content of the next source, broken up as a series of lines.
	char* get_next_source();

	// Writes the refactored content ot the current corresponding destination.
	void write( zpl_string refactored );
}
