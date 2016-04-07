#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/time.h>
#include <unistd.h>
#include "hpss_api.h"
#include "u_signed64.h"
#include "tools.h"

#define BUFSIZE (32*1024*1024)

#define PERMS   0666

int
main (int argc, char *argv[])
{
  int i, fd, hfd, rc, cosid, perms, len, aflag, rflag, arg, time;
  char *hpss_file, *size_str, *buf;
  double start, stop, elapsed, double_time (), xfer_rate;
  u_signed64 size64, totalsize64;
  hpss_cos_hints_t hints_in, hints_out;
  hpss_cos_priorities_t hints_pri;
  char str64[32];

  memset (&hints_in, 0, sizeof (hpss_cos_hints_t));
  memset (&hints_pri, 0, sizeof (hpss_cos_priorities_t));
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
	case 'c':
	  cosid = atoi (optarg);
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
  hpss_file = argv[optind + 1];
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
 * Set COS on cosid argument or tell HPSS size of file
 */
  if (cosid)
    {
      hints_in.COSId = cosid;
      hints_pri.COSIdPriority = REQUIRED_PRIORITY;
    }
  else
    {
      hints_in.MinFileSize = totalsize64;
      hints_in.MaxFileSize = totalsize64;
      hints_pri.MinFileSizePriority = REQUIRED_PRIORITY;
      hints_pri.MaxFileSizePriority = REQUIRED_PRIORITY;
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
  authenticate ();

  hfd = hpss_Open (hpss_file, O_WRONLY | O_CREAT | (aflag ? O_APPEND : 0),
		   perms, &hints_in, &hints_pri, &hints_out);
  if (hfd < 0)
    {
      fprintf (stderr, "Could not open/create: ");
      perror (hpss_file);
      hpss_exit (1);
    }
/*
 * Loop through writes until complete
 */
  start = stop = double_time ();
  size64 = cast64m (0);
  while (lt64m (size64, totalsize64))
    {
      if (time && (start + time) < stop)
	break;
      if (lt64m (sub64m (totalsize64, size64), cast64m (BUFSIZE)))
	len = low32m (sub64m (totalsize64, size64));
      else
	len = BUFSIZE;
/*
 * Write data to file
 */
      rc = hpss_Write (hfd, buf, len);
      if (rc != len)
	{
	  errno = -rc;
	  perror ("hpss_Write");
	  hpss_exit (0);
	}
      size64 = add64m (size64, cast64m (len));
      stop = double_time ();
    }
/*
 * End Loop
 */
  hpss_Close (hfd);
/*
 * Print transfer statistics
 */
  elapsed = double_time () - start;
  xfer_rate = ((double) cast32m (div64m (size64, 1024 * 1024))) / elapsed;
  u64_to_decchar (size64, str64);
  fprintf (stderr, "%s: \"%s\" %s Bytes in %.3f sec -> %.3f MB/sec\n",
	   argv[0], hpss_file, str64, elapsed, xfer_rate);

  hpss_exit (0);
}

int
usage ()
{
  printf ("Usage: hpssputdata [-a] [-c <COSid>] [-r] size <dstfl>\n");
}
