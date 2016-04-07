#include <fcntl.h>
#include <sys/time.h>
#include <stdio.h>
#include <errno.h>
#include "hpss_api.h"
#include "u_signed64.h"
#include "tools.h"

char *UserName = NULL;
char *PathToKeytab = NULL;
char *AuthMech = NULL;

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
    }

  return 1;
}

authenticate ()
{
  int rc;
  hpss_authn_mech_t hpss_authn_mech = -1;
  if (UserName && PathToKeytab && AuthMech)
    {
      if (access (PathToKeytab, F_OK) != -1)
	{
	  if (strcmp (AuthMech, "unix") == 0)
	    {
	      hpss_authn_mech = hpss_authn_mech_unix;
	    }
	  else if (strcmp (AuthMech, "krb5") == 0)
	    {
	      hpss_authn_mech = hpss_authn_mech_krb5;
	    }
	  if (hpss_authn_mech == -1)
	    {
	      fprintf (stderr,
		       "Unknown authentication Mech : %s. Valid options are \"krb5\" and \"unix\".\n",
		       AuthMech);
	      exit (1);
	    }
	  rc = hpss_SetLoginCred (UserName, hpss_authn_mech_krb5,
				  hpss_rpc_cred_client,
				  hpss_rpc_auth_type_keytab, PathToKeytab);
	  if (rc != 0)
	    {
	      fprintf (stderr,
		       "hpss_SetLoginCred(): %s\n", strerror (-rc));
	      exit (-rc);
	    }
	}
      else
	{
	  fprintf (stderr, "Cannot read keytab: %s\n",
		   PathToKeytab);
	  exit (2);
	}
    }
  else
    {
      fprintf (stderr,
	       "All of username (-u <username> ), path to keytab (-t "
	       "<path to keytab>) and -m <authentication mechnism> "
	       "required.\n");

      exit (2);
    }
  return 0;
}


int
hpss_exit (int rc)
{
/* unauthenticate */
  hpss_ClientAPIReset ();
  hpss_PurgeLoginCred ();
  exit (rc);
}
