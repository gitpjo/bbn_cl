/* -*-C-*-

Copyright (c) 1987 Massachusetts Institute of Technology

This material was developed by the Scheme project at the Massachusetts
Institute of Technology, Department of Electrical Engineering and
Computer Science.  Permission to copy this software, to redistribute
it, and to use it for any purpose is granted, subject to the following
restrictions and understandings.

1. Any copy made of this software must include this copyright notice
in full.

2. Users of this software agree to make their best efforts (a) to
return to the MIT Scheme project any improvements or extensions that
they make, so that these may be included in future releases; and (b)
to inform MIT of noteworthy uses of this software.

3. All materials developed as a consequence of the use of this
software shall duly acknowledge such use, in accordance with the usual
standards of acknowledging credit in academic research.

4. MIT has made no warrantee or representation that the operation of
this software will be error-free, and MIT is under no obligation to
provide any services, by way of maintenance, update, or otherwise.

5. In conjunction with products arising from the use of this material,
there shall be no use of the name of the Massachusetts Institute of
Technology nor of any adaptation thereof in any advertising,
promotional, or sales literature without prior written consent from
MIT in each case. */

/* $Header: comlin.c,v 10.0 88/12/07 13:06:18 las Exp $
 * $MIT-Header: comlin.c,v 1.2 88/03/12 16:04:02 GMT jinx Rel $
 *
 * This file contains the scheme command parser.
 *
 */

#include <stdio.h>
#ifndef toupper
#include <ctype.h>
#endif

#include "comlin.h"

/* Some string utilities */

char *
remove_initial_substring(sub, string)
     register char *sub, *string;
{
  for ( ; *sub != '\0'; sub++, string++)
  {
    if (*sub != *string)
    {
      return ((char *) NULL);
    }
  }
  return (string);
}

boolean
STREQUAL(s1, s2)
     register char *s1, *s2;
{
  for ( ; *s1 != '\0'; s1++, s2++)
  {
    if (toupper(*s1) != toupper(*s2))
    {
      return (false);
    }
  }
  return (*s2 == '\0');
}

/* Usage information */

void
print_usage_and_exit(options, val)
     struct keyword_struct options[];
     int val;
{
  register int i;

  fprintf(stderr, "usage: %s", program_name);

  if ((options[0].type_tag) == LAST_KYWRD)
  {
    fprintf(stderr, "\n");
    exit(val);
  }

  fprintf(stderr, " [args]\n");
  fprintf(stderr, "    where args are as follows:\n");

  for (i = 0;
       ((options[i].type_tag) != LAST_KYWRD);
       i++)
  {
    switch (options[i].type_tag)
    {
      case BOOLEAN_KYWRD:
	fprintf(stderr, "        %s={true,false}\n",
		options[i].keyword);
	break;

      case INT_KYWRD:
      case DOUBLE_KYWRD:
	fprintf(stderr, "        %s=%s\n",
		options[i].keyword, options[i].format);
	break;

      case STRING_KYWRD:
	fprintf(stderr, "        %s=%%s\n",
		options[i].keyword);
	break;
    }
  }
  exit(val);
}

void
supply(options, j)
     struct keyword_struct options[];
     int j;
{
  if (options[j].supplied_p != ((boolean *) NULL))
  {
    if (*(options[j].supplied_p))
    {
      fprintf(stderr,
	      "parse_keywords: Repeated keyword: %s\n",
	      options[j].keyword);
      print_usage_and_exit(&options[0], 1);
    }
    else
    {
      *(options[j].supplied_p) = true;
    }
  }
  return;
}

char *program_name;

/* This parser assumes that no keyword is an initial
   substring of another.
 */

