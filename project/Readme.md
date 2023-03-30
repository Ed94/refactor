# Documentation

The current implementation is divided into 4 parts:

* Bloat : General library provider.
* IO : File I/O processing.
* Spec : Specification parsing.
* Refactor : Entrypoint, argument parsing, and refactoring process.

The files are setup to compile as one unit. As such the source files for Bloat, IO, and Spec are located within `refactor.cpp`.

Bloat contains some aliasing of some C++ keywords and does not use the standard library. Instead a library called ZPL is used (Single header replacement).

The program has pretty much no optimizations made to it, its just regular loops with no threading.  
Just tried to keep the memory at a reasonable size for what it does.

The program execution is pretty much outlined quite clearly in `int main()`.

1. Setup initial reserve of global memory in an arena.
2. Parse the arguments provided.
3. Prepare IO's memory for retreviing content.
4. Reserve memory for the refactor buffer.
5. Parse the specification file
6. Iterate through all provided files to refactor and write the refactored content to the specificed destintation files.
7. Cleanup all reserves of memory`*`


`*` This technically can be skipped on windows, may be worth doing to reduce latency of process shutdown.

There are constraints for specific variables;

* `Path_Size_Largest` : Longest path size is set to 1 KB of characters.
* `Token_Max_Length` : Set to 1 KB characters as well.
* `Array_Reserve_Num` : Is set to 4 KB. 
* Initial Global arena size : Set to 2 megabytes.

The `Path_Size_Largest` and `Token_Max_Length` are compile-time constraints that the runtime will not have a fallback for, if 1 KB is not enough it will need to be changed for your use case.

`Array_Reserve_Num` is used to dictate the assumed amount of tokens will be held in total for any of spec's arrays holding ignores and refactor entries. If any of the array's exceed 4 KB they will grow triggering a resize which will bog down the speed of the refactor. Adjust if you think you can increase or lower for use case.

Initial Global arena size is a finicy thing, its most likely going to be custom allocator at one point so that it can handle growing properly, right now its just grows if the amount of memory file paths will need for sources is greater than 1 MB.  
