#include <stdio.h>
#include <stdlib.h>
#include <libgen.h>
#include <unistd.h>
#include <string.h>

/* allusion to internal functions */

static void printUsage (char *cmdName);

/**
 *  \brief Main function.
 *
 *  \param argc number of words of the command line
 *  \param argv list of words of the command line
 *
 *  \return status of operation
 */

int main (int argc, char *argv[])
{
  /* process command line options */

  int opt;                                       /* selected option */
  char *fName = "no name";                       /* file name (initialized to "no name" by default) */
  int val = -1;                                  /* numeric value (initialized to -1 by default) */

  opterr = 0;
  do
  { switch ((opt = getopt (argc, argv, "f:n:h")))
    { case 'f': /* file name */
    	        if (optarg[0] == '-')
                { fprintf (stderr, "%s: file name is missing\n", basename (argv[0]));
                  printUsage (basename (argv[0]));
                  return EXIT_FAILURE;
                }
                fName = optarg;
                break;
      case 'n': /* numeric argument */
                if (atoi (optarg) <= 0)
                   { fprintf (stderr, "%s: non positive number\n", basename (argv[0]));
                     printUsage (basename (argv[0]));
                     return EXIT_FAILURE;
                   }
                val = (int) atoi (optarg);
                break;
      case 'h': /* help mode */
                printUsage (basename (argv[0]));
                return EXIT_SUCCESS;
      case '?': /* invalid option */
                fprintf (stderr, "%s: invalid option\n", basename (argv[0]));
  	            printUsage (basename (argv[0]));
                return EXIT_FAILURE;
      case -1:  break;
    }
  } while (opt != -1);
  if (argc == 1)
     { fprintf (stderr, "%s: invalid format\n", basename (argv[0]));
       printUsage (basename (argv[0]));
       return EXIT_FAILURE;
     }

  int o;                                          /* counting variable */

  printf ("File name = %s\n", fName);
  printf ("Numeric value = %d\n", val);
  for (o = 0; o < argc; o++)
    printf ("Word %d = %s\n", o, argv[o]);

  /* that's all */

  return EXIT_SUCCESS;

} /* end of main */

/**
 *  \brief Print command usage.
 *
 *  A message specifying how the program should be called is printed.
 *
 *  \param cmdName string with the name of the command
 */

static void printUsage (char *cmdName)
{
  fprintf (stderr, "\nSynopsis: %s OPTIONS [filename / positive number]\n"
           "  OPTIONS:\n"
           "  -h      --- print this help\n"
           "  -f      --- filename\n"
           "  -n      --- positive number\n", cmdName);
}