void
parse_keywords(argc, argv, options, allow_others_p)
     int argc;
     char **argv;
     struct keyword_struct options[];
     boolean allow_others_p;
{
  register int i, j, length;
  char *argument;

  program_name = argv[0];
  argv += 1;
  argc -= 1;

  /* Initialize defaults */

  for (length = 0;
       ((options[length].type_tag) != LAST_KYWRD);
       length++)
  {
    if (options[length].supplied_p != ((boolean *) NULL))
    {
      *(options[length].supplied_p) = false;
    }

    switch (options[length].type_tag)
    {
      case BOOLEAN_KYWRD:
        if (options[length].format != BFRMT)
	{
	  fprintf(stderr,
		  "parse_keywords: format (%s) for boolean keyword %s\n",
		  options[length].format,
		  options[length].keyword);
	  exit(1);
	}
	break;

      case INT_KYWRD:
	break;

      case DOUBLE_KYWRD:
	break;

      case STRING_KYWRD:
        if (options[length].format != SFRMT)
	{
	  fprintf(stderr,
		  "parse_keywords: format (%s) for string keyword %s\n",
		  options[length].format,
		  options[length].keyword);
	  exit(1);
	}
	break;

      default:
        fprintf(stderr, "parse_keywords: bad type %d\n",
		options[length].type_tag);
	exit(1);
    }
  }

  for (i = 0; i < argc; i++)
  {
    for (j = 0; j < length; j++)
    {
      argument = remove_initial_substring(options[j].keyword,argv[i]);
      if (argument != ((char *) NULL))
      {
	switch (options[j].type_tag)
	{

	  case BOOLEAN_KYWRD:
	  {
	    boolean value;

	    if (*argument != '\0')
	    {
	      if (*argument != '\=')
	      {
		fprintf(stderr,
			"parse_keywords: unrecognized parameter: %s\n",
			argv[i]);
		print_usage_and_exit(&options[0], 1);
	      }
	      else
	      {
		argument = &argument[1];
		if (STREQUAL(argument,"t") || STREQUAL(argument,"true"))
		{
		  value = true;
		}
		else if (STREQUAL(argument,"f") ||
			 STREQUAL(argument,"false") ||
			 STREQUAL(argument,"nil"))
		{
		  value = false;
		}
		else
		{
		  fprintf(stderr,
			  "parse_keywords: Invalid boolean value: %s\n",
			  argv[i]);
		  print_usage_and_exit(&options[0, 1]);
		}
	      }
	    }
	    else
	    {
	      value = true;
	    }
	    supply(options, j);
	    *(BOOLEAN_LVALUE(options[j])) = value;
	    break;
	  }

	  case INT_KYWRD:
	    if (*argument != '=')
	    {
	      {
		fprintf(stderr,
			"parse_keywords: %s: %s\n",
			((*argument == '\0')	?
			 "missing integer value"	:
			 "unrecognized parameter"),
			argv[i]);
		print_usage_and_exit(&options[0], 1);
	      }
	    }
	    supply(options, j);
	    sscanf(&argument[1], options[j].format, INT_LVALUE(options[j]));
	    break;

	  case DOUBLE_KYWRD:
	    if (*argument != '=')
	    {
	      {
		fprintf(stderr,
			"parse_keywords: %s: %s\n",
			((*argument == '\0')		?
			 "missing floating point value"	:
			 "unrecognized parameter"),
			argv[i]);
		print_usage_and_exit(&options[0], 1);
	      }
	    }
	    supply(options, j);
	    sscanf(&argument[1], options[j].format, DOUBLE_LVALUE(options[j]));
	    break;

	  case STRING_KYWRD:
	    if (*argument != '=')
	    {
	      {
		fprintf(stderr,
			"parse_keywords: %s: %s\n",
			((*argument == '\0')	?
			 "missing string value"	:
			 "unrecognized parameter"),
			argv[i]);
		print_usage_and_exit(&options[0], 1);
	      }
	    }
	    supply(options, j);
	    *(STRING_LVALUE(options[j])) = &argument[1];
	    break;
	  }
	break;
      }
    }
    if ((j >= length) && (!allow_others_p))
    {
      fprintf(stderr,
	      "parse_keywords: unrecognized parameter: %s\n",
	      argv[i]);
      print_usage_and_exit(&options[0], 1);
    }
  }
  return;
}
