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

/* $Header: dump.c,v 10.0 88/12/07 13:06:52 las Exp $
 * $MIT-Header: dump.c,v 9.26 88/02/06 20:39:50 GMT jinx Exp $
 *
 * This file contains common code for dumping internal format binary files.
 */

extern int Debug_Flags[];

extern Pointer compiler_utilities;
extern long compiler_interface_version, compiler_processor_type;

void
prepare_dump_header(Buffer, Dumped_Object,
		    Heap_Count, Heap_Relocation,
		    Constant_Count, Constant_Relocation,
		    Code_Count, Code_Relocation,
		    table_length, table_size,
		    cc_code_p, band_p)
     Pointer
       *Buffer, *Dumped_Object,
       *Heap_Relocation, *Constant_Relocation, *Code_Relocation;
     long
       Heap_Count, Constant_Count, Code_Count,
       table_length, table_size;
     Boolean cc_code_p, band_p;
{
  long i;

#ifdef DEBUG

#ifndef Heap_In_Low_Memory
  fprintf(stderr, "\nMemory_Base = 0x%x\n", Memory_Base);
#endif /* Heap_In_Low_Memory */

  fprintf(stderr, "\nHeap_Relocation=0x%x, dumped as 0x%x\n",
	  Heap_Relocation, Make_Pointer(TC_BROKEN_HEART, Heap_Relocation));
  fprintf(stderr, "\nDumped object=0x%x, dumped as 0x%x\n",
	  Dumped_Object, Make_Pointer(TC_BROKEN_HEART, Dumped_Object));
#endif /* DEBUG */

  Buffer[FASL_Offset_Marker] = FASL_FILE_MARKER;
  Buffer[FASL_Offset_Heap_Count] =
    Make_Non_Pointer(TC_BROKEN_HEART, Heap_Count);
  Buffer[FASL_Offset_Heap_Base] =
    Make_Pointer(TC_BROKEN_HEART, Heap_Relocation);
  Buffer[FASL_Offset_Dumped_Obj] =
    Make_Pointer(TC_BROKEN_HEART, Dumped_Object);
  Buffer[FASL_Offset_Const_Count] =
    Make_Non_Pointer(TC_BROKEN_HEART, Constant_Count);
  Buffer[FASL_Offset_Const_Base] =
    Make_Pointer(TC_BROKEN_HEART, Constant_Relocation);
  Buffer[FASL_Offset_Version] =
    Make_Version(FASL_FORMAT_VERSION,
		 FASL_SUBVERSION, FASL_INTERNAL_FORMAT);
  Buffer[FASL_Offset_Stack_Top] =

#ifdef USE_STACKLETS
    Make_Pointer(TC_BROKEN_HEART, 0);	/* Nothing in stack area */
#else
    Make_Pointer(TC_BROKEN_HEART, Stack_Top);
#endif /* USE_STACKLETS */

  Buffer[FASL_Offset_Prim_Length] = 
    Make_Non_Pointer(TC_BROKEN_HEART, table_length);
  Buffer[FASL_Offset_Prim_Size] = 
    Make_Non_Pointer(TC_BROKEN_HEART, table_size);

  if (cc_code_p)
  {
    Buffer[FASL_Offset_Ci_Version] =
      MAKE_CI_VERSION(band_p,
		      compiler_interface_version,
		      compiler_processor_type);
    Buffer[FASL_Offset_Ut_Base] = compiler_utilities;
  }
  else
  {
    /* If there is no compiled code in the file,
       flag it as if dumped without compiler support, so
       it can be loaded anywhere.
     */
    Buffer[FASL_Offset_Ci_Version] = MAKE_CI_VERSION(band_p, 0, 0);
    Buffer[FASL_Offset_Ut_Base] = NIL;
  }
  Buffer[FASL_Offset_Code_Count] =
    Make_Non_Pointer(TC_BROKEN_HEART, Code_Count);
  Buffer[FASL_Offset_Code_Base] =
    Make_Pointer(TC_BROKEN_HEART, Code_Relocation);
  for (i = FASL_Offset_First_Free; i < FASL_HEADER_LENGTH; i++)
  {
    Buffer[i] = NIL;
  }
  return;
}

Boolean
Write_File(Dumped_Object, Heap_Count, Heap_Data, Heap_Relocation,
           Constant_Count, Constant_Relocation,
	   Code_Count, Code_Relocation,
	   table_start, table_length, table_size,
	   cc_code_p, band_p)
     Pointer
       *Dumped_Object,
       *Heap_Relocation, *Constant_Relocation, *Code_Relocation,
       *table_start;
     long
       Heap_Count, Constant_Count, Code_Count,
       table_length, table_size;
     Boolean cc_code_p, band_p;
{
  Pointer Buffer[FASL_HEADER_LENGTH];

  prepare_dump_header(Buffer, Dumped_Object,
		      Heap_Count, Heap_Relocation,
		      Constant_Count, Constant_Relocation,
		      Code_Count, Code_Relocation,
		      table_length, table_size, cc_code_p, band_p);

  if (Debug_Flags[13])
    printf("Writing header %x for %x\n", Buffer, FASL_HEADER_LENGTH);
  if (Write_Data(FASL_HEADER_LENGTH, ((char *) Buffer)) !=
      FASL_HEADER_LENGTH)
  {
    if (Debug_Flags[13])
      printf("Unable to write fasl header\n");
    return (false);
  }
  if (Debug_Flags[13])
    printf("Writing heap %x for %x\n", Heap_Data, Heap_Count);
  if (Heap_Count != 0)
  {
    if (Write_Data(Heap_Count, ((char *) Heap_Data)) !=
	Heap_Count)
    {
      if (Debug_Flags[13])
	printf("Unable to write %d pointers of heap\n", Heap_Count);
      return (false);
    }
  }
  if (Debug_Flags[13])
    printf("Writing pure/constant %x for %x\n", Constant_Relocation, Constant_Count);
  if (Constant_Count != 0)
  {
    if (Write_Data(Constant_Count, ((char *) Constant_Relocation)) !=
	Constant_Count)
    {
      if (Debug_Flags[13])
	printf("Unable to write %d pointers of constant\n", Constant_Count);
      return (false);
    }
  }
  if (Debug_Flags[13])
    printf("Writing code %x for %x\n", Code_Relocation, Code_Count);
  if (Code_Count != 0)
  {
    if (Write_Data(Code_Count, ((char *) Code_Relocation)) !=
	Code_Count) 
      { if (Debug_Flags[13])
	  printf("Unable to write %d pointers of code\n", Code_Count);
	return false;
      }
  }
  if (Debug_Flags[13])
    printf("table_size = %x\n", table_size);
  if (table_size != 0)
  {
    if (Debug_Flags[13]) 
      printf("Writing table at %x for %x\n", table_start, table_size);
    if (Write_Data(table_size, ((char *) table_start)) != table_size)
    {
      if (Debug_Flags[13])
	printf("Unable to write %d pointers of table\n", table_size);
      return (false);
    }
  }
  return (true);
}
