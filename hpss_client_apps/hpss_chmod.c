#include <stdio.h>
#include <errno.h>
#include "hpss_api.h"
#include "tools.h"

int
main (int argc, char *argv[])
{
  int rc, mode;
  char *file;
  char c;

  while ((c = getopt (argc, argv, "u:t:m:h?")) != -1)
    {
      switch (c)
	{
	case 'h':
	case '?':
	  usage ();
	  exit (0);
	default:
	  if (scan_cmdline_authargs (c, optarg))
	    {
	      usage ();
	      exit (1);
	    }
	}
    }

  if (optind != argc - 2)
    {
      usage ();
      exit (1);
    }


  file = argv[optind];
  mode = strtol (argv[optind + 1], NULL, 8);

  if (mode < 0)
    {
      fprintf (stderr, "hpsschmod: Bad mode (0%o) specified\n", mode);
      exit (1);
    }

  authenticate ();

  rc = hpss_Chmod (file, mode);
  if (rc < 0)
    {
      fprintf (stderr, "rc = %d, errno = %d\n", rc, errno);
      hpss_exit (1);
    }

  printf ("Succesfully changed mode on \"%s\" to 0%o\n", file, mode);

  hpss_exit (0);
}


usage ()
{
  fprintf (stderr,
	   "Usage: hpsschmod -u <username> -t <path to keytab> -m <krb5|unix> <path> <mode>\n");
}
