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

/* $Header: object.h,v 10.0 88/12/07 13:09:26 las Exp $
 * $MIT-Header: object.h,v 9.27 87/12/04 22:18:23 GMT jinx Exp $
 */

/* This file contains definitions pertaining to the C view of 
   Scheme pointers: widths of fields, extraction macros, pre-computed
   extraction masks, etc. */

/* The C type Pointer is defined at the end of CONFIG.H
   The definition of POINTER_LENGTH here assumes that Pointer is the same
   as unsigned long.  If that ever changes, this definition must also.
   POINTER_LENGTH is defined this way to make it available to
   the preprocessor. */

#define POINTER_LENGTH		ULONG_SIZE
#define TYPE_CODE_LENGTH	8	/* Not CHAR_SIZE!! */
#define MAX_TYPE_CODE		0xFF	/* ((1<<TYPE_CODE_LENGTH) - 1) */

#ifndef b32			/* Portable versions */

#define ADDRESS_LENGTH		(POINTER_LENGTH-TYPE_CODE_LENGTH)
#define ADDRESS_MASK		((1 << ADDRESS_LENGTH) - 1)
#define TYPE_CODE_MASK		(~ADDRESS_MASK)
/* FIXNUM_LENGTH does NOT include the sign bit! */
#define FIXNUM_LENGTH		(ADDRESS_LENGTH - 1)
#define FIXNUM_SIGN_BIT		(1 << FIXNUM_LENGTH)
#define SIGN_MASK		(TYPE_CODE_MASK | FIXNUM_SIGN_BIT)
#define SMALLEST_FIXNUM		(-1 << FIXNUM_LENGTH)
#define BIGGEST_FIXNUM		(~(-1 << FIXNUM_LENGTH))

#define HALF_ADDRESS_LENGTH	(ADDRESS_LENGTH / 2)
#define HALF_ADDRESS_MASK	((1 << HALF_ADDRESS_LENGTH) - 1)

#else				/* 32 bit word versions */

#define ADDRESS_LENGTH		24
#define ADDRESS_MASK		0x00FFFFFF
#define TYPE_CODE_MASK		0xFF000000
#define FIXNUM_LENGTH		23
#define FIXNUM_SIGN_BIT		0x00800000
#define SIGN_MASK		0xFF800000
#define SMALLEST_FIXNUM		0xFF800000
#define BIGGEST_FIXNUM		0x007FFFFF

#define HALF_ADDRESS_LENGTH	12
#define HALF_ADDRESS_MASK	0x00000FFF

#endif

#ifndef UNSIGNED_SHIFT		/* Portable version */
#define OBJECT_TYPE(P)		(((P) >> ADDRESS_LENGTH) & MAX_TYPE_CODE)
#else				/* Faster for logical shifts */
#define OBJECT_TYPE(P)		((P) >> ADDRESS_LENGTH)
#endif

#define OBJECT_DATUM(P)		((P) & ADDRESS_MASK)

/* compatibility definitions */
#define Type_Code(P)		(OBJECT_TYPE (P))
#define Datum(P)		(OBJECT_DATUM (P))

#define pointer_type(P)		(OBJECT_TYPE (P))
#define pointer_datum(P)	(OBJECT_DATUM (P))

#define Make_Object(TC, D)					\
((((unsigned) (TC)) << ADDRESS_LENGTH) | (OBJECT_DATUM (D)))

#ifndef Heap_In_Low_Memory	/* Portable version */

typedef Pointer *relocation_type; /* Used to relocate pointers on fasload */

extern Pointer *Memory_Base;

/* The "-1" in the value returned is a guarantee that there is one
   word reserved exclusively for use by the garbage collector. */

#define Allocate_Heap_Space(space)						\
  (Memory_Base = ((Pointer *) (malloc ((sizeof (Pointer)) * (space)))),		\
   Heap = Memory_Base,								\
   ((Memory_Base + (space)) - 1))

#define Get_Pointer(P) ((Pointer *) (Memory_Base + (OBJECT_DATUM (P))))
#define C_To_Scheme(P) ((Pointer) ((P) - Memory_Base))

#else				/* Storing absolute addresses */

typedef long relocation_type;	/* Used to relocate pointers on fasload */

#define Allocate_Heap_Space(space)				\
  (Heap = ((Pointer *) (malloc ((sizeof (Pointer)) * (space)))), \
   ((Heap + (space)) - 1))

#ifdef spectrum

