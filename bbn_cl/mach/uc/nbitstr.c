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

/* $Header: nbitstr.c,v 10.0 88/12/07 13:09:19 las Exp $

   Bit string primitives. 

*/

/*

Memory layout of bit strings:

+-------+-------+-------+-------+
|  NMV	|  GC size (longwords)	| 0
+-------+-------+-------+-------+
|	   Size in bits		| 1
+-------+-------+-------+-------+
|MSB				| 2
+-------+-------+-------+-------+
|				| 3
+-------+-------+-------+-------+
.				. .
.				. .
.				. .
+-------+-------+-------+-------+
|			     LSB| N
+-------+-------+-------+-------+

The first data word (marked as word "2" above) is where any excess
bits are kept.

The "size in bits" is a C "long" integer.

Conversions between nonnegative integers and bit strings are
implemented here; they use the standard binary encoding, in which
each index selects the bit corresponding to that power of 2.  Thus
bit 0 is the LSB.

*/

#include "scheme.h"
#include "primitive.h"
#include "bignum.h"

#define bits_to_pointers( bits)					\
(((bits) + (POINTER_LENGTH - 1)) / POINTER_LENGTH)

#define bit_string_length( bit_string)				\
(Fast_Vector_Ref( bit_string, NM_ENTRY_COUNT))

#define bit_string_start_ptr( bit_string)			\
(Nth_Vector_Loc( bit_string, NM_DATA))

#define bit_string_end_ptr( bit_string)				\
(Nth_Vector_Loc( bit_string, (Vector_Length( bit_string) + 1)))

#define any_mask( nbits, offset) (low_mask( nbits) << (offset))
#define low_mask( nbits) ((1 << (nbits)) - 1)

Pointer
allocate_bit_string( length)
     long length;
{
  long total_pointers;
  Pointer result;

  total_pointers = (NM_HEADER_LENGTH + bits_to_pointers( length));
  Primitive_GC_If_Needed( total_pointers);
  Free[NM_VECTOR_HEADER] = 
    Make_Non_Pointer( TC_MANIFEST_NM_VECTOR, (total_pointers - 1));
  Free[NM_ENTRY_COUNT] = length;
  result = Make_Pointer( TC_BIT_STRING, Free);
  Free += total_pointers;
  return result;
}

/* (BIT-STRING-ALLOCATE length)
   [Primitive number 0xD1]
   Returns an uninitialized bit string of the given length. */

Built_In_Primitive( Prim_bit_string_allocate, 1, "BIT-STRING-ALLOCATE")
{
  Primitive_1_Arg();

  Arg_1_Type( TC_FIXNUM);
  return allocate_bit_string( Get_Integer( Arg1));
}

/* (BIT-STRING? object)
   [Primitive number 0xD3]
   Returns true iff object is a bit string. */

Built_In_Primitive( Prim_bit_string_p, 1, "BIT-STRING?")
{
  Primitive_1_Arg();

  Touch_In_Primitive( Arg1, Arg1);
  return ((Type_Code( Arg1) == TC_BIT_STRING) ? TRUTH : NIL);
}

void
fill_bit_string( bit_string, sense)
     Pointer bit_string;
     Boolean sense;
{
  Pointer *scanner;
  Pointer filler;
  long i;

  filler = ((Pointer) (sense ? -1 : 0));
  scanner = bit_string_start_ptr( bit_string);
  for (i = bits_to_pointers( bit_string_length( bit_string));
       (i > 0); i -= 1)
    *scanner++ = filler;
}

void
clear_bit_string( bit_string)
     Pointer bit_string;
{
  Pointer *scanner;
  long i;

  scanner = bit_string_start_ptr( bit_string);
  for (i = bits_to_pointers( bit_string_length( bit_string));
       (i > 0); i -= 1)
    *scanner++ = 0;
}

/* (MAKE-BIT-STRING size initialization)
   [Primitive number 0xD2] 
   Returns a bit string of the specified size with all the bits
   set to zero if the initialization is false, one otherwise. */

Built_In_Primitive( Prim_make_bit_string, 2, "MAKE-BIT-STRING")
{
  Pointer result;
  Primitive_2_Args();

  Arg_1_Type( TC_FIXNUM);
  result = allocate_bit_string( Get_Integer( Arg1));
  fill_bit_string( result, (Arg2 != NIL));
  return result;
}

/* (BIT-STRING-FILL! bit-string initialization)
   [Primitive number 0x197]
   Fills the bit string with zeros if the initialization is false,
   otherwise fills it with ones. */

