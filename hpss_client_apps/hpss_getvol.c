/*
 * Program: Example6 Answer
 * Description: Get tape physical volume of a file
 * Assumptions: None
 */

#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/time.h>
#include "hpss_api.h"
#include "u_signed64.h"
#include "tools.h"

int
main (int argc, char *argv[])
{
  int rc, i, j, k;
  hpss_xfileattr_t attributes;
  char size_str[64];
  char c;
/*
 * parse command lines
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

  if (optind != argc - 1)
    {
      usage ();
      exit (1);
    }

/*
 * Authenticate process
 */
  authenticate ();
/*
 * Get extended attributes of a file
 */
  rc = hpss_FileGetXAttributes (argv[optind], API_GET_STATS_FOR_ALL_LEVELS,
				0, &attributes);
  if (rc < 0)
    {
      fprintf (stderr, "Could not get attributes for: ");
      perror (argv[1]);
      hpss_exit (1);
    }
/*
 * Loop through all levels of the returned structure
 */

  for (i = 0; i < HPSS_MAX_STORAGE_LEVELS; i++)
    {
/*
 * Only interested in those levels with data and that are tape
 */
      if (attributes.SCAttrib[i].Flags &&
	  (BFS_BFATTRS_DATAEXISTS_AT_LEVEL || BFS_BFATTRS_LEVEL_IS_TAPE))
	{
/*
 * Print the number of bytes at this level
 */
	  size_str[0] = '\0';
	  u64_to_decchar (attributes.SCAttrib[i].BytesAtLevel, size_str);
	  printf ("Level %d: Bytes = %s\n", i, size_str);
/*
 * Potential that a level has more than 1 VV
 */
	  for (j = 0; j <= attributes.SCAttrib[i].NumberOfVVs; j++)
	    {
	      pv_list_t *pv_list_ptr;

	      printf ("Level %d, VV[%d]: Relative Position = %d\n", i, j,
		      attributes.SCAttrib[i].VVAttrib[j].RelPosition);
	      pv_list_ptr = attributes.SCAttrib[i].VVAttrib[j].PVList;
	      if (pv_list_ptr != NULL)
		{
/*
 * Each VV may be composed of multiple PVs as well
 */
		  for (k = 0; k < pv_list_ptr->List.List_len; k++)
		    {
		      printf ("Level %d, PV[%d]: %s\n", i, k,
			      pv_list_ptr->List.List_val[k].Name);
		    }
		}
	    }
	}
/*
 * Go clean up memory allocation
 */
      for (j = 0; j < attributes.SCAttrib[i].NumberOfVVs; j++)
	{
	  if (attributes.SCAttrib[i].VVAttrib[j].PVList != NULL)
	    {
	      free (attributes.SCAttrib[i].VVAttrib[j].PVList);
	    }
	}
    }
  hpss_exit (0);
}

usage ()
{
  fprintf (stderr,
	   "Usage: hpssgetvol -u <username> -t <path to keytab> -m <krb5|unix> <path to file>\n");
}
