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

/* $Header: types.h,v 10.0 88/12/07 13:11:32 las Exp $
 * $MIT-Header: types.h,v 9.28 88/03/12 16:07:56 GMT jinx Rel $
 *
 * Type code definitions, numerical order
 *
 */

/*	Name				Value	Previous Name */

#define TC_NULL	                	0x00
#define TC_LIST				0x01
#define TC_CHARACTER			0x02
#define	TC_SCODE_QUOTE                 	0x03
#define TC_PCOMB2			0x04
#define TC_UNINTERNED_SYMBOL		0x05
#define TC_BIG_FLONUM			0x06
#define TC_COMBINATION_1		0x07
#define TC_TRUE				0x08
#define TC_EXTENDED_PROCEDURE		0x09
#define TC_VECTOR			0x0A
#define TC_RETURN_CODE 			0x0B
#define TC_COMBINATION_2		0x0C
#define TC_MANIFEST_CLOSURE		0x0D /* MANIFEST_CLOSURE */
#define TC_BIG_FIXNUM			0x0E
#define TC_PROCEDURE			0x0F
#define TC_ENTITY			0x10 /* PRIMITIVE_EXTERNAL */
#define TC_DELAY			0x11
#define TC_ENVIRONMENT			0x12
#define TC_DELAYED			0x13
#define TC_EXTENDED_LAMBDA		0x14
#define TC_COMMENT			0x15
#define TC_NON_MARKED_VECTOR		0x16
#define TC_LAMBDA			0x17
#define TC_PRIMITIVE			0x18
#define TC_SEQUENCE_2			0x19

#define TC_FIXNUM			0x1A
#define TC_PCOMB1			0x1B
#define TC_CONTROL_POINT		0x1C
#define TC_INTERNED_SYMBOL		0x1D
#define TC_CHARACTER_STRING		0x1E
#define TC_ACCESS			0x1F
#define TC_HUNK3_A			0x20 /* EXTENDED_FIXNUM */
#define TC_DEFINITION			0x21
#define TC_BROKEN_HEART			0x22
#define TC_ASSIGNMENT			0x23
#define TC_HUNK3_B			0x24
#define TC_IN_PACKAGE			0x25
#define TC_COMBINATION			0x26
#define TC_MANIFEST_NM_VECTOR		0x27
#define TC_COMPILED_ENTRY		0x28 /* COMPILED_EXPRESSION */
#define TC_LEXPR			0x29
#define TC_PCOMB3  			0x2A
#define TC_MANIFEST_SPECIAL_NM_VECTOR	0x2B
#define TC_VARIABLE			0x2C
#define TC_THE_ENVIRONMENT		0x2D
#define TC_FUTURE			0x2E
#define TC_VECTOR_1B			0x2F
#define TC_PCOMB0			0x30
#define TC_VECTOR_16B			0x31
#define TC_REFERENCE_TRAP		0x32 /* UNASSIGNED */
#define TC_SEQUENCE_3			0x33
#define TC_CONDITIONAL			0x34
#define TC_DISJUNCTION			0x35
#define TC_CELL				0x36
#define TC_WEAK_CONS			0x37
#define TC_QUAD				0x38 /* TRAP */
#define TC_LINKAGE_SECTION		0x39 /* RETURN_ADDRESS */
#define TC_RATNUM			0x3A /* COMPILER_LINK */
#define TC_STACK_ENVIRONMENT		0x3B
#define TC_COMPLEX			0x3C
#define TC_COMPILED_CODE_BLOCK		0x3D
/* the following added for spice lisp compatibility */
/*					0x3E	/* EMPTY SLOT */
/*					0x3F	/* EMPTY SLOT */
#define TC_G_VECTOR			0x40
/* For getting error codes from os's without consing */
#define TC_IO_ERROR_CODE                0x41
/* Common Lisp */
#define TC_CL_PACKAGE			0x42
#define TC_CLSAV                        0x43
#define TC_RATIO                        0x44
#define TC_CL_STREAM                    0x45
/* General, but needed for Common lisp streams */
#define TC_VECTOR_32B                   0x46
#define TC_CL_ARRAY			0x47
#define TC_CL_IVECTOR			0x48

/* If you add a new type, don't forget to update gccode.h, gctype.c,
   and the type name table below.
 */

#define LAST_TYPE_CODE			0X48