#define Quad1_Tag 	0x40000000
#define Get_Pointer(P)	((Pointer *) (((P) & ADDRESS_MASK) | Quad1_Tag))
#define C_To_Scheme(P)  ((Pointer) (((long) (P)) & ADDRESS_MASK))

#else /* Not Spectrum, fast case */

#define Get_Pointer(P)		((Pointer *) (OBJECT_DATUM (P)))
#define C_To_Scheme(P)          ((Pointer) (P))

#endif /* spectrum */
#endif /* Heap_In_Low_Memory */

#define Make_Pointer(TC, A)	Make_Object((TC), C_To_Scheme(A))
#define Make_Non_Pointer(TC, D)	Make_Object(TC, ((Pointer) (D)))

/* (Make_New_Pointer (TC, A)) may be more efficient than
   (Make_Pointer (TC, (Get_Pointer (A)))) */

#define Make_New_Pointer(TC, A) (Make_Object (TC, ((Pointer) A)))

#define Store_Type_Code(P, TC)	P = (Make_Object ((TC), (P)))

#define Store_Address(P, A)					\
  P = (((P) & TYPE_CODE_MASK) | (OBJECT_DATUM ((Pointer) (A))))

#define Address(P) (OBJECT_DATUM (P))

/* These are used only where the object is known to be immutable.
   On a parallel processor they don't require atomic references */

#define Fast_Vector_Ref(P, N)		((Get_Pointer(P))[N])
#define Fast_Vector_Set(P, N, S)	Fast_Vector_Ref(P, N) = (S)
#define Fast_User_Vector_Ref(P, N) 	Fast_Vector_Ref(P, (N)+1)
#define Fast_User_Vector_Set(P, N, S)	Fast_Vector_Set(P, (N)+1, S)
#define Nth_Vector_Loc(P, N)		(&(Fast_Vector_Ref(P, N)))
#define Vector_Length(P)		(Get_Integer(Fast_Vector_Ref((P), 0)))

/* General case vector handling requires atomicity for parallel processors */

#define Vector_Ref(P, N)		Fetch(Fast_Vector_Ref(P, N))
#define Vector_Set(P, N, S)     	Store(Fast_Vector_Ref(P, N), S)
#define User_Vector_Ref(P, N)		Vector_Ref(P, (N)+1)
#define User_Vector_Set(P, N, S)  	Vector_Set(P, (N)+1, S)

#define FIXNUM_P(object) ((OBJECT_TYPE (object)) == TC_FIXNUM)
#define BIGNUM_P(object) ((OBJECT_TYPE (object)) == TC_BIG_FIXNUM)
#define FLONUM_P(object) ((OBJECT_TYPE (object)) == TC_BIG_FLONUM)
#define COMPLEX_P(object) ((OBJECT_TYPE (object)) == TC_COMPLEX)
#define CHARACTER_P(object) ((OBJECT_TYPE (object)) == TC_CHARACTER)
#define STRING_P(object) ((OBJECT_TYPE (object)) == TC_CHARACTER_STRING)
#define BIT_STRING_P(object) ((OBJECT_TYPE (object)) == TC_BIT_STRING)
#define CELL_P(object) ((OBJECT_TYPE (object)) == TC_CELL)
#define PAIR_P(object) ((OBJECT_TYPE (object)) == TC_LIST)
#define WEAK_PAIR_P(object) ((OBJECT_TYPE (object)) == TC_WEAK_CONS)
#define VECTOR_P(object) ((OBJECT_TYPE (object)) == TC_VECTOR)
#define REFERENCE_TRAP_P(object) ((OBJECT_TYPE (object)) == TC_REFERENCE_TRAP)

#define NON_MARKED_VECTOR_P(object)					\
  ((OBJECT_TYPE (object)) == TC_NON_MARKED_VECTOR)

#define SYMBOL_P(object)						\
  (((OBJECT_TYPE (object)) == TC_INTERNED_SYMBOL) ||			\
   ((OBJECT_TYPE (object)) == TC_UNINTERNED_SYMBOL))

#define INTEGER_P(object)						\
  (((OBJECT_TYPE (object)) == TC_FIXNUM) ||				\
   ((OBJECT_TYPE (object)) == TC_BIG_FIXNUM))

#define REAL_P(object)							\
  (((OBJECT_TYPE (object)) == TC_FIXNUM) ||				\
   ((OBJECT_TYPE (object)) == TC_BIG_FIXNUM) ||				\
   ((OBJECT_TYPE (object)) == TC_BIG_FLONUM))

