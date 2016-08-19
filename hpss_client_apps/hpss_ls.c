#include <stdio.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/param.h>
#include <errno.h>
#include <getopt.h>
#include "hpss_api.h"
#include "u_signed64.h"
#include "tools.h"



main (argc, argv)
     int argc;
     char *argv[];
{
  int dir_handle;
  struct hpss_dirent *ent;
  char ent_buf[sizeof (struct hpss_dirent) + HPSS_MAX_FILE_NAME + 1];
  hpss_Attrs_t AttrOut;
  char path[512], c;
  int arg = 1;
  int aflag = 0;
  int lflag = 0;
  int lastdir = 0;
  int numarg;
  int i, rc;

  ent =
    (struct hpss_dirent *) malloc (sizeof (struct hpss_dirent) +
				   HPSS_MAX_FILE_NAME + 1);
  if (ent == (struct hpss_dirent *) NULL)
    {
      perror ("malloc");
      exit (1);
    }
  while ((c = getopt (argc, argv, "la"COMMON_OPTS)) != -1)
    {
      switch (c)
	{
	case 'h':
	case '?':
	  usage ();
	  exit (0);
	case 'a':
	  aflag = 1;
	  break;
	case 'l':
	  lflag = 1;
	  break;
	default:
	  if (scan_cmdline_authargs (c, optarg))
	    {
	      usage();
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
  arg = optind;
  for (; arg < argc; arg++)
    {
      if ((rc = hpss_GetListAttrs (argv[arg], &AttrOut)) < 0)
	{
	  fprintf (stderr, "hpss_GetListAttrs(): %s\n", strerror (-rc));
	  continue;
	}

      if (AttrOut.Type != NS_OBJECT_TYPE_DIRECTORY)
	{
	  if (lastdir)
	    printf ("\n");
	  lastdir = 0;
	  if (lflag)
	    printline (argv[arg], argv[arg], &AttrOut);
	  else
	    printf ("%s\n", argv[arg]);
	  continue;
	}

      dir_handle = hpss_Opendir (argv[arg]);
      if (dir_handle < 0)
	{
	  fprintf (stderr, "hpss_Opendir(): %s\n", strerror (-rc));
	  hpss_exit (1);
	}

      if (numarg > 1)
	{
	  printf ("\n");
	  lastdir = 1;
	  printf ("%s:\n", argv[arg]);
	}

      if (hpss_Readdir (dir_handle, ent) < 0)
	{
	  perror ("hpss_Readdir");
	  hpss_exit (1);
	}
      while (strlen (ent->d_name) != 0)
	{
	  if ((strcmp (ent->d_name, ".") != 0
	       && strcmp (ent->d_name, "..") != 0) || aflag)
	    {
	      strcpy (path, argv[arg]);
	      if (path[strlen (path) - 1] != '/')
		strcat (path, "/");
	      strcat (path, ent->d_name);
	      if (lflag)
		{
		  if (hpss_GetListAttrs (path, &AttrOut) < 0)
		    {
		      perror (path);
		      if (hpss_Readdir (dir_handle, ent) < 0)
			{
			  perror ("hpss_Readdir");
			  hpss_exit (1);
			}
		      continue;
		    }
		  printline (path, ent->d_name, &AttrOut);
		}
	      else
		{
		  printf ("%s\n", ent->d_name);
		}
	    }
	  if (hpss_Readdir (dir_handle, ent) < 0)
	    {
	      perror ("hpss_Readdir");
	      hpss_exit (1);
	    }
	}
      hpss_Closedir (dir_handle);
    }
  hpss_exit (0);
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
  int res;
  int TIME;
  int i;
  int cosid, acct;
  mode_t posix_mode;

  API_ConvertModeToPosixMode (AttrOut, &posix_mode);

  switch (posix_mode & S_IFMT)
    {

    case S_IFDIR:

      printf ("d");
      break;

    case S_IFLNK:

      printf ("l");
      break;

    case S_IFREG:
    default:

      printf ("-");
      break;
    }
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

  ltime = (time_t) AttrOut->TimeModified;
  timeptr = (char *) ctime (&ltime);
  TIME = time (0);
  u64_to_decchar (AttrOut->DataLength, filesize);
  if ((TIME < AttrOut->TimeModified)
      || ((TIME - AttrOut->TimeModified) > (60 * 60 * 24 * 180)))
    {
      printf ("%10s %--7.7s %-4.4s %s", filesize,
	      timeptr + 4, timeptr + 20, name);
    }
  else
    {
      printf ("%10s %-12.12s %s", filesize, timeptr + 4, name);
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
	  return;
	}
      if (hpss_Readlink (path, sympath, 1024) < 0)
	{
	  free (sympath);
	  printf ("-> cannot read symbolic link\n");
	  fflush (stdout);
	  return;
	}
      printf ("@ -> %s", sympath);
      free (sympath);

    }
  printf ("\n");
  fflush (stdout);
}

usage ()
{
  fprintf (stderr,
	   "Usage: hpssls [-al] [auth-options] <dir>\n");
  common_usage();
}
