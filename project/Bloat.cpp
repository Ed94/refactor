#define BLOAT_IMPL
#include "Bloat.hpp"

namespace Global
{
	bool ShouldShowDebug = false;
}

namespace Memory
{
	zpl_arena Global_Arena {};

	void setup()
	{
		zpl_arena_init_from_allocator( & Global_Arena, zpl_heap(), Initial_Reserve );

		if ( Global_Arena.total_size == 0 )
		{
			zpl_assert_crash( "Failed to reserve memory for Tests:: Global_Arena" );
		}
	}

	void resize( uw new_size )
	{
		void* new_memory = zpl_resize( zpl_heap(), Global_Arena.physical_start, Global_Arena.total_size, new_size );

		if ( new_memory == nullptr )
		{
			fatal("Failed to resize global arena!");
		}

		Global_Arena.physical_start = new_memory;
		Global_Arena.total_size     = new_size;
	}

	void cleanup()
	{
		zpl_arena_free( & Global_Arena);
	}
}


bool opts_custom_add(zpl_opts* opts, zpl_opts_entry *t, char* b)
{
	if (t->type != ZPL_OPTS_STRING)
	{
		return false;
	}

	t->text = zpl_string_append_length(t->text, " ", 1);
	t->text = zpl_string_appendc( t->text, b );

	return true;
}

b32 opts_custom_compile(zpl_opts *opts, int argc, char **argv)
{
	zpl_b32 had_errors = false;

	for (int i = 1; i < argc; ++i)
	{
		char* arg = argv[i];

		if (*arg)
		{
			arg = (char*)zpl_str_trim(arg, false);

			if (*arg == '-')
			{
				zpl_opts_entry* entry = 0;
				zpl_b32 checkln = false;
				if ( *(arg + 1) == '-')
				{
					checkln = true;
					++arg;
				}

				char *b = arg + 1, *e = b;

				while (zpl_char_is_alphanumeric(*e) || *e == '-' || *e == '_') {
					++e;
				}

				entry = zpl__opts_find(opts, b, (e - b), checkln);

				if (entry)
				{
					char *ob = b;
					b = e;

					/**/
					if (*e == '=')
					{
						if (entry->type == ZPL_OPTS_FLAG)
						{
							*e = '\0';
							zpl__opts_push_error(opts, ob, ZPL_OPTS_ERR_EXTRA_VALUE);
							had_errors = true;

							continue;
						}

						b = e = e + 1;
					}
					else if (*e == '\0')
					{
						char *sp = argv[i+1];

						if (sp && *sp != '-' && (zpl_array_count(opts->positioned) < 1  || entry->type != ZPL_OPTS_FLAG))
						{
							if (entry->type == ZPL_OPTS_FLAG)
							{
								zpl__opts_push_error(opts, b, ZPL_OPTS_ERR_EXTRA_VALUE);
								had_errors = true;

								continue;
							}

							arg = sp;
							b = e = sp;
							++i;
						}
						else
						{
							if (entry->type != ZPL_OPTS_FLAG)
							{
								zpl__opts_push_error(opts, ob, ZPL_OPTS_ERR_MISSING_VALUE);
								had_errors = true;
								continue;
							}

							entry->met = true;

							continue;
						}
					}

					e = (char *)zpl_str_control_skip(e, '\0');
					zpl__opts_set_value(opts, entry, b);

					if ( (i + 1) < argc )
					{
						for ( b = argv[i + 1]; i < argc && b[0] != '-'; i++, b = argv[i + 1] )
						{
							opts_custom_add(opts, entry, b );
						}
					}
				}
				else
				{
					zpl__opts_push_error(opts, b, ZPL_OPTS_ERR_OPTION);
					had_errors = true;
				}
			}
			else if (zpl_array_count(opts->positioned))
			{
				zpl_opts_entry *l = zpl_array_back(opts->positioned);
				zpl_array_pop(opts->positioned);
				zpl__opts_set_value(opts, l, arg);
			}
			else
			{
				zpl__opts_push_error(opts, arg, ZPL_OPTS_ERR_VALUE);
				had_errors = true;
			}
		}
	}

	return !had_errors;
}
