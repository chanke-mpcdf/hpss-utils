#include <stdio.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/param.h>
#include <errno.h>
#include "hpss_api.h"
#include "u_signed64.h"
#include "tools.h"
#include "ns_Constants.h"

int recursion = 0;
int lflag = 0;
int epoch = 0;
int hpss_attrs = 0;
int status = 0;

int sum_directories = 0;
int sum_junctions = 0;
int sum_symlinks = 0;
int sum_files = 0;
int objects = 0;

main (argc, argv)
     int argc;
     char *argv[];
{
  hpss_Attrs_t AttrOut;
  char path[1024], path2[1024];
  char c;
  int arg = 1;
  int numarg, i, rc, start_time;

  start_time = time (0);
  while ((c = getopt (argc, argv, "u:t:m:lRZHSh?")) != -1)
    {
      switch (c)
	{
	case 'h':
	case '?':
	  usage ();
	  exit (0);
	case 'l':
	  lflag = 1;
	  break;
	case 'R':
	  recursion = 1;
	  break;
	case 'Z':
	  epoch = 1;
	  break;
	case 'H':
	  hpss_attrs = 1;
	  break;
	case 'S':
	  status = 1;
	  break;
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
  arg = optind;
  authenticate ();


  for (; arg < argc; arg++)
    {
      if ((rc = hpss_GetListAttrs (argv[arg], &AttrOut)) < 0)
	{
	  fprintf (stderr, "hpss_GetListAttrs(): %s\n", strerror (-rc));
	  continue;
	}
      if ((AttrOut.Type == NS_OBJECT_TYPE_DIRECTORY)
	  || (AttrOut.Type == NS_OBJECT_TYPE_JUNCTION))
	{
	  /* strip trailing '/' */
	  if (argv[arg][strlen (argv[arg]) - 1] == '/')
	    {
	      argv[arg][strlen (argv[arg]) - 1] = '\0';
	    }
	  if ((rc = hpss_Chdir (argv[arg])) < 0)
	    {
	      fprintf (stderr, "%s: %s\n", argv[arg], strerror (-rc));
	      continue;
	    };
	  if (hpss_Getcwd (path, sizeof (path)) < 0)
	    {
	      perror ("hpss_Getcwd");
	      hpss_exit (1);
	    }
	  if (strcmp (path, argv[arg]) == 0)
	    {
	      strcpy (path2, argv[arg]);
	    }
	  else
	    {
	      strcpy (path2, path);
	      strcat (path2, "/");
	      strcat (path2, argv[arg]);
	    }
	  list_dir (path2);
	  if ((rc = hpss_Chdir (path)) < 0)
	    {
	      fprintf (stderr, "%s: %s\n", path, strerror (-rc));
	      continue;
	    };
	}
      else
	{
	  strcpy (path2, path);
	  strcat (path2, "/");
	  strcat (path2, argv[arg]);
	  list_dir (path2);
	}
    }
  if (lflag)
    {
      fprintf (stderr, "\n\nNamespace Statistics\n");
      fprintf (stderr, "Directories: %d\n", sum_directories);
      fprintf (stderr, "Junctions:   %d\n", sum_junctions);
      fprintf (stderr, "SymLinks:    %d\n", sum_symlinks);
      fprintf (stderr, "Files:       %d\n", sum_files);
      fprintf (stderr, "\nSeconds to process: %d\n", time (0) - start_time);
    }
  hpss_exit (0);

}

/*
 * Using global variable to reduce memory usage on heap.  Though function
 * is recursive, variable is not being used when function is called again.
 */
#define	NUMENTRIES	64
ns_DirEntry_t dir_buf[NUMENTRIES];

int
list_dir (basedir)
     char *basedir;
{
  int dir_handle;
  char path3[1024], *ptr;
  char **list_names;
  hpss_Attrs_t **list_attrs;
  int i, rc, num_list, list_size, num_entries;
  hpss_Attrs_t AttrOut, *a_ptr;
  u_signed64 offsetin, offsetout;
  unsigned32 endflag;

  dir_handle = hpss_Opendir (".");
  if (dir_handle < 0)
    {
      fprintf (stderr, "hpss_Opendir(%s): %s\n", basedir, strerror (-errno));
      hpss_exit (1);
    }

  num_list = 0;
  list_size = NUMENTRIES;
  list_names = (char **) malloc (sizeof (char *) * list_size);
  if (list_names == (char **) NULL)
    {
      perror ("malloc");
      hpss_exit (1);
    }
  list_attrs = (hpss_Attrs_t **) malloc (sizeof (hpss_Attrs_t *) * list_size);
  if (list_attrs == (hpss_Attrs_t **) NULL)
    {
      perror ("malloc");
      hpss_exit (1);
    }

  offsetin = bld64m (0, 0);
  num_entries = hpss_ReadAttrs (dir_handle,
				offsetin,
				sizeof (ns_DirEntry_t) * NUMENTRIES,
				1, &endflag, &offsetout, dir_buf);
  if (num_entries < 0)
    {
      perror ("hpss_ReadAttrs");
      hpss_exit (1);
    }
  while (num_entries > 0)
    {
      for (i = 0; i < num_entries; i++)
	{
	  if ((strcmp (dir_buf[i].Name, ".") != 0
	       && strcmp (dir_buf[i].Name, "..") != 0))
	    {
	      ptr = malloc (strlen (dir_buf[i].Name) + 1);
	      if (ptr == (char *) NULL)
		{
		  perror ("malloc");
		  hpss_exit (1);
		}
	      a_ptr = (hpss_Attrs_t *) malloc (sizeof (hpss_Attrs_t));
	      if (a_ptr == (hpss_Attrs_t *) NULL)
		{
		  perror ("malloc");
		  hpss_exit (1);
		}
	      strcpy (ptr, dir_buf[i].Name);
	      memcpy (a_ptr, &dir_buf[i].Attrs, sizeof (hpss_Attrs_t));
	      list_names[num_list] = ptr;
	      list_attrs[num_list++] = a_ptr;
	      if (num_list + 1 >= list_size)
		{
		  list_size += NUMENTRIES;
		  list_names =
		    realloc (list_names, sizeof (char **) * list_size);
		  if (list_names == (char **) NULL)
		    {
		      perror ("realloc");
		      hpss_exit (1);
		    }
		  list_attrs =
		    realloc (list_attrs,
			     sizeof (hpss_Attrs_t **) * list_size);
		  if (list_attrs == (hpss_Attrs_t **) NULL)
		    {
		      perror ("realloc");
		      hpss_exit (1);
		    }
		}
	    }
	}
      offsetin = offsetout;
      num_entries = hpss_ReadAttrs (dir_handle,
				    offsetin,
				    sizeof (ns_DirEntry_t) * NUMENTRIES,
				    1, &endflag, &offsetout, dir_buf);
      if (num_entries < 0)
	{
	  perror ("hpss_ReadAttrs");
	  hpss_exit (1);
	}
    }
  hpss_Closedir (dir_handle);


  sort_list (list_names, list_attrs, num_list);

  for (i = 0; i < num_list; i++)
    {
      strcpy (path3, basedir);
      if (lflag)
	{
	  printline (basedir, list_names[i], list_attrs[i]);
	}
      else
	{
	  printf ("%s\n", list_names[i]);
	}
      if (recursion)
	{
	  if (list_attrs[i]->Type != NS_OBJECT_TYPE_DIRECTORY)
	    {
	      free (list_names[i]);
	      list_names[i] = (char *) NULL;
	    }
	}
      else
	{
	  free (list_names[i]);
	}
      free (list_attrs[i]);
    }
  free (list_attrs);

  if (recursion)
    {
      for (i = 0; i < num_list; i++)
	{
	  if (list_names[i])
	    {
	      strcpy (path3, basedir);
	      strcat (path3, "/");
	      strcat (path3, list_names[i]);
	      if ((rc = hpss_Chdir (list_names[i])) < 0)
		{
		  fprintf (stderr, "%s: %s\n", list_names[i], strerror (-rc));
		  continue;
		};
	      list_dir (path3);
	      hpss_Chdir ("..");
	    }
	  free (list_names[i]);
	}
    }

  free (list_names);

  return 0;
}

static char *modestr[] = { "---", "--x", "-w-", "-wx",
  "r--", "r-x", "rw-", "rwx"
};

static char *setid_modestr[] = { "--s", "--s", "-ws", "-ws",
  "r-s", "r-s", "rws", "rws"
};

printline (path, name, AttrOut)
     char *path;
     char *name;
     hpss_Attrs_t *AttrOut;
{
  time_t ltime;
  char *timeptr, filesize[256];
  char **owner_modestr, **group_modestr;
  char fullpath[1024];
  int res;
  int TIME;
  int i;
  int cosid, acct;
  mode_t posix_mode;

  switch (AttrOut->Type)
    {

    case NS_OBJECT_TYPE_DIRECTORY:
      sum_directories++;
      printf ("d");
      break;

    case NS_OBJECT_TYPE_JUNCTION:
      sum_junctions++;
      printf ("j");
      break;

    case NS_OBJECT_TYPE_SYM_LINK:
      sum_symlinks++;
      printf ("l");
      break;

    default:
      sum_files++;
      printf ("-");
      break;
    }
  if (status)
    {
      objects++;
      if ((objects % 1000) == 0)
	{
	  fprintf (stderr, "Processed: %d\r", objects);
	  fflush (stderr);
	}
    }

  API_ConvertModeToPosixMode (AttrOut, &posix_mode);

  if (posix_mode & 04000)
    owner_modestr = setid_modestr;
  else
    owner_modestr = modestr;
  if (posix_mode & 02000)
    group_modestr = setid_modestr;
  else
    group_modestr = modestr;
  printf ("%s%s%s %3d ", owner_modestr[(posix_mode >> 6) & 07],
	  group_modestr[(posix_mode >> 3) & 07],
	  modestr[(posix_mode) & 07], AttrOut->LinkCount);
  printf ("%-6d  ", AttrOut->UID);
  printf ("%-6d ", AttrOut->GID);

  if (hpss_attrs)
    {
      if (((posix_mode & S_IFMT) == S_IFDIR)
	  || ((posix_mode & S_IFMT) == S_IFLNK))
	{
	  printf ("NS  NS  NS  ");
	}
      else
	{
/*
 * Ignoring any errors on all these functions 
 */
	  cosid = AttrOut->COSId;
	  acct = AttrOut->Account;
	  printf ("%-3d %-3d %-3d ", cosid, acct, AttrOut->FamilyId);
	}
    }

  u64_to_decchar (AttrOut->DataLength, filesize);
  if (epoch)
    {
      printf ("%10s %10u %s/%s", filesize, AttrOut->TimeModified, path, name);
    }
  else
    {
      ltime = (time_t) AttrOut->TimeModified;
      timeptr = (char *) ctime (&ltime);
      TIME = time (0);
      if ((TIME < AttrOut->TimeModified)
	  || ((TIME - AttrOut->TimeModified) > (60 * 60 * 24 * 180)))
	{
	  printf ("%10s %--7.7s %-4.4s %s/%s", filesize,
		  timeptr + 4, timeptr + 20, path, name);
	}
      else
	{
	  printf ("%10s %-12.12s %s/%s", filesize, timeptr + 4, path, name);
	}
    }
  if ((posix_mode & S_IFMT) == S_IFLNK)
    {
      char *sympath;
      int err;

      /*
       *  We must run the symbolic link
       */

      if ((sympath = (char *) malloc (1024)) == 0)
	{
	  printf ("nomemory\n");
	  fflush (stdout);
	  return 0;
	}
      strcpy (fullpath, path);
      strcat (fullpath, "/");
      strcat (fullpath, name);
      if (hpss_Readlink (fullpath, sympath, 1024) < 0)
	{
	  free (sympath);
	  printf ("-> cannot read symbolic link %s\n", fullpath);
	  fflush (stdout);
	  return 0;
	}
      printf ("@ -> %s", sympath);
      free (sympath);

    }
  printf ("\n");
  fflush (stdout);

  return 0;
}

usage ()
{
  fprintf (stderr,
	   "Usage: hpss_recursive_ls -u <username> -t <path to keytab> -m <krb5|unix> [-lRZHS] <dir>\n");
  fprintf (stderr, "       R - recursion\n");
  fprintf (stderr, "       Z - Time in seconds since Epoch\n");
  fprintf (stderr,
	   "       H - HPSS specific attributes (COS, AcctID, Family)\n");
  fprintf (stderr, "       S - Report status (to stderr)\n");
}

sort_list (list_names, list_attrs, num_list)
     char *list_names[];
     hpss_Attrs_t *list_attrs[];
     int num_list;
{
  int i, j, inner_list;
  char *ptr;
  hpss_Attrs_t *attrs_ptr;

  inner_list = num_list;
  for (i = 0; i < num_list - 1; i++)
    {
      for (j = 0; j < inner_list - 1; j++)
	{
	  if (strcmp (list_names[j], list_names[j + 1]) > 0)
	    {
	      ptr = list_names[j + 1];
	      list_names[j + 1] = list_names[j];
	      list_names[j] = ptr;
	      attrs_ptr = list_attrs[j + 1];
	      list_attrs[j + 1] = list_attrs[j];
	      list_attrs[j] = attrs_ptr;
	    }
	}
      inner_list--;
    }
}
