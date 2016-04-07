#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

#define BUFSIZE (32*1024*1024)

#define PERMS   0666

int
main (int argc, char *argv[])
{
  int i, fd, lfd, rc, cosid, perms, len, aflag, rflag, arg, time;
  char *local_file, *size_str, *buf;
  double start, stop, elapsed, double_time (), xfer_rate;
  unsigned long long size64, totalsize64;
  char str64[32];

  perms = PERMS;
  cosid = aflag = rflag = time = 0;

  while ((arg = getopt (argc, argv, "arc:t:h?")) != EOF)
    {
      switch (arg)
	{
	case 'a':
	  aflag = 1;
	  break;
	case 'r':
	  rflag = 1;
	  break;
	case 't':
	  time = atoi (optarg);
	  break;
	case 'h':
	case '?':
	  usage ();
	  exit (0);
	default:
	  fprintf (stderr, "Unknown option \"%c\"\n", arg);
	  usage ();
	  exit (1);
	}
    }
  if ((optind + 2) != argc)
    {
      usage ();
      exit (1);
    }
  size_str = argv[optind];
  local_file = argv[optind + 1];
/*
 * Convert size argument to 64 bit integer
 */
  if (decchar_to_u64 (size_str, &totalsize64, 0) != 0)
    {
      fprintf (stderr, "Could not convert \"%s\" to 64 bit integer: ",
	       size_str);
      perror ("decchar_to_u64");
      exit (1);
    }
/*
 * Allocate the data buffer
 */
  buf = (char *) malloc (BUFSIZE);
  if (buf == NULL)
    {
      fprintf (stderr, "hpssput: could not allocate memory\n");
      exit (0);
    }
/*
 * Randomize buffer or clear it
 */
  if (rflag)
    {
      for (i = 0; i < BUFSIZE; i++)
	buf[i] = (char) ((char) random () & 0xff);
    }
  else
    {
      memset (buf, 0x00, BUFSIZE);
    }
/*
 * Create/open HPSS file
 */
  lfd = open (local_file, O_WRONLY | O_CREAT | (aflag ? O_APPEND : 0), perms);
  if (lfd < 0)
    {
      fprintf (stderr, "Could not open/create: ");
      perror (local_file);
      exit (1);
    }
/*
 * Loop through writes until complete
 */
  start = stop = double_time ();
  size64 = 0L;
  while (size64 < totalsize64)
    {
      if (time && (start + time) < stop)
	break;
      if ((totalsize64 - size64) < BUFSIZE)
	len = (totalsize64 - size64);
      else
	len = BUFSIZE;
/*
 * Write data to file
 */
      rc = write (lfd, buf, len);
      if (rc != len)
	{
	  errno = -rc;
	  perror ("write");
	  exit (0);
	}
      size64 += len;
      stop = double_time ();
    }
/*
 * End Loop
 */
  close (lfd);
/*
 * Print transfer statistics
 */
  elapsed = double_time () - start;
  xfer_rate = ((double) (size64 / (1024 * 1024))) / elapsed;
  u64_to_decchar (size64, str64);
  fprintf (stderr, "%s: \"%s\" %s Bytes in %.3f sec -> %.3f MB/sec\n",
	   argv[0], local_file, str64, elapsed, xfer_rate);

  exit (0);
}

int
usage ()
{
  printf ("Usage: localputdata [-a] [-r] size <dstfl>\n");
  exit (0);
}
