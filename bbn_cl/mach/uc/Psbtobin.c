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

/* $Header: Psbtobin.c,v 10.0 88/12/07 13:03:16 las Exp $
 * $MIT-Header: Psbtobin.c,v 9.34 88/04/25 17:30:02 GMT cph Exp $
 *
 * This File contains the code to translate portable format binary
 * files to internal format.
 *
 */

/* Cheap renames */

#include "psbmap.h"
#define portable_file input_file
#define internal_file output_file

static Boolean
  band_p = false,
  allow_compiled_p = false,
  allow_nmv_p = false;

static long
  Dumped_Object_Addr,
  Dumped_Heap_Base, Heap_Objects, Heap_Count,
  Dumped_Constant_Base, Constant_Objects, Constant_Count,
  Dumped_Pure_Base, Pure_Objects, Pure_Count,
  Primitive_Table_Length;

static Pointer
  *Heap,
  *Heap_Base, *Heap_Table, *Heap_Object_Base, *Free,
  *Constant_Base, *Constant_Table,
  *Constant_Object_Base, *Free_Constant,
  *Pure_Base, *Pure_Table, *Pure_Object_Base, *Free_Pure,
  *primitive_table, *primitive_table_end,
  *Stack_Top;

long
Write_Data(Count, From_Where)
     long Count;
     Pointer *From_Where;
{
  extern int fwrite();

  return (fwrite(((char *) From_Where), sizeof(Pointer),
		 Count, internal_file));
}

