/*!\file 
*\brief read a file from HPSS and store it locally on the proxy
*/
#include <unistd.h>

/*
 * helper 
 */
#include "util.h"
#include "hpss_methods.h"

#define BUFSIZE (32*1024*1024)

int
hpss_get_to_proxy(struct evbuffer *out_evb, char *given_path,
		  const char *flags, char *local_path, char *mode_str)
{

	int rc = 0;
	int local_open_mode;
	char *buf;
	char *escaped_path;
	int read = 0, written = 0, hfd, fd;
	mode_t posix_mode;
	u_signed64 count64 = 0, size64 = 0;
	double start, elapsed, xfer_rate;
	hpss_fileattr_t Attr;
	struct stat stat_info;

	/*
	 * initialize 
	 */
	start = double_time();
	escaped_path = escape_path(given_path, flags);
	posix_mode = get_posix_mode(mode_str);

	if (serverInfo.LogLevel <= LL_TRACE) {
		fprintf(serverInfo.LogFile,
			"%s:%d:: entering hpss_get_to_proxy with escaped_path=%s, local_path=%s\n",
			__FILE__, __LINE__, escaped_path, local_path);
		fflush(serverInfo.LogFile);
	}

	evbuffer_add_printf(out_evb, "{ ");
        evbuffer_add_printf(out_evb, "\"action\" : \"get_to_proxy\", ");
	/*
	 * see if local file already exists 
	 */
	if (lstat(local_path, &stat_info) >= 0) {	/* is at least link */
		if (!have_flag("f")) {	/* fail if force flag is not given */
			evbuffer_add_printf(out_evb, "\"errno\" : \"17\", ");
			evbuffer_add_printf(out_evb,
					    "\"errstr\" : \"local_path=%s already exists.\", ",
					    local_path);
			rc = 17;
			goto end;
		}
		/*
		 * remove it, if we don't append 
		 */
		if (!have_flag("a"))
			unlink(local_path);
	}

	/*
	 * Get the length of the source file 
	 */
	if (hpss_FileGetAttributes(escaped_path, &Attr) < 0) {
		evbuffer_add_printf(out_evb, "\"errno\" : \"%d\", ", errno);
		evbuffer_add_printf(out_evb,
				    "\"errstr\" : \"cannot stat hpss-path %s.\", ",
				    escaped_path);
		goto end;
	}

	size64 = Attr.Attrs.DataLength;

	/*
	 * Allocate the data buffer 
	 */
	buf = (char *)malloc(BUFSIZE);
	if (buf == NULL) {
		evbuffer_add_printf(out_evb, "\"errno\" : \"12\", ");
		evbuffer_add_printf(out_evb,
				    "\"errstr\" : \"malloc error. The dungeon collapses.\", ");
		goto end;
	}

	/*
	 * open local file 
	 */
	local_open_mode = O_WRONLY | O_CREAT;
	if (have_flag("a"))
		local_open_mode |= O_APPEND;

	if ((fd = open(local_path, local_open_mode, posix_mode)) < 0) {
		evbuffer_add_printf(out_evb, "\"errno\" : \"%d\", ", errno);
		evbuffer_add_printf(out_evb,
				    "\"errstr\" : \"could not open local file %s\", ",
				    local_path);
		goto end;
	}

	/*
	 * Open the HPSS file 
	 */
	hfd = hpss_Open(escaped_path, O_RDONLY, 0, NULL, NULL, NULL);
	if (hfd < 0) {
		evbuffer_add_printf(out_evb, "\"errno\" : \"%d\", ", errno);
		evbuffer_add_printf(out_evb,
				    "\"errstr\" : \"cannot open hpss-path %s.\", ",
				    escaped_path);
		goto end;
	}

	/*
	 * Copy the src file to the dst file 
	 */

	while ((lt64m(count64, size64)) || (count64 == 0)) {
		/*
		 * Read in the next chunk of data 
		 */
		read = hpss_Read(hfd, buf, BUFSIZE);
		if (read <= 0) {
			rc = 5;
			evbuffer_add_printf(out_evb, "\"errno\" : \"%d\", ",
					    errno);
			evbuffer_add_printf(out_evb,
					    "\"errstr\" : \"cannot read from hpss-path %s. Read 0 bytes.\", ",
					    escaped_path);
			goto end;
		}
		/*
		 * Write the data to standard output 
		 */
		written = write(fd, buf, read);
		if (written != read) {
			rc = 5;
			evbuffer_add_printf(out_evb, "\"errno\" : \"%d\", ",
					    errno);
			evbuffer_add_printf(out_evb,
					    "\"errstr\" : \"cannot write to local path %s. Wrote only %d bytes\", ",
					    local_path, written);
			goto end;
		}
		count64 = add64m(count64, cast64m(written));
		if (serverInfo.LogLevel <= LL_TRACE) {
			fprintf(serverInfo.LogFile, "%s:%d:: read %d bytes\n",
				__FILE__, __LINE__, read);
		}
	}
	if (serverInfo.LogLevel <= LL_DEBUG) {
		fprintf(serverInfo.LogFile,
			"%s:%d:: hpss_get(\"%s\") read %d bytes\n", __FILE__,
			__LINE__, escaped_path, read);
	}

 end:
	elapsed = double_time() - start;
	xfer_rate = ((double)cast32m(div64m(size64, 1024 * 1024))) / elapsed;
	evbuffer_add_printf(out_evb, "\"xfer_rate\" : \"%.3f\", ", xfer_rate);
	evbuffer_add_printf(out_evb, "\"elapsed\" : \"%.3f\"}", elapsed);

	/*
	 * Close the files 
	 */
	close(fd);
	hpss_Close(hfd);

	if (serverInfo.LogLevel <= LL_TRACE)
		fprintf(serverInfo.LogFile,
			"%s:%d:: Leaving method with with rc=%d, elapsed=%3.3f\n",
			__FILE__, __LINE__, rc, elapsed);
	fflush(serverInfo.LogFile);

	/*
	 * free if we do have an escaped path 
	 */
	if (escaped_path != given_path)
		free(escaped_path);
        /*
         * free buffer, if allocation was successful
         */
        if (buf)
            free(buf);

	return rc;
}
