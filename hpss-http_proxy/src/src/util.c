/*!\file 
*\brief utility functions required by multiple hpss_ functions
*/
#include <fcntl.h>
#include <sys/time.h>
#include <unistd.h>

#include "util.h"

/*
 * ! \brief authenticate against HPSS with a kerberos keytab 
 */
int authenticate(char *UserName, char *PathToKeytab, char *AuthMech)
{
	int rc;
	hpss_authn_mech_t hpss_authn_mech = -1;

	if (UserName && PathToKeytab && AuthMech) {
		if( access( PathToKeytab, F_OK ) != -1 ) {
			if ( strcmp(AuthMech, "unix")  == 0 ) {
				hpss_authn_mech = hpss_authn_mech_unix;
			} else if ( strcmp(AuthMech, "krb5") == 0 ) {
				hpss_authn_mech = hpss_authn_mech_krb5;
			}
			if ( hpss_authn_mech == -1 ) {
				fprintf(stderr, "Unknown authentication Mech : %s. Valid options are \"krb5\" and \"unix\".\n", AuthMech);
				exit(1);
			}
			rc = hpss_SetLoginCred(UserName, hpss_authn_mech_krb5,
					       hpss_rpc_cred_client,
					       hpss_rpc_auth_type_keytab,
					       PathToKeytab);
			if (rc != 0) {
				fprintf(serverInfo.LogFile,
					"hpss_SetLoginCred(): %s\n",
					strerror(-rc));
				exit(-rc);
			}
		} else {
			fprintf(serverInfo.LogFile, "Cannot read keytab: %s\n",
				PathToKeytab);
			exit(2);
		}
	} else {
		fprintf(serverInfo.LogFile,
                        "All of username (-u <username> ), path to keytab (-t "\
                        "<path to keytab>) and -m <authentication mechnism> "\
                        "required.\n");

		exit(2);
	}
	return 0;
}

/*
 * ! \brief cleanup HPSS resources and exit.
 */
int hpss_exit(int rc)
{
	/*
	 * unauthenticate 
	 */
	hpss_ClientAPIReset();
	hpss_PurgeLoginCred();
	exit(rc);
}

/*
 * ! \brief translate a mode-string like "rwxr-x---" to mode_t 
 */
mode_t get_posix_mode(const char *mode_str)
{
	int i;
	static char allowed_mode_str[9] = "rwxrwxrwx";
	mode_t posix_mode = 0;
	if (strlen(mode_str) != 9) {
		fprintf(serverInfo.LogFile,
			"mode_str must be 9 characters long.");
	}
	for (i = 0; i < 8; i++) {
		if ((mode_str[i] != '-')
		    && mode_str[i] != allowed_mode_str[i % 3])
			fprintf(serverInfo.LogFile,
				"illegal character in mode_str at position %d. Should be '-' or '%c'",
				i, allowed_mode_str[i % 3]);
	}

	/*
	 * this is lengthy, but readable 
	 */
	if (mode_str[0] == 'r')
		posix_mode |= S_IRUSR;
	if (mode_str[1] == 'w')
		posix_mode |= S_IWUSR;
	if (mode_str[2] == 'x')
		posix_mode |= S_IXUSR;
	if (mode_str[3] == 'r')
		posix_mode |= S_IRGRP;
	if (mode_str[4] == 'w')
		posix_mode |= S_IWGRP;
	if (mode_str[5] == 'x')
		posix_mode |= S_IXGRP;
	if (mode_str[6] == 'r')
		posix_mode |= S_IROTH;
	if (mode_str[7] == 'w')
		posix_mode |= S_IWOTH;
	if (mode_str[8] == 'x')
		posix_mode |= S_IXOTH;

	return posix_mode;
}

/*
 * ! \brief translate a mode_t to a string like "rwxr-x---" 
 */
