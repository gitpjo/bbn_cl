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

/* $Header: future.c,v 10.0 88/12/07 13:07:40 las Exp $
   $MIT-Header: future.c,v 9.25 87/11/17 08:11:25 GMT jinx Exp $

   Support code for futures
*/

#include "scheme.h"
#include "primitive.h"
#include "locks.h"

#ifndef COMPILE_FUTURES
#include "Error: future.c is useless without COMPILE_FUTURES"
#endif

/* This is how we support future numbering for external metering */
#ifndef New_Future_Number
#define New_Future_Number() NIL
#else
Pointer Get_New_Future_Number();
#endif

/*

A future is a VECTOR starting with <determined?>, <locked?> and 
<waiting queue / value>,

where <determined?> is #!false if no value is known yet,
                       #!true if value is known and future can vanish at GC,
                       otherwise value is known, but keep the slot

and where <locked> is #!true if someone wants slot kept for a time.

*/

Define_Primitive(Prim_Touch, 1, "TOUCH")
{
  Pointer Result;
  Primitive_1_Arg();

  Touch_In_Primitive(Arg1, Result);
  return Result;
}

Define_Primitive(Prim_Future_P, 1, "FUTURE?")
{
  Primitive_1_Arg();

  return (Type_Code(Arg1) == TC_FUTURE) ? TRUTH : NIL;
}

/* Utility setting routine for use by the various test and set if
   equal operators.
*/

long
Set_If_Equal(Base, Offset, New, Wanted)
     Pointer Base, Wanted, New;
     long Offset;
{
  Lock_Handle lock;
  Pointer Old_Value, Desired, Remember_Value;
  long success;

  Touch_In_Primitive(Wanted, Desired);
Try_Again:
  Remember_Value = Vector_Ref(Base, Offset);
  Touch_In_Primitive(Remember_Value, Old_Value);
  lock = Lock_Cell(Nth_Vector_Loc(Base, Offset));
  if (Remember_Value != Fast_Vector_Ref(Base, Offset))
  {
    Unlock_Cell(lock);
    goto Try_Again;
  }
  if (Old_Value == Desired)
  {
    Do_Store_No_Lock(Nth_Vector_Loc(Base, Offset), New);
    success = true;
  }
  else
  {
    success = false;
  }
  Unlock_Cell(lock);
  return success;
}

/* (SET-CAR-IF-EQ?! <CONS Cell> <New Value> <Old Value>)
   Replaces the CAR of <CONS Cell> with <New Value> if it used to contain
   <Old Value>.  The value returned is either <CONS Cell> (if the modification
   takes place) or '() if it does not.
*/
Define_Primitive(Prim_Set_Car_If_Eq, 3, "SET-CAR-IF-EQ?!")
{
  Primitive_3_Args();

  Arg_1_Type(TC_LIST);
  if (Set_If_Equal(Arg1, CONS_CAR, Arg2, Arg3))
  {
    return Arg1;
  }
  else
  {
    return NIL;
  }
}
  
/* (SET-CDR-IF-EQ?! <CONS Cell> <New Value> <Old Value>)
   Replaces the CDR of <CONS Cell> with <New Value> if it used to contain
   <Old Value>.  The value returned is either <CONS Cell> (if the modification
   takes place) or '() if it does not.
*/
Define_Primitive(Prim_Set_Cdr_If_Eq, 3, "SET-CDR-IF-EQ?!")
{
  Primitive_3_Args();
  Arg_1_Type(TC_LIST);

  if (Set_If_Equal(Arg1, CONS_CDR, Arg2, Arg3))
  {
    return Arg1;
  }
  else
  {
    return NIL;
  }
}

/* (VECTOR-SET-IF-EQ?! <Vector> <Offset> <New Value> <Old Value>)
   Replaces the <Offset>th element of <Vector> with <New Value> if it used
   to contain <Old Value>.  The value returned is either <Vector> (if
   the modification takes place) or '() if it does not.
*/
Define_Primitive(Prim_Vector_Set_If_Eq, 4, "VECTOR-SET-IF-EQ?!")
{
  long Offset;
  Primitive_4_Args();

  Arg_1_Type(TC_VECTOR);
  Arg_2_Type(TC_FIXNUM);
  Range_Check(Offset, Arg2,
              0, Vector_Length(Arg1)-1, ERR_ARG_2_BAD_RANGE);
  if (Set_If_Equal(Arg1, Offset+1, Arg3, Arg4)) return Arg1;
  else return NIL;
}

