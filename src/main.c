/* GNU m4 -- A simple macro processor
   Copyright 1989-1994, 1999, 2000 Free Software Foundation, Inc.
  
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or 
   (at your option) any later version.
 
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
 
   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
   02111-1307  USA
*/

#include <getopt.h>
#include <signal.h>

#include "m4.h"
#include "m4private.h"
#include "error.h"

void print_program_name M4_PARAMS((void));


/* Name of frozen file to digest after initialization.  */
const char *frozen_file_to_read = NULL;

/* Name of frozen file to produce near completion.  */
const char *frozen_file_to_write = NULL;

/* If nonzero, display usage information and exit.  */
static int show_help = 0;

/* If nonzero, print the version on standard output and exit.  */
static int show_version = 0;

/* If nonzero, import the environment as macros.  */
static int import_environment = 0;

typedef struct macro_definition
{
  struct macro_definition *next;
  int code;			/* D, U or t */
  const char *macro;
} macro_definition;


/* Error handling functions.  */

/*----------------------------------------------------------------------.
| Print program name, source file and line reference on standard        |
| error, as a prefix for error messages.  Flush standard output first.  |
`----------------------------------------------------------------------*/

void
print_program_name (void)
{
  fflush (stdout);
  fprintf (stderr, "%s: ", program_name);
  if (m4_current_line != 0)
    fprintf (stderr, "%s: %d: ", m4_current_file, m4_current_line);
}


#ifdef USE_STACKOVF

/*---------------------------------------.
| Tell user stack overflowed and abort.	 |
`---------------------------------------*/

static void
stackovf_handler (void)
{
  M4ERROR ((EXIT_FAILURE, 0,
	    _("ERROR: Stack overflow.  (Infinite define recursion?)")));
}

#endif /* USE_STACKOV */



/*---------------------------------------------.
| Print a usage message and exit with STATUS.  |
`---------------------------------------------*/

static void
usage (int status)
{
  if (status != EXIT_SUCCESS)
    fprintf (stderr, _("Try `%s --help' for more information.\n"),
	     program_name);
  else
    {
      printf (_("Usage: %s [OPTION]... [FILE]...\n"), program_name);
      fputs (_("\
Mandatory or optional arguments to long options are mandatory or optional\n\
for short options too.\n\
\n\
Operation modes:\n\
      --help                   display this help and exit\n\
      --version                output version information and exit\n\
  -c, --discard-comments       do not copy comments to the output\n\
  -e, --interactive            unbuffer output, ignore interrupts\n\
  -E, --fatal-warnings         stop execution after first warning\n\
  -Q, --quiet, --silent        suppress some warnings for builtins\n\
  -P, --prefix-builtins        force a `m4_' prefix to all builtins\n"),
	     stdout);
#ifdef ENABLE_CHANGEWORD
      fputs (_("\
  -W, --word-regexp=REGEXP     use REGEXP for macro name syntax\n"),
	     stdout);
#endif
      fputs (_("\
\n\
Dynamic loading features:\n\
  -M, --module-directory=DIRECTORY  add DIRECTORY to the module search path\n\
  -m, --load-module=MODULE          load dynamic MODULE from M4MODPATH\n"),
	     stdout);
      fputs (_("\
\n\
Preprocessor features:\n\
  -I, --include=DIRECTORY      search this directory second for includes\n\
  -D, --define=NAME[=VALUE]    enter NAME has having VALUE, or empty\n\
  -U, --undefine=NAME          delete builtin NAME\n\
  -s, --synclines              generate `#line NO \"FILE\"' lines\n"),
	     stdout);
      fputs (_("\
\n\
Limits control:\n\
  -G, --traditional            suppress all GNU extensions\n\
  -H, --hashsize=PRIME         set symbol lookup hash table size\n\
  -L, --nesting-limit=NUMBER   change artificial nesting limit\n"),
	     stdout);
      fputs (_("\
\n\
Frozen state files:\n\
  -F, --freeze-state=FILE      produce a frozen state on FILE at end\n\
  -R, --reload-state=FILE      reload a frozen state from FILE at start\n"),
	     stdout);
      fputs (_("\
\n\
Debugging:\n\
  -d, --debug=[FLAGS]          set debug level (no FLAGS implies `aeq')\n\
  -t, --trace=NAME             trace NAME when it will be defined\n\
  -l, --arglength=NUM          restrict macro tracing size\n\
  -o, --error-output=FILE      redirect debug and trace output\n"),
	     stdout);
      fputs (_("\
\n\
FLAGS is any of:\n\
  t   trace for all macro calls, not only traceon'ed\n\
  a   show actual arguments\n\
  e   show expansion\n\
  q   quote values as necessary, with a or e flag\n\
  c   show before collect, after collect and after call\n\
  x   add a unique macro call id, useful with c flag\n\
  f   say current input file name\n\
  l   say current input line number\n\
  p   show results of path searches\n\
  i   show changes in input files\n\
  V   shorthand for all of the above flags\n"),
	     stdout);
      fputs (_("\
\n\
If no FILE or if FILE is `-', standard input is read.\n"),
	     stdout);

      fputs (_("\nReport bugs to <bug-m4@gnu.org>.\n"), stdout);
    }
  exit (status);
}