void get_mode_str(char mode_str[], mode_t posix_mode)
{
	if (posix_mode & S_IRUSR)
		mode_str[0] = 'r';
	if (posix_mode & S_IWUSR)
		mode_str[1] = 'w';
	if (posix_mode & S_IXUSR)
		mode_str[2] = 'x';
	if (posix_mode & S_IRGRP)
		mode_str[3] = 'r';
	if (posix_mode & S_IWGRP)
		mode_str[4] = 'w';
	if (posix_mode & S_IXGRP)
		mode_str[5] = 'x';
	if (posix_mode & S_IROTH)
		mode_str[6] = 'r';
	if (posix_mode & S_IWOTH)
		mode_str[7] = 'w';
	if (posix_mode & S_IXOTH)
		mode_str[8] = 'x';
	mode_str[9] = '\0';

	return;
}

/*
 * ! \brief return timestamp of now in double-precision 
 */
double double_time()
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return ((double)(tv.tv_sec) + (double)(tv.tv_usec) / 1000000.0);
}

/*
 * ! \brief escape a UTF-8 encoded string into an ASCII. * usually used
 * for path-names only. * counterpart to unescape_path * * This function
 * allocates memory which must be freed by the caller. 
 */
char *escape_path(const char *path, const char *flags)
{
	/*
	 * escape path if required. 
	 */
	char *escaped_path;
	int processedBytes;

	if (serverInfo.LogLevel < LL_DEBUG)
		fprintf(serverInfo.LogFile, "escape_path: got path %s\n", path);

	if (have_flag("U") || serverInfo.do_encode) {
		escaped_path = malloc(MAX_FILENAME_LEN * sizeof(char));
		if (escaped_path == NULL) {
			fprintf(serverInfo.LogFile, "%s:%d:: malloc error.",
				__FILE__, __LINE__);
			return NULL;
		}
		processedBytes =
		    u8_escape(escaped_path, MAX_FILENAME_LEN, (char *)path, 1);
		if (serverInfo.LogLevel < LL_DEBUG)
			fprintf(serverInfo.LogFile,
				"escape_path: path=\"%s\", processedBytes=%d, strlen=%zd\n",
				path, processedBytes, strlen(path));
		return escaped_path;
	}

	return (char *)path;
}

/*
 * ! \brief unescape ASCII string into UTF-8. * counterpart to escape_path
 * * usually used for path-names only. * * This function allocates memory
 * which must be freed by the caller. 
 */
char *unescape_path(const char *path, const char *flags)
{
	char *unescaped_path;
	int processedBytes;

	if (serverInfo.LogLevel < LL_DEBUG)
		fprintf(serverInfo.LogFile, "unescape_path: got path %s\n",
			path);

	if (have_flag("U") || serverInfo.do_encode) {
		unescaped_path = malloc(MAX_FILENAME_LEN * sizeof(char));
		if (unescaped_path == NULL) {
			fprintf(serverInfo.LogFile, "%s:%d:: malloc error.",
				__FILE__, __LINE__);
			return NULL;
		}
		processedBytes =
		    u8_unescape(unescaped_path, MAX_FILENAME_LEN, (char *)path);
		if (serverInfo.LogLevel < LL_DEBUG)
			fprintf(serverInfo.LogFile,
				"unescape_path: path=\"%s\", processedBytes=%d, strlen=%zd\n",
				path, processedBytes, strlen(path));
		return unescaped_path;
	}

	return (char *)path;
}

/*
 * ! \brief return boolean if mtime of HPSS-object lies in given
 * time-range. * 
 */
int filter_by_mtime(char *escaped_path, int mtime, int older, int newer)
{

	if (older != -1 && mtime > older) {
		if (serverInfo.LogLevel <= LL_TRACE) {
			fprintf(serverInfo.LogFile,
				"%s:%d:: ignoring \"%s\", too young: %d > %d\n",
				__FILE__, __LINE__, escaped_path, mtime, older);
			fflush(serverInfo.LogFile);
		}
		return 1;
	}
	if (newer != -1 && mtime < newer) {
		if (serverInfo.LogLevel <= LL_TRACE) {
			fprintf(serverInfo.LogFile,
				"%s:%d:: ignoring \"%s\", too old: %d < %d \n",
				__FILE__, __LINE__, escaped_path, mtime, newer);
			fflush(serverInfo.LogFile);
		}
		return 1;
	}
	return 0;
}
