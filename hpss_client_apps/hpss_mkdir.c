#include <stdio.h>
#include <errno.h>
#include "hpss_api.h"
#include "tools.h"

int
main (int argc, char *argv[])
{
  int rc;
  int withParents = 0;
  char c;
  char dir[1024], fullPath[1024];
  char *str, *saveptr, *subtoken;
  while ((c = getopt (argc, argv, "p"COMMON_OPTS)) != -1)
    {
      switch (c)
	{
	case 'h':
	case '?':
	  usage ();
	  exit (0);
	case 'p':
	  withParents = 1;
	  break;
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
  if (withParents)
    {
      strcpy (dir, argv[optind]);
      if (dir[0] == '/')
	strcpy (fullPath, "/");
      for (str = dir;; str = NULL)
	{
	  subtoken = strtok_r (str, "/", &saveptr);
	  if (subtoken == NULL)
	    break;
	  strcat (fullPath, subtoken);
	  strcat (fullPath, "/");
	  if ((rc = hpss_Chdir (fullPath)) < 0)
	    {
	      hpss_Mkdir (fullPath, DIR_PERMS);
	    };
	}
    }
  else
    {
      rc = hpss_Mkdir (argv[optind], DIR_PERMS);
      if (rc < 0)
	{
	  fprintf (stderr, "rc = %d, errno = %d\n", rc, errno);
	  hpss_exit (1);
	}
    }

  printf ("Succesfully created directory %s\n", argv[optind]);

  hpss_exit (0);
}

usage ()
{
  fprintf (stderr,
	   "Usage: hpssmkdir [auth-options] [-p] <dir>\n");
  common_usage();
}
