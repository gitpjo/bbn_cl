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

/* $Header: Ppband.c,v 10.0 88/12/07 13:03:12 las Exp $
 * $MIT-Header: Ppband.c,v 9.32 88/02/12 16:49:27 GMT jinx Exp $
 *
 * Dumps Scheme FASL in user-readable form .
 */

#include <stdio.h>
#include "config.h"
#include "types.h"
#include "const.h"
#include "object.h"
#include "sdata.h"

#define fast register

/* These are needed by load.c */

static Pointer *Memory_Base;

long
Load_Data(Count, To_Where)
     long Count;
     FILE *To_Where;
{
  extern int fread();

  return (fread(To_Where, sizeof(Pointer), Count, stdin));
}

long
Write_Data()
{
  fprintf(stderr, "Write_Data called\n");
  exit(1);
}

Boolean
Open_Dump_File()
{
  fprintf(stderr, "Open_Dump_File called\n");
  exit(1);
}

Boolean
Close_Dump_File()
{
  fprintf(stderr, "Close_Dump_File called\n");
  exit(1);
}

#define INHIBIT_COMPILED_VERSION_CHECK
#include "load.c"

#ifdef Heap_In_Low_Memory
#ifdef spectrum
#define File_To_Pointer(P)	((((long) (P)) & ADDRESS_MASK) / sizeof(Pointer))
#else
#define File_To_Pointer(P)	((P) / sizeof(Pointer))
#endif /* spectrum */
#else
#define File_To_Pointer(P)	(P)
#endif

#ifndef Conditional_Bug
#define Relocate(P)						\
	(((long) (P) < Const_Base) ?				\
	 File_To_Pointer(((long) (P)) - Heap_Base) :		\
	 (Heap_Count + File_To_Pointer(((long) (P)) - Const_Base)))
#else
#define Relocate_Into(What, P)
if (((long) (P)) < Const_Base)
  (What) = File_To_Pointer(((long) (P)) - Heap_Base);
else
  (What) = Heap_Count + File_To_Pointer(((long) P) - Const_Base);

static long Relocate_Temp;
#define Relocate(P)	(Relocate_Into(Relocate_Temp, P), Relocate_Temp)
#endif

static Pointer *Data, *end_of_memory;

Boolean
scheme_string(From, Quoted)
     long From;
     Boolean Quoted;
{
  fast long i, Count;
  fast char *Chars;

  Chars = ((char *) &Data[From +  STRING_CHARS]);
  if (Chars < ((char *) end_of_memory))
  {
    Count = ((long) (Data[From + STRING_LENGTH]));
    if (&Chars[Count] < ((char *) end_of_memory))
    {
      if (Quoted)
      {
	putchar('\"');
      }
      for (i = 0; i < Count; i++)
      {
	printf("%c", *Chars++);
      }
      if (Quoted)
      {
	putchar('\"');
      }
      putchar('\n');
      return (true);
    }
  }
  if (Quoted)
  {
    printf("String not in memory; datum = %lx\n", From);
  }
  return (false);
}

#define via(File_Address)	Relocate(OBJECT_DATUM(Data[File_Address]))

void
cl_or_scheme_symbol(From)
     long From;
{
  Pointer *symbol, name;

  if (&Data[From] > end_of_memory)
    {
      printf("target of symbol out of range %x\n", From);
      return;
    }
  name = Data[From + SYMBOL_NAME];
  if (OBJECT_TYPE(name) == TC_INTERNED_SYMBOL) /*  */
    {				/* Common Lisp function symbol */
      printf("function value for ");
      name = Data[Relocate(OBJECT_DATUM(name)) + 0];
    }
  if (&Data[Relocate(OBJECT_DATUM(name))] > end_of_memory)
    {
      printf("target of symbol out of range %x\n", name);
      return;
    }
  if (OBJECT_TYPE(name) == TC_CLSAV)
    {				/* Common Lisp symbol */
      printf("CL symbol ");
      name = Data[Relocate(OBJECT_DATUM(name)) + 1];
    }
  if (&Data[Relocate(OBJECT_DATUM(name))] > end_of_memory)
    {
      printf("target of symbol out of range %x\n", name);
      return;
    }
  if (OBJECT_TYPE(name) == TC_CHARACTER_STRING)
    {				/* Ordinary symbol */
      name = Relocate(OBJECT_DATUM(name));
      if ((((Pointer *) name) >= end_of_memory) ||
	  (!(scheme_string(name, false))))
	{
	  printf("symbol not in memory; datum = %lx\n", From);
	}
    }
  else printf("bogus symbol name %x\n", name);
  return;
}

static char string_buffer[10];

#define PRINT_OBJECT(type, datum)					\
{									\
  printf("[%s %lx]", type, datum);					\
}

#define NON_POINTER(string)						\
{									\
  the_string = string;							\
  Points_To = The_Datum;						\
  break;								\
}

#define POINTER(string)							\
{									\
  the_string = string;							\
  break;								\
}

char *Type_Names[] = TYPE_NAME_TABLE;

