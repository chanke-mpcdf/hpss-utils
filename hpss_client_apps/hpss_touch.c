#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/time.h>
#include "hpss_api.h"
#include "u_signed64.h"
#include "tools.h"

#define BUFSIZE (1*1024*1024)

int
main (int argc, char *argv[])
{
  int i, fd, hfd, rc, len;
  double start, elapsed, double_time ();
  char *buf;
  u_signed64 size64, count64;
  hpss_fileattr_t attr;
  char c;

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


  /* allocate the data buffer */
  buf = (char *) malloc (BUFSIZE);
  if (buf == NULL)
    {
      fprintf (stderr, "%s: could not allocate memory\n", argv[0]);
      exit (0);
    }

  authenticate ();

  for (i = optind; i < argc; i++)
    {
      /* open the source file */
      hfd = hpss_Open (argv[i], O_RDONLY, 0, NULL, NULL, NULL);
      if (hfd < 0)
	{
	  fprintf (stderr, "hpss_Open: ");
	  perror (argv[i]);
	  hpss_exit (1);
	}

      /* get the length of the source file */
      if (hpss_FileGetAttributes (argv[i], &attr) < 0)
	{
	  fprintf (stderr, "hpss_FileGetAttributes: ");
	  perror (argv[i]);
	  hpss_exit (1);
	}
      size64 = attr.Attrs.DataLength;

      /* loop through the source files */
      count64 = cast64m (0);
      {

	/* read in the next chunk of data */
	len = hpss_Read (hfd, buf, BUFSIZE);
	if (len <= 0)
	  {
	    perror ("hpss_Read");
	    hpss_exit (1);
	  }
      }
      hpss_Close (hfd);
    }

  hpss_exit (0);
}

usage ()
{
  fprintf (stderr,
	   "Usage: hpsstouch -u <username> -t <path to keytab> -m <krb5|unix> <srcfl> [<srcfl> ...]\n");
}
