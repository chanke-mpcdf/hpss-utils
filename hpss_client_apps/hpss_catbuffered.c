#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/time.h>
#include "hpss_api.h"
#include "u_signed64.h"

#define BUFSIZE (32*1024*1024)

main (int argc, char *argv[])
{
  int i, fd, hfd, rc, len;
  HPSS_FILE *hfp;
  double start, elapsed, double_time (), xfer_rate;
  char *buf, *hpss_file, xfer_rate64_str[256], size64_str[256], c;
  u_signed64 size64, count64;
  hpss_fileattr_t attr;
  hpss_stat_t file_stat;

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

  if (optind == argc)
    {
      usage ();
      exit (1);
    }

  hpss_file = argv[optind + 1];
/*
 * Allocate the data buffer
 */
  buf = (char *) malloc (BUFSIZE);
  if (buf == NULL)
    {
      fprintf (stderr, "hpsscat: could not allocate memory\n");
      exit (0);
    }

  authenticate ();
/*
 * Open the HPSS source file
 */
  hfp = hpss_Fopen (hpss_file, "r");
  if (hfp == (HPSS_FILE *) NULL)
    {
      fprintf (stderr, "hpss_Fopen: ");
      perror (hpss_file);
      hpss_exit (0);
    }
/*
 * Get the length of the source file
 */
  if (hpss_Stat (hpss_file, &file_stat) != 0)
    {
      fprintf (stderr, "hpss_Fopen: ");
      perror (hpss_file);
      hpss_exit (0);
    }
  size64 = file_stat.st_size;
/*
 * Loop through the source files
 */
  start = double_time ();
  count64 = cast64m (0);
  while (lt64m (count64, size64))
    {
/*
 * Read in the next chunk of data
 */
      len = hpss_Fread (buf, 1, BUFSIZE, hfp);
      if (len <= 0)
	{
	  perror ("hpss_Fread");
	  hpss_exit (0);
	}
/*
 * Write the data to standard output
 */
      rc = fwrite (buf, 1, len, stdout);
      if (rc != len)
	{
	  perror ("fwrite");
	  hpss_exit (0);
	}
      count64 = add64m (count64, cast64m (len));
    }
/*
 * Close HPSS file and display transfer statistics
 */
  hpss_Fclose (hfp);
  elapsed = double_time () - start;
  xfer_rate = ((double) cast32m (div64m (size64, 1024 * 1024))) / elapsed;
  u64_to_decchar (size64, size64_str);
  fprintf (stderr, "%s: \"%s\" %s Bytes in %.3f sec -> %.3f MB/sec\n",
	   argv[0], hpss_file, size64_str, elapsed, xfer_rate);

  hpss_exit (0);
}

usage ()
{
  fprintf (stderr, "Usage: hpsscatbuffer -u <username> -t <path to keytab> -m <krb5|unix> <srcfl>\n");
}
