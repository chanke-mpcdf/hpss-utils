/*!\file 
*\brief get internal HPSS storage info of a file
*/

/*
 * helper 
 */
#include "util.h"
#include "hpss_methods.h"

int hpss_get_storage_info(struct evbuffer *out_evb, /**< [in] mongoose connection */
			  char *given_path,
		      /** [in] path to work on */
			  const char *flags /** [in] allowed flags "u" */ )
{
	int rc, i, j, k;
	int already_printed_level = 0;
	hpss_xfileattr_t attributes;
	char size_str[64];
	char *escaped_path;
	double start, elapsed;

	/*
	 * initialize 
	 */
	start = double_time();
	escaped_path = escape_path(given_path, flags);

	if (serverInfo.LogLevel <= LL_TRACE) {
		fprintf(serverInfo.LogFile,
			"%s:%d:: Entering method with hpss_path=%s, flags=%s\n",
			__FILE__, __LINE__, escaped_path, flags);
		fflush(serverInfo.LogFile);
	}

	evbuffer_add_printf(out_evb, "{");

	/*
	 * Get extended attributes of a file
	 */
	if ((rc =
	     hpss_FileGetXAttributes(escaped_path,
				     API_GET_STATS_FOR_ALL_LEVELS, 0,
				     &attributes)) < 0) {
		if (serverInfo.LogLevel <= LL_ERROR) {
			fprintf(serverInfo.LogFile,
				"hpss_FileGetXAttributes(%s, ...) failed with rc=%d\n",
				escaped_path, -rc);
		}
		evbuffer_add_printf(out_evb, "\"errno\" : \"%d\", ", -rc);
		evbuffer_add_printf(out_evb,
				    "\"errstr\" : \"could not get attributes for %s\" ",
				    escaped_path);
		evbuffer_add_printf(out_evb, "}");
		return rc;
	}

	/*
	 * Loop through all levels of the returned structure
	 */

	evbuffer_add_printf(out_evb, "\"levels\" : [ ");
	already_printed_level = 0;
	for (i = 0; i < HPSS_MAX_STORAGE_LEVELS; i++) {

		/*
		 * Only interested in those levels with data and that are tape
		 */
		if (attributes.SCAttrib[i].Flags &&
		    (BFS_BFATTRS_DATAEXISTS_AT_LEVEL ||
		     BFS_BFATTRS_LEVEL_IS_TAPE)) {
			/*
			 * Print the number of bytes at this level
			 */
			size_str[0] = '\0';
			u64_to_decchar(attributes.SCAttrib[i].BytesAtLevel,
				       size_str);
			if (serverInfo.LogLevel <= LL_TRACE) {
				fprintf(serverInfo.LogFile,
					"%s:%d:: Level %d: Bytes =%s\n",
					__FILE__, __LINE__, i, size_str);
				fflush(serverInfo.LogFile);
			}
			/*
			 * Potential that a level has more than 1 VV
			 */
			if (already_printed_level) {
				evbuffer_add_printf(out_evb, ", ");
			} else {
				already_printed_level = 1;
			}
			evbuffer_add_printf(out_evb, " { \"num\" : \"%d\", ",
					    i);
			evbuffer_add_printf(out_evb, "\"bytes\" : \"%s\", ",
					    size_str);
			evbuffer_add_printf(out_evb, " \"VV\" :  [ ");
			for (j = 0; j <= attributes.SCAttrib[i].NumberOfVVs;
			     j++) {
				pv_list_t *pv_list_ptr;

				evbuffer_add_printf(out_evb,
						    " { \"num\" : \"%d\", ", j);
				evbuffer_add_printf(out_evb,
						    "\"relative_position\" : \"%d\"",
						    attributes.SCAttrib[i].
						    VVAttrib[j].RelPosition);
				pv_list_ptr =
				    attributes.SCAttrib[i].VVAttrib[j].PVList;
				if (pv_list_ptr != NULL) {
					/*
					 * Each VV may be composed of multiple PVs as well
					 */
					evbuffer_add_printf(out_evb, ", ");	/* After
										 * relative_position 
										 */
					evbuffer_add_printf(out_evb,
							    "\"PV\" : [ ");
					for (k = 0;
					     k < pv_list_ptr->List.List_len;
					     k++) {
						evbuffer_add_printf(out_evb,
								    "{ \"num\" : \"%d\", ",
								    k);
						evbuffer_add_printf(out_evb,
								    "\"uuid\" : \"%s\" ",
								    pv_list_ptr->List.
								    List_val[k].
								    Name);
						if (k <
						    pv_list_ptr->List.List_len -
						    1) {
							evbuffer_add_printf
							    (out_evb, "}, ");
						} else {
							evbuffer_add_printf
							    (out_evb, "}");
						}
					}
					evbuffer_add_printf(out_evb, " ]  ");
				}
				/*
				 * decide if we need a trailing comma or not 
				 */
				if (j < attributes.SCAttrib[i].NumberOfVVs) {
					evbuffer_add_printf(out_evb, "}, ");
				} else {
					evbuffer_add_printf(out_evb, "}");
				}
			}
			evbuffer_add_printf(out_evb, " ]}  ");
		}
		/*
		 * Go clean up memory allocation
		 */
		for (j = 0; j < attributes.SCAttrib[i].NumberOfVVs; j++) {
			if (attributes.SCAttrib[i].VVAttrib[j].PVList != NULL) {
				free(attributes.SCAttrib[i].VVAttrib[j].PVList);
			}
		}
	}
	evbuffer_add_printf(out_evb, " ], ");

	elapsed = double_time() - start;
	evbuffer_add_printf(out_evb, " \"elapsed\" : \"%.3f\"}", elapsed);

	/*
	 * free if we do have an escaped path 
	 */
	if (escaped_path != given_path)
		free(escaped_path);
	return rc;
}