/* (SET-CXR-IF-EQ?! <Triple> <Offset> <New Value> <Old Value>)
   Replaces the <Offset>th CXR of <Triple> with <New Value> if it used to
   contain <Old Value>.  The value returned is either <Triple> (if
   the modification takes place) or '() if it does not.
*/
Define_Primitive(Prim_Set_Cxr_If_Eq, 4, "SET-CXR-IF-EQ?!")
{
  Pointer Arg4;
  long Offset;
  Primitive_3_Args();

  Arg4 = Stack_Ref(3);
  Arg_1_Type(TC_HUNK3);
  Arg_2_Type(TC_FIXNUM);
  Range_Check(Offset, Arg2, 0, 2, ERR_ARG_2_BAD_RANGE);
  if (Set_If_Equal(Arg1, Offset, Arg3, Arg4))
  {
    return Arg1;
  }
  else
  {
    return NIL;
  }
}

/* (FUTURE-REF <Future> <Offset>)
   Returns the <Offset>th slot from the future object.  This is
   the equivalent of SYSTEM-VECTOR-REF but works only on future
   objects and doesn't touch.
*/
Define_Primitive(Prim_Future_Ref, 2, "FUTURE-REF")
{
  long Offset;
  Primitive_2_Args();

  Arg_1_Type(TC_FUTURE);
  Arg_2_Type(TC_FIXNUM);
  Range_Check(Offset, Arg2,
              0, Vector_Length(Arg1)-1, ERR_ARG_2_BAD_RANGE);
  return User_Vector_Ref(Arg1, Offset);
}

/* (FUTURE-SET! <Future> <Offset> <New Value>)
   Modifies the <Offset>th slot from the future object.  This is
   the equivalent of SYSTEM-VECTOR-SET! but works only on future
   objects and doesn't touch.
*/
Define_Primitive(Prim_Future_Set, 3, "FUTURE-SET!")
{
  long Offset;
  Pointer Result;
  Primitive_3_Args();

  Arg_1_Type(TC_FUTURE);
  Arg_2_Type(TC_FIXNUM);
  Range_Check(Offset, Arg2,
              0, Vector_Length(Arg1)-1, ERR_ARG_2_BAD_RANGE);
  Result = User_Vector_Ref(Arg1, Offset);
  User_Vector_Set(Arg1, Offset,Arg3);
  return Result;
}

/* (FUTURE-SIZE <Future>)
   Returns the number of slots in the future object.  This is
   the equivalent of SYSTEM-VECTOR-SIZE but works only on future
   objects and doesn't touch.
*/
Define_Primitive(Prim_Future_Size, 1, "FUTURE-SIZE")
{
  Primitive_1_Arg();

  Arg_1_Type(TC_FUTURE);
  return Make_Unsigned_Fixnum(Vector_Length(Arg1));
}

/* (LOCK-FUTURE! <Future>)
   Sets the lock flag on the future object, so that it won't be 
   spliced-out by the garbage collector. Returns #!false if the
   argument isn't a future (might have been determined in the
   interim), #!TRUE if it is a future.  Hangs as long as necessary
   for the lock to take, since Scheme code operates while locked.
   Opposite of UNLOCK-FUTURE!.
*/

Define_Primitive(Prim_Lock_Future, 1, "LOCK-FUTURE!")
{
  Primitive_1_Arg();

  return Lock_The_Future(Arg1) ? TRUTH : NIL; }


/* (UNLOCK-FUTURE! <Future>)
   Clears the lock flag on a locked future object, otherwise nothing.
*/

Define_Primitive(Prim_Unlock_Future, 1, "UNLOCK-FUTURE!")
{
  Primitive_1_Arg();

  return Unlock_The_Future(Arg1) ? TRUTH : NIL; }