int Debug_Flags[40] =
{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

#include "fasl.h"
#include "dump.c"

void
inconsistency()
{
  /* Provide some context (2 lines). */
  char yow[100];

  fgets(&yow[0], 100, portable_file);
  fprintf(stderr, "%s\n", &yow[0]);
  fgets(&yow[0], 100, portable_file);
  fprintf(stderr, "%s\n", &yow[0]);

  quit(1);
  /*NOTREACHED*/
}

#define OUT(c)	return ((long) ((c) & MAX_CHAR))

long
read_a_char()
{
  fast char C;

  C = getc(portable_file);
  if (C != '\\')
  {
    OUT(C);
  }
  C = getc(portable_file);
  switch(C)
  {
    case 'n':  OUT('\n');
    case 't':  OUT('\n');
    case 'r':  OUT('\r');
    case 'f':  OUT('\f');
    case '0':  OUT('\0');
    case 'X':
    {
      long Code;

      fprintf(stderr,
	      "%s: File is not Portable.  Character Code Found.\n",
	      program_name);
      fscanf(portable_file, "%ld", &Code);
      getc(portable_file);			/* Space */
      OUT(Code);
    }
    case '\\': OUT('\\');
    default  : OUT(C);
  }
}

Pointer *
read_a_string_internal(To, maxlen)
     Pointer *To;
     long maxlen;
{
  long ilen, Pointer_Count;
  fast char *str;
  fast long len;

  str = ((char *) (&To[STRING_CHARS]));
  fscanf(portable_file, "%ld", &ilen);
  len = ilen;

  if (maxlen == -1)
  {
    maxlen = len;
  }

  /* Null terminated */

  maxlen += 1;

  Pointer_Count = STRING_CHARS + char_to_pointer(maxlen);
  To[STRING_HEADER] =
    Make_Non_Pointer(TC_MANIFEST_NM_VECTOR, (Pointer_Count - 1));
  To[STRING_LENGTH] = ((Pointer) len);

  /* Space */

  getc(portable_file);
  while (--len >= 0)
  {
    *str++ = ((char) read_a_char());
  }
  *str = '\0';
  return (To + Pointer_Count);
}

Pointer *
read_a_string(To, Slot)
     Pointer *To, *Slot;
{
  long maxlen;

  *Slot = Make_Pointer(TC_CHARACTER_STRING, To);
  fscanf(portable_file, "%ld", &maxlen);
  return (read_a_string_internal(To, maxlen));
}

/*
   The following two lines appears by courtesy of your friendly
   VMS C compiler and runtime library.

   Bug in version 4 VMS scanf.
 */

#ifndef vms

#define VMS_BUG(stmt)

#define read_hex_digit(var)						\
{									\
  fscanf(portable_file, "%1lx", &var);					\
}

#else

#define VMS_BUG(stmt)			stmt

#define read_hex_digit(var)						\
{									\
  var = read_hex_digit_procedure();					\
}

long
read_hex_digit_procedure()
{
  long digit;
  int c;

  while ((c = fgetc(portable_file)) == ' ')
  {};
  digit = ((c >= 'a') ? (c - 'a' + 10)
	   : ((c >= 'A') ? (c - 'A' + 10)
	      : ((c >= '0') ? (c - '0')
	         : fprintf(stderr, "Losing big: %d\n", c))));
  return (digit);
}

#endif

Pointer *
read_an_integer(The_Type, To, Slot)
     int The_Type;
     Pointer *To;
     Pointer *Slot;
{
  Boolean negative;
  long size_in_bits;

  getc(portable_file);				/* Space */
  negative = ((getc(portable_file)) == '-');
  fscanf(portable_file, "%ld", &size_in_bits);
  if ((size_in_bits <= fixnum_to_bits) &&
      (The_Type == TC_FIXNUM))
  {
    fast long Value = 0;
    fast int Normalization;
    fast long ndigits;
    long digit;

    if (size_in_bits != 0)
    {
      for(Normalization = 0,
	  ndigits = hex_digits(size_in_bits);
	  --ndigits >= 0;
	  Normalization += 4)
      {
	read_hex_digit(digit);
	Value += (digit << Normalization);
      }
    }
    if (negative)
    {
      Value = -Value;
    }
    *Slot = MAKE_SIGNED_FIXNUM(Value);
    return (To);
  }
  else if (size_in_bits == 0)
  {
    bigdigit *REG = BIGNUM(To);

    Prepare_Header(REG, 0, POSITIVE);
    *Slot = Make_Pointer(TC_BIG_FIXNUM, To);
    return (To + Align(0));
  }
  else
  {
    fast bigdigit *The_Bignum;
    fast long size, nbits, ndigits;
    fast unsigned long Temp;
    long Length;

    if ((The_Type == TC_FIXNUM) && (!compact_p))
    {
      fprintf(stderr,
	      "%s: Fixnum too large, coercing to bignum.\n",
	      program_name);
    }
    size = bits_to_bigdigit(size_in_bits);
    ndigits = hex_digits(size_in_bits);
    Length = Align(size);
    The_Bignum = BIGNUM(To);
    Prepare_Header(The_Bignum, size, (negative ? NEGATIVE : POSITIVE));
    for (The_Bignum = Bignum_Bottom(The_Bignum), nbits = 0, Temp = 0;
	 --size >= 0;
	 )
    {
      for ( ;
	   (nbits < SHIFT) && (ndigits > 0);
	   ndigits -= 1, nbits += 4)
      {
	long digit;

	read_hex_digit(digit);
	Temp |= (((unsigned long) digit) << nbits);
      }
      *The_Bignum++ = Rem_Radix(Temp);
      Temp = Div_Radix(Temp);
      nbits -= SHIFT;
    }
    *Slot = Make_Pointer(TC_BIG_FIXNUM, To);
    return (To + Length);
  }
}

Pointer *
read_a_bit_string(To, Slot)
     Pointer *To, *Slot;
{
  long size_in_bits, size_in_words;
  Pointer the_bit_string;

  fscanf(portable_file, "%ld", &size_in_bits);
  size_in_words = (1 + bits_to_pointers (size_in_bits));

  the_bit_string = Make_Pointer(TC_BIT_STRING, To);
  *To++ = Make_Non_Pointer(TC_MANIFEST_NM_VECTOR, size_in_words);
  *To = size_in_bits;
  To += size_in_words;

  if (size_in_bits != 0)
  {
    unsigned long temp;
    fast Pointer *scan;
    fast long bits_remaining, bits_accumulated;
    fast Pointer accumulator, next_word;

    accumulator = 0;
    bits_accumulated = 0;
    scan = bit_string_low_ptr(the_bit_string);
    for(bits_remaining = size_in_bits;
	bits_remaining > 0;
	bits_remaining -= 4)
    {
      read_hex_digit(temp);
      if ((bits_accumulated + 4) > POINTER_LENGTH)
      {
	accumulator |=
	  ((temp & low_mask(POINTER_LENGTH - bits_accumulated)) <<
	   bits_accumulated);
	*(inc_bit_string_ptr(scan)) = accumulator;
	accumulator = (temp >> (POINTER_LENGTH - bits_accumulated));
	bits_accumulated -= (POINTER_LENGTH - 4);
	temp &= low_mask(bits_accumulated);
      }
      else
      {
	accumulator |= (temp << bits_accumulated);
	bits_accumulated += 4;
      }
    }
    if (bits_accumulated != 0)
    {
      *(inc_bit_string_ptr(scan)) = accumulator;
    }
  }
  *Slot = the_bit_string;
  return (To);
}

/* Underflow and Overflow */

/* dflmax and dflmin exist in the Berserkely FORTRAN library */

static double the_max = 0.0;

#define dflmin()	0.0	/* Cop out */
#define dflmax()	((the_max == 0.0) ? compute_max() : the_max)

double 
compute_max()
{
  fast double Result;
  fast int expt;

  Result = 0.0;
  for (expt = MAX_FLONUM_EXPONENT;
       expt != 0;
       expt >>= 1)
  {
    Result += ldexp(1.0, expt);
  }
  the_max = Result;
  return (Result);
}

double 
read_a_flonum()
{
  Boolean negative;
  long size_in_bits, exponent;
  fast double Result;

  getc(portable_file);				/* Space */
  negative = ((getc(portable_file)) == '-');
  VMS_BUG(exponent = 0);
  VMS_BUG(size_in_bits = 0);
  fscanf(portable_file, "%ld %ld", &exponent, &size_in_bits);
  if (size_in_bits == 0)
  {
    Result = 0.0;
  }
  else if ((exponent > MAX_FLONUM_EXPONENT) ||
	   (exponent < -MAX_FLONUM_EXPONENT))
  {
    /* Skip over mantissa */

    while (getc(portable_file) != '\n')
    {};
    fprintf(stderr,
	    "%s: Floating point exponent too %s!\n",
	    program_name,
	    ((exponent < 0) ? "small" : "large"));
    Result = ((exponent < 0) ? dflmin() : dflmax());
  }
  else
  {
    fast long ndigits;
    fast double Normalization;
    long digit;

    if (size_in_bits > FLONUM_MANTISSA_BITS)
    {
      fprintf(stderr,
	      "%s: Some precision may be lost.",
	      program_name);
    }
    getc(portable_file);			/* Space */
    for (ndigits = hex_digits(size_in_bits),
	 Result = 0.0,
	 Normalization = (1.0 / 16.0);
	 --ndigits >= 0;
	 Normalization /= 16.0)
    {
      read_hex_digit(digit);
      Result += (((double ) digit) * Normalization);
    }
    Result = ldexp(Result, ((int) exponent));
  }
  if (negative)
  {
    Result = -Result;
  }
  return (Result);
}

Pointer *
Read_External(N, Table, To)
     long N;
     fast Pointer *Table, *To;
{
  fast Pointer *Until = &Table[N];
  int The_Type;

  while (Table < Until)
  {
    fscanf(portable_file, "%2x", &The_Type);
    switch(The_Type)
    {
      case TC_CHARACTER_STRING:
        To = read_a_string(To, Table++);
	continue;

      case TC_BIT_STRING:
	To = read_a_bit_string(To, Table++);
	continue;

      case TC_FIXNUM:
      case TC_BIG_FIXNUM:
	To = read_an_integer(The_Type, To, Table++);
	continue;

      case TC_CHARACTER:
      {
	long the_char_code;

	getc(portable_file);	/* Space */
	VMS_BUG(the_char_code = 0);
	fscanf( portable_file, "%3lx", &the_char_code);
	*Table++ = Make_Non_Pointer( TC_CHARACTER, the_char_code);
	continue;
      }

      case TC_BIG_FLONUM:
      {
	double The_Flonum = read_a_flonum();

	Align_Float(To);
	*Table++ = Make_Pointer(TC_BIG_FLONUM, To);
	*To++ = Make_Non_Pointer(TC_MANIFEST_NM_VECTOR, (float_to_pointer));
	*((double *) To) = The_Flonum;
	To += float_to_pointer;
	continue;
      }

      default:
	fprintf(stderr,
		"%s: Unknown external object found; Type = 0x%02x\n",
		program_name, The_Type);
	inconsistency();
	/*NOTREACHED*/
    }
  }
  return (To);
}

#if false

void
Move_Memory(From, N, To)
     fast Pointer *From, *To;
     long N;
{
  fast Pointer *Until;

  Until = &From[N];
  while (From < Until)
  {
    *To++ = *From++;
  }
  return;
}

#endif

void
Relocate_Objects(from, how_many, disp)
     fast Pointer *from;
     fast long disp;
     long how_many;
{
  fast Pointer *Until;

  Until = &from[how_many];
  while (from < Until)
  {
    switch(OBJECT_TYPE(*from))
    {
      case TC_FIXNUM:
      case TC_CHARACTER:
        from += 1;
        break;

      case TC_BIG_FIXNUM:
      case TC_BIG_FLONUM:
      case TC_CHARACTER_STRING:
	*from++ == Make_Object(OBJECT_TYPE(*from), (disp + OBJECT_DATUM(*from)));
	break;

      default:
	fprintf(stderr,
		"%s: Unknown External Object Reference with Type 0x%02x",
		program_name,
		OBJECT_TYPE(*from));
	inconsistency();
    }
  }
  return;
}

#define Relocate_Into(Where, Addr)					\
{									\
  if ((Addr) < Dumped_Pure_Base)					\
  {									\
    (Where) = &Heap_Object_Base[(Addr) - Dumped_Heap_Base];		\
  }									\
  else if ((Addr) < Dumped_Constant_Base)				\
  {									\
    (Where) = &Pure_Base[(Addr) - Dumped_Pure_Base];			\
  }									\
  else									\
  {									\
    (Where) = &Constant_Base[(Addr) - Dumped_Constant_Base];		\
  }									\
}

#ifndef Conditional_Bug

#define Relocate(Addr)							\
(((Addr) < Dumped_Pure_Base) ?						\
 &Heap_Object_Base[(Addr) - Dumped_Heap_Base] :				\
 (((Addr) < Dumped_Constant_Base) ?					\
  &Pure_Base[(Addr) - Dumped_Pure_Base] :				\
  &Constant_Base[(Addr) - Dumped_Constant_Base]))

#else

static Pointer *Relocate_Temp;

#define Relocate(Addr)							\
  (Relocate_Into(Relocate_Temp, Addr), Relocate_Temp)

#endif

Pointer *
Read_Pointers_and_Relocate(how_many, to)
     fast long how_many;
     fast Pointer *to;
{
  int The_Type;
  long The_Datum;

#if false
  Align_Float(to);
#endif

  while (--how_many >= 0)
  {
    VMS_BUG(The_Type = 0);
    VMS_BUG(The_Datum = 0);
    fscanf(portable_file, "%2x %lx", &The_Type, &The_Datum);
    switch(The_Type)
    {
      case CONSTANT_CODE:
	*to++ = Constant_Table[The_Datum];
	continue;
	
      case HEAP_CODE:
	*to++ = Heap_Table[The_Datum];
	continue;
	
      case TC_MANIFEST_NM_VECTOR:
	*to++ = Make_Non_Pointer(The_Type, The_Datum);
        {
	  fast long count;
	  
	  count = The_Datum;
	  how_many -= count;
	  while (--count >= 0)
	  {
	    VMS_BUG(*to = 0);
	    fscanf(portable_file, "%lx", to++);
	  }
	}
	continue;

      case TC_COMPILED_ENTRY:
      {
	Pointer *temp;
	long base_type, base_datum;

	fscanf(portable_file, "%02x %lx", &base_type, &base_datum);
	temp = Relocate(base_datum);
	*to++ = Make_Pointer(base_type,
			     ((Pointer *) (&(((char *) temp)[The_Datum]))));
	break;
      }

      case TC_BROKEN_HEART:
	if (The_Datum != 0)
	{
	  fprintf(stderr, "%s: Broken Heart found.\n", program_name);
	  inconsistency();
	}
	/* fall through */

      case TC_PCOMB0:
      case TC_PRIMITIVE:
      case TC_MANIFEST_SPECIAL_NM_VECTOR:
      case_simple_Non_Pointer:
	*to++ = Make_Non_Pointer(The_Type, The_Datum);
	continue;

      case TC_MANIFEST_CLOSURE:
      case TC_LINKAGE_SECTION:
      {
	fprintf(stderr, "%s: File contains linked compiled code.\n",
		program_name);
	inconsistency();
      }

      case TC_REFERENCE_TRAP:
	if (The_Datum <= TRAP_MAX_IMMEDIATE)
	{
	  *to++ = Make_Non_Pointer(The_Type, The_Datum);
	  continue;
	}
	/* It is a pointer, fall through. */

      default:
	/* Should be stricter */
	*to++ = Make_Pointer(The_Type, Relocate(The_Datum));
	continue;
    }
  }
#if false
  Align_Float(to);
#endif
  return (to);
}

static Boolean primitive_warn = false;

Pointer *
read_primitives(how_many, where)
     fast long how_many;
     fast Pointer *where;
{
  long arity;

  while (--how_many >= 0)
  {
    fscanf(portable_file, "%ld", &arity);
    if (arity == ((long) UNKNOWN_PRIMITIVE_ARITY))
    {
      primitive_warn = true;
    }
    *where++ = MAKE_SIGNED_FIXNUM(arity);
    where = read_a_string_internal(where, ((long) -1));
  }
  return (where);
}

#ifdef DEBUG

void
print_external_objects(area_name, Table, N)
     char *area_name;
     fast Pointer *Table;
     fast long N;
{
  fast Pointer *Table_End = &Table[N];

  fprintf(stderr, "%s External Objects:\n", area_name);
  fprintf(stderr, "Table = 0x%x; N = %d\n", Table, N);

  for( ; Table < Table_End; Table++)
  {
    switch (Type_Code(*Table))
    {
      case TC_FIXNUM:
      {
	long The_Number;

	Sign_Extend(*Table, The_Number);
        fprintf(stderr,
		"Table[%6d] = Fixnum %d\n",
		(N - (Table_End - Table)),
		The_Number);
	break;
      }
      case TC_CHARACTER:
        fprintf(stderr,
		"Table[%6d] = Character %c = 0x%02x\n",
		(N - (Table_End - Table)),
		Get_Integer(*Table),
		Get_Integer(*Table));
	break;

      case TC_CHARACTER_STRING:
        fprintf(stderr,
		"Table[%6d] = string \"%s\"\n",
		(N - (Table_End - Table)),
		((char *) Nth_Vector_Loc(*Table, STRING_CHARS)));
	break;

      case TC_BIG_FIXNUM:
	fprintf(stderr,
		"Table[%6d] = Bignum\n",
		(N - (Table_End - Table)));
	break;

      case TC_BIG_FLONUM:
	fprintf(stderr,
		"Table[%6d] = Flonum %lf\n",
		(N - (Table_End - Table)),
		(* ((double *) Nth_Vector_Loc(*Table, 1))));
	break;

      default:
        fprintf(stderr,
		"Table[%6d] = Unknown External Object 0x%8x\n",
		(N - (Table_End - Table)),
		*Table);
	break;
    }
  }
  return;
}

#define DEBUGGING(action)		action

#define WHEN(condition, message)	when(condition, message, block_count)

void
when(what, message, block_count)
     Boolean what;
     char *message;
     long block_count;
{
  if (what)
  {
    fprintf(stderr, "%s: Inconsistency in block %d: %s!\n",
	    program_name, block_count, message);
    quit(1);
  }
  return;
}

#define READ_HEADER(string, format, value)				\
{									\
 if (fscanf(portable_file, format, &(value)) == EOF)			\
   short_header_read(block_count);					\
 fprintf(stderr, "%s: ", (string));					\
 fprintf(stderr, (format), (value));					\
 fprintf(stderr, "\n");							\
}

static Boolean read_header_result;

#define READ_HEADER_CAREFULLY(string, format, value)			\
  (read_header_result =							\
    (fscanf(portable_file, format, &(value)) != EOF),			\
   fprintf(stderr, "%s: ", (string)),					\
   fprintf(stderr, (format), (value)),					\
   fprintf(stderr, "\n"),						\
   read_header_result)

#else /* not DEBUG */

#define DEBUGGING(action)

#define WHEN(what, message)

#define READ_HEADER_CAREFULLY(string, format, value)			\
  (fscanf(portable_file, format, &(value)) != EOF)

#define READ_HEADER(string, format, value)				\
{									\
  if (fscanf(portable_file, format, &(value)) == EOF)			\
  {									\
    short_header_read(block_count);					\
  }									\
}

#endif /* DEBUG */

void
short_header_read(block_count)
     long block_count;
{
  fprintf(stderr, "%s: Header (block %d) is not complete!\n",
	  program_name, block_count);
  quit(1);
}

#define Deallocate_Heap_Space(ptr, size_in_Pointers)			\
{									\
  free((char *) ptr);							\
}

static Pointer *Original_Heap;
static long Original_Size;

long
Read_Header_and_Allocate(block_count)
     long block_count;
{
  long
    Portable_Version, Machine,
    Version, Sub_Version, Flags,
    NFlonums, NIntegers, NBits,
    NBitstrs, NBBits, NStrings, NChars,
    NPChars,
    Size;

  if (!READ_HEADER_CAREFULLY("Portable Version", "%ld", Portable_Version))
  {
    if (block_count == 1)
      short_header_read(block_count);
    else
      return (-1);
  }

  if (Portable_Version != PORTABLE_VERSION)
  {
    fprintf(stderr, "%s: Portable version mismatch in block %d:\n",
	    program_name, block_count);
    fprintf(stderr, "Portable File Version %4d\n", Portable_Version);
    fprintf(stderr, "Expected:     Version %4d\n", PORTABLE_VERSION);
    quit(1);
  }

  READ_HEADER("Machine", "%ld", Machine);
  READ_HEADER("Version", "%ld", Version);
  READ_HEADER("Sub Version", "%ld", Sub_Version);

  if ((Version != FASL_FORMAT_VERSION)		||
      (Sub_Version != FASL_SUBVERSION))
  {
    fprintf(stderr, "%s: Binary version mismatch in block %d:\n",
	    program_name, block_count);
    fprintf(stderr,
	    "Portable File Version %4d; Binary Version %4d; Subversion %4d\n",
	    Portable_Version, Version, Sub_Version);
    fprintf(stderr,
	    "Expected:     Version %4d; Binary Version %4d; Subversion %4d\n",
	    PORTABLE_VERSION, FASL_FORMAT_VERSION, FASL_SUBVERSION);
    quit(1);
  }

  READ_HEADER("Flags", "%ld", Flags);
  READ_FLAGS(Flags);

  if (((compiled_p && (! allow_compiled_p)) ||
       (nmv_p && (! allow_nmv_p))) &&
      (Machine != FASL_INTERNAL_FORMAT))
  {
    if (compiled_p)
    {
      fprintf(stderr, "%s: %s in block %d.\n",
	      program_name,
	      "Portable file contains \"non-portable\" compiled code",
	      block_count);
    }
    else
    {
      fprintf(stderr, "%s: %s in block %d.\n",
	      program_name,
	      "Portable file contains \"unexpected\" non-marked vectors",
	      block_count);
    }
    fprintf(stderr, "Machine specified in the portable file: %4d\n",
	    Machine);
    fprintf(stderr, "Machine Expected:                       %4d\n",
	    FASL_INTERNAL_FORMAT);
    quit(1);
  }

  READ_HEADER("Heap Count", "%ld", Heap_Count);
  READ_HEADER("Dumped Heap Base", "%ld", Dumped_Heap_Base);
  READ_HEADER("Heap Objects", "%ld", Heap_Objects);
  
  READ_HEADER("Constant Count", "%ld", Constant_Count);
  READ_HEADER("Dumped Constant Base", "%ld", Dumped_Constant_Base);
  READ_HEADER("Constant Objects", "%ld", Constant_Objects);
  
  READ_HEADER("Pure Count", "%ld", Pure_Count);
  READ_HEADER("Dumped Pure Base", "%ld", Dumped_Pure_Base);
  READ_HEADER("Pure Objects", "%ld", Pure_Objects);
  
  READ_HEADER("& Dumped Object", "%ld", Dumped_Object_Addr);
  
  READ_HEADER("Number of flonums", "%ld", NFlonums);
  READ_HEADER("Number of integers", "%ld", NIntegers);
  READ_HEADER("Number of bits in integers", "%ld", NBits);
  READ_HEADER("Number of bit strings", "%ld", NBitstrs);
  READ_HEADER("Number of bits in bit strings", "%ld", NBBits);
  READ_HEADER("Number of character strings", "%ld", NStrings);
  READ_HEADER("Number of characters in strings", "%ld", NChars);
  
  READ_HEADER("Primitive Table Length", "%ld", Primitive_Table_Length);
  READ_HEADER("Number of characters in primitives", "%ld", NPChars);
  
  READ_HEADER("CPU type", "%ld", compiler_processor_type);
  READ_HEADER("Compiled code interface version", "%ld",
	      compiler_interface_version);
#if false
  READ_HEADER("Compiler utilities vector", "%ld", compiler_utilities);
#endif

  Size = (6 +						/* SNMV */
	  HEAP_BUFFER_SPACE +
	  Heap_Count + Heap_Objects +
	  Constant_Count + Constant_Objects +
	  Pure_Count + Pure_Objects +
	  flonum_to_pointer(NFlonums) +
	  ((NIntegers * (1 + bignum_header_to_pointer)) +
	   (bigdigit_to_pointer(bits_to_bigdigit(NBits)))) +
	  ((NStrings * (1 + STRING_CHARS)) +
	   (char_to_pointer(NChars))) +
	  ((NBitstrs * (1 + BIT_STRING_FIRST_WORD)) +
	   (bits_to_pointers(NBBits))) +
	  ((Primitive_Table_Length * (2 + STRING_CHARS)) +
	   (char_to_pointer(NPChars))));
	  
  Allocate_Heap_Space(Size);
  if (Heap == NULL)
  {
    fprintf(stderr, "%s: Memory allocation for block %d failed.",
	    program_name, block_count);
    fprintf(stderr, "  Size = %ld Scheme Pointers\n", Size);
    quit(1);
  }
  Original_Size = Size;
  Original_Heap = Heap;
  Heap += HEAP_BUFFER_SPACE;
  Initial_Align_Float(Heap);
  return (Size - HEAP_BUFFER_SPACE);
}

void
do_it()
{
  Pointer *primitive_table_end;
  Boolean result;
  long
    block_count,
    Size;

  allow_nmv_p = (allow_nmv_p || allow_compiled_p);
  block_count = 0;

  while(true)
  {
    block_count += 1;
    Size = Read_Header_and_Allocate(block_count);

    if (Size < 0)
      return;

    Stack_Top = &Heap[Size];

    Heap_Table = &Heap[0];
    Heap_Base = &Heap_Table[Heap_Objects];
    Heap_Object_Base =
      Read_External(Heap_Objects, Heap_Table, Heap_Base);

    /* The various 2s below are for SNMV headers. */

    Pure_Table = &Heap_Object_Base[Heap_Count];
    Pure_Base = &Pure_Table[Pure_Objects + 2];
    Pure_Object_Base =
      Read_External(Pure_Objects, Pure_Table, Pure_Base);

    Constant_Table = &Heap[Size - Constant_Objects];
    Constant_Base = &Pure_Object_Base[Pure_Count + 2];
    Constant_Object_Base =
      Read_External(Constant_Objects, Constant_Table, Constant_Base);

    primitive_table = &Constant_Object_Base[Constant_Count + 2];

    WHEN((primitive_table > Constant_Table),
	 "primitive_table overran Constant_Table");

    DEBUGGING(fprintf(stderr, "Block number %d\n", block_count));
    DEBUGGING(print_external_objects("Heap", Heap_Table, Heap_Objects));
    DEBUGGING(print_external_objects("Pure", Pure_Table, Pure_Objects));
    DEBUGGING(print_external_objects("Constant",
				     Constant_Table,
				     Constant_Objects));
  
    /* Read the normal objects */

    Free =
      Read_Pointers_and_Relocate(Heap_Count, Heap_Object_Base);

    WHEN((Free > Pure_Table),
	 "Free overran Pure_Table");
    WHEN((Free < Pure_Table),
	 "Free did not reach Pure_Table");

    Free_Pure =
      Read_Pointers_and_Relocate(Pure_Count, Pure_Object_Base);

    WHEN((Free_Pure > (Constant_Base - 2)),
	 "Free_Pure overran Constant_Base");
    WHEN((Free_Pure < (Constant_Base - 2)),
	 "Free_Pure did not reach Constant_Base");

    Free_Constant =
      Read_Pointers_and_Relocate(Constant_Count, Constant_Object_Base);

    WHEN((Free_Constant > (primitive_table - 2)),
	 "Free_Constant overran primitive_table");
    WHEN((Free_Constant < (primitive_table - 2)),
	 "Free_Constant did not reach primitive_table");

    primitive_table_end =
      read_primitives(Primitive_Table_Length, primitive_table);

    /*
      primitive_table_end can be well below Constant_Table, since
      the memory allocation is conservative (it rounds up), and all
      the slack ends up between them.
     */     

    WHEN((primitive_table_end > Constant_Table),
	 "primitive_table_end overran Constant_Table");

    if (primitive_warn)
    {
      fprintf(stderr, "%s: (block %d)\n", program_name, block_count);
      fprintf(stderr,
	      "NOTE: The binary file contains primitives with unknown arity.\n");
    }
  
    /* Dump the objects */

    {
      Pointer *Dumped_Object;

      Relocate_Into(Dumped_Object, Dumped_Object_Addr);

      DEBUGGING(fprintf(stderr, "Dumping block %d:\n", block_count));
      DEBUGGING(fprintf(stderr,
			"Heap = 0x%x; Heap Count = %d\n",
			Heap_Base, (Free - Heap_Base)));
      DEBUGGING(fprintf(stderr,
			"Pure Space = 0x%x; Pure Count = %d\n",
			Pure_Base, (Free_Pure - Pure_Base)));
      DEBUGGING(fprintf(stderr,
			"Constant Space = 0x%x; Constant Count = %d\n",
			Constant_Base, (Free_Constant - Constant_Base)));
      DEBUGGING(fprintf(stderr,
			"& Dumped Object = 0x%x; Dumped Object = 0x%x\n",
			Dumped_Object, *Dumped_Object));
      DEBUGGING(fprintf(stderr, "Primitive_Table_Length = %ld; ",
			Primitive_Table_Length));
      DEBUGGING(fprintf(stderr, "Primitive_Table_Size = %ld\n",
			(primitive_table_end - primitive_table)));
  
      /* Is there a Pure/Constant block? */

      if ((Constant_Objects == 0) && (Constant_Count == 0) &&
	  (Pure_Objects == 0) && (Pure_Count == 0))
      {
	result = Write_File(Dumped_Object, (Free - Heap_Base),
			    Heap_Base, Heap_Base,
			    0, Stack_Top,
			    0, Stack_Top,
			    primitive_table, Primitive_Table_Length,
			    ((long) (primitive_table_end - primitive_table)),
			    compiled_p, band_p);
      }
      else
      {
	long Pure_Length, Total_Length;

	Pure_Length = (Constant_Base - Pure_Base) + 1;
	Total_Length = (Free_Constant - Pure_Base) + 4;
	Pure_Base[-2] =
	  Make_Non_Pointer(TC_MANIFEST_SPECIAL_NM_VECTOR, (Pure_Length - 1));
	Pure_Base[-1] =
	  Make_Non_Pointer(PURE_PART, Total_Length);
	Constant_Base[-2] =
	  Make_Non_Pointer(TC_MANIFEST_SPECIAL_NM_VECTOR, 1);
	Constant_Base[-1] =
	  Make_Non_Pointer(CONSTANT_PART, (Pure_Length - 1));
	Free_Constant[0] =
	  Make_Non_Pointer(TC_MANIFEST_SPECIAL_NM_VECTOR, 1);
	Free_Constant[1] =
	  Make_Non_Pointer(END_OF_BLOCK, Total_Length);

	result = Write_File(Dumped_Object, (Free - Heap_Base),
			    Heap_Base, Heap_Base,
			    Total_Length, (Pure_Base - 2),
			    0, Stack_Top,
			    primitive_table, Primitive_Table_Length,
			    ((long) (primitive_table_end - primitive_table)),
			    compiled_p, band_p);
      }
    }
    if (!result)
    {
      fprintf(stderr, "%s: Error writing block %d to the output file.\n",
	      program_name, block_count);
      quit(1);
    }

    /* Get ready for the next iteration. */
    Deallocate_Heap_Space(Original_Heap, Original_Size);
  }
  /*NOTREACHED*/
}

/* Top level */

static Boolean
  help_p = false,
  help_sup_p;

static struct keyword_struct
  options[] = {
    KEYWORD("allow_nmv", &allow_nmv_p, BOOLEAN_KYWRD, BFRMT, NULL),
    KEYWORD("allow_cc", &allow_compiled_p, BOOLEAN_KYWRD, BFRMT, NULL),
    KEYWORD("help", &help_p, BOOLEAN_KYWRD, BFRMT, &help_sup_p),
    OUTPUT_KEYWORD(),
    INPUT_KEYWORD(),
    END_KEYWORD()
    };

main(argc, argv)
     int argc;
     char *argv[];
{
  parse_keywords(argc, argv, options, false);
  if (help_sup_p && help_p)
  {
    print_usage_and_exit(options, 0);
    /*NOTREACHED*/
  }
  setup_io();
  do_it();
  quit(0);
}
