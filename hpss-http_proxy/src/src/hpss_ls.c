/*!file
*\brief ls in HPSS. supports recursion and filtering by time
*/

#include <libgen.h>

/*
 * helper 
 */
#include "util.h"
#include "hpss_methods.h"

struct hpss_ls_payload {
	const char *flags;
	int older;
	int newer;
	int max_recursion_level;
};

int print_x_attributes(struct evbuffer *out_evb, char *escaped_path);
int print_attributes(struct evbuffer *out_evb,
		     const char *path, hpss_Attrs_t AttrOut, const char *flags);
int recurse(struct evbuffer *out_evb, char *escaped_path,
	    hpss_Attrs_t AttrOut, int recursion_level, int rc,
	    struct hpss_ls_payload *payloadP);

int
do_hpss_ls(struct evbuffer *out_evb, char *escaped_path,
	   hpss_Attrs_t AttrOut, int display_full_path, void *payloadP,
	   int enclosed_json)
{
	int rc = 0;
	struct hpss_ls_payload *payload;
	const char *flags, *unescaped_path;

	payload = (struct hpss_ls_payload *)payloadP;
	flags = payload->flags;

	if (serverInfo.LogLevel <= LL_TRACE) {
		fprintf(serverInfo.LogFile,
			"%s:%d:: entering do_hpss_ls with escaped_path=%s\n",
			__FILE__, __LINE__, escaped_path);
		fflush(serverInfo.LogFile);
	}

	if (enclosed_json) {
		evbuffer_add_printf(out_evb, "{");
	}
	if (display_full_path) {
		unescaped_path = unescape_path(escaped_path, flags);
	} else {
		unescaped_path = unescape_path(basename(escaped_path), flags);
	}
	evbuffer_add_printf(out_evb, "\"path\" : \"%s\" ", unescaped_path);

	if (have_flag("l")) {
		fprintf(serverInfo.LogFile, "%s:%d:: CHECKPOINT\n", __FILE__,
			__LINE__);
		fflush(serverInfo.LogFile);
		rc = print_attributes(out_evb, unescaped_path, AttrOut, flags);
		if (rc) {
			if (serverInfo.LogLevel < LL_ERROR) {
				fprintf(serverInfo.LogFile,
					"%s:%d:: got no attributes for \"%s\".\n",
					__FILE__, __LINE__, escaped_path);
				fflush(serverInfo.LogFile);
			}
		}
	}

	if (have_flag("X")) {
		rc = print_x_attributes(out_evb, escaped_path);
		if (rc) {
			if (serverInfo.LogLevel < LL_ERROR) {
				fprintf(serverInfo.LogFile,
					"%s:%d:: got no x_attributes for \"%s\".\n",
					__FILE__, __LINE__, escaped_path);
				fflush(serverInfo.LogFile);
			}
		}
	}
	if (enclosed_json) {
		evbuffer_add_printf(out_evb, "}");
	}

	if (serverInfo.LogLevel <= LL_TRACE) {
		fprintf(serverInfo.LogFile, "%s:%d:: do_hpss_ls returning %d\n",
			__FILE__, __LINE__, rc);
		fflush(serverInfo.LogFile);
	}
	return rc;
}