Built_In_Primitive( Prim_bit_string_fill_x, 2, "BIT-STRING-FILL!")
{
  Primitive_2_Args();

  Arg_1_Type( TC_BIT_STRING);
  fill_bit_string( Arg1, (Arg2 != NIL));
  return NIL;
}

/* (BIT-STRING-LENGTH bit-string)
   [Primitive number 0xD4]
   Returns the number of bits in BIT-STRING. */

Built_In_Primitive(Prim_bit_string_length, 1, "BIT-STRING-LENGTH")
{
  Primitive_1_Arg();

  Arg_1_Type( TC_BIT_STRING);
  return Make_Non_Pointer( TC_FIXNUM, bit_string_length( Arg1));
}

/* The computation of the variable `word' is especially clever.  To
   understand it, note that the index of the last pointer of a vector is
   also the GC length of the vector, so that all we need do is subtract
   the zero-based word index from the GC length. */

#define index_check( To_Where, P, Low, High, Error)		\
{								\
  To_Where = Get_Integer( P);					\
  if ((To_Where < (Low)) || (To_Where >= (High)))		\
    Primitive_Error( Error)					\
}

#define index_to_word( bit_string, index)			\
(Vector_Length( bit_string) - (index / POINTER_LENGTH))

#define ref_initialization()					\
long index, word, mask;						\
Primitive_2_Args();						\
								\
Arg_1_Type( TC_BIT_STRING);					\
Arg_2_Type( TC_FIXNUM);						\
index_check( index, Arg2, 0, bit_string_length( Arg1),		\
	    ERR_ARG_2_BAD_RANGE);				\
								\
word = index_to_word( Arg1, index);				\
mask = (1 << (index % POINTER_LENGTH));

/* (BIT-STRING-REF bit-string index)
   [Primitive number 0xD5]
   Returns the boolean value of the indexed bit. */

Built_In_Primitive( Prim_bit_string_ref, 2, "BIT-STRING-REF")
{
  ref_initialization();

  if ((Fast_Vector_Ref( Arg1, word) & mask) == 0)
    return NIL;
  else
    return TRUTH;
}

/* (BIT-STRING-CLEAR! bit-string index)
   [Primitive number 0xD8]
   Sets the indexed bit to zero, returning its previous value
   as a boolean. */

Built_In_Primitive( Prim_bit_string_clear_x, 2, "BIT-STRING-CLEAR!")
{
  ref_initialization();

  if ((Fast_Vector_Ref( Arg1, word) & mask) == 0)
    return NIL;
  else
    {
      Fast_Vector_Ref( Arg1, word) &= ~mask;
      return TRUTH;
    }
}

/* (BIT-STRING-SET! bit-string index)
   [Primitive number 0xD7]
   Sets the indexed bit to one, returning its previous value
   as a boolean. */

Built_In_Primitive( Prim_bit_string_set_x, 2, "BIT-STRING-SET!")
{
  ref_initialization();

  if ((Fast_Vector_Ref( Arg1, word) & mask) == 0)
    {
      Fast_Vector_Ref( Arg1, word) |= mask;
      return NIL;
    }
  else
    return TRUTH;
}

#define zero_section_p( start)					\
{								\
  long i;							\
  Pointer *scan;						\
								\
  scan = Nth_Vector_Loc( Arg1, (start));			\
  for (i = (length / POINTER_LENGTH); (i > 0); i -= 1)		\
    if (*scan++ != 0)						\
      return NIL;						\
  return TRUTH;							\
}

/* (BIT-STRING-ZERO? bit-string)
   [Primitive number 0xD9]
   Returns true the argument has no "set" bits. */

Built_In_Primitive( Prim_bit_string_zero_p, 2, "BIT-STRING-ZERO?")
{
  long length, odd_bits;
  Primitive_1_Args();

  Arg_1_Type(TC_BIT_STRING);

  length = bit_string_length( Arg1);
  odd_bits = (length % POINTER_LENGTH);
  if (odd_bits == 0)
    zero_section_p( NM_DATA)
  else if ((Fast_Vector_Ref( Arg1, NM_DATA) & low_mask( odd_bits)) != 0)
    return NIL;
  else
    zero_section_p( NM_DATA + 1)
}

