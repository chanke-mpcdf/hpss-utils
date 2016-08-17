/*!\file
*\brief prototypes for server methods
*/

int hpss_ls(struct evbuffer *out_evb, char *given_path,
	    const char *flags, int depth, int newer_than, int older_than);
int hpss_mkdir(struct evbuffer *out_evb, char *given_path,
	       const char *flags, char *mode_str);
int hpss_rm(struct evbuffer *out_evb, char *given_path, const char *flags);
int hpss_put_from_proxy(struct evbuffer *out_evb, char *given_path,
		   const char *flags, char *local_path,
		   char *mode_str, int cos_id);
int hpss_chmod(struct evbuffer *out_evb, char *given_path,
	       const char *flags, int max_recursion_level,
	       int older, int newer, char *mode_str);
int hpss_chown(struct evbuffer *out_evb, char *given_path,
	       const char *flags, int uid, int gid);
int hpss_stat(struct evbuffer *out_evb, char *given_path,
	      const char *flags, int max_recursion_level, int older, int newer);
int hpss_link(struct evbuffer *out_evb, char *given_path,
	      const char *flags, char *dest_path);
int hpss_get_storage_info(struct evbuffer *out_evb,
			  char *given_path, const char *flags);
int hpss_rename(struct evbuffer *out_evb, char *given_path,
		const char *flags, char *new_path);
int hpss_get_udas(struct evbuffer *out_evb, char *given_path,
		  const char *flags);
int hpss_set_uda(struct evbuffer *out_evb, char *given_path,
		 const char *flags, char *key, char *value);
int hpss_del_uda(struct evbuffer *out_evb, char *given_path,
		 const char *flags, char *key);
int hpss_get_to_proxy(struct evbuffer *out_evb,
		      char *given_path, const char *flags,
		      char *local_path, char *mode_str);
