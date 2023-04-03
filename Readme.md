# refactor

Refactor c/c++ files (and problably others) with ease.

## Parameters 

* `-num` : Used if more than one source file is provided (if used, number of destination files provided MUST MATCH).
* `-src` : Source file to refactor
* `-dst` : Destination file after the refactor (omit to use the same as source)
* `-spec` : Specification containing rules to use for the refactor.
* `-debug` : Use if on debug build and desire to attach to process, or to get a more verbose log.

## Syntax

* `not` Omit an a signature. (Modifies include, word, namespace, etc)
* `include` Preprocessor include `<file path>` related identifiers.
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

### Example scripts

See `scripts/template_reafactor.ps1` and the `test/*.refactor` related scripts on intended usage.

This app is not very nice to use directly from CLI. Instead run from a script after gathering the arguments.

There is a desire also to get this setup as a single-header library and also alternative with a minimalist GUI for simple refactors.

### Notes

* Building for debug provides some nice output with context on a per-line basis.  
* Release will only show errors for asserts (that will kill the refactor early).  
* If the refactor crashes, the files previously written to will retain their changes.
  * Make sure to have the code backed up on a VCS or in some other way.
* The scripts used for building and otherwise are in the scripts directory and are all in powershell (with exception to the meson.build). Techncially there should be a powershell package available on other platorms but worst case it should be pretty easy to port these scripts to w/e shell script you'd perfer.

## Building

The project has all build configuration in the `scripts` directory.  

* `build.ci.ps1` is intended for a continuous intergration setup (GH-worfklow for now).  
* `build.ps1` is just a wrap of build.ci that just calls cls.
* `clean.ps1` will clean the workspace of all generated files.
* `get_sources.ps1` is used to gather sources since meson devs refuse to add dynamic retrival of sources for a build.

The project uses [meson](https://github.com/mesonbuild/meson) as the build tool.  
Compiler : clang  
OS: Windows 11 (windows-latest for github actions)

There should theoretically not be anything stopping it from building on other plaforms.
The library's main dependency is [zpl](https://github.com/zpl-c) which seems to support all major platforms.

## Testing

If the `test` parameter is provided to the build scripts, the project and thirdparty code will be refactored based on the specificiation files `*.refactor` residing in `test`.

With the refactors applied a meson configuraiton is setup (`meson.build` in test) and run to build. So long as it compiles, the feature set of the current version should work fine.

* There is an extra file `stb_image` that is parsed but unused during compilation.
  * Planned for use in the namespace addition todo.

## TODO:  

* Possibly come up with a better name.
* Test to see how much needs to be ported for other platforms (if at all)
* Provide binaries in the release page for github. (debug and release builds)
* Ability to run and not emit any changes to files unless all files sucessfully are refactored.
  * Would fix issue where a refactor overwrites files but failed to complete
  * Can have a heavy memory cost, so most likely do not want on by default.
* Directive to ignore comments (with a way to specify the comment signature). Right now comments that
meet the signature of words or namespaces are refactored.
* Make comments ignored by default, and just have ability to specify custom comments.
  * Would need a directive to add refactors to comments.
* Directive to add cpp namespaces on specific lines of a file, or near specific signatures.  
  * This can honestly be done also with placing words on specific lines..  
* Provide a GUI build.
* Provide as a single-header library.
  * Could add a test case where this library is refactored into pure C (most likely c99 or c11).
* Better tests:
  * Automatically pull the zpl repo, refactor and format the library, and package the single header before using it in testing.
