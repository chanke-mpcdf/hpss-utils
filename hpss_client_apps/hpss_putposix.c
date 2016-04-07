#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <unistd.h>
#include "hpss_api.h"
#include "u_signed64.h"
#include "tools.h"

#define BUFSIZE (32*1024*1024)

int
main (int argc, char *argv[])
{
  int arg, fd, hfd, perms, rc;
  double start, elapsed, double_time (), xfer_rate;
  char *buf;
  int eof, len, i, cosid, aflag = 0;
  char *file, *hpss_file, size64_str[256], cos_str[256];
  u_signed64 size64;
  hpss_cos_hints_t hints_in, hints_out;
  hpss_cos_priorities_t hints_pri;
  struct stat stat_info;

  if (argc < 2)
    usage ();

  memset (&hints_in, 0, sizeof (hpss_cos_hints_t));
  memset (&hints_pri, 0, sizeof (hpss_cos_priorities_t));
  perms = 0666;
  cosid = 0;

/*
 * Process arguments
 */
  while ((arg = getopt (argc, argv, "u:t:m:ac:h?")) != EOF)
    {
      switch (arg)
	{
	case 'a':
	  aflag = 1;
	  break;
	case 'c':
	  cosid = atoi (optarg);
	  break;
	case 'h':
	case '?':
	  usage ();
	  exit (0);
	default:
	  if (scan_cmdline_authargs (arg, optarg))
	    {
	      fprintf (stderr, "Unknown option \"%c\"\n", arg);
	      usage ();
	      exit (1);
	    }
	}
    }
  if ((optind + 2) != argc)
    {
      usage ();
      exit (1);
    }
  hpss_file = argv[optind];
  file = argv[optind + 1];
/*
 * Set file COS based on argument passed or size of file
 */
  if (cosid)
    {
      sprintf (cos_str, "%d\n", cosid);
      if (setenv ("HPSS_COS", cos_str, 1) != 0)
	{
	  fprintf (stderr, "Unable to set HPSS_COS in environment\n");
	  perror ("setenv");
	  exit (1);
	}
    }
  else
    {
      if (strcmp (file, "-") == 0)
	{
	  fprintf (stderr, "Must provide COS when reading standard input\n");
	  exit (1);
	}
      if (stat (file, &stat_info) < 0)
	{
	  fprintf (stderr, "Could not stat file \"%s\"\n", file);
	  perror ("stat");
	  exit (1);
	}
      hints_in.MinFileSize = cast64m (stat_info.st_size);
      hints_in.MaxFileSize = cast64m (stat_info.st_size);
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
      exit (1);
    }
/*
 * Open the src file
 */
  if (strcmp (file, "-") == 0)
    {
      if ((fd = fileno (stdin)) < 0)
	{
	  fprintf (stderr, "Could not open \"stdin\"");
	  perror ("fileno");
	  free (buf);
	  exit (1);
	}
    }
  else if ((fd = open (file, O_RDONLY)) < 0)
    {
      fprintf (stderr, "Could not open: ");
      perror (file);
      free (buf);
      exit (1);
    }
/*
 * Open the HPSS file
 */
  authenticate ();

  hfd =
    hpss_PosixOpen (hpss_file, O_WRONLY | O_CREAT | (aflag ? O_APPEND : 0),
		    perms);
  if (hfd < 0)
    {
      fprintf (stderr, "Could not open/create: ");
      perror (hpss_file);
      free (buf);
      close (fd);
      hpss_exit (1);
    }
/*
 * Copy the src file to the dst file
 */
  start = double_time ();
  size64 = cast64m (0);
  for (eof = 0; !eof;)
    {
/*
 * Read in the next chunk of data
 */
      for (len = 0; !eof && len < BUFSIZE; len += rc)
	{
	  rc = read (fd, buf + len, BUFSIZE - len);
	  if (rc <= 0)
	    eof = 1;
	}
/*
 * Write out the next chunk of data
 */
      if (len > 0)
	{
	  rc = hpss_Write (hfd, buf, len);
	  if (rc != len)
	    {
	      perror ("hpss_Write");
	      hpss_exit (0);
	    }
	}
      size64 = add64m (size64, cast64m (len));
    }
/*
 * Close the files
 */
  close (fd);
  hpss_Close (hfd);
/*
 * Print transfer statistics
 */
  elapsed = double_time () - start;
  xfer_rate = ((double) cast32m (div64m (size64, 1024 * 1024))) / elapsed;
  u64_to_decchar (size64, size64_str);
  fprintf (stderr, "%s: \"%s\" %s Bytes in %.3f sec -> %.3f MB/sec\n",
	   argv[0], hpss_file, size64_str, elapsed, xfer_rate);

  hpss_exit (0);
}

int
usage ()
{
  printf
    ("Usage: hpssput -u <username> -t <path to keytab> -m <krb5|unix> [-a] [-c <cosid>] <dstfl> { <srcfl> | \"-\" } \n");
  printf ("         where \"-\" indicates reading from standard input\n");
}
