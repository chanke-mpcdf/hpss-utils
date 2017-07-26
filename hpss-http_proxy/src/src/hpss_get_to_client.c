/*!\file 
*\brief read a file from HPSS and upload it to the client
*/
#include <unistd.h>

/*
 * helper 
 */
#include "util.h"
#include "hpss_methods.h"

#define BUFSIZE (32*1024*1024)


void fail(struct evbuffer *out_evb, int rc, char *errstr, ...) {
        va_list a_list;
        va_start(a_list, *errstr);
	evbuffer_add_printf(out_evb, "{ \"action\" : \"get_to_client\", ");
	evbuffer_add_printf(out_evb, " \"errno\" : \"%d\", ", rc);
	evbuffer_add_printf(out_evb, " \"errstr\" : \"");
        evbuffer_add_vprintf(out_evb, errstr, a_list);
        evbuffer_add_printf(out_evb, "\" } ");
	fprintf(serverInfo.LogFile, "hpss_get_to_client failed with rc=%d, msg=", rc);
        vfprintf(serverInfo.LogFile, errstr, a_list);
        fprintf(serverInfo.LogFile, "\n");
}

int
hpss_get_to_client(struct evbuffer *out_evb, char *given_path,
		  const char *flags)
{

	int rc = 0;
	char first_illegal_flag;
	char *buf = NULL;
	char *escaped_path;
	int read = 0, hfd = -1;
	u_signed64 count64 = 0, size64 = 0;
	double start, elapsed, xfer_rate;
	hpss_fileattr_t Attr;

	/*
	 * initialize 
	 */
	start = double_time();
	escaped_path = escape_path(given_path, flags);

	if (serverInfo.LogLevel <= LL_TRACE) {
		fprintf(serverInfo.LogFile,
			"%s:%d:: entering hpss_get_to_client with escaped_path=%s\n",  
			__FILE__, __LINE__, escaped_path);
		fflush(serverInfo.LogFile);
	}

	if ((first_illegal_flag = check_given_flags("", flags))) {
                rc = 22;
                fail(out_evb, rc, "Illegal flag %c given." , first_illegal_flag);
		goto end;
	}
	/*
	 * Get the length of the source file 
	 */
        rc = hpss_FileGetAttributes(escaped_path, &Attr);
	if (rc) {
		fail(out_evb, rc, "cannot stat hpss-path %s.", escaped_path); 
		goto end;
	}

	size64 = Attr.Attrs.DataLength;

	/*
	 * Allocate the data buffer 
	 */
	buf = (char *)malloc(BUFSIZE);
	if (buf == NULL) {
		rc = 12;
		fail(out_evb, rc, "malloc error. The dungeon collapses.");
		goto end;
	}

	/*
	 * Open the HPSS file 
	 */

	hfd = hpss_Open(escaped_path, O_RDONLY, 0, NULL, NULL, NULL);
	if (hfd < 0) {
                rc = errno;
                fail(out_evb, rc, "cannot open hpss-path %s.", escaped_path);
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
			fail(out_evb, rc, "Cannot read from hpss-path %s.", escaped_path);
			goto end;
		}
		/*
		 * Write the data to the output buffer 
		 */
		rc = evbuffer_add(out_evb, buf, read);
		if (rc == -1) {
			rc = 5;
			fail(out_evb, rc, "Failed writing to client.");
			goto end;
		}
		count64 = add64m(count64, cast64m(read));
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

	/*
	 * Close the files, if required
	 */
	if (hfd > 0) {
		hpss_Close(hfd);
	}

	if (serverInfo.LogLevel <= LL_TRACE)
		fprintf(serverInfo.LogFile,
			"%s:%d:: Leaving method with with rc=%d, elapsed=%.3f, xfer_rate=%.3f\n",
			__FILE__, __LINE__, rc, elapsed, xfer_rate);
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
