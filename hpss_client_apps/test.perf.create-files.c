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
  int cos, inst, i;
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

  if (optind > argc - 3)
    {
      usage ();
      exit (1);
    }
  cos = atoi (argv[optind + 1]);
  path = argv[optind + 2];
  inst = atoi (argv[optind + 3]);


  allocate_and_random_buffer ();

  authenticate ();

  start = double_time ();
  for (i = 0; i < NUM_ENTRIES; i++)
    {
      do_simple_file (path, i, inst, cos);
    }

  elapsed = double_time () - start;

  printf ("File Creation completed: %f seconds\n", elapsed);
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
  call_hpss_open (__LINE__, &hfd, filename, O_WRONLY | O_CREAT, cos);

  call_hpss_write (__LINE__, hfd, ONE_KFILE);
/* PPS
        call_hpss_write(__LINE__, hfd, TRANS_FILE_SIZE);
        call_hpss_write(__LINE__, hfd, ONE_MEGFILE);
        call_hpss_write(__LINE__, hfd, ONE_GIGFILE);
*/
  call_hpss_close (__LINE__, hfd, filename);

  return;
}

allocate_and_random_buffer ()
{
  int i;

  Buf = (char *) malloc (ONE_MEGFILE + 128);
/* PPS
       Buf = (char *)malloc(TRANS_FILE_SIZE+128);
*/
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

call_hpss_write (int line, int hfd, int buffer_size)
{
  int rc, retry_cnt = 0;

  while (1)
    {
      rc = hpss_Write (hfd, Buf, buffer_size);
      if ((rc == buffer_size) || (retry_cnt > RETRY_LIMIT))
	break;
      retry_cnt++;
      Num_Retries++;
      sleep (1);
    }
  if (rc != buffer_size)
    {
      fprintf (stderr, "%d: hpss_Write(bytes=%d): rc = %d, errno = %d\n",
	       line, buffer_size, rc, errno);
      hpss_exit (1);
    }
  return;
}

call_hpss_open (int line, int *hfd, char *filename, int flags, int cos)
{
  int retry_cnt = 0;
  hpss_cos_hints_t hints_in, hints_out;
  hpss_cos_priorities_t hints_pri;

  memset (&hints_in, 0, sizeof (hpss_cos_hints_t));
  memset (&hints_pri, 0, sizeof (hpss_cos_priorities_t));
  hints_in.COSId = cos;
  hints_pri.COSIdPriority = REQUIRED_PRIORITY;

  while (1)
    {
      *hfd = hpss_Open (filename, flags, FILE_PERMS, &hints_in,
			&hints_pri, &hints_out);
      if ((*hfd >= 0) || (retry_cnt > RETRY_LIMIT))
	break;
      retry_cnt++;
      Num_Retries++;
      sleep (1);
    }
  if (*hfd < 0)
    {
      fprintf (stderr, "%d: hpss_Open(%s, 0x%x, 0x%x, cos=%d): rc = %d,"
	       " errno = %d\n", line, filename, flags, FILE_PERMS, cos,
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
	   "Usage: test.perf.create-files -u <username> -t <path to keytab> <cos> <path> <inst>\n");
}
