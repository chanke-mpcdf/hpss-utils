#include <stdio.h>
#include <errno.h>
#include "hpss_api.h"
#include "tools.h"

int
main (int argc, char *argv[])
{
  int rc;
  char c;
  while ((c = getopt (argc, argv, COMMON_OPTS)) != -1)
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

  if (optind != argc - 1)
    {
      usage ();
      exit (1);
    }

  authenticate ();

  rc = hpss_Unlink (argv[optind]);
  if (rc < 0)
    {
      fprintf (stderr, "rc = %d, errno = %d\n", rc, errno);
      hpss_exit (1);
    }

  printf ("Succesfully removed object %s\n", argv[optind]);

  hpss_exit (0);
}

usage ()
{
  fprintf (stderr,
	   "Usage: hpssrm [auth-options] <path to file>\n");
  common_usage();
}
