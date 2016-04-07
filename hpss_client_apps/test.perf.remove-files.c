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
      do_simple_file (path, i, inst);
    }

  elapsed = double_time () - start;

  printf ("File Removes completed: %f seconds\n", elapsed);
  if (Num_Retries)
    {
      printf ("Number of retries: %d\n", Num_Retries);
    }

  hpss_exit (0);
}

int
do_simple_file (char *path, int file_number, int instance, int cos)
{
  int hfd;
  int buffer_size, written = 0;
  char filename[1024];

  sprintf (filename, "%s/file.%d.%d", path, file_number, instance);

  call_hpss_unlink (__LINE__, filename);

  return;
}

call_hpss_unlink (int line, char *filename)
{
  int rc, retry_cnt = 0;

  while (1)
    {
      rc = hpss_Unlink (filename);
      if (rc == -ENOENT)
	{
	  fprintf (stderr, "File: %s missing.  Continuing...\n", filename);
	  rc = 0;
	}
      if ((rc == 0) || (retry_cnt > RETRY_LIMIT))
	break;
      retry_cnt++;
      Num_Retries++;
      sleep (1);
    }
  if (rc != 0)
    {
      fprintf (stderr, "%d: hpss_Unlink(%s): rc = %d, errno = %d\n",
	       line, filename, rc, errno);
      hpss_exit (1);
    }
  return;
}

usage ()
{
  fprintf (stderr,
	   "Usage: test.perf.remove-files -u <username> -t <path to keytab> <path> <inst>\n");
}