int
hpss_ls(struct evbuffer *out_evb, char *given_path, const char *flags,
	int max_recursion_level, int older, int newer)
{
	int rc;
	char *escaped_path;
	struct hpss_ls_payload payload;
	double start, elapsed;
	hpss_Attrs_t AttrOut;

	/*
	 * initialize 
	 */
	start = double_time();
	escaped_path = escape_path(given_path, flags);

	/*
	 * HPSS part 
	 */
	payload.flags = flags;
	payload.older = older;
	payload.newer = newer;
	payload.max_recursion_level = max_recursion_level;

	if (serverInfo.LogLevel <= LL_TRACE) {
		fprintf(serverInfo.LogFile,
			"%s:%d:: entering hpss_ls with path=%s\n", __FILE__,
			__LINE__, escaped_path);
		fprintf(serverInfo.LogFile, "%s:%d:: older=%d, newer=%d\n",
			__FILE__, __LINE__, older, newer);
		fflush(serverInfo.LogFile);
	}

	evbuffer_add_printf(out_evb, "{");
	evbuffer_add_printf(out_evb, "\"action\" : \"hpss_ls\", ");
	if ((rc = hpss_GetListAttrs(escaped_path, &AttrOut)) < 0) {
		evbuffer_add_printf(out_evb, " \"errno\" : \"%d\", ", -rc);
		evbuffer_add_printf(out_evb,
				    " \"errstr\" : \"Cannot read base dir\" ");
		goto end;
	}

	if (serverInfo.LogLevel <= LL_TRACE) {
		fprintf(serverInfo.LogFile, "%s:%d:: got AttrOut isdir=%d\n",
			__FILE__, __LINE__,
			(AttrOut.Type == NS_OBJECT_TYPE_DIRECTORY));
		fflush(serverInfo.LogFile);
	}

	rc = do_hpss_ls(out_evb, escaped_path, AttrOut, 1, &payload, 0);
	if (rc && rc != IGNORE_BY_TIME) {
		evbuffer_add_printf(out_evb, "}");
		return rc;
	}
	if (rc == IGNORE_BY_TIME) {
		evbuffer_add_printf(out_evb, "}");
		return 0;
	}

	/*
	 * is dir 
	 */
	if (AttrOut.Type == NS_OBJECT_TYPE_DIRECTORY) {
		evbuffer_add_printf(out_evb, ", \"contents\" : [");
		if (have_flag("r")) {
			rc = recurse(out_evb, escaped_path, AttrOut, 0, 0,
				     &payload);
		} else {
			rc = recurse(out_evb, escaped_path, AttrOut, 1, 0,
				     &payload);
		}
		evbuffer_add_printf(out_evb, "]");
	}

 end:
	elapsed = double_time() - start;
	evbuffer_add_printf(out_evb, ", \"elapsed\" : \"%.3f\" }", elapsed);

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

int print_x_attributes(struct evbuffer *out_evb, char *escaped_path)
{
	int i, j, k, rc = 0;
	int Flags = API_GET_STATS_FOR_ALL_LEVELS;
	int StorageLevel = 0;
	hpss_xfileattr_t AttrOut;
	bf_sc_attrib_t *scattr_ptr;

	if (serverInfo.LogLevel <= LL_TRACE)
		fprintf(serverInfo.LogFile,
			"%s:%d:: getting xattrs for path=%s\n", __FILE__,
			__LINE__, escaped_path);
	evbuffer_add_printf(out_evb, ", \"extended_attributes\" : {");
	rc = hpss_FileGetXAttributes(escaped_path, Flags, StorageLevel,
				     &AttrOut);
	if (rc) {
		evbuffer_add_printf(out_evb,
				    "\"errstrt\" : \"Cannot get extended attributes\", \"errno\" : \"%d\"}",
				    rc);
		return rc;
	}

	evbuffer_add_printf(out_evb, "\"storage_levels\" : [");
	for (i = 0; i < HPSS_MAX_STORAGE_LEVELS; i++) {
		if (i > 0) {
			evbuffer_add_printf(out_evb, ", ");
		}
		evbuffer_add_printf(out_evb, "{ \"level\" : \"%d\"", i);
		scattr_ptr = &AttrOut.SCAttrib[i];
		if (scattr_ptr->Flags & BFS_BFATTRS_DATAEXISTS_AT_LEVEL) {
			if (scattr_ptr->Flags & BFS_BFATTRS_LEVEL_IS_DISK) {
				evbuffer_add_printf(out_evb,
						    ", \"media\" : \"disk\"");
			}
			if (scattr_ptr->Flags & BFS_BFATTRS_LEVEL_IS_TAPE) {
				evbuffer_add_printf(out_evb,
						    ", \"media\" : \"tape\"");
			}
			evbuffer_add_printf(out_evb, ", \"size\" : \"%lu\"",
					    scattr_ptr->BytesAtLevel);
			/*
			 * Potential that a level has more than 1 VV
			 */
			evbuffer_add_printf(out_evb,
					    ", \"VirtualVolumes\" : {");
			for (j = 0; j <= scattr_ptr->NumberOfVVs; j++) {
				if (j > 0) {
					evbuffer_add_printf(out_evb, ", ");
				}
				pv_list_t *pv_list_ptr;

				evbuffer_add_printf(out_evb, "\"VV\" : \"%d\"",
						    j);
				evbuffer_add_printf(out_evb,
						    ", \"realtivePosition\" : \"%d\"",
						    scattr_ptr->
						    VVAttrib[j].RelPosition);
				pv_list_ptr = scattr_ptr->VVAttrib[j].PVList;
				evbuffer_add_printf(out_evb,
						    ", \"PhysicalVolumes\" : {");
				if (pv_list_ptr != NULL) {
					/*
					 * Each VV may be composed of multiple PVs as well
					 */
					for (k = 0;
					     k < pv_list_ptr->List.List_len;
					     k++) {
						if (k > 0) {
							evbuffer_add_printf
							    (out_evb, ", ");
						}
						evbuffer_add_printf(out_evb,
								    "\"PV\" : {");
						evbuffer_add_printf(out_evb,
								    "\"ID\" : \"%d\"",
								    k);
						evbuffer_add_printf(out_evb,
								    ", \"Name\" : \"%s\"",
								    pv_list_ptr->List.
								    List_val[k].
								    Name);
						evbuffer_add_printf(out_evb,
								    "}");
					}
				}
				evbuffer_add_printf(out_evb, "}");	/* PhysicalVolumes 
									 */
			}
			evbuffer_add_printf(out_evb, "}");	/* VirtualVolumes */

		}
		evbuffer_add_printf(out_evb, "}");
	}
	evbuffer_add_printf(out_evb, "]");
	evbuffer_add_printf(out_evb, "}");
	return 0;
}

/*
 * Here we use the unescaped path, but just for debugging 
 */
int
print_attributes(struct evbuffer *out_evb, const char *unescaped_path,
		 hpss_Attrs_t AttrOut, const char *flags)
{

	char mode_str[10] = "---------\0";
	mode_t posix_mode;
	char obj_type[2];

	API_ConvertModeToPosixMode(&AttrOut, &posix_mode);
	switch (posix_mode & S_IFMT) {
	case S_IFDIR:
		obj_type[0] = 'd';
		break;
	case S_IFLNK:
		obj_type[0] = 'l';
		break;
	case S_IFREG:
	default:
		obj_type[0] = '-';
		break;
	}
	obj_type[1] = '\0';
	get_mode_str(mode_str, posix_mode);
	if (serverInfo.LogLevel < LL_DEBUG)
		fprintf(serverInfo.LogFile,
			"%s:%d:: print_attributes: path = %s, mode_str = %s, posix_mode = %o m_time = %d\n",
			__FILE__, __LINE__, unescaped_path, mode_str,
			posix_mode, AttrOut.TimeModified);

	evbuffer_add_printf(out_evb, ", \"attributes\" : {");

	evbuffer_add_printf(out_evb, "\"type\" : \"%s\", ", obj_type);
	evbuffer_add_printf(out_evb, "\"mode_str\" : \"%s\", ", mode_str);
	evbuffer_add_printf(out_evb, "\"linkcount\" : \"%d\", ",
			    AttrOut.LinkCount);
	evbuffer_add_printf(out_evb, "\"uid\" : \"%d\", ", AttrOut.UID);
	evbuffer_add_printf(out_evb, "\"gid\" : \"%d\", ", AttrOut.GID);
	evbuffer_add_printf(out_evb, "\"cosid\" : \"%d\", ", AttrOut.COSId);
	evbuffer_add_printf(out_evb, "\"account\" : \"%d\", ", AttrOut.Account);
	evbuffer_add_printf(out_evb, "\"FamilyId\" : \"%d\", ",
			    AttrOut.FamilyId);
	evbuffer_add_printf(out_evb, "\"ctime\" : \"%d\", ",
			    AttrOut.TimeCreated);
	evbuffer_add_printf(out_evb, "\"atime\" : \"%d\", ",
			    AttrOut.TimeLastRead);
	evbuffer_add_printf(out_evb, "\"wtime\" : \"%d\", ",
			    AttrOut.TimeLastWritten);
	evbuffer_add_printf(out_evb, "\"mtime\" : \"%d\"",
			    AttrOut.TimeModified);

	evbuffer_add_printf(out_evb, " }");
	return 0;
}

/*
 * recurse into path. path must point to a directory algorithm is
 * top-down UTF8: must be called with escaped path. internally we get
 * other pathes from HPSS, so they are escaped anyway. 
 */

int
recurse(struct evbuffer *out_evb, char *escaped_path, hpss_Attrs_t AttrOut,
	int recursion_level, int rc, struct hpss_ls_payload *payloadP)
{
	struct hpss_dirent *ent;
	int dir_handle;
	int is_first_entry;
	char *unescaped_path;
	mode_t obj_posix_mode;

	is_first_entry = 1;
	if (serverInfo.LogLevel <= LL_TRACE)
		fprintf(serverInfo.LogFile,
			"%s:%d:: entering recurse with path=%s, max_recursion_level=%d, recursion_level=%d\n",
			__FILE__, __LINE__, escaped_path,
			payloadP->max_recursion_level, recursion_level);

	if (recursion_level > payloadP->max_recursion_level)
		return 0;

	if ((dir_handle = hpss_Opendir(escaped_path)) < 0) {
		rc = -dir_handle;
		/*
		 * ignore dirs we are not allowed to read without an error 
		 */
		if (rc == EACCES) {
			if (serverInfo.LogLevel <= LL_TRACE)
				fprintf(serverInfo.LogFile,
					"%s:%d::  ignoring because of EACCES\n",
					__FILE__, __LINE__);
			return 0;
		}
		unescaped_path = unescape_path(escaped_path, payloadP->flags);
	}

	ent =
	    (struct hpss_dirent *)malloc(sizeof(struct hpss_dirent) +
					 HPSS_MAX_FILE_NAME + 1);

	if (ent == (struct hpss_dirent *)NULL) {
		rc = ENOMEM;
		fprintf(serverInfo.LogFile, "%s:%d:: malloc error.", __FILE__,
			__LINE__);
		return rc;
	}

	if ((rc = hpss_Readdir(dir_handle, ent)) < 0) {
		rc = -rc;
		fprintf(serverInfo.LogFile,
			"hpss error: cannot read next item in dir %s, rc=%d",
			unescaped_path, rc);
		return rc;
	}
	while (strlen(ent->d_name) != 0) {
		char full_path[1024];

		/*
		 * build full path 
		 */
		strcpy(full_path, escaped_path);
		if (full_path[strlen(full_path) - 1] != '/')
			strcat((char *)full_path, "/");
		strcat((char *)full_path, ent->d_name);
		if (serverInfo.LogLevel <= LL_TRACE)
			fprintf(serverInfo.LogFile, "%s:%d:: processing %s\n",
				__FILE__, __LINE__, full_path);

		/*
		 * ignore "." and ".." 
		 */
		if (strcmp(ent->d_name, ".") == 0 ||
		    strcmp(ent->d_name, "..") == 0)
			goto read_next;

		if ((rc = hpss_GetListAttrs((char *)full_path, &AttrOut)) < 0) {
			rc = -rc;
			fprintf(serverInfo.LogFile,
				"hpss error:  cannot get attributes for entity %s  rc=%d",
				full_path, rc);
			return rc;
		}

		API_ConvertModeToPosixMode(&AttrOut, &obj_posix_mode);
		if ((obj_posix_mode & S_IFMT) == S_IFDIR) {

			if (is_first_entry) {
				is_first_entry = 0;
			} else {
				evbuffer_add_printf(out_evb, ", ");
			}

			evbuffer_add_printf(out_evb, "{ ");
			rc = do_hpss_ls(out_evb, full_path, AttrOut, 0,
					payloadP, 0);
			if (rc) {
				fprintf(serverInfo.LogFile,
					"%s:%d:: do_hpss_ls returned rc=%d\n",
					__FILE__, __LINE__, rc);
				return rc;
			}
			evbuffer_add_printf(out_evb, ", \"contents\" : [");
			rc = recurse(out_evb, full_path, AttrOut,
				     recursion_level + 1, rc, payloadP);
			evbuffer_add_printf(out_evb, " ]} ");
		} else {
			if (filter_by_mtime
			    (full_path, AttrOut.TimeCreated, payloadP->older,
			     payloadP->newer)) {
				goto read_next;
			} else {
				if (is_first_entry) {
					is_first_entry = 0;
				} else {
					evbuffer_add_printf(out_evb, ", ");
				}
				rc = do_hpss_ls(out_evb, full_path, AttrOut, 0,
						payloadP, 1);
				if (rc) {
					fprintf(serverInfo.LogFile,
						"%s:%d:: do_hpss_ls returned rc=%d\n",
						__FILE__, __LINE__, rc);
					return rc;
				}
			}
		}

		/*
		 * read next item in dir 
		 */
 read_next:
		if ((rc = hpss_Readdir(dir_handle, ent)) < 0) {
			unescaped_path =
			    unescape_path(escaped_path, payloadP->flags);
			rc = -rc;
			fprintf(serverInfo.LogFile,
				"hpss error: cannot read next item in dir %s, rc=%d",
				unescaped_path, rc);
			return rc;
		}
	}

	if (serverInfo.LogLevel < LL_TRACE)
		fprintf(serverInfo.LogFile,
			"%s:%d:: leaving recursion loop max_recursion_level=%d, recursion_level=%d\n",
			__FILE__, __LINE__, payloadP->max_recursion_level,
			recursion_level);

	hpss_Closedir(dir_handle);

	fflush(serverInfo.LogFile);

	return rc;
}