#define equal_sections_p( start)				\
{								\
  long i;							\
  Pointer *scan1, *scan2;					\
								\
  scan1 = Nth_Vector_Loc( Arg1, (start));			\
  scan2 = Nth_Vector_Loc( Arg2, (start));			\
  for (i = (length / POINTER_LENGTH); (i > 0); i -= 1)		\
    if (*scan1++ != *scan2++)					\
      return NIL;						\
  return TRUTH;							\
}

/* (BIT-STRING=? bit-string-1 bit-string-2)
   [Primitive number 0x19D]
   Returns true iff the two bit strings contain the same bits. */

Built_In_Primitive( Prim_bit_string_equal_p, 2, "BIT-STRING=?")
{
  long length;
  Primitive_2_Args();

  Arg_1_Type(TC_BIT_STRING);
  Arg_2_Type(TC_BIT_STRING);

  length = bit_string_length( Arg1);
  if (length != bit_string_length( Arg2))
    return NIL;
  else
    {
      long odd_bits;

      odd_bits = (length % POINTER_LENGTH);
      if (odd_bits == 0)
	equal_sections_p( NM_DATA)
      else
	{
	  long mask;

	  mask = low_mask( odd_bits);
	  if ((Fast_Vector_Ref( Arg1, NM_DATA) & mask)
	      != (Fast_Vector_Ref( Arg2, NM_DATA) & mask))
	    return NIL;
	  else
	    equal_sections_p( NM_DATA + 1)
	}
    }
}

#define bitwise_op( action)					\
{								\
  Primitive_2_Args();						\
								\
  if (bit_string_length( Arg1) != bit_string_length( Arg2))	\
    Primitive_Error( ERR_ARG_1_BAD_RANGE)			\
  else								\
    {								\
      long i;							\
      Pointer *scan1, *scan2;					\
								\
      scan1 = bit_string_start_ptr( Arg1);			\
      scan2 = bit_string_start_ptr( Arg2);			\
      for (i = (Vector_Length( Arg1) - 1); (i > 0); i -= 1)	\
	*scan1++ action() (*scan2++);				\
    }								\
  return (NIL);							\
}

#define bit_string_move_x_action() =
#define bit_string_movec_x_action() = ~
#define bit_string_or_x_action() |=
#define bit_string_and_x_action() &=
#define bit_string_andc_x_action() &= ~

Built_In_Primitive( Prim_bit_string_move_x, 2, "BIT-STRING-MOVE!")
     bitwise_op( bit_string_move_x_action)

Built_In_Primitive( Prim_bit_string_movec_x, 2, "BIT-STRING-MOVEC!")
     bitwise_op( bit_string_movec_x_action)

Built_In_Primitive( Prim_bit_string_or_x, 2, "BIT-STRING-OR!")
     bitwise_op( bit_string_or_x_action)

Built_In_Primitive( Prim_bit_string_and_x, 2, "BIT-STRING-AND!")
     bitwise_op( bit_string_and_x_action)

Built_In_Primitive( Prim_bit_string_andc_x, 2, "BIT-STRING-ANDC!")
     bitwise_op( bit_string_andc_x_action)

/* (BIT-SUBSTRING-MOVE-RIGHT! source start1 end1 destination start2)
   [Primitive number 0xD6]
   Destructively copies the substring of SOURCE between START1 and
   END1 into DESTINATION at START2.  The copying is done from the
   MSB to the LSB (which only matters when SOURCE and DESTINATION
   are the same). */

Built_In_Primitive( Prim_bit_substring_move_right_x, 5,
		   "BIT-SUBSTRING-MOVE-RIGHT!")
{
  long start1, end1, start2, end2, nbits;
  long end1_mod, end2_mod;
  void copy_bits();
  Primitive_5_Args();

  Arg_1_Type( TC_BIT_STRING);
  Arg_2_Type( TC_FIXNUM);
  Arg_3_Type( TC_FIXNUM);
  Arg_4_Type( TC_BIT_STRING);
  Arg_5_Type( TC_FIXNUM);

  start1 = Get_Integer( Arg2);
  end1 = Get_Integer( Arg3);
  start2 = Get_Integer( Arg5);
  nbits = (end1 - start1);
  end2 = (start2 + nbits);

  if ((start1 < 0) || (start1 > end1))
    Primitive_Error( ERR_ARG_2_BAD_RANGE);
  if (end1 > bit_string_length( Arg1))
    Primitive_Error( ERR_ARG_3_BAD_RANGE);
  if ((start2 < 0) || (end2 > bit_string_length( Arg4)))
    Primitive_Error( ERR_ARG_5_BAD_RANGE);

  end1_mod = (end1 % POINTER_LENGTH);
  end2_mod = (end2 % POINTER_LENGTH);

  /* Using `index_to_word' here with -1 offset will work in every
     case except when the `end' is 0.  In this case the result of
     the expression `(-1 / POINTER_LENGTH)' is either 0 or -1, at
     the discretion of the C compiler being used.  This doesn't
     matter because if `end' is zero, then no bits will be moved. */

  copy_bits( Nth_Vector_Loc( Arg1, index_to_word( Arg1, (end1 - 1))),
	    ((end1_mod == 0) ? 0 : (POINTER_LENGTH - end1_mod)),
	    Nth_Vector_Loc( Arg4, index_to_word( Arg4, (end2 - 1))),
	    ((end2_mod == 0) ? 0 : (POINTER_LENGTH - end2_mod)),
	    nbits);
  return (NIL);
}

