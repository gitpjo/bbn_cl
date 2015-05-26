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

/* $Header: fasl.h,v 10.0 88/12/07 13:07:10 las Exp $
   $MIT-Header: fasl.h,v 9.27 88/02/10 04:21:32 GMT jinx Exp $

   Contains information relating to the format of FASL files.
   The machine/opsys information is contained in config.h
   The processor and compiled code version information is
   contained in the appropriate cmp* file, or compiler.c
*/

extern long Load_Data(), Write_Data();
extern Boolean Open_Dump_File(), Close_Dump_File();

/* FASL Version */

#define FASL_FILE_MARKER	0xFAFAFAFA

/* The FASL file has a header which begins as follows: */

#define FASL_HEADER_LENGTH	50	/* Scheme objects in header */

#define FASL_Offset_Marker	0	/* Marker to indicate FASL format */
#define FASL_Offset_Heap_Count	1	/* Count of objects in heap */
#define FASL_Offset_Heap_Base	2	/* Address of heap when dumped */
#define FASL_Offset_Dumped_Obj	3	/* Where dumped object was */
#define FASL_Offset_Const_Count	4	/* Count of objects in const. area */
#define FASL_Offset_Const_Base	5	/* Address of const. area at dump */
#define FASL_Offset_Version	6	/* FASL format version info. */ 
#define FASL_Offset_Stack_Top	7	/* Top of stack when dumped */
#define FASL_Offset_Prim_Length 8	/* Number of entries in primitive table */
#define FASL_Offset_Prim_Size	9	/* Size of primitive table in Pointers */
#define FASL_Offset_Ci_Version	10	/* Version number for compiled code interface */
#define FASL_Offset_Ut_Base	11	/* Address of the utilities vector */
#define FASL_Offset_Code_Count	12	/* Count of objects in code space */
#define FASL_Offset_Code_Base	13 	/* Address of code space when dumped */

#define FASL_Offset_First_Free	14	/* Used to clear header */

/* Aliases for backwards compatibility. */

/* Where ext. prims. vector is */
#define FASL_Offset_Ext_Loc	FASL_Offset_Prim_Length

/* Version information encoding */

#define MACHINE_TYPE_LENGTH	(POINTER_LENGTH / 2)
#define MACHINE_TYPE_MASK	((1 << MACHINE_TYPE_LENGTH) - 1)
#define The_Machine_Type(P)	((P) & MACHINE_TYPE_MASK)
#define SUBVERSION_LENGTH	(MACHINE_TYPE_LENGTH - TYPE_CODE_LENGTH)
#define SUBVERSION_MASK		((1 << SUBVERSION_LENGTH) - 1)
#define The_Sub_Version(P)	(((P) >> MACHINE_TYPE_LENGTH) & SUBVERSION_MASK)
#define The_Version(P)		OBJECT_TYPE(P)
#define Make_Version(V, S, M)					\
  Make_Non_Pointer((V), (((S) << MACHINE_TYPE_LENGTH) | (M)))

#define CI_MASK			((1 << (ADDRESS_LENGTH / 2)) - 1)
#define CI_VERSION(P)		(((P) >> (ADDRESS_LENGTH / 2)) & CI_MASK)
#define CI_PROCESSOR(P)		((P) & CI_MASK)
#define CI_BAND_P(P)		(OBJECT_TYPE(P) == TC_TRUE)
#define MAKE_CI_VERSION(Band_p, Version, Processor_Type)	\
  Make_Non_Pointer(((Band_p) ? TC_TRUE : TC_NULL),		\
		   (((Version) << (ADDRESS_LENGTH / 2)) |	\
		    (Processor_Type)))

#define WRITE_FLAG		"w"
#define OPEN_FLAG		"r"


/* "Memorable" FASL versions -- ones where we modified something
   and want to remain backwards compatible.
*/

/* Versions. */

#define FASL_FORMAT_ADDED_STACK	1

/* Subversions of highest numbered version. */

#define FASL_LONG_HEADER	3
#define FASL_DENSE_TYPES	4
#define FASL_PADDED_STRINGS	5
#define FASL_REFERENCE_TRAP	6
#define FASL_MERGED_PRIMITIVES	7
#define FASL_INTERFACE_VERSION	8
#define FASL_CODE_SPACE		9

/* Current parameters.  Always used on output. */

#define FASL_FORMAT_VERSION	FASL_FORMAT_ADDED_STACK
#define FASL_SUBVERSION		FASL_CODE_SPACE

/*
  The definitions below correspond to the ones above.  They usually
  have the same values.  They differ when the format is changing: A
  system can be built which reads the old format, but dumps the new one.
 */

#ifndef FASL_READ_VERSION
#define FASL_READ_VERSION	FASL_FORMAT_VERSION
#endif

#ifndef FASL_READ_SUBVERSION
#define FASL_READ_SUBVERSION	FASL_SUBVERSION
#endif

/* These are for Bintopsb.
   They are the values of the oldest supported formats.
 */

#define FASL_OLDEST_VERSION	FASL_FORMAT_ADDED_STACK
#define FASL_OLDEST_SUBVERSION	FASL_PADDED_STRINGS
