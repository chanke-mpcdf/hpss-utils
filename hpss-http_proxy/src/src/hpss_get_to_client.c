/*!\file
*\brief read a file from HPSS and upload it to the client
*/
#include <unistd.h>

/*
 * helper
 */
#include "util.h"
#include "hpss_methods.h"

void end_connection(struct chunk_req_state *state) {
	evhttp_send_reply_end(state->req);
	event_free(state->timer);
	if (state->hfd > 0) {
		hpss_Close(state->hfd);
	}

        /*
         * free buffer, if allocation was successful
         */
        if (state->data_buf)
            free(state->data_buf);
	if (state->escaped_path)
	    free(state->escaped_path);

	free(state);
}

void
schedule_trickle(struct chunk_req_state *state, int ms)
{
        struct timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = ms * 1000;
        // XXX TODO why no base argument in evtimer_add? well, because it was already given in evtimer_new!
        evtimer_add(state->timer, &tv);
}

void fail(struct chunk_req_state *state, int rc, char *errstr, ...) {
	struct evbuffer *response_buf = evbuffer_new();
        va_list a_list;

        va_start(a_list, *errstr);
        evbuffer_add_printf(response_buf, "{ \"action\" : \"get_to_client\", \"errno\" : \"%d\",  \"errstr\" : \"", rc);
        evbuffer_add_vprintf(response_buf, errstr, a_list);
        evbuffer_add_printf(response_buf, "\" } ");
	fprintf(serverInfo.LogFile, "hpss_get_to_client failed with rc=%d, msg=", rc);
        vfprintf(serverInfo.LogFile, errstr, a_list);
        fprintf(serverInfo.LogFile, "\n");
        evhttp_send_reply_chunk(state->req, response_buf);
        evbuffer_free(response_buf);
        end_connection(state);
}

void
http_chunked_trickle_cb(evutil_socket_t fd, short events, void *arg)
{
        struct chunk_req_state *state = arg;
        struct evbuffer *chunk_evb = evbuffer_new();
	int read = 0;

	read = hpss_Read(state->hfd, state->data_buf, state->buffer_size);
	if (read <= 0) {
		fail(state, read, "Cannot read from hpss-file: %s. Got rc=%d.", state->escaped_path, read);
		return;
	}
	if (serverInfo.LogLevel <= LL_DEBUG) {
		fprintf(serverInfo.LogFile, "%s:%d:: read %d bytes\n",
			__FILE__, __LINE__, read);
		fprintf(serverInfo.LogFile, "state->count64=%lu, state->size64=%lu\n", state->count64, state->size64);
		fflush(serverInfo.LogFile);
	}
	state->count64 = add64m(state->count64, cast64m(read));

	/*
	 * Write the data to the output buffer
	 */

        evbuffer_add(chunk_evb, state->data_buf, read);
        evhttp_send_reply_chunk(state->req, chunk_evb);
        evbuffer_free(chunk_evb);
        if (state->count64 == state->size64) {
		return;
        } else {
		schedule_trickle(state, 0);
	}
}

int
hpss_get_to_client(struct chunk_req_state *state, char *given_path,
		  const char *flags)
{

	int rc = 0;
	char first_illegal_flag;
	char *escaped_path;
	hpss_fileattr_t Attr;


	/*
	 * initialize
	 */
	escaped_path = escape_path(given_path, flags);
	state->escaped_path = (char *) malloc(strlen(escaped_path) + 1);

        strncpy(state->escaped_path, (const char *)escaped_path, strlen(escaped_path)+1);
	if (serverInfo.LogLevel <= LL_DEBUG) {
		fprintf(serverInfo.LogFile,
			"%s:%d:: entering hpss_get_to_client with escaped_path=%s and buffer_size=%d\n",
			__FILE__, __LINE__, state->escaped_path, state->buffer_size);
		fflush(serverInfo.LogFile);
	}

	if ((first_illegal_flag = check_given_flags("", flags))) {
                rc = 22;
                fail(state, rc, "Illegal flag %c given." , first_illegal_flag);
		goto end;
	}
	/*
	 * Get the length of the source file
	 */
        rc = hpss_FileGetAttributes(escaped_path, &Attr);
	if (rc) {
		fail(state, rc, "Cannot stat hpss-path %s.", escaped_path);
		goto end;
	}

	/*
	 * Allocate the data buffer
	 */

	state->size64 = Attr.Attrs.DataLength;

	/*
	 * Open the HPSS file
	 */

	state->hfd = hpss_Open(escaped_path, O_RDONLY, 0, NULL, NULL, NULL);
	if (state->hfd < 0) {
                rc = errno;
                fail(state, rc, "Cannot open hpss-path %s.", escaped_path);
		goto end;
	}

	/*
	 * Copy the src file to the dst file
	 */

	if (serverInfo.LogLevel <= LL_DEBUG) {
			fprintf(serverInfo.LogFile, "%s:%d:: should read %lu bytes\n",
				__FILE__, __LINE__, state->size64);
	}


	if (serverInfo.LogLevel <= LL_TRACE)
		fprintf(serverInfo.LogFile,
			"%s:%d:: Timer scheduled.\n",
			__FILE__, __LINE__);

        state->data_buf = (char *) malloc(state->buffer_size);
	schedule_trickle(state, 0);
end:
	fflush(serverInfo.LogFile);

	return rc;
}
