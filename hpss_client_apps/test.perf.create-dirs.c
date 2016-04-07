#include <fcntl.h>
#include <sys/time.h>
#include <stdio.h>
#include <errno.h>
#include "hpss_api.h"
#include "u_signed64.h"
#include "tools.h"

int Num_Retries = 0;

int
main (int argc, char *argv[])
{
  int inst, i;
  char *path, c;
  double start, elapsed, double_time ();

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

  if (optind > argc - 2)
    {
      usage ();
      exit (1);
    }

  path = argv[optind + 1];
  inst = atoi (argv[optind + 2]);

  authenticate ();

  start = double_time ();

  for (i = 0; i < NUM_ENTRIES; i++)
    {
      do_simple_dir (path, i, inst);
    }

  elapsed = double_time () - start;

  printf ("Directory Creation completed: %f seconds\n", elapsed);
  if (Num_Retries)
    {
      printf ("Number of retries: %d\n", Num_Retries);
    }

  hpss_exit (0);
}

int
do_simple_dir (char *path, int file_number, int instance)
{
  int hfd;
  int buffer_size, written = 0;
  char dirname[1024];

  sprintf (dirname, "%s/dir.%d.%d", path, file_number, instance);

  call_hpss_mkdir (__LINE__, dirname);

  return;
}

call_hpss_mkdir (int line, char *dirname)
{
  int rc, retry_cnt = 0;

  while (1)
    {
      rc = hpss_Mkdir (dirname, DIR_PERMS);
      if ((rc == 0) || (retry_cnt > RETRY_LIMIT))
	break;
      retry_cnt++;
      Num_Retries++;
      sleep (1);
    }
  if (rc != 0)
    {
      fprintf (stderr, "%d: hpss_Mkdir(%s): rc = %d, errno = %d\n",
	       line, dirname, rc, errno);
      hpss_exit (1);
    }
  return;
}

call_hpss_chdir (int line, char *dirname)
{
  int rc, retry_cnt = 0;

  while (1)
    {
      rc = hpss_Chdir (dirname);
      if ((rc == 0) || (retry_cnt > RETRY_LIMIT))
	break;
      retry_cnt++;
      Num_Retries++;
      sleep (1);
    }
  if (rc != 0)
    {
      fprintf (stderr, "%d: hpss_Chdir(%s): rc = %d, errno = %d\n",
	       line, dirname, rc, errno);
      hpss_exit (1);
    }
  return;
}

usage ()
{
  fprintf (stderr,
	   "Usage: test.perf.create-dirs -u <username> -t <path to keytab> <path> <inst>\n");
}
