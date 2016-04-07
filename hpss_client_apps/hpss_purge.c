/*
 * purge.c		- purge a list of files
 *
 * Description:
 *	This program reads from standard input a list of files and attempts
 *	to purge any that have data at the top storage level.
 *
 * Syntax:
 *	purge_all
 */

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include "hpss_api.h"
#include "u_signed64.h"
#include "tools.h"

extern int errno;

/*
 * main function
 */

int
main (int argc, char *argv[])
{
  /*
   * bitfile_xattrs   extended bitfile attributes (includes whether there are
   *                  bytes on disk, at top storagelevel)
   * bytes_purged     how many bytes were purged
   * error            error code
   * fd               file descriptor for files to purge
   * filename         temporary buffer for reading in filenames
   * fullname         the full pathname of the file to purge
   * i                general purpose integer
   * p                temporary pointer for building the full pathname
   * part             a piece of the input file name, used for building
   *                  a level of the pathname
   */

  hpss_xfileattr_t bitfile_xattrs;
  u_signed64 bytes_purged;
  int error;
  int fd;
  int i, arg_use;
  char *filename;
  char c;

  /*
   * Read in the list of file names, build the full pathnames, and purge
   */
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

  authenticate ();

  arg_use = optind - 1;
  while (argv[++arg_use])
    {

      /*
       * Determine whether the file has any segments on disk.  (Or, strictly
       * speaking, at the top storage level.  But then, all our hierarchies
       * have disk at the top level.)
       *
       * Apparently there are three possible flags to pass to the api call
       * which result in three corresponding flags being passed to the bfs
       * call.  The flags and meanings seem to be:
       *
       * API_GET_STATS_FOR_LEVEL
       * BFS_GETATTRS_STATS_FOR_LEVEL
       *      If this is set, the bfs retrieves stats for the specified
       *      storage level only.  If it is not set, the bfs retrieves
       *      stats for all storage levels beginning with the specified
       *      storage level and below it.  Jae's queuing code uses this
       *      flag.
       *
       * API_GET_STATS_FOR_1STLEVEL
       * BFS_GETATTRS_STATS_FOR_1STLEVEL
       *      This seems not to be expected with the STATS_FOR_LEVEL flag,
       *      and Jae's code doesn't use it.  Not sure but I think it searches
       *      all the storage levels begining with the level specified and
       *      working down, and finds the first level at which there is data.
       *
       * API_GET_STATS_OPTIMIZE
       * BFS_GETATTRS_STATS_OPTIMIZE
       *      This doesn't seem to be used anywhere in the bfs.  The client
       *      api passes it through to the bfs, but otherwise doesn't use it,
       *      either.
       *
       * So we pick STATS_FOR_LEVEL and StorageLevel 0 to get stats only
       * for storage level 0 (the top).
       */
      filename = argv[arg_use];

      error = hpss_FileGetXAttributes (filename, API_GET_STATS_FOR_LEVEL,
				       0, &bitfile_xattrs);

      if (error)
	{
	  fprintf (stderr, "can't get attributes for %s, error %d\n",
		   filename, error);
	  continue;
	}

      if (bitfile_xattrs.SCAttrib[0].Flags & BFS_BFATTRS_DATAEXISTS_AT_LEVEL)
	{
	  /*
	   * There is data on disk.  Purge the file.  Requires opening it.
	   */

	  fd = hpss_Open (filename, O_RDWR, 0755, NULL, NULL, NULL);

	  if (fd < 0)
	    {
	      fprintf (stderr, "can't open file %s, errno %d\n", filename,
		       errno);
	      continue;
	    }

	  error = hpss_Purge (fd, cast64m (0), cast64m (0), 0, BFS_PURGE_ALL,
			      &bytes_purged);

	  if (error < 0)
	    {
	      fprintf (stderr, "can't purge file %s, errno %d\n", filename,
		       error);
	    }
	  else
	    {
	      printf ("purged %s\n", filename);
	    }

	  hpss_Close (fd);

	}			/* end if there is data at the top storage level */

      else
	{
	  printf ("no data to purge for %s\n", filename);
	}

      fflush (stdout);

    }				/* end when there are no more arguments */

  hpss_exit (0);

}				/* end function main */

usage ()
{
  fprintf (stderr,
	   "Usage: hpsspurge -u <username> -t <path to keytab> -m <krb5|unix> <file> [ <file> ... ]\n");
}
