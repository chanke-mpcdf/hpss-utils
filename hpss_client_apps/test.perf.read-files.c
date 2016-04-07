#include <fcntl.h>
#include <sys/time.h>
#include <stdio.h>
#include <errno.h>
#include "hpss_api.h"
#include "u_signed64.h"
#include "tools.h"

char *Buf;
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


  allocate_and_random_buffer ();

  authenticate ();

  start = double_time ();

  for (i = 0; i < NUM_ENTRIES; i++)
    {
      do_simple_file (path, i, inst);
    }

  elapsed = double_time () - start;

  printf ("File Reads completed: %f seconds\n", elapsed);
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

  call_hpss_open2 (__LINE__, &hfd, filename, O_RDONLY);

  call_hpss_read (__LINE__, hfd, TRANS_FILE_SIZE);

  call_hpss_close (__LINE__, hfd, filename);

  return;
}

allocate_and_random_buffer ()
{
  int i;

  Buf = (char *) malloc (TRANS_FILE_SIZE);
  if (Buf == NULL)
    {
      fprintf (stderr, "hpssput: could not allocate memory\n");
      exit (0);
    }
  for (i = 0; i < TRANS_FILE_SIZE; i++)
    {
      Buf[i] = (char) ((char) random () & 0xff);
    }
  return;
}

call_hpss_read (int line, int hfd, int buffer_size)
{
  int rc, retry_cnt = 0;

  while (1)
    {
      rc = hpss_Read (hfd, Buf, buffer_size);
      if ((rc == buffer_size) || (retry_cnt > RETRY_LIMIT))
	break;
      retry_cnt++;
      Num_Retries++;
      sleep (1);
    }
  if (rc != buffer_size)
    {
      fprintf (stderr, "%d: hpss_Read(bytes=%d): rc = %d, errno = %d\n",
	       line, buffer_size, rc, errno);
      hpss_exit (1);
    }
  return;
}

call_hpss_open2 (int line, int *hfd, char *filename, int flags)
{
  int retry_cnt = 0;

  while (1)
    {
      *hfd = hpss_Open (filename, flags, FILE_PERMS, NULL, NULL, NULL);
      if ((*hfd >= 0) || (retry_cnt > RETRY_LIMIT))
	break;
      retry_cnt++;
      Num_Retries++;
      sleep (1);
    }
  if (*hfd < 0)
    {
      fprintf (stderr, "%d: hpss_Open(%s, 0x%x, 0x%x): rc = %d,"
	       " errno = %d\n", line, filename, flags, FILE_PERMS,
	       *hfd, errno);
      hpss_exit (1);
    }
  return;
}

call_hpss_close (int line, int hfd, char *filename)
{
  int rc, retry_cnt = 0;

  while (1)
    {
      rc = hpss_Close (hfd);
      if ((rc == 0) || (retry_cnt > RETRY_LIMIT))
	break;
      retry_cnt++;
      Num_Retries++;
      sleep (1);
    }
  if (rc != 0)
    {
      fprintf (stderr, "%d: hpss_Close(%s): rc = %d, errno = %d\n",
	       line, filename, rc, errno);
      hpss_exit (1);
    }
  return;
}

usage ()
{
  fprintf (stderr,
	   "Usage: test.perf.read-files -u <username> -t <path to keytab> <path> <inst>\n");
}
