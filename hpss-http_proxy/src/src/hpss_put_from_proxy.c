/*!\file 
*\brief copy a file from the local fs into HPSS
*/

#include <unistd.h>

/*
 * helper 
 */
#include "util.h"
#include "hpss_methods.h"

#define BUFSIZE (32*1024*1024)

int
hpss_put_from_proxy(struct evbuffer *out_evb, char *given_path,
	       const char *flags, char *local_path, char *mode_str, int cos_id)
{
	int rc;
	char *buf;
	char *escaped_path;
	int eof, len, written = 0, hfd, fd;
	mode_t posix_mode;
	hpss_cos_hints_t hints_in, hints_out;
	hpss_cos_priorities_t hints_pri;
	struct stat stat_info;
	u_signed64 size64;
	double start, elapsed, xfer_rate;

	/*
	 * initialize 
	 */
	start = double_time();
	memset(&hints_in, 0, sizeof(hpss_cos_hints_t));
	memset(&hints_pri, 0, sizeof(hpss_cos_priorities_t));
	escaped_path = escape_path(given_path, flags);
	posix_mode = get_posix_mode(mode_str);

	if (serverInfo.LogLevel < LL_TRACE) {
		fprintf(serverInfo.LogFile,
			"%s:%d:: Entering method with hpss_path=%s, flags=%s\n",
			__FILE__, __LINE__, escaped_path, flags);
		fprintf(serverInfo.LogFile, "%s:%d:: local_path=%s\n", __FILE__,
			__LINE__, local_path);
	}

	evbuffer_add_printf(out_evb, "{");
	/*
	 * Set file COS based on argument passed or size of file 
	 */
	if (cos_id) {
		hints_in.COSId = cos_id;
		hints_pri.COSIdPriority = REQUIRED_PRIORITY;
	} else {
		if ((rc = stat(local_path, &stat_info)) < 0) {
			evbuffer_add_printf(out_evb,
					    "\"errstr\" : \"could not stat local file %s\", ",
					    local_path);
			evbuffer_add_printf(out_evb, "\"errno\" : \"%d\", ",
					    rc);
			goto end;
		}
		hints_in.MinFileSize = cast64m(stat_info.st_size);
		hints_in.MaxFileSize = cast64m(stat_info.st_size);
		hints_pri.MinFileSizePriority = REQUIRED_PRIORITY;
		hints_pri.MaxFileSizePriority = REQUIRED_PRIORITY;
	}

	/*
	 * Allocate the data buffer 
	 */
	buf = (char *)malloc(BUFSIZE);
	if (buf == NULL) {
		evbuffer_add_printf(out_evb,
				    "\"errstr\" : \"could not malloc. The dungeon collapses.\", ");
		evbuffer_add_printf(out_evb, "\"errno\" : \"12\", ");
		goto end;
	}
	/*
	 * open local file 
	 */
	if ((fd = open(local_path, O_RDONLY)) < 0) {
		free(buf);
		evbuffer_add_printf(out_evb,
				    "\"errstr\" : \"could not open local file %s\", ",
				    local_path);
		evbuffer_add_printf(out_evb, "\"errno\" : \"%d\", ", errno);
		goto end;
	}

	/*
	 * Open the HPSS file 
	 */
	hfd =
	    hpss_Open(escaped_path,
		      O_WRONLY | O_CREAT | (have_flag("a") ? O_APPEND : 0),
		      posix_mode, &hints_in, &hints_pri, &hints_out);
	if (hfd < 0) {
		free(buf);
		evbuffer_add_printf(out_evb,
				    "\"errstr\" : \"hpss error cannot open/create  hpss-path %s rc=%d\", ",
				    escaped_path, hfd);
		evbuffer_add_printf(out_evb, "\"errno\" : \"%d\", ", errno);
		goto end;
	}
	/*
	 * Copy the src file to the dst file 
	 */
	size64 = cast64m(0);
	for (eof = 0; !eof;) {
		/*
		 * Read in the next chunk of data 
		 */
		for (len = 0; !eof && len < BUFSIZE; len += rc) {
			rc = read(fd, buf + len, BUFSIZE - len);
			if (rc <= 0)
				eof = 1;
		}
		/*
		 * Write out the next chunk of data 
		 */
		if (len > 0) {
			written = hpss_Write(hfd, buf, len);
			if (written != len) {
				free(buf);
				evbuffer_add_printf(out_evb,
						    "\"errstr\" : \"hpss error cannot write to hpss-path %s wrote only %d instead of %d bytes.\", ",
						    escaped_path, written, len);
				evbuffer_add_printf(out_evb,
						    "\"errno\" : \"%d\", ",
						    errno);
				rc = -written;	/* XXX should be errno !? */
				goto end;
			}
		}
		size64 = add64m(size64, cast64m(len));
	}
	/*
	 * Close the files 
	 */
	close(fd);
	hpss_Close(hfd);

 end:
	elapsed = double_time() - start;
	xfer_rate = ((double)cast32m(div64m(size64, 1024 * 1024))) / elapsed;
	evbuffer_add_printf(out_evb, "\"xfer_rate\" : \"%.3f\", ", xfer_rate);
	evbuffer_add_printf(out_evb, "\"elapsed\" : \"%.3f\"}", elapsed);

	if (serverInfo.LogLevel <= LL_TRACE)
		fprintf(serverInfo.LogFile,
			"%s:%d:: leaving with rc=%d, elapsed=%3.3f\n", __FILE__,
			__LINE__, rc, elapsed);
	fflush(serverInfo.LogFile);
	/*
	 * free if we do have an escaped path 
	 */
	if (escaped_path != given_path)
		free(escaped_path);

	return rc;
}
