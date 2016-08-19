#include <stdio.h>
#include <errno.h>
#include "hpss_api.h"
#include "tools.h"

int
main (int argc, char *argv[])
{
  int rc, uid, gid;
  char *file;
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

  if (optind != argc - 3)
    {
      usage ();
      exit (1);
    }


  file = argv[optind];
  uid = atoi (argv[optind + 1]);
  gid = atoi (argv[optind + 2]);

  if (uid < 0 || gid < 0)
    {
      fprintf (stderr, "Bad uid or gid specified\n");
      exit (1);
    }

  authenticate ();

  rc = hpss_Chown (file, uid, gid);
  if (rc < 0)
    {
      fprintf (stderr, "rc = %d, errno = %d\n", rc, errno);
      hpss_exit (1);
    }

  printf ("Succesfully changed owner for \"%s\"\n", file);

  hpss_exit (0);
}

usage ()
{
  fprintf (stderr,
	   "Usage: hpsschown [auth-options] <path> <uid> <gid>\n");
  common_usage();
}
