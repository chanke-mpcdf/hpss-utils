#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/time.h>
#include "hpss_api.h"

void print_time_string(time_t);

main (int argc, char *argv[])
{
  int rc;
  hpss_stat_t buf;
  char s64_str[256];
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

  if (optind != argc - 1)
    {
      usage ();
      exit (1);
    }


  authenticate ();
/*
 * Get the stats
 */
  rc = hpss_Stat (argv[optind], &buf);
  if (rc != 0)
    {
      fprintf (stderr, "hpss_Stat: failed with %d\n", rc);
      hpss_exit (1);
    }

  printf ("device       : %u\n", buf.st_dev);
  u64_to_decchar (buf.st_ino, s64_str);
  printf ("inode        : %s\n", s64_str);
  printf ("links        : %u\n", buf.st_nlink);
  printf ("flags        : 0x%x\n", buf.st_flag);
  printf ("uid          : %u\n", buf.st_uid);
  printf ("gid          : %u\n", buf.st_gid);
  printf ("rdevice      : %u\n", buf.st_rdev);
  u64_to_decchar (buf.st_ssize, s64_str);
  printf ("ssize        : %s\n", s64_str);
  printf("access time  : ");
  print_time_string( (time_t) buf.hpss_st_atime );
  printf("modify time  : ");
  print_time_string((time_t) buf.hpss_st_mtime);
  printf("change time  : ");
  print_time_string((time_t) buf.hpss_st_ctime);
  printf ("blksize      : %u\n", buf.st_blksize);
  printf ("blocks       : %u\n", buf.st_blocks);
  printf ("vfstype      : %d\n", buf.st_vfstype);
  printf ("vfs          : %u\n", buf.st_vfs);
  printf ("vnode type   : %u\n", buf.st_type);
  printf ("gen          : %u\n", buf.st_gen);
  u64_to_decchar (buf.st_size, s64_str);
  printf ("size         : %s\n", s64_str);
  printf ("mode         : 0o%o\n", buf.st_mode);

  hpss_exit (0);
}

void print_time_string(time_t ltime) {
  char *timeptr;
  int TIME;

  timeptr = (char *) ctime(&ltime);
  TIME = time(0);
  if ((TIME < ltime) || ((TIME - ltime) > (60 * 60 * 24 * 180)))
    {  
      printf("%--7.7s %-4.4s\n", timeptr + 4, timeptr + 20);
    }
      else
    {  
      printf("%-12.12s\n", timeptr + 4);
    }

    return;
}


usage ()
{
  fprintf (stderr,
	   "Usage: hpssstat -u <username> -t <path to keytab> -m <krb5|unix> <hpssobject>\n");
}