#define masked_transfer( source, destination, nbits, offset)	\
{								\
  long mask;							\
								\
  mask = any_mask( nbits, offset);				\
  *destination = ((*source & mask) | (*destination & ~mask));	\
}

/* This procedure copies bits from one place to another.
   The offsets are measured from the MSB of the first Pointer of
   each of the arguments SOURCE and DESTINATION.  It copies the bits
   starting with the MSB of a bit string and moving down. */

void
copy_bits( source, source_offset, destination, destination_offset, nbits)
     Pointer *source, *destination;
     long source_offset, destination_offset, nbits;
{

  /* This common case can be done very quickly, by splitting the
     bit string into three parts.  Since the source and destination are
     aligned relative to one another, the main body of bits can be
     transferred as Pointers, and only the `head' and `tail' need be
     treated specially. */

  if (source_offset == destination_offset)
    {
      if (source_offset != 0)
	{
	  long head;

	  head = (POINTER_LENGTH - source_offset);
	  if (nbits <= head)
	    {
	      masked_transfer( source, destination, nbits, (head - nbits));
	      nbits = 0;
	    }
	  else
	    { Pointer temp;
	      long mask;

	      mask = low_mask( head);
	      *destination = ((*source++ & mask) | (*destination & ~mask));
	      destination++;	/* Carefully disambiguate - sas */
	      nbits -= head;
	    }
	}
      if (nbits > 0)
	{
	  long nwords, tail;

	  for (nwords = (nbits / POINTER_LENGTH); (nwords > 0); nwords -= 1)
	    *destination++ = *source++;

	  tail = (nbits % POINTER_LENGTH);
	  if (tail > 0)
	    masked_transfer( source, destination, tail,
			    (POINTER_LENGTH - tail));
	}
    }

  else if (source_offset < destination_offset)
    {
      long offset1, offset2, head;

      offset1 = (destination_offset - source_offset);
      offset2 = (POINTER_LENGTH - offset1);
      head = (POINTER_LENGTH - destination_offset);

      if (nbits <= head)
	{
	  long mask;

	  mask = any_mask( nbits, (head - nbits));
	  *destination =
	    (((*source >> offset1) & mask) | (*destination & ~mask));
	}
      else
	{
	  long mask1, mask2;

	  { Pointer temp;
	    long mask;

	    mask = low_mask( head);
	    *destination =
	      (((*source >> offset1) & mask) | (*destination & ~mask));
	    destination++;	/* Carefully disambiguate. */
	  }
	  nbits -= head;
	  mask1 = low_mask( offset1);
	  mask2 = low_mask( offset2);
	  {
	    long nwords, i;

	    for (nwords = (nbits / POINTER_LENGTH); (nwords > 0); nwords -= 1)
	      {
		i = ((*source++ & mask1) << offset2);
		*destination++ = (((*source >> offset1) & mask2) | i);
	      }
	  }

	  {
	    long tail, dest_tail;

	    tail = (nbits % POINTER_LENGTH);
	    dest_tail = (*destination & low_mask( POINTER_LENGTH - tail));
	    if (tail <= offset1)
	      *destination =
		(((*source & any_mask( tail, (offset1 - tail))) << offset2)
		 | dest_tail);
	    else
	      {
		long i, j;

		i = ((*source++ & mask1) << offset2);
		j = (tail - offset1);
		*destination =
		  (((*source & any_mask( j, (POINTER_LENGTH - j))) >> offset1)
		    | i | dest_tail);
	      }
	  }
	}
    }

  else /* if (source_offset > destination_offset) */
    {
      long offset1, offset2, head;

      offset1 = (source_offset - destination_offset);
      offset2 = (POINTER_LENGTH - offset1);
      head = (POINTER_LENGTH - source_offset);

      if (nbits <= head)
	{
	  long mask;

	  mask = any_mask( nbits, (offset1 + (head - nbits)));
	  *destination =
	    (((*source << offset1) & mask) | (*destination & ~mask));
	}
      else
	{
	  long dest_buffer, mask1, mask2;

	  {
	    long mask;

	    mask = any_mask( head, offset1);
	    dest_buffer =
	      ((*destination & ~mask)
	       | ((*source++ << offset1) & mask));
	  }
	  nbits -= head;
	  mask1 = low_mask( offset1);
	  mask2 = any_mask( offset2, offset1);
	  {
	    long nwords;

	    nwords = (nbits / POINTER_LENGTH);
	    if (nwords > 0)
	      dest_buffer &= mask2;
	    for (; (nwords > 0); nwords -= 1)
	      {
		*destination++ =
		  (dest_buffer | ((*source >> offset2) & mask1));
		dest_buffer = (*source++ << offset1);
	      }
	  }

	  {
	    long tail;

	    tail = (nbits % POINTER_LENGTH);
	    if (tail <= offset1)
	      *destination =
		(dest_buffer
		 | (*destination & low_mask( offset1 - tail))
		 | ((*source >> offset2) & any_mask( tail, (offset1 - tail))));
	    else
	      {
		long mask;

		*destination++ =
		  (dest_buffer | ((*source >> offset2) & mask1));
		mask = low_mask( POINTER_LENGTH - tail);
		*destination =
		  ((*destination & ~mask) | ((*source << offset1) & mask));
	      }
	  }
	}
    }
}

