#include <errno.h>
#include <fcntl.h>
#include <pwd.h>
#include <sys/time.h>
#include <stdio.h>
#include <unistd.h>
#include "hpss_api.h"
#include "hpss_Getenv.h"
#include "u_signed64.h"
#include "tools.h"

char *UserName = NULL;
int Keytab=0;
char *PathToKeytab = NULL;
char *AuthMech = NULL;
int newCredential=0;
hpss_authn_mech_t MechType = hpss_authn_mech_krb5;
api_config_t      APIConfig;

double
double_time ()
{
  struct timeval tv;

  gettimeofday (&tv, NULL);

  return ((double) (tv.tv_sec) + (double) (tv.tv_usec) / 1000000.0);
}


scan_cmdline_authargs (char c, char *optarg)
{
  switch (c)
    {
    case 'u':
      UserName = malloc (strlen (optarg) + 1);
      strcpy (UserName, optarg);
      return 0;
    case 't':
      PathToKeytab = malloc (strlen (optarg) + 1);
      strcpy (PathToKeytab, optarg);
      return 0;
    case 'm':
      AuthMech = optarg;
      return 0;
    case 'N':
      newCredential++;
      return 0;
    case 'k':
      Keytab++;
      newCredential++;
      return 0;
    }

  return 1;
}

authenticate ()
{
  /* inspired by scrub.c */
  int Status;
  if (!AuthMech) {
      AuthMech = hpss_Getenv("HPSS_CLIENT_AUTHN_MECH");
  }

  Status = hpss_AuthnMechTypeFromString(AuthMech, 
					  &MechType);
  if(Status != 0) {
      printf("invalid authentication type %s\n\n",AuthMech);
      common_usage();
      exit(-1);
    }

  if(newCredential && (MechType == hpss_authn_mech_krb5) && !Keytab) {
    printf("new login credential for krb5 is not supported\n\n");
    common_usage();
    exit(-1);
  }

  /* Set the authentication type to use */

  Status = hpss_GetConfiguration(&APIConfig);
  if(Status != 0) {
    fprintf(stderr, "hpss_GetConfiguration failed with Status=%d\n", Status);
    exit( Status );
  }

  APIConfig.AuthnMech = MechType;
  APIConfig.Flags |= API_USE_CONFIG;

  /* If needed get a login credential
   */

  if(newCredential) {
     struct passwd *pwent;
     char                 principal[HPSS_MAX_PRINCIPAL_NAME+1];
     char                 new_principal[HPSS_MAX_PRINCIPAL_NAME+1];
     char                 *passwd;
     hpss_rpc_auth_type_t authenticator_type = hpss_rpc_auth_type_passwd;
     void                 *authenticator;

     if(UserName == NULL) {
        *principal = '\0';
        pwent = getpwuid(getuid());
        if(pwent != NULL) {
	   strcpy(principal, pwent->pw_name);
        }
        printf("[%s] username: ",principal);
        fgets(new_principal,HPSS_MAX_PRINCIPAL_NAME,stdin);
        if(*new_principal != '\n') {
	   strcpy(principal, new_principal);
	   principal[strlen(principal)-1] = '\0';
        }
     }
     else
        strcpy(principal, UserName);

     if(!Keytab) {
	passwd = getpass("password: ");
	Status = hpss_SetLoginCred(principal,
				   MechType,
				   hpss_rpc_cred_client,
				   authenticator_type,
				   passwd);
        if(passwd != NULL && strlen(passwd))
	   memset(passwd,0x0,strlen(passwd));
	if(Status != 0) {
	   fprintf(stderr, "hpss_SetLoginCred failed with %s\n", strerror(-Status));
	   exit( Status );
	}
     } 
     else {
	if(PathToKeytab == NULL  && MechType == hpss_authn_mech_unix)
	   PathToKeytab = hpss_Getenv("HPSS_UNIX_KEYTAB_FILE");
	else if(PathToKeytab == NULL  && MechType == hpss_authn_mech_krb5)
	   PathToKeytab = hpss_Getenv("HPSS_KRB5_KEYTAB_FILE");
        if (PathToKeytab == NULL) {
           fprintf(stderr, "Path to keytabfile not given.\n");
           exit(1);
         }
        if (access(PathToKeytab, R_OK) == -1) {
           fprintf(stderr, "Cannot read keytabfile %s. Errno=%d\n", PathToKeytab, errno);
           exit(errno);
        }
        /* parse the authenticator string */
        Status = hpss_ParseAuthString(PathToKeytab,
                                      &MechType,
                                      &authenticator_type,
                                      &authenticator);
        if (Status != 0) {   
           fprintf(stderr, "hpss_ParseAuthString failed with %s\n",  strerror(-Status));
           exit( Status );
        }

	Status = hpss_SetLoginCred(principal,
				   MechType,
				   hpss_rpc_cred_client,
				   authenticator_type,
				   authenticator);
	if(Status != 0) {
	   fprintf(stderr, "hpss_SetLoginCred failed with %s\n", strerror(-Status));
	   exit( Status );
	}
     }
 }

  return 0;
}

int common_usage() {
    fprintf(stderr, "with auth-options:");
    fprintf(stderr, "\t-u <username>\n");
    fprintf(stderr, "\t-k login using a keytab file\n");
    fprintf(stderr, "\t-t <path to Keytab>\n");
    fprintf(stderr, "\t-m <krb5|unix>\n");
    fprintf(stderr, "\t-N create new login credential (UNIX only)\n");
    return(0);
}


int
hpss_exit (int rc)
{
/* unauthenticate */
  hpss_ClientAPIReset ();
  hpss_PurgeLoginCred ();
  exit (rc);
}
