#include "bobbin-internal.h"

#include <stddef.h>

#include "ac-config.h"

#ifdef HAVE_LIBREADLINE
#include <readline/readline.h>
#else
#define readline(s) (NULL)
#endif