/*--------------------------------------.
| Decode options and launch execution.  |
`--------------------------------------*/

static const struct option long_options[] =
{
  {"arglength", required_argument, NULL, 'l'},
  {"debug", optional_argument, NULL, 'd'},
  {"discard-comments", no_argument, NULL, 'c'},
  {"diversions", required_argument, NULL, 'N'},
  {"error-output", required_argument, NULL, 'o'},
  {"fatal-warnings", no_argument, NULL, 'E'},
  {"freeze-state", required_argument, NULL, 'F'},
  {"hashsize", required_argument, NULL, 'H'},
  {"include", required_argument, NULL, 'I'},
  {"interactive", no_argument, NULL, 'e'},
  {"load-module", required_argument, NULL, 'm'},
  {"module-directory", required_argument, NULL, 'M'},
  {"nesting-limit", required_argument, NULL, 'L'},
  {"prefix-builtins", no_argument, NULL, 'P'},
  {"quiet", no_argument, NULL, 'Q'},
  {"reload-state", required_argument, NULL, 'R'},
  {"silent", no_argument, NULL, 'Q'},
  {"synclines", no_argument, NULL, 's'},
  {"traditional", no_argument, NULL, 'G'},
  {"word-regexp", required_argument, NULL, 'W'},

  {"import-environment", no_argument, &import_environment, 1},

  {"help", no_argument, &show_help, 1},
  {"version", no_argument, &show_version, 1},

  /* These are somewhat troublesome.  */
  { "define", required_argument, NULL, 'D' },
  { "undefine", required_argument, NULL, 'U' },
  { "trace", required_argument, NULL, 't' },

  { 0, 0, 0, 0 },
};

#ifdef ENABLE_CHANGEWORD
#  define CHANGEWORD_SHORTOPT	"W:"
#else
#  define CHANGEWORD_SHORTOPT	""
#endif
#ifdef WITH_MODULES
#  define MODULE_SHORTOPT	"m:"
#  define MODULEPATH_SHORTOPT	"M:"
#else
#  define MODULE_SHORTOPT	""
#  define MODULEPATH_SHORTOPT	""
#endif

#define OPTSTRING "B:D:EF:GH:I:L:M:N:PQR:S:T:U:"/**/CHANGEWORD_SHORTOPT/**/":cd::el:m:o:st:"
#include <dlfcn.h>