/* (FUTURE->VECTOR <Future>)
   Create a COPY of <future> but with type code vector.
*/
Define_Primitive(Prim_Future_To_Vector, 1, "FUTURE->VECTOR")
{
  Pointer Result;
  long Size, i;
  Primitive_1_Arg();

  Result = Make_Pointer(TC_VECTOR, Free);
  if (Type_Code(Arg1) != TC_FUTURE)
    return NIL;

  Size = Vector_Length(Arg1);
  Primitive_GC_If_Needed(Size + 1);
  for (i = 0; i <= Size; i++)
    *Free++ = Vector_Ref(Arg1, i);

  return Result; }

Define_Primitive(Prim_Future_Eq, 2, "NON-TOUCHING-EQ?")
{
  Primitive_2_Args();

  return ((Arg1==Arg2) ? TRUTH : NIL); }

/* MAKE-INITIAL-PROCESS is called to create a small stacklet which
 * will just call the specified thunk and then end the computation
 */

Define_Primitive(Prim_Make_Initial_Process, 1, "MAKE-INITIAL-PROCESS")
{
  Pointer Result;
  long Useful_Length;
  Primitive_1_Arg();

  Result = Make_Pointer(TC_CONTROL_POINT, Free);
  Useful_Length = (3 * CONTINUATION_SIZE) + STACK_ENV_EXTRA_SLOTS + 1;

#ifdef USE_STACKLETS

{
  long Allocated_Length, Waste_Length;

  Allocated_Length = (Useful_Length + STACKLET_SLACK + STACKLET_HEADER_SIZE);
  if (Allocated_Length < Default_Stacklet_Size)
  {
    Allocated_Length = Default_Stacklet_Size;
    Waste_Length = ((Allocated_Length + 1) -
		    (Useful_Length + STACKLET_HEADER_SIZE));
  }
  else
  {
    Waste_Length = (STACKLET_SLACK + 1);
  }
  Primitive_GC_If_Needed(Allocated_Length + 1);
  Free[STACKLET_LENGTH] =
    Make_Pointer(TC_MANIFEST_VECTOR, Allocated_Length);
  Free[STACKLET_REUSE_FLAG] = TRUTH;
  Free[STACKLET_UNUSED_LENGTH] =
    Make_Non_Pointer(TC_MANIFEST_NM_VECTOR, Waste_Length);
  Free += (Allocated_Length + 1) - Useful_Length;
}

#else /* not USE_STACKLETS */

  Free[STACKLET_LENGTH] =
    Make_Non_Pointer(TC_MANIFEST_VECTOR,
	     	     Useful_Length + STACKLET_HEADER_SIZE - 1);
  Free[STACKLET_REUSE_FLAG] = NIL;
  Free[STACKLET_UNUSED_LENGTH] =
    Make_Non_Pointer(TC_MANIFEST_NM_VECTOR, 0);
  Free += STACKLET_HEADER_SIZE;

#endif /* USE_STACKLETS */

  Free[CONTINUATION_EXPRESSION] = MAKE_SIGNED_FIXNUM(FETCH_INTERRUPT_MASK());
  Free[CONTINUATION_RETURN_CODE] = 
    Make_Non_Pointer(TC_RETURN_CODE, RC_RESTORE_INT_MASK);
  Free += CONTINUATION_SIZE;
  Free[CONTINUATION_EXPRESSION] = NIL;
  Free[CONTINUATION_RETURN_CODE] = 
    Make_Non_Pointer(TC_RETURN_CODE, RC_INTERNAL_APPLY);
  Free += CONTINUATION_SIZE;
  *Free++ = STACK_FRAME_HEADER;
  *Free++ = Arg1;
  Free[CONTINUATION_EXPRESSION] = Arg1;	/* For testing & debugging */
  Free[CONTINUATION_RETURN_CODE] = 
    Make_Non_Pointer(TC_RETURN_CODE, RC_END_OF_COMPUTATION);
  Free += CONTINUATION_SIZE;
  return Result;
}

