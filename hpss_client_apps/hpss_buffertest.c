#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <unistd.h>
#include "hpss_api.h"
#include "u_signed64.h"
#include "tools.h"

#define BUFSIZE (48*1024*1024)

int
usage ()
{
  printf ("Usage: hpssbuffertest [-c <cosid>] <dstfl>\n");
}

int
main (int argc, char *argv[])
{
  int arg, perms, rc;
  double start, elapsed, double_time (), xfer_rate;
  char *buf, *buf2;
  int eof, len, i, cosid;
  char *hpss_file, size64_str[256], cos_str[256];
  u_signed64 size64;
  struct stat stat_info;
  HPSS_FILE *hfp;
  size_t position;

  perms = 0666;
  cosid = 0;

/*
 * Process arguments
 */
  while ((arg = getopt (argc, argv, "c:h?")) != EOF)
    {
      switch (arg)
	{
	case 'c':
	  cosid = atoi (optarg);
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
  if ((optind + 1) != argc)
    {
      usage ();
      exit (1);
    }
  hpss_file = argv[optind];
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
/*
 * Allocate the data buffer2
 */
  printf ("Allocating random buffers....\n");
  buf = (char *) malloc (BUFSIZE);
  if (buf == NULL)
    {
      fprintf (stderr, "hpssbuffertest: 'buf' can't allocate memory\n");
      exit (1);
    }
  for (i = 0; i < BUFSIZE; i++)
    {
      buf[i] = (char) ((char) random () & 0xff);
    }

  buf2 = (char *) malloc (BUFSIZE);
  if (buf2 == NULL)
    {
      fprintf (stderr, "hpssbuffertest: 'buf2' can't allocate memory\n");
      exit (1);
    }
  printf ("Buffers allocation complete\n");
/*
 * Open the HPSS file
 */
  authenticate ();
  hfp = hpss_Fopen (hpss_file, "w+");

  if (hfp == (HPSS_FILE *) NULL)
    {
      fprintf (stderr, "Could not open/create with hpss_Fopen: ");
      perror (hpss_file);
      free (buf);
      free (buf2);
      hpss_exit (1);
    }
/*
 * Write the random data to the HPSS file
 */
  start = double_time ();
  len = 4 * 1024 * 1024;
  for (i = 0; i < BUFSIZE;)
    {
      printf ("Position = %d\n", i);
      rc = hpss_Fwrite (&buf[i], 1, len, hfp);
      if (rc != len)
	{
	  fprintf (stderr, "Error at position %d, rc = %d\n", i, rc);
	  perror ("hpss_Fwrite");
	  goto error_out;
	}
      i = i + len;
    }
  printf ("Completed writing the random data to the HPSS file\n");
  position = hpss_Ftell (hfp);
  printf ("End of file position is %d\n", position);
  rc = hpss_Fseek (hfp, 0, SEEK_SET);
  if (rc != 0)
    {
      perror ("hpss_Fseek(, 0 , SEEK_SET)");
      goto error_out;
    }
  position = hpss_Ftell (hfp);
  printf ("Beginning file position is %d\n", position);
  rc = hpss_Fseek (hfp, 0, SEEK_END);
  if (rc != 0)
    {
      perror ("hpss_Fseek(, 0 , SEEK_END)");
      goto error_out;
    }
  position = hpss_Ftell (hfp);
  printf ("End file position using hpss_Fseek is %d\n", position);
/*
 * Close the files
 */
  hpss_Fclose (hfp);

  free (buf);
  free (buf2);
  hpss_exit (0);

error_out:
  free (buf);
  free (buf2);
  hpss_exit (1);
}
