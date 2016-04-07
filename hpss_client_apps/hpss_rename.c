#include <stdio.h>
#include <errno.h>
#include "hpss_api.h"
#include "tools.h"

int
main (int argc, char *argv[])
{
  int rc, uid, gid;
  char *old, *new;
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

  old = argv[optind];
  new = argv[optind + 1];

  rc = hpss_Rename (old, new);
  if (rc < 0)
    {
      fprintf (stderr, "rc = %d, errno = %d\n", rc, errno);
      hpss_exit (1);
    }

  printf ("Succesfully renamed from %s to %s\n", old, new);
  hpss_exit (0);
}

usage ()
{
  fprintf (stderr,
	   "hpss_rename -u <username> -t <path to keytab> -m <krb5|unix> <old> <new>\n");
}
