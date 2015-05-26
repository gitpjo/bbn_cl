/* -*-C-*-

$Header: edwin.h,v 10.0 88/12/07 13:06:56 las Exp $
$MIT-Header: edwin.h,v 1.1 87/05/11 17:47:53 GMT cph Rel $

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

/* Definitions for Edwin data structures.
   This MUST match the definitions in the Edwin source code. */

#define GROUP_P(object) ((pointer_type (object)) == TC_VECTOR)
#define GROUP_TEXT(group) (User_Vector_Ref ((group), 1))
#define GROUP_GAP_START(group) (Get_Integer (User_Vector_Ref ((group), 2)))
#define GROUP_GAP_LENGTH(group) (Get_Integer (User_Vector_Ref ((group), 3)))
#define GROUP_GAP_END(group) (Get_Integer (User_Vector_Ref ((group), 4)))
#define GROUP_START_MARK(group) (User_Vector_Ref ((group), 6))
#define GROUP_END_MARK(group) (User_Vector_Ref ((group), 7))

#define MARK_GROUP(mark) (User_Vector_Ref ((mark), 1))
#define MARK_POSITION(mark) (Get_Integer (User_Vector_Ref ((mark), 2)))
#define MARK_LEFT_INSERTING(mark) ((User_Vector_Ref ((mark), 3)) != NIL)
