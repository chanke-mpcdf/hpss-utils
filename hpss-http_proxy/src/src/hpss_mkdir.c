/*!\file 
*\brief create a directory or directory-tree in hpss
*/

/*
 * helper 
 */
#include "util.h"
#include "hpss_methods.h"

/*!\brief Execute hpss_hpss_Mkdir.
* If flag "p" is given, create missing parent-dirs.
*/
int hpss_mkdir(struct evbuffer *out_evb /**< [in] ev buffer */ ,
	       char *given_path, /**< [in] path to work on. */
	       const char *flags /**< [in] "p" for parents. */ ,
	       char *mode_str /**< [in] mode_str a la "rwx------"*/ )
{

	mode_t posix_mode;
	char const *subtoken, *str;
	char *saveptr;
	char fullPath[1024];
	int rc;
	char first_illegal_flag;
	double start, elapsed;
	char *escaped_path;

	/*
	 * initialize 
	 */
	start = double_time();
	escaped_path = escape_path(given_path, flags);
	posix_mode = get_posix_mode(mode_str);

	if (serverInfo.LogLevel < LL_TRACE) {
		fprintf(serverInfo.LogFile,
			"%s:%d:: Entering method with hpss_path=%s, flags=%s\n",
			__FILE__, __LINE__, escaped_path, flags);
		fprintf(serverInfo.LogFile,
			"%s:%d:: mode_str=%s == posix_mode=%o\n", __FILE__,
			__LINE__, mode_str, posix_mode);
	}

	evbuffer_add_printf(out_evb, "{");
	evbuffer_add_printf(out_evb, "\"action\" : \"mkdir\", ");
	if ((first_illegal_flag = check_given_flags("p", flags))) {
		evbuffer_add_printf(out_evb, " \"errno\" : \"22\", ");
		evbuffer_add_printf(out_evb,
			" \"errstr\" : \"Illegal flag %c given.\", ", first_illegal_flag);
		goto end;
	}
	if (have_flag("p")) {
		if (escaped_path[0] == '/')
			strcpy(fullPath, "/");
		for (str = escaped_path;; str = NULL) {
			subtoken = strtok_r((char *)str, "/", &saveptr);
			if (subtoken == NULL)
				break;
			strcat(fullPath, subtoken);
			strcat(fullPath, "/");
			if ((rc = hpss_Chdir(fullPath)) < 0) {
				if ((rc = hpss_Mkdir(fullPath, posix_mode) < 0)) {
					if (serverInfo.LogLevel <= LL_ERROR) {
						fprintf(serverInfo.LogFile,
							"hpss_Mkdir(%s, %o) failed with rc=%d\n",
							fullPath, posix_mode,
							-rc);
					}
					evbuffer_add_printf(out_evb,
							    "\"errno\" : \"%d\", ",
							    -rc);
					evbuffer_add_printf(out_evb,
							    "\"errstr\" : \"hpss_Mkdir %s failed.\", ",
							    fullPath);
				}
			}
		}
	} else {
		if ((rc = hpss_Mkdir((char *)escaped_path, posix_mode)) < 0) {
			if (serverInfo.LogLevel <= LL_ERROR) {
				fprintf(serverInfo.LogFile,
					"hpss_Mkdir(%s, %o) failed with rc=%d\n",
					escaped_path, posix_mode, rc);
				evbuffer_add_printf(out_evb,
						    "\"errno\" : \"%d\", ",
						    -rc);
				evbuffer_add_printf(out_evb,
						    "\"errstr\" : \"hpss_Mkdir %s failed.\", ",
						    escaped_path);
			}
		}
	}

end:
	elapsed = double_time() - start;
	evbuffer_add_printf(out_evb, " \"elapsed\" : \"%.3f\"}", elapsed);
	if (serverInfo.LogLevel <= LL_TRACE) {
		fprintf(serverInfo.LogFile,
			"%s:%d:: leaving with rc=%d, elapsed=%3.3f\n", __FILE__,
			__LINE__, rc, elapsed);
		fflush(serverInfo.LogFile);
	}
	/*
	 * free if we do have an escaped path 
	 */
	if (escaped_path != given_path)
		free(escaped_path);
	return rc;
}