/* Integer <-> Bit-string Conversions */

long
count_significant_bits( number, start)
     long number, start;
{
  long significant_bits, i;

  significant_bits = start;
  for (i = (1 << (start - 1)); (i >= 0); i >>= 1)
    {
      if (number >= i)
	break;
      significant_bits -= 1;
    }
  return significant_bits;
}

long
long_significant_bits( number)
     long number;
{
  if (number < 0)
    return ULONG_SIZE;
  else
    return count_significant_bits( number, (ULONG_SIZE - 1));
}

Pointer
zero_to_bit_string( length)
     long length;
{
  Pointer result;

  result = allocate_bit_string( length);
  clear_bit_string( result);
  return result;
}

Pointer
long_to_bit_string( length, number)
     long length, number;
{
  if (number < 0)
    Primitive_Error( ERR_ARG_2_BAD_RANGE)
  else if (number == 0)
    zero_to_bit_string( length);
  else
    {
      if (length < long_significant_bits( number))
	Primitive_Error( ERR_ARG_2_BAD_RANGE)
      else
	{
	  Pointer result;

	  result = allocate_bit_string( length);
	  clear_bit_string( result);
	  Fast_Vector_Set( result, Vector_Length( result), number);
	  return result;
	}
    }
}

Pointer
bignum_to_bit_string( length, bignum)
     long length;
     Pointer bignum;
{
  bigdigit *bigptr;
  long ndigits;

  bigptr = BIGNUM( Get_Pointer( bignum));
  if (NEG_BIGNUM( bigptr))
    Primitive_Error( ERR_ARG_2_BAD_RANGE);
  ndigits = LEN( bigptr);
  if (ndigits == 0)
    zero_to_bit_string( length);
  else
    {
      if (length <
	  (count_significant_bits( *(Bignum_Top( bigptr)), SHIFT)
	   + (SHIFT * (ndigits - 1))))
	Primitive_Error( ERR_ARG_2_BAD_RANGE)
      else
	{
	  Pointer result;
	  bigdigit *scan1, *scan2;

	  result = allocate_bit_string( length);
	  scan1 = Bignum_Bottom( bigptr);
	  scan2 = ((bigdigit *) bit_string_end_ptr( result));
	  for (; (ndigits > 0); ndigits -= 1)
	    *--scan2 = *scan1++;
	  return result;
	}
    }
}

/* (UNSIGNED-INTEGER->BIT-STRING length integer)
   [Primitive number 0xDC]
   INTEGER, which must be a non-negative integer, is converted to
   a bit-string of length LENGTH.  If INTEGER is too large, an
   error is signalled. */

