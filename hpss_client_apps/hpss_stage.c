#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/time.h>
#include <getopt.h>
#include "hpss_api.h"
#include "u_signed64.h"
#include "tools.h"

#define BUFSIZE (32*1024*1024)

main (int argc, char *argv[])
{
  int i, fd, hfd, rc, len;
  double start, elapsed, double_time (), xfer_rate;
  char *buf, *hpss_file, xfer_rate64_str[256], size64_str[256];
  char c;
  u_signed64 size64, count64;
  hpss_fileattr_t attr;

  while ((c = getopt (argc, argv, COMMON_OPTS)) != -1)
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
  hpss_file = argv[optind];
  authenticate ();
/*
 * Open the HPSS source file
 */
  hfd = hpss_Open (hpss_file, O_RDONLY, 0, NULL, NULL, NULL);
  if (hfd < 0)
    {
      fprintf (stderr, "hpss_Open: ");
      perror (hpss_file);
      hpss_exit (0);
    }
/*
 * Stage the file
 */
 rc  = hpss_Stage(hfd, 0, cast64m(0), 0, BFS_STAGE_ALL);

  hpss_Close (hfd);
  elapsed = double_time () - start;
  xfer_rate = ((double) cast32m (div64m (size64, 1024 * 1024))) / elapsed;
  u64_to_decchar (size64, size64_str);
  fprintf (stderr, "%s: \"%s\" %s Bytes in %.3f sec -> %.3f MB/sec\n",
	   argv[0], hpss_file, size64_str, elapsed, xfer_rate);

  hpss_exit (0);
}

usage ()
{
  fprintf (stderr,
	   "stages an hpss-file to Sotragie class 0 (Disk). \nUsage: hpss_stage [auth-options] <srcfl>\n");
  common_usage();
}