int
main (int argc, char *const *argv, char *const *envp)
{
  macro_definition *head;	/* head of deferred argument list */
  macro_definition *tail;
  macro_definition *new;
  int optchar;			/* option character */

  macro_definition *defines;
  FILE *fp;
  char *filename;

  int exit_status;

  program_name = argv[0];
  error_print_progname = print_program_name;

  setlocale (LC_ALL, "");
#ifdef ENABLE_NLS
  textdomain(PACKAGE);
#endif

  LTDL_SET_PRELOADED_SYMBOLS();
  
  m4_module_init ();
  m4_debug_init ();
  m4_include_init ();
  m4_symtab_init ();

#ifdef USE_STACKOVF
  setup_stackovf_trap (argv, envp, stackovf_handler);
#endif

  if (isatty (STDIN_FILENO))
    interactive = TRUE;

  /* First, we decode the arguments, to size up tables and stuff.  */

  head = tail = NULL;

  while (optchar = getopt_long (argc, argv, OPTSTRING, long_options, NULL),
	 optchar != EOF)
    switch (optchar)
      {
      default:
	usage (EXIT_FAILURE);

      case 0:
	break;

      case 'B':			/* compatibility junk */
      case 'N':
      case 'S':
      case 'T':
	break;

      case 'D':
      case 'U':
      case 't':
      case 'm':
	/* Arguments that cannot be handled until later are accumulated.  */

	new = (macro_definition *) xmalloc (sizeof (macro_definition));
	new->code = optchar;
	new->macro = optarg;
	new->next = NULL;

	if (head == NULL)
	  head = new;
	else
	  tail->next = new;
	tail = new;

	break;

      case 'E':
	warning_status = EXIT_FAILURE;
	break;

      case 'F':
	frozen_file_to_write = optarg;
	break;

      case 'G':
	no_gnu_extensions = 1;
	break;

      case 'H':
	hash_table_size = atoi (optarg);
	if (hash_table_size <= 0)
	  hash_table_size = HASHMAX;
	break;

      case 'I':
	m4_add_include_directory (optarg);
	break;

      case 'L':
	nesting_limit = atoi (optarg);
	break;
      case 'M':
	{
	  const char *search_path = lt_dlgetsearchpath ();
	  if (lt_dlsetsearchpath (optarg) != 0)
	  {
	    const char *dlerr = lt_dlerror();
	    if (dlerr == NULL)
	      M4ERROR ((EXIT_FAILURE, 0,
			_("ERROR: failed to add search directory `%s'"),
			optarg));
	    else
	      M4ERROR ((EXIT_FAILURE, 0,
			_("ERROR: failed to add search directory `%s': %s"),
			optarg, dlerr));
	  }
	  if (search_path)
	    lt_dladdsearchdir (search_path);
	}
	break;

      case 'P':
	prefix_all_builtins = 1;
	break;

      case 'Q':
	suppress_warnings = 1;
	break;

      case 'R':
	frozen_file_to_read = optarg;
	break;

#ifdef ENABLE_CHANGEWORD
      case 'W':
	user_word_regexp = optarg;
	break;
#endif

      case 'c':
	discard_comments = 1;
	break;

      case 'd':
	debug_level = m4_debug_decode (optarg);
	if (debug_level < 0)
	  {
	    error (0, 0, _("Bad debug flags: `%s'"), optarg);
	    debug_level = 0;
	  }
	break;

      case 'e':
	interactive = 1;
	break;

      case 'l':
	max_debug_argument_length = atoi (optarg);
	if (max_debug_argument_length <= 0)
	  max_debug_argument_length = 0;
	break;

      case 'o':
	if (!m4_debug_set_output (optarg))
	  error (0, errno, optarg);
	break;

      case 's':
	sync_output = 1;
	break;
      }

  if (show_version)
    {
      printf ("GNU %s %s", PACKAGE, VERSION);
#if defined(WITH_GMP) || defined(ENABLE_CHANGEWORD)
      fputs(_(" (options:"), stdout);
#ifdef WITH_GMP
      fputs(" gmp", stdout);
#endif /* WITH_GMP */
#ifdef ENABLE_CHANGEWORD
      fputs(" changeword", stdout);
#endif /* ENABLE_CHANGEWORD */
      fputs(")", stdout);
#endif /* defined WITH_GMP ||�ENABLE_CHANGEWORD */
      fputs("\n", stdout);
      exit (EXIT_SUCCESS);
    }

  if (show_help)
    usage (EXIT_SUCCESS);

  /* Do the basic initialisations.  */

  m4_input_init ();
  m4_output_init ();
  m4_include_env_init ();

  if (frozen_file_to_read)
    {
      reload_frozen_state (frozen_file_to_read);
    }
  else
    {
      m4_syntax_init    ();
      m4_module_install ("m4");
      m4_module_install (no_gnu_extensions ? "traditional" : "gnu");
    }
  
  /* Import environment variables as macros.  The definition are
     preprended to the macro definition list, so -U can override
     environment variables. */

  if (import_environment)
    {
      char *const *env;

      for (env = envp; *env != NULL; env++)
	{
	  new = (macro_definition *) xmalloc (sizeof (macro_definition));
	  new->code = 'D';
	  new->macro = *env;
	  new->next = head;
	  head = new;
	}
    }

  /* Handle deferred command line macro definitions.  Must come after
     initialisation of the symbol table.  */

  defines = head;

  while (defines != NULL)
    {
      macro_definition *next;
      char *macro_value;
      m4_symbol *symbol;

      switch (defines->code)
	{
	case 'D':
	  macro_value = strchr (defines->macro, '=');
	  if (macro_value == NULL)
	    macro_value = "";
	  else
	    *macro_value++ = '\0';
	  m4_macro_define (NULL, defines->macro, macro_value, M4_SYMBOL_INSERT);
	  break;

	case 'U':
	  m4_lookup_symbol (defines->macro, M4_SYMBOL_DELETE);
	  break;

	case 't':
	  symbol = m4_lookup_symbol (defines->macro, M4_SYMBOL_INSERT);
	  SYMBOL_TRACED (symbol) = TRUE;
	  break;

	case 'm':
	  m4_module_install (defines->macro);
	  break;
	
	default:
	  M4ERROR ((warning_status, 0,
		    _("INTERNAL ERROR: Bad code in deferred arguments")));
	  abort ();
	}

      next = defines->next;
      xfree ((VOID *) defines);
      defines = next;
    }

  /* Interactive mode means unbuffered output, and interrupts ignored.  */

  if (interactive)
    {
      signal (SIGINT, SIG_IGN);
      setbuf (stdout, (char *) NULL);
    }

  /* Handle the various input files.  Each file is pushed on the input,
     and the input read.  Wrapup text is handled separately later.  */

  exit_status = EXIT_SUCCESS;
  if (optind == argc)
    {
      m4_push_file (stdin, "stdin");
      m4_expand_input ();
    }
  else
    for (; optind < argc; optind++)
      {
	if (strcmp (argv[optind], "-") == 0)
	  m4_push_file (stdin, "stdin");
	else
	  {
	    fp = m4_path_search (argv[optind], &filename);
	    if (fp == NULL)
	      {
		error (0, errno, argv[optind]);
		exit_status = EXIT_FAILURE;
		continue;
	      }
	    else
	      {
		m4_push_file (fp, filename);
		xfree (filename);
	      }
	  }
	m4_expand_input ();
      }
#undef NEXTARG

  /* Now handle wrapup text.  */

  while (m4_pop_wrapup ())
    m4_expand_input ();

  if (frozen_file_to_write)
    produce_frozen_state (frozen_file_to_write);
  else
    {
      m4_make_diversion (0);
      m4_undivert_all ();
    }

  m4_module_unload_all();

  exit (exit_status);
}