Built_In_Primitive( Prim_unsigned_integer_to_bit_string, 2,
		   "UNSIGNED-INTEGER->BIT-STRING")
{
  long length;
  Primitive_2_Args();

  Arg_1_Type( TC_FIXNUM);
  length = Get_Integer( Arg1);
  if (length < 0)
    Primitive_Error( ERR_ARG_1_BAD_RANGE)
  else if (Type_Code( Arg2) == TC_FIXNUM)
    return long_to_bit_string( length, Get_Integer( Arg2));
  else if (Type_Code( Arg2) == TC_BIG_FIXNUM)
    return bignum_to_bit_string( length, Arg2);
  else
    Primitive_Error( ERR_ARG_2_WRONG_TYPE)
}

/* (BIT-STRING->UNSIGNED-INTEGER bit-string)
   [Primitive number 0xDD]
   BIT-STRING is converted to the appropriate non-negative integer.
   This operation is the inverse of `integer->bit-string'. */

Built_In_Primitive( Prim_bit_string_to_unsigned_integer, 1,
		   "BIT-STRING->UNSIGNED-INTEGER")
{
  Pointer *scan;
  long nwords, nbits, ndigits, align_ndigits, word;
  bigdigit *bignum, *scan1, *scan2;
  
  Primitive_1_Arg();

  Arg_1_Type( TC_BIT_STRING);

  /* Count the number of significant bits.*/
  scan = bit_string_start_ptr( Arg1);
  nbits = (bit_string_length( Arg1) % POINTER_LENGTH);
  word = ((nbits > 0) ? (*scan++ & low_mask( nbits)) : *scan++);
  for (nwords = (Vector_Length( Arg1) - 1); (nwords > 0); nwords -= 1)
    {
      if (word != 0)
	break;
      else
	word = *scan++;
    }
  if (nwords == 0)
    return FIXNUM_0;
  nbits = (((nwords - 1) * POINTER_LENGTH) + long_significant_bits( word));

  /* Handle fixnum case. */
  if (nbits < FIXNUM_LENGTH)
    return (Make_Unsigned_Fixnum( word));

  /* Now the interesting one, we must make a bignum. */
  ndigits = ((nbits + (SHIFT - 1)) / SHIFT);
  align_ndigits = Align( ndigits);
  Primitive_GC_If_Needed( align_ndigits);
  bignum = BIGNUM( Free);
  Free += align_ndigits;
  Prepare_Header( bignum, ndigits, POSITIVE);

  scan1 = ((bigdigit *) bit_string_end_ptr( Arg1));
  scan2 = Bignum_Bottom( bignum);
  for (; (ndigits > 0); ndigits -= 1)
    *scan2++ = *--scan1;
  nbits = (nbits % SHIFT);
  if (nbits != 0)
    *scan2 = (*--scan2 & low_mask( nbits));

  return Make_Pointer( TC_BIG_FIXNUM, bignum);
}

/* These primitives should test the type of their first argument to
   verify that it is a pointer. */

/* (READ-BITS! pointer offset bit-string)
   [Primitive number 0xDF]
   Read the contents of memory at the address (POINTER,OFFSET)
   into BIT-STRING. */

Built_In_Primitive( Prim_read_bits_x, 3, "READ-BITS!")
{
  long end, end_mod;
  Primitive_3_Args();

  Arg_2_Type( TC_FIXNUM);
  Arg_3_Type( TC_BIT_STRING);
  end = bit_string_length( Arg3);
  end_mod = (end % POINTER_LENGTH);
  copy_bits( Nth_Vector_Loc( Arg1, 0), Get_Integer( Arg2),
	    Nth_Vector_Loc( Arg3, index_to_word( Arg3, (end - 1))),
	    ((end_mod == 0) ? 0 : (POINTER_LENGTH - end_mod)),
	    end);
  return (NIL);
}

/* (WRITE-BITS! pointer offset bit-string)
   [Primitive number 0xE0]
   Write the contents of BIT-STRING in memory at the address
   (POINTER,OFFSET). */

Built_In_Primitive( Prim_write_bits_x, 3, "WRITE-BITS!")
{
  long end, end_mod;
  Primitive_3_Args();

  Arg_2_Type( TC_FIXNUM);
  Arg_3_Type( TC_BIT_STRING);
  end = bit_string_length( Arg3);
  end_mod = (end % POINTER_LENGTH);
  copy_bits( Nth_Vector_Loc( Arg3, index_to_word( Arg3, (end - 1))),
	    ((end_mod == 0) ? 0 : (POINTER_LENGTH - end_mod)),
	    Nth_Vector_Loc( Arg1, 0), Get_Integer( Arg2),
	    end);
  return (NIL);
}
