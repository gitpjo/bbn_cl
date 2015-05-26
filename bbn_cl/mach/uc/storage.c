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

/* $Header: storage.c,v 10.0 88/12/07 13:10:52 las Exp $
   $MIT-Header: storage.c,v 9.42 88/02/12 16:52:28 GMT jinx Exp $

   This file defines the storage for global variables for
   the Scheme Interpreter. 
*/

#include "scheme.h"
#include "gctype.c"

                         /*************/
                         /* REGISTERS */
                         /*************/

Pointer
 *Ext_History,		/* History register */
 *Free,			/* Next free word in storage */
 *MemTop,		/* Top of free space available */
 *Ext_Stack_Pointer,	/* Next available slot in control stack */
 *Stack_Top,		/* Top of control stack */
 *Stack_Guard,		/* Guard area at end of stack */
 *Free_Stacklets,	/* Free list of stacklets */
#ifndef butterfly
 *Constant_Space,	/* Bottom of constant+pure space */
 *Free_Constant,	/* Next free cell in constant+pure area */
 *Constant_Top,		/* Top of constant+pure space */
#endif
 *Heap_Top,		/* Top of current heap */
 *Heap_Bottom,		/* Bottom of current heap */
 *Unused_Heap_Top,	/* Top of other heap */
 *Unused_Heap,		/* Bottom of other heap */
 *Local_Heap_Base,	/* Per-processor CONSing area */
 *Heap,			/* Bottom of entire heap */
  Current_State_Point = NIL, /* Used by dynamic winder */
  Fluid_Bindings = NIL,	/* Fluid bindings AList */
 *last_return_code,	/* Address of the most recent return code in the stack.
			   This is only meaningful while in compiled code.
			   *** This must be changed when stacklets are used. ***
			 */
 Swap_Temp;		/* Used by Swap_Pointers in default.h */

long
  IntCode,		/* Interrupts requesting */
  IntEnb,		/* Interrupts enabled */
  temp_long,		/* temporary for sign extension */
  GC_Reserve = 4500,	/* Scheme pointer overflow space in heap */
  GC_Space_Needed = 0, /* Amount of space needed when GC triggered */
  /* Used to signal microcode errors from compiled code. */
  compiled_code_error_code;

Declare_Fixed_Objects();

#ifdef butterfly
FILE *Photo_File_Handle;
#else
FILE *(Channels[FILE_CHANNELS]), *Photo_File_Handle;
#endif

int Saved_argc;
char **Saved_argv;
char *OS_Name, *OS_Variant;

Boolean Photo_Open = false; /* Photo file open */

Boolean Trapping;

Pointer Old_Return_Code, *Return_Hook_Address;

Pointer *Prev_Restore_History_Stacklet;
long Prev_Restore_History_Offset;

jmp_buf *Back_To_Eval; /* Buffer for set/longjmp */

long Heap_Size, Constant_Size, Stack_Size;
Pointer *Highest_Allocated_Address;

#ifndef Heap_In_Low_Memory

Pointer *Memory_Base;

#endif

                    /**********************/
                    /* DEBUGGING SWITCHES */
                    /**********************/

#ifdef ENABLE_DEBUGGING_TOOLS

Boolean Eval_Debug	= false;
Boolean Hex_Input_Debug	= false;
Boolean File_Load_Debug	= false;
Boolean Reloc_Debug	= false;	
Boolean Intern_Debug	= false;
Boolean Cont_Debug	= false;
Boolean Primitive_Debug	= false;
Boolean Lookup_Debug	= false;
Boolean Define_Debug	= false;
Boolean GC_Debug	= false;
Boolean Upgrade_Debug	= false;
Boolean Dump_Debug	= false;
Boolean Trace_On_Error	= false;
Boolean Bignum_Debug    = false;
Boolean Per_File	= true;
Boolean Fluids_Debug	= false;
More_Debug_Flag_Allocs();

int debug_slotno = 0;
int debug_nslots = 0;
int local_slotno = 0;
int local_nslots = 0;

#if false /* MHWU */
int debug_circle[debug_maxslots];
int local_circle[debug_maxslots];
#endif /* false */

int debug_circle[100];
int local_circle[100];
#endif /* ENABLE_DEBUGGING_TOOLS */

		/****************************/
		/* Debugging Macro Messages */
		/****************************/

char *CONT_PRINT_RETURN_MESSAGE =   "Save_Cont, return code";
char *CONT_PRINT_EXPR_MESSAGE   =   "Save_Cont, expression";
char *RESTORE_CONT_RETURN_MESSAGE = "Restore_Cont, return code";
char *RESTORE_CONT_EXPR_MESSAGE =   "Restore_Cont, expression";

/* Interpreter code name and message tables */

long MAX_RETURN = MAX_RETURN_CODE;

extern char *Return_Names[];
char *Return_Names[] = RETURN_NAME_TABLE;	/* in returns.h */

extern char *Type_Names[];
char *Type_Names[] = TYPE_NAME_TABLE;		/* in types.h */

extern char *Abort_Names[];
char *Abort_Names[] = ABORT_NAME_TABLE;		/* in const.h */

extern char *Error_Names[];
char *Error_Names[] = ERROR_NAME_TABLE;		/* in errors.h */

extern char *Term_Names[];
char *Term_Names[] = TERM_NAME_TABLE;		/* in errors.h */

extern char *Term_Messages[];
char *Term_Messages[] = TERM_MESSAGE_TABLE;	/* in errors.h */
