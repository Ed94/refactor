
project( 'refactor', 'c', 'cpp', default_options : ['buildtype=debug'] )

include_thirdparty = include_directories( '../thirdparty' )

# add_global_arguments('-E', language : 'cpp')

if get_option('buildtype').startswith('debug')

  add_project_arguments('-DBuild_Debug', language : 'cpp')

endif

executable( 'refactor', 'refactor.cpp', include_directories : include_thirdparty )

