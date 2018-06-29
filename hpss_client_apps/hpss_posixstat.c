#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/time.h>
#include "hpss_api.h"
#include "hpss_posix_api.h"
#include "tools.h"

main(
	int	argc,
	char	*argv[])
{
	int		rc;
        int             c;
        struct stat     buf;

        while ((c = getopt (argc, argv, COMMON_OPTS)) != -1) {
            switch (c) {
                case 'h':
                case '?':
                    usage();
                    exit(0);
                default :
                    if (scan_cmdline_authargs(c, optarg) ) {
                        usage();
                        exit(1);
                    }
             }
        }   
            
        if ( optind != argc-1 )
        {
            usage();
            exit(1);
        }

        authenticate();
/*
 * Get the "Posix" stats
 */
        rc = hpss_PosixStat(argv[optind], &buf);
        if (rc != 0)
        {
	    fprintf(stderr, "hpss_PosixStat: failed with %d\n", rc);
	    hpss_exit(1);
        }

        printf("device       : %u\n", buf.st_dev);
        printf("inode        : %u\n", buf.st_ino);
        printf("mode         : 0o%o\n", buf.st_mode);
        printf("links        : %u\n", buf.st_nlink);
        printf("uid          : %u\n", buf.st_uid);
        printf("gid          : %u\n", buf.st_gid);
        printf("dev type     : %u\n", buf.st_rdev);
        printf("size         : %lld\n", (long long) buf.st_size);
        printf("blksize      : %ld\n", (long) buf.st_blksize);
        printf("blocks       : %lld\n", (long long) buf.st_blocks);
        printf("access time  : %s", ctime(&buf.st_atime));
        printf("modify time  : %s", ctime(&buf.st_mtime));
        printf("change time  : %s", ctime(&buf.st_ctime));

        hpss_exit(0);
}

usage ()
{
  fprintf (stderr,
	   "Usage: hpss_posixstat [auth-options] <hpssobject>\n");
  common_usage();
}
