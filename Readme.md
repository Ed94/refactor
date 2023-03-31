# refactor

Refactor c/c++ files (and problably others) with ease.

## Parameters : 

* `-num` : Used if more than one source file is provided (if used, number of destination files provided MUST MATCH).
* `-src` : Source file to refactor
* `-dst` : Destination file after the refactor (omit to use the same as source)
* `-spec` : Specification containing rules to use for the refactor.
* `-debug` : Use only if on debug build and desire to attach to process.

## Syntax : 

* `not` Omit word or namespace.
* `include` Preprocessor include <file> related identifiers.
* `word` Fixed sized identifier.
* `namespace` Variable sized identifiers, mainly intended to redefine c-namespace of an identifier.
* `,` is used to delimit arguments (if doing a find and replace).
* `L-Value` is the signature to modify.
* `R-Value` is the substitute ( only available if rule does not use `not` keyword )

The only keyword here excluisve to c/c++ is the `include` as it does search specifically for `#include <L-Value>`.  
However, the rest of the categorical keywords (word, namespace), can really be used for any langauge.

There is no semantic awareness this is truely just a simple find and replace, but with some filters specifiable, and  
words/namespaces only being restricted to the rules for C/C++ identifiers (alphanumeric or underscores only)

The main benefit for using this over alts is its problably more ergonomic and performant for large refactors on libraries you may want to have automated in a script.

There are other programs more robust for doing that sort of thing but I was not able to find something this simple.

### Note

* Building for debug provides some nice output with context on a per-line basis.  
* Release will only show errors for asserts (that will kill the refactor early).  
* If the refactor crashes, the files previously written to will retain their changes.
Make sure to have the code backed up on a VCS or in some other way.
* This was compiled using meson with ninja and clang on windows 11. The ZPL library used however should work fine on the other major os platforms and compiler venders.
* The scripts used for building and otherwise are in the scripts directory and are all in powershell (with exception to the meson.build). Techncially there should be a powershell package available on other platorms but worst case it should be pretty easy to port these scripts to w/e shell script you'd perfer.

TODO:  

* Possibly come up with a better name.
* Test to see how much needs to be ported for other platforms (if at all)
* Provide binaries in the release page for github. (debug and release builds)
* Directive to ignore comments (with a way to specify the comment signature). Right now comments that meet the signature of words or namespaces are refactored.
* Provide a GUI build.
* Provide as a single-header library.
