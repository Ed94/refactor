project( 'refactor', 'c', 'cpp', default_options : ['buildtype=debug'] )

# add_global_arguments('-E', language : 'cpp')

if get_option('buildtype').startswith('debug')

  add_project_arguments('-DBuild_Debug', language : 'cpp')

endif

executable( 'refactor', 'refactor.refactored.cpp' )