#define NUMBER_P(object)						\
  (((OBJECT_TYPE (object)) == TC_FIXNUM) ||				\
   ((OBJECT_TYPE (object)) == TC_BIG_FIXNUM) ||				\
   ((OBJECT_TYPE (object)) == TC_BIG_FLONUM)				\
   ((OBJECT_TYPE (object)) == TC_COMPLEX))

#define HUNK3_P(object)							\
  (((OBJECT_TYPE(object)) == TC_HUNK3_A) ||				\
   ((OBJECT_TYPE(object)) == TC_HUNK3_B))

#define MAKE_FIXNUM(N) (Make_Non_Pointer (TC_FIXNUM, (N)))
#define FIXNUM_NEGATIVE_P(fixnum) (((fixnum) & FIXNUM_SIGN_BIT) != 0)
#define MAKE_UNSIGNED_FIXNUM(N)	(FIXNUM_ZERO + (N))
#define UNSIGNED_FIXNUM_VALUE(fixnum) (OBJECT_DATUM (fixnum))
#define MAKE_SIGNED_FIXNUM Make_Signed_Fixnum
#define long_to_object C_Integer_To_Scheme_Integer

#define FIXNUM_VALUE(fixnum, target)					\
do									\
{									\
  (target) = (UNSIGNED_FIXNUM_VALUE (fixnum));				\
  if (FIXNUM_NEGATIVE_P (target))					\
    (target) |= (-1 << ADDRESS_LENGTH);					\
} while (0)

#define BOOLEAN_TO_OBJECT(expression) ((expression) ? TRUTH : NIL)

#define Make_Broken_Heart(N)	(BROKEN_HEART_ZERO + (N))
#define Make_Unsigned_Fixnum(N)	(FIXNUM_ZERO + (N))
#define Make_Signed_Fixnum(N)	Make_Non_Pointer( TC_FIXNUM, (N))
#define Get_Float(P)   (* ((double *) (Nth_Vector_Loc ((P), 1))))
#define Get_Integer(P) (OBJECT_DATUM (P))

#define Sign_Extend(P, S)					\
{								\
  (S) = (Get_Integer (P));					\
  if (((S) & FIXNUM_SIGN_BIT) != 0)				\
    (S) |= (-1 << ADDRESS_LENGTH);				\
}

#define Fixnum_Fits(x)						\
  ((((x) & SIGN_MASK) == 0) ||					\
   (((x) & SIGN_MASK) == SIGN_MASK))

#define BYTES_TO_POINTERS(nbytes)					\
  (((nbytes) + ((sizeof (Pointer)) - 1)) / (sizeof (Pointer)))

#define Is_Constant(address) 					\
  (((address) >= Constant_Space) && ((address) < Free_Constant))

#define Is_Pure(address)					\
  ((Is_Constant (address)) && (Pure_Test (address)))

#define Side_Effect_Impurify(Old_Pointer, Will_Contain)		\
if ((Is_Constant (Get_Pointer (Old_Pointer))) &&		\
    (GC_Type (Will_Contain) != GC_Non_Pointer) &&		\
    (! (Is_Constant (Get_Pointer (Will_Contain)))) &&		\
    (Pure_Test (Get_Pointer (Old_Pointer))))			\
  Primitive_Error (ERR_WRITE_INTO_PURE_SPACE);

#ifdef FLOATING_ALIGNMENT

#define FLOATING_BUFFER_SPACE		\
	((FLOATING_ALIGNMENT + 1)/sizeof(Pointer))

#define HEAP_BUFFER_SPACE		\
	(TRAP_MAX_IMMEDIATE + 1 + FLOATING_BUFFER_SPACE)

/* The space is there, find the correct position. */

#define Initial_Align_Float(Where)					\
{									\
  while ((((long) ((Where) + 1)) & FLOATING_ALIGNMENT) != 0)		\
    Where -= 1;								\
}

#define Align_Float(Where)						\
{									\
  while ((((long) ((Where) + 1)) & FLOATING_ALIGNMENT) != 0)		\
    *Where++ = (Make_Non_Pointer (TC_MANIFEST_NM_VECTOR, 0));		\
}

#else not FLOATING_ALIGNMENT

#define HEAP_BUFFER_SPACE		 (TRAP_MAX_IMMEDIATE + 1)

#define Initial_Align_Float(Where)
#define Align_Float(Where)

#endif FLOATING_ALIGNMENT