#define TYPE_NAME_TABLE							\
{									\
  /* 0x00 */	                "NULL",					\
  /* 0x01 */			"LIST",					\
  /* 0x02 */			"CHARACTER",				\
  /* 0x03 */                 	"SCODE-QUOTE",				\
  /* 0x04 */			"PCOMB2",				\
  /* 0x05 */			"UNINTERNED-SYMBOL",			\
  /* 0x06 */			"BIG-FLONUM",				\
  /* 0x07 */			"COMBINATION-1",			\
  /* 0x08 */			"TRUE",					\
  /* 0x09 */			"EXTENDED-PROCEDURE",			\
  /* 0x0A */			"VECTOR",				\
  /* 0x0B */ 			"RETURN-CODE",				\
  /* 0x0C */			"COMBINATION-2",			\
  /* 0x0D */			"MANIFEST-CLOSURE",			\
  /* 0x0E */			"BIG-FIXNUM",				\
  /* 0x0F */			"PROCEDURE",				\
  /* 0x10 */			"ENTITY",				\
  /* 0x11 */			"DELAY",				\
  /* 0x12 */			"ENVIRONMENT",				\
  /* 0x13 */			"DELAYED",				\
  /* 0x14 */			"EXTENDED-LAMBDA",			\
  /* 0x15 */			"COMMENT",				\
  /* 0x16 */			"NON-MARKED-VECTOR",			\
  /* 0x17 */			"LAMBDA",				\
  /* 0x18 */			"PRIMITIVE",				\
  /* 0x19 */			"SEQUENCE-2",				\
  /* 0x1A */			"FIXNUM",				\
  /* 0x1B */			"PCOMB1",				\
  /* 0x1C */			"CONTROL-POINT",			\
  /* 0x1D */			"INTERNED-SYMBOL",			\
  /* 0x1E */			"CHARACTER-STRING",			\
  /* 0x1F */			"ACCESS",				\
  /* 0x20 */			"HUNK3-A",				\
  /* 0x21 */			"DEFINITION",				\
  /* 0x22 */			"BROKEN-HEART",				\
  /* 0x23 */			"ASSIGNMENT",				\
  /* 0x24 */			"HUNK3-B",				\
  /* 0x25 */			"IN-PACKAGE",				\
  /* 0x26 */			"COMBINATION",				\
  /* 0x27 */			"MANIFEST-NM-VECTOR",			\
  /* 0x28 */			"COMPILED-ENTRY",			\
  /* 0x29 */			"LEXPR",				\
  /* 0x2A */  			"PCOMB3",				\
  /* 0x2B */			"MANIFEST-SPECIAL-NM-VECTOR",		\
  /* 0x2C */			"VARIABLE",				\
  /* 0x2D */			"THE-ENVIRONMENT",			\
  /* 0x2E */			"FUTURE",				\
  /* 0x2F */			"VECTOR-1B",				\
  /* 0x30 */			"PCOMB0",				\
  /* 0x31 */			"VECTOR-16B",				\
  /* 0x32 */			"REFERENCE-TRAP",			\
  /* 0x33 */			"SEQUENCE-3",				\
  /* 0x34 */			"CONDITIONAL",				\
  /* 0x35 */			"DISJUNCTION",				\
  /* 0x36 */			"CELL",					\
  /* 0x37 */			"WEAK-CONS",				\
  /* 0x38 */			"QUAD",					\
  /* 0x39 */			"LINKAGE-SECTION",			\
  /* 0x3A */			"RATNUM",				\
  /* 0x3B */			"STACK-ENVIRONMENT",			\
  /* 0x3C */			"COMPLEX",				\
  /* 0x3D */			"COMPILED-CODE-BLOCK",   		\
  /* 0x3E */                    "",                                     \
  /* 0x3F */                    "",                                     \
  /* 0x40 */                    "G-VECTOR",                             \
  /* 0x41 */                    "IO-ERROR-CODE",                        \
  /* 0x42 */                    "CL-PACKAGE",                           \
  /* 0x43 */                    "CLSAV",                                \
  /* 0x44 */                    "RATIO",                                \
  /* 0x45 */                    "CL-STREAM",                            \
  /* 0x46 */                    "VECTOR-32B",                           \
  /* 0x47 */                    "CL-ARRAY",                             \
  /* 0x48 */                    "CL-IVECTOR"                            \
  }

/* Flags and aliases */

/* Type code 0x10 (used to be TC_PRIMITIVE_EXTERNAL) has been reused. */

#define PRIMITIVE_EXTERNAL_REUSED

/* Aliases */

#define TC_FALSE	        	TC_NULL
#define TC_MANIFEST_VECTOR		TC_NULL
#define GLOBAL_ENV			TC_NULL
#define TC_BIT_STRING			TC_VECTOR_1B
#define TC_VECTOR_8B			TC_CHARACTER_STRING
#define TC_ADDRESS			TC_FIXNUM
#define TC_HUNK3			TC_HUNK3_B

#define UNMARKED_HISTORY_TYPE		TC_HUNK3_A
#define MARKED_HISTORY_TYPE		TC_HUNK3_B