void
Display(Location, Type, The_Datum)
     long Location, Type, The_Datum;
{
  char *the_string;
  long Points_To;

  printf("%5lx: %2lx|%6lx     ", Location, Type, The_Datum);
  Points_To = Relocate((Pointer *) The_Datum);

  switch (Type)
  { /* "Strange" cases */
    case TC_NULL:
      if (The_Datum == 0)
      {
	printf("NIL\n");
	return;
      }
      NON_POINTER("NULL");

    case TC_TRUE:
      if (The_Datum == 0)
      {
	printf("TRUE\n");
	return;
      }
      /* fall through */


    case TC_CHARACTER:
    case TC_RETURN_CODE:
    case TC_PRIMITIVE:
    case TC_THE_ENVIRONMENT:
    case TC_PCOMB0:
    case TC_MANIFEST_SPECIAL_NM_VECTOR:
    case TC_MANIFEST_NM_VECTOR:
      NON_POINTER(Type_Names[Type]);

    case TC_INTERNED_SYMBOL:
      PRINT_OBJECT("INTERNED-SYMBOL", Points_To);
      printf(" = ");
      cl_or_scheme_symbol(Points_To);
      return;

    case TC_UNINTERNED_SYMBOL: 
      PRINT_OBJECT("UNINTERNED-SYMBOL", Points_To);
      printf(" = ");
      cl_or_scheme_symbol(Points_To);
      return;

    case TC_CHARACTER_STRING:
      PRINT_OBJECT("CHARACTER-STRING", Points_To);
      printf(" = ");
      scheme_string(Points_To, true);
      return;

    case TC_FIXNUM:
      PRINT_OBJECT("FIXNUM", The_Datum);
      Sign_Extend(The_Datum, Points_To);
      printf(" = %ld\n", Points_To);
      return;

    case TC_REFERENCE_TRAP:
      if (The_Datum <= TRAP_MAX_IMMEDIATE)
      {
	NON_POINTER("REFERENCE-TRAP");
      }
      else
      {
	POINTER("REFERENCE-TRAP");
      }

    case TC_BROKEN_HEART:
      if (The_Datum == 0)
      {
	Points_To = 0;
      }
    default:
      if (Type <= LAST_TYPE_CODE)
      {
	POINTER(Type_Names[Type]);
      }
      else
      {
	sprintf(&the_string[0], "0x%02lx ", Type);
	POINTER(&the_string[0]);
      }
  }
  PRINT_OBJECT(the_string, Points_To);
  putchar('\n');
  return;
}

Pointer *
show_area(area, size, name)
     fast Pointer *area;
     fast long size;
     char *name;
{
  fast long i;

  printf("\n%s contents:\n\n", name);
  for (i = 0; i < size;  area++, i++)
  {
    if (OBJECT_TYPE(*area) == TC_MANIFEST_NM_VECTOR)
    {
      fast long j, count;

      count = Get_Integer(*area);
      Display(i, OBJECT_TYPE(*area), OBJECT_DATUM(*area));
      area += 1;
      for (j = 0; j < count ; j++, area++)
      {
        printf("          %02lx%06lx\n",
               OBJECT_TYPE(*area), OBJECT_DATUM(*area));
      }
      i += count;
      area -= 1;
    }
    else
    {
      Display(i, OBJECT_TYPE(*area),  OBJECT_DATUM(*area));
    }
  }
  return (area);
}

main(argc, argv)
     int argc;
     char **argv;
{
  fast Pointer *Next;
  long total_length, load_length;

  while (true) {
    if (argc == 1)
      {
	if (Read_Header() != FASL_FILE_FINE)
	  {
	    fprintf(stderr,
		    "%s: Input does not appear to be in correct FASL format.\n",
		    argv[0]);
	    exit(1);
	  }
	print_fasl_information();
	printf("Dumped object (relocated) at 0x%lx\n", Relocate(Dumped_Object));
      }
    else
      {
	Const_Count = 0;
	Primitive_Table_Size = 0;
	sscanf(argv[1], "%x", &Heap_Base);
	sscanf(argv[2], "%x", &Const_Base);
	sscanf(argv[3], "%d", &Heap_Count);
	printf("Heap Base = 0x%lx; Constant Base = 0x%lx; Heap Count = %ld\n",
	       Heap_Base, Const_Base, Heap_Count);
      }    

    load_length = (Heap_Count + Const_Count + Primitive_Table_Size);
    Data = ((Pointer *) malloc(sizeof(Pointer) * (load_length + 4)));
    if (Data == NULL)
      {
	fprintf(stderr, "Allocation of %ld words failed.\n", (load_length + 4));
	exit(1);
      }
    total_length = Load_Data(load_length, Data);
    end_of_memory = &Data[total_length];
    if (total_length != load_length)
      {
	printf("The FASL file does not have the right length.\n");
	printf("Expected %d objects.  Obtained %ld objects.\n\n",
	       load_length, total_length);
	if (total_length < Heap_Count)
	  {
	    Heap_Count = total_length;
	  }
	total_length -= Heap_Count;
	if (total_length < Const_Count)
	  {
	    Const_Count = total_length;
	  }
	total_length -= Const_Count;
	if (total_length < Primitive_Table_Size)
	  {
	    Primitive_Table_Size = total_length;
	  }
      }

    if (Heap_Count > 0)
      {
	Next = show_area(Data, Heap_Count, "Heap");
      }
    if (Const_Count > 0)
      {
	Next = show_area(Next, Const_Count, "Constant Space");
      }
    if ((Primitive_Table_Size > 0) && (Next < end_of_memory))
      {
	long arity, size;
	fast long entries, count;
	
	/* This is done in case the file is short. */
	end_of_memory[0] = ((Pointer) 0);
	end_of_memory[1] = ((Pointer) 0);
	end_of_memory[2] = ((Pointer) 0);
	end_of_memory[3] = ((Pointer) 0);
	
	entries = Primitive_Table_Length;
	printf("\nPrimitive table: number of entries = %ld\n\n", entries);
	
	for (count = 0;
	     ((count < entries) && (Next < end_of_memory));
	     count += 1)
	  {
	    Sign_Extend(*Next++, arity);
	    size = Get_Integer(*Next);
	    printf("Number = %3lx; Arity = %2ld; Name = ", count, arity);
	    scheme_string((Next - Data), true);
	    Next += (1 + size);
	  }
	printf("\n");
      } }
  exit(0);
}
