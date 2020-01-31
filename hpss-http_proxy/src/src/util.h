/*!\file 
*\brief some magic macros and globals
*/
#ifndef HAVE_UTIL_H

#define HAVE_UTIL_H 1

#include<stdio.h>
#include <hpss_api.h>
#include <u_signed64.h>
#include "utf8.h"
#include <errno.h>

#include <event2/event.h>
#include <event2/http.h>
#include <event2/buffer.h>
#include <event2/util.h>
#include <event2/keyvalq_struct.h>

#define LL_TRACE 	0
#define LL_DEBUG 	10
#define LL_INFO 	20
#define LL_WARN 	30
#define LL_ERROR        40
#define LL_CRIT 	50
#define LL_FATAL 	60

#define UDA_HPSS_PREFIX "/hpss"
#define MAX_FILENAME_LEN 1024

#define IGNORE_BY_TIME -666

/*
 * ! \def MAX_QS_KEY_LEN \brief Maximum length of an querystring
 * parameter-name 
 */
#define MAX_QS_KEY_LEN 20

/*
 * ! \def MAX_QS_VALUE_LEN \brief Maximum length of an querystring
 * parameter-value 
 */
#define MAX_QS_VALUE_LEN 2048

/*
 * ! \def MIN_ALLOWED_CHAR \brief Mnimum ord-number of a char in the
 * action. " " 
 */
#define MIN_ALLOWED_CHAR 32
/*
 * ! \def MAX_ALLOWED_CHAR \brief Mnimum ord-number of a char in the
 * action.  "~" 
 */
#define MAX_ALLOWED_CHAR 126

typedef struct {
	int LogLevel;
	FILE *LogFile;
	int do_encode;
	char *all_actions;
	char *allowed_actions;
	char *forbidden_actions;
	char *uda_prefix;
} serverInfo_t;

struct chunk_req_state {
	struct event_base *base;
	struct evhttp_request *req;
	struct event *timer;
	char *escaped_path;
        int hfd;
        u_signed64 size64;
        u_signed64 count64;
	int buffer_size;
        char *data_buf;
};

serverInfo_t serverInfo;

int authenticate(char *UserName, char *PathToKeytab, char *AuthMech);
double double_time();

void get_mode_str(char mode_str[], mode_t posix_mode);
mode_t get_posix_mode(const char *mode_str);
char *escape_path(const char *path, const char *flags);
char *unescape_path(const char *path, const char *flags);
int filter_by_mtime(char *escaped_path, int mtime, int older, int newer);
char check_given_flags(const char *allowed_flags, const char *given_flags);

#define have_flag(X) strpbrk(flags,X)

#define LOG_RETURN_INT(rc) { if (serverInfo.LogLevel <= LL_DEBUG) { fprintf(serverInfo.LogFile,"%s:%d:: returning %d\n", __FILE__, __LINE__, rc); }\
                          return rc; }

#endif
