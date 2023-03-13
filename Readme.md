# refactor

A code identifier refactoring app. Intended for c/c++ like identifiers.

Parameters :

* `-source`, `-src` : Source file to refactor
* `-destination`, `-dst` : Destination file after the refactor (omit to use the same as source)
* `-specification`, `-spec` : Specification containing rules to use for the refactor.

Syntax :

* `not` Omit word or namespace.
* `word` Fixed sized identifier.
* `namespace` Variable sized identifiers, mainly intended to redefine c-namespace of an identifier.
* `,` is used to delimit arguments to word or namespace.
* `L-Value` is the signature to modify.
* `R-Value` is the substitute ( only available if rule does not use `not` keyword )


TODO:  
* Possibly come up with a better name.
* Cleanup memory usage (it hogs quite a bit or what it does..)
