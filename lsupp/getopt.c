/*---------------------------------------------------------------------------*/
/* getopt replacement for Windows; condensed from                            */
/* http://www.opensource.apple.com/source/bc/bc-18/bc/lib/getopt.c           */
/*---------------------------------------------------------------------------*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "getopt.h"

char *optarg = NULL;
int optind = 0, opterr = 1, optopt = '?';
static char *nextchar;
static enum { REQUIRE_ORDER, PERMUTE, RETURN_IN_ORDER } ordering;
static char *posixly_correct;
#define	my_index	strchr
static int first_nonopt;
static int last_nonopt;

static void _getopt_exchange(char **argv)
{
int bottom = first_nonopt;
int middle = last_nonopt;
int top = optind;
char *tem;

while (top > middle && middle > bottom)
  {
  if (top - middle > middle - bottom)
    {
    int len = middle - bottom;
    register int i;
    for (i = 0; i < len; i++)
      {
      tem = argv[bottom + i];
      argv[bottom + i] = argv[top - (middle - bottom) + i];
      argv[top - (middle - bottom) + i] = tem;
      }
    top -= len;
    }
  else
    {
    int len = top - middle;
    register int i;
    for (i = 0; i < len; i++)
      {
      tem = argv[bottom + i];
      argv[bottom + i] = argv[middle + i];
      argv[middle + i] = tem;
      }
    bottom += len;
    }
  }
first_nonopt += (optind - last_nonopt);
last_nonopt = optind;
}

static const char *_getopt_initialize (const char *optstring)
{
first_nonopt = last_nonopt = optind = 1;
nextchar = NULL;
posixly_correct = getenv ("POSIXLY_CORRECT");
if (optstring[0] == '-')
  {
  ordering = RETURN_IN_ORDER;
  ++optstring;
  }
else if (optstring[0] == '+')
  {
  ordering = REQUIRE_ORDER;
  ++optstring;
  }
else if (posixly_correct != NULL)
  ordering = REQUIRE_ORDER;
else
  ordering = PERMUTE;
return optstring;
}

int
_getopt_internal(int argc, char *const *argv,
    const char *optstring, const struct option *longopts,
    int *longind, int long_only)
{
optarg = NULL;

if (optind == 0)
optstring = _getopt_initialize (optstring);

if (nextchar == NULL || *nextchar == '\0')
  {
  if (ordering == PERMUTE)
    {
    if (first_nonopt != last_nonopt && last_nonopt != optind)
      _getopt_exchange ((char **) argv);
    else if (last_nonopt != optind)
      first_nonopt = optind;
    while (optind < argc
      && (argv[optind][0] != '-' || argv[optind][1] == '\0'))
      optind++;
    last_nonopt = optind;
    }
  if (optind != argc && !strcmp (argv[optind], "--"))
    {
    optind++;

    if (first_nonopt != last_nonopt && last_nonopt != optind)
      _getopt_exchange ((char **) argv);
    else if (first_nonopt == last_nonopt)
      first_nonopt = optind;
    last_nonopt = argc;

    optind = argc;
    }
  if (optind == argc)
    {
    if (first_nonopt != last_nonopt)
      optind = first_nonopt;
    return EOF;
    }
  if ((argv[optind][0] != '-' || argv[optind][1] == '\0'))
    {
    if (ordering == REQUIRE_ORDER)
      return EOF;
    optarg = argv[optind++];
    return 1;
    }
  nextchar = (argv[optind] + 1
    + (longopts != NULL && argv[optind][1] == '-'));
  }
if (longopts != NULL
    && (argv[optind][1] == '-'
    || (long_only && (argv[optind][2] || !my_index (optstring, argv[optind][1])))))
  {
  char *nameend;
  const struct option *p;
  const struct option *pfound = NULL;
  int exact = 0;
  int ambig = 0;
  int indfound;
  int option_index;

  for (nameend = nextchar; *nameend && *nameend != '='; nameend++)
    ;

  for (p = longopts, option_index = 0; p->name; p++, option_index++)
    if (!strncmp (p->name, nextchar, nameend - nextchar))
      {
      if (nameend - nextchar == strlen (p->name))
        {
        pfound = p;
        indfound = option_index;
        exact = 1;
        break;
        }
      else if (pfound == NULL)
        {
        pfound = p;
        indfound = option_index;
        }
      else
        ambig = 1;
      }

    if (ambig && !exact)
      {
      if (opterr)
        fprintf (stderr, "%s: option `%s' is ambiguous\n",
        argv[0], argv[optind]);
      nextchar += strlen (nextchar);
      optind++;
      return '?';
      }

    if (pfound != NULL)
      {
      option_index = indfound;
      optind++;
      if (*nameend)
        {
        if (pfound->has_arg)
          optarg = nameend + 1;
        else
          {
          if (opterr)
            {
            if (argv[optind - 1][1] == '-')
              /* --option */
              fprintf (stderr,
              "%s: option `--%s' doesn't allow an argument\n",
              argv[0], pfound->name);
            else
              /* +option or -option */
              fprintf (stderr,
              "%s: option `%c%s' doesn't allow an argument\n",
              argv[0], argv[optind - 1][0], pfound->name);
            }
          nextchar += strlen (nextchar);
          return '?';
          }
        }
      else if (pfound->has_arg == 1)
        {
        if (optind < argc)
          optarg = argv[optind++];
        else
          {
          if (opterr)
            fprintf (stderr, "%s: option `%s' requires an argument\n",
            argv[0], argv[optind - 1]);
          nextchar += strlen (nextchar);
          return optstring[0] == ':' ? ':' : '?';
          }
        }
      nextchar += strlen (nextchar);
      if (longind != NULL)
        *longind = option_index;
      if (pfound->flag)
        {
        *(pfound->flag) = pfound->val;
        return 0;
        }
      return pfound->val;
      }
    if (!long_only || argv[optind][1] == '-'
      || my_index (optstring, *nextchar) == NULL)
      {
      if (opterr)
        {
        if (argv[optind][1] == '-')
          /* --option */
          fprintf (stderr, "%s: unrecognized option `--%s'\n",
          argv[0], nextchar);
        else
          /* +option or -option */
          fprintf (stderr, "%s: unrecognized option `%c%s'\n",
          argv[0], argv[optind][0], nextchar);
        }
      nextchar = (char *) "";
      optind++;
      return '?';
      }
  }
  {
  char c = *nextchar++;
  char *temp = my_index (optstring, c);
  if (*nextchar == '\0')
    ++optind;

  if (temp == NULL || c == ':')
    {
    if (opterr)
      {
      if (posixly_correct)
        fprintf (stderr, "%s: illegal option -- %c\n", argv[0], c);
      else
        fprintf (stderr, "%s: invalid option -- %c\n", argv[0], c);
      }
    optopt = c;
    return '?';
    }
  if (temp[1] == ':')
    {
    if (temp[2] == ':')
      {
      if (*nextchar != '\0')
        {
        optarg = nextchar;
        optind++;
        }
      else
        optarg = NULL;
      nextchar = NULL;
      }
    else
      {
      if (*nextchar != '\0')
        {
        optarg = nextchar;
        optind++;
        }
      else if (optind == argc)
        {
        if (opterr)
          {
          fprintf (stderr, "%s: option requires an argument -- %c\n",
            argv[0], c);
          }
        optopt = c;
        if (optstring[0] == ':')
          c = ':';
        else
          c = '?';
        }
      else
        optarg = argv[optind++];
      nextchar = NULL;
      }
    }
  return c;
  }
}

int getopt(int argc, char * const argv[], const char *optstring)
{
return _getopt_internal (argc, argv, optstring,
			   (const struct option *) 0,
			   (int *) 0,
			   0);
}

int
getopt_long (int argc, char *const *argv, const char *options, const struct option *long_options, int *opt_index)
{
  return _getopt_internal (argc, argv, options, long_options, opt_index, 0);
}

/* Like getopt_long, but '-' as well as '--' can indicate a long option.
   If an option that starts with '-' (not '--') doesn't match a long option,
   but does match a short option, it is parsed as a short option
   instead.  */

int
getopt_long_only (int argc, char *const *argv, const char *options, const struct option *long_options, int *opt_index)
{
  return _getopt_internal (argc, argv, options, long_options, opt_index, 1);
}
