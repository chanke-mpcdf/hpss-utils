#include <stdio.h>
#include <errno.h>
#include "hpss_api.h"
#include "tools.h"

int
main (int argc, char *argv[])
{
  int rc;
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


  authenticate ();

  rc = hpss_Symlink (argv[optind], argv[optind + 1]);
  if (rc < 0)
    {
      fprintf (stderr, "rc = %d, errno = %d\n", rc, errno);
      hpss_exit (1);
    }

  printf ("Succesfully created symlink\n");

  hpss_exit (0);
}

usage ()
{
  fprintf (stderr,
	   "Usage: hpsssymlink -u <username> -t <path to keytab> -m <krb5|unix> <sym-path> <path>\n");
}
