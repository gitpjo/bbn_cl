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

/* $Header: bchmmg.c,v 10.0 88/12/07 13:03:34 las Exp $
 * $MIT-Header: bchmmg.c,v 9.41 88/03/21 21:09:57 GMT jinx Rel $ 
 */

/* Memory management top level.  Garbage collection to disk.

   The algorithm is basically the same as for the 2 space collector,
   except that new space is on the disk, and there are two windows to
   it (the scan and free buffers).  The two windows are physically the
   same whent hey correspond to the same section of the disk.

   For information on the 2 space collector, read the comments in the
   replaced files.

   The memory management code is spread over 3 files:
   - bchmmg.c: initialization and top level.  Replaces memmag.c
   - bchgcl.c: main garbage collector loop.   Replaces gcloop.c
   - bchpur.c: constant/pure space hacking.   Replaces purify.c
   - bchdmp.c: object world image dumping.    Replaces fasdump.c

   Problems with this implementation right now:
   - Purify kills Scheme if there is not enough space in constant space
     for the new object.
   - Floating alignment is not implemented.
   - It only works on Unix (or systems which support Unix i/o calls).
   - Dumpworld cannot work because the file is not closed at dump time or
     reopened at restart time.
   - Command line supplied gc files are not locked, so two processes can try
     to share them and get very confused.
*/

#include "scheme.h"
#include "primitive.h"
#include "bchgcc.h"

/* Exports */

extern void Clear_Memory(), Setup_Memory(), Reset_Memory();

/* 	Memory Allocation, sequential processor,
	garbage collection to disk version:

   ------------------------------------------
   |        GC Buffer Space                 |
   |                                        |
   ------------------------------------------
   |         Control Stack        ||        |
   |                              \/        |
   ------------------------------------------
   |     Constant + Pure Space    /\        |
   |                              ||        |
   ------------------------------------------
   |          Heap Space                    |
   |                                        |
   ------------------------------------------

   Each area has a pointer to its starting address and a pointer to
   the next free cell.  The GC buffer space contains two equal size
   buffers used during the garbage collection process.  Usually one is
   the scan buffer and the other is the free buffer, and they are
   dumped and loaded from disk as necessary.  Sometimes during the
   garbage collection (especially at the beginning and at the end)
   both buffers are identical, since transporting will occur into the
   area being scanned.
*/

/* Local declarations */

static long scan_position, free_position;
static Pointer *gc_disk_buffer_1, *gc_disk_buffer_2;
Pointer *scan_buffer_top, *scan_buffer_bottom;
Pointer *free_buffer_top, *free_buffer_bottom;

static Boolean extension_overlap_p;
static long extension_overlap_length;

/* Hacking the gc file */

extern char *mktemp();

int gc_file;
static long current_disk_position;
static char *gc_file_name;
static char gc_default_file_name[FILE_NAME_LENGTH] = GC_DEFAULT_FILE_NAME;

void
open_gc_file(size)
     int size;
{
  int position;
  int flags;

  (void) mktemp(gc_default_file_name);
  flags = GC_FILE_FLAGS;

  position = Parse_Option("-gcfile", Saved_argc, Saved_argv, true);
  if ((position != NOT_THERE) &&
      (position != (Saved_argc - 1)))
  {
    gc_file_name = Saved_argv[position + 1];
  }
  else
  {
    gc_file_name = gc_default_file_name;
    flags |= O_EXCL;
  }

  while(true)
  {
    gc_file = open(gc_file_name, flags, GC_FILE_MASK);
    if (gc_file != -1)
    {
      break;
    }
    if (gc_file_name != gc_default_file_name)
    {
      fprintf(stderr,
	      "%s: GC file \"%s\" cannot be opened; ",
	      Saved_argv[0]), gc_file_name;
      gc_file_name = gc_default_file_name;
      fprintf(stderr,
	      "Using \"%s\" instead.\n",
	      gc_file_name);
      flags |= O_EXCL;
      continue;
    }
    fprintf(stderr,
	    "%s: GC file \"%s\" cannot be opened; ",
	    Saved_argv[0]), gc_file_name;
    fprintf(stderr, "Aborting.\n");
    exit(1);
  }
#ifdef hpux
  if (gc_file_name == gc_default_file_name)
  {
    extern prealloc();
    prealloc(gc_file, size);
  }
#endif
  current_disk_position = 0;
  return;
}

void
close_gc_file()
{
  if (close(gc_file) == -1)
  {
    fprintf(stderr,
	    "%s: Problems closing GC file \"%s\".\n",
	    Saved_argv[0], gc_file_name);
  }
  if (gc_file_name == gc_default_file_name)
  {
    unlink(gc_file_name);
  }
  return;
}

void 
Clear_Memory (Our_Heap_Size, Our_Stack_Size, Our_Constant_Size)
     int Our_Heap_Size, Our_Stack_Size, Our_Constant_Size;
{
  Heap_Top = (Heap_Bottom + Our_Heap_Size);
  SET_MEMTOP(Heap_Top - GC_Reserve);
  Free = Heap_Bottom;
  Constant_Top = (Constant_Space + Our_Constant_Size);
  Free_Constant = Constant_Space;
  Set_Pure_Top ();
  Initialize_Stack ();
  return;
}

void
Setup_Memory(Our_Heap_Size, Our_Stack_Size, Our_Constant_Size)
     int Our_Heap_Size, Our_Stack_Size, Our_Constant_Size;
{
  int Real_Stack_Size;

  Real_Stack_Size = Stack_Allocation_Size(Our_Stack_Size);

  /* Consistency check 1 */
  if (Our_Heap_Size == 0)
  {
    fprintf(stderr, "Configuration won't hold initial data.\n");
    exit(1);
  }

  /* Allocate.
     The two GC buffers are not included in the valid Scheme memory.
  */
  Highest_Allocated_Address = 
    Allocate_Heap_Space(Real_Stack_Size + Our_Heap_Size +
			Our_Constant_Size + (2 * GC_BUFFER_SPACE) +
			HEAP_BUFFER_SPACE);

  /* Consistency check 2 */
  if (Heap == NULL)
  {
    fprintf(stderr, "Not enough memory for this configuration.\n");
    exit(1);
  }

  /* Trim the system buffer space. */

  Highest_Allocated_Address -= (2 * GC_BUFFER_SPACE);
  Heap += HEAP_BUFFER_SPACE;
  Initial_Align_Float(Heap);

  Constant_Space = Heap + Our_Heap_Size;
  gc_disk_buffer_1 = Constant_Space + Our_Constant_Size + Real_Stack_Size;
  gc_disk_buffer_2 = (gc_disk_buffer_1 + GC_BUFFER_SPACE);

  /* Consistency check 3 */
  if (((C_To_Scheme(Highest_Allocated_Address)) & TYPE_CODE_MASK) != 0)
  {
    fprintf(stderr,
	    "Largest address does not fit in datum field of Pointer.\n");
    fprintf(stderr,
	    "Allocate less space or re-compile without Heap_In_Low_Memory.\n");
    exit(1);
  }

  Heap_Bottom = Heap;
  Clear_Memory(Our_Heap_Size, Our_Stack_Size, Our_Constant_Size);

  open_gc_file(Our_Heap_Size * sizeof(Pointer));
  return;
}

void
Reset_Memory()
{
  close_gc_file();
  return;
}

void
dump_buffer(from, position, nbuffers, name, success)
     Pointer *from;
     long *position, nbuffers;
     char *name;
     Boolean *success;
{
  long bytes_written;

  if ((current_disk_position != *position) &&
      (lseek(gc_file, *position, 0) == -1))
  {
    if (success == NULL)
    {
      fprintf(stderr,
	      "\nCould not position GC file to write the %s buffer.\n",
	      name);
      Microcode_Termination(TERM_EXIT);
      /*NOTREACHED*/
    }
    *success = false;
    return;
  }
  if ((bytes_written = write(gc_file, from, (nbuffers * GC_BUFFER_BYTES))) ==
      -1)
  {
    if (success == NULL)
    {
      fprintf(stderr, "\nCould not write out the %s buffer.\n", name);
      Microcode_Termination(TERM_EXIT);
      /*NOTREACHED*/
    }
    *success = false;
    return;
  }

  *position += bytes_written;
  current_disk_position = *position;
  return;
}

void
load_buffer(position, to, nbytes, name)
     long position;
     Pointer *to;
     long nbytes;
     char *name;
{
  long bytes_read;

  if (current_disk_position != position)
  {
    if (lseek(gc_file, position, 0) == -1)
    {
      fprintf(stderr, "\nCould not position GC file to read %s.\n", name);
      Microcode_Termination(TERM_EXIT);
      /*NOTREACHED*/
    }
    current_disk_position = position;
  }
  if ((bytes_read = read(gc_file, to, nbytes)) != nbytes)
  {
    fprintf(stderr, "\nCould not read into %s.\n", name);
    Microcode_Termination(TERM_EXIT);
    /*NOTREACHED*/
  }
  current_disk_position += bytes_read;
  return;
}

void
reload_scan_buffer()
{
  if (scan_position == free_position)
  {
    scan_buffer_bottom = free_buffer_bottom;
    scan_buffer_top = free_buffer_top;
    return;
  }
  load_buffer(scan_position, scan_buffer_bottom,
	      GC_BUFFER_BYTES, "the scan buffer");
  *scan_buffer_top = Make_Pointer(TC_BROKEN_HEART, scan_buffer_top);
  return;
}

Pointer *
initialize_scan_buffer()
{
  scan_position = 0;
  scan_buffer_bottom = ((free_buffer_bottom == gc_disk_buffer_1) ?
			gc_disk_buffer_2 :
			gc_disk_buffer_1);
  scan_buffer_top = scan_buffer_bottom + GC_DISK_BUFFER_SIZE;
  reload_scan_buffer();
  return (scan_buffer_bottom);
}

/* This hacks the scan buffer also so that Scan is always below
   scan_buffer_top until the scan buffer is initialized.
   Various parts of the garbage collector depend on scan_buffer_top
   always pointing to a valid buffer.
*/
Pointer *
initialize_free_buffer()
{
  free_position = 0;
  free_buffer_bottom = gc_disk_buffer_1;
  free_buffer_top = free_buffer_bottom + GC_DISK_BUFFER_SIZE;
  extension_overlap_p = false;
  scan_position = -1;
  scan_buffer_bottom = gc_disk_buffer_2;
  scan_buffer_top = scan_buffer_bottom + GC_DISK_BUFFER_SIZE;
  return (free_buffer_bottom);
}

void
end_transport(success)
     Boolean *success;
{
  dump_buffer(scan_buffer_bottom, &scan_position, 1, "scan", success);
  free_position = scan_position;
  return;
}

/* These utilities are needed when pointers fall accross window boundaries.

   Between both they effectively do a dump_and_reload_scan_buffer, in two
   stages.

   Having bcopy would be nice here.
*/

void
extend_scan_buffer(to_where, current_free)
     fast char *to_where;
     Pointer *current_free;
{
  long new_scan_position;

  new_scan_position = (scan_position + GC_BUFFER_BYTES);

  /* Is there overlap?, ie. is the next bufferfull the one cached
     in the free pointer window? */

  if (new_scan_position == free_position)
  {
    fast char *source, *dest;
    long temp;

    extension_overlap_p = true;
    source = ((char *) free_buffer_bottom);
    dest = ((char *) scan_buffer_top);
    extension_overlap_length = (to_where - dest);
    temp = (((char *) current_free) - source);
    if (temp < extension_overlap_length)
    {
      /* This should only happen when Scan and Free are very close. */
      extension_overlap_length = temp;
    }

    while (dest < to_where)
    {
      *dest++ = *source++;
    }
  }
  else
  {
    extension_overlap_p = false;
    load_buffer(new_scan_position, scan_buffer_top,
		GC_BUFFER_OVERLAP_BYTES, "the scan buffer");
  }
  return;
}

char *
end_scan_buffer_extension(to_relocate)
     char *to_relocate;
{
  char *result;

  dump_buffer(scan_buffer_bottom, &scan_position, 1, "scan",
	      ((Boolean *) NULL));
  if (!extension_overlap_p)
  {
    /* There was no overlap */

    fast Pointer *source, *dest, *limit;

    source = scan_buffer_top;
    dest = scan_buffer_bottom;
    limit = &source[GC_EXTRA_BUFFER_SIZE];
    result = (((char *) scan_buffer_bottom) +
	      (to_relocate - ((char *) scan_buffer_top)));

    while (source < limit)
    {
      *dest++ = *source++;
    }
    load_buffer((scan_position + GC_BUFFER_OVERLAP_BYTES),
		dest,
		GC_BUFFER_REMAINDER_BYTES,
		"the scan buffer");
    *scan_buffer_top = Make_Pointer(TC_BROKEN_HEART, scan_buffer_top);
  }
  else
  {
    fast char *source, *dest, *limit;

    source = ((char *) scan_buffer_top);
    dest = ((scan_position == free_position) ?
	    ((char *) free_buffer_bottom) :
	    ((char *) scan_buffer_bottom));
    limit = &source[extension_overlap_length];
    result = &dest[to_relocate - source];

    while (source < limit)
    {
      *dest++ = *source++;
    }
    if (scan_position == free_position)
    {
      /* There was overlap, and there still is. */

      scan_buffer_bottom = free_buffer_bottom;
      scan_buffer_top = free_buffer_top;
    }
    else
    {
      /* There was overlap, but there no longer is. */

      load_buffer((scan_position + extension_overlap_length),
		  dest,
		  (GC_BUFFER_BYTES - extension_overlap_length),
		  "the scan buffer");
      *scan_buffer_top = Make_Pointer(TC_BROKEN_HEART, scan_buffer_top);
    }
  }
  extension_overlap_p = false;
  return (result);
}

Pointer *
dump_and_reload_scan_buffer(number_to_skip, success)
     long number_to_skip;
     Boolean *success;
{
  dump_buffer(scan_buffer_bottom, &scan_position, 1, "scan", success);
  if (number_to_skip != 0)
  {
    scan_position += (number_to_skip * GC_BUFFER_BYTES);
  }
  reload_scan_buffer();
  return (scan_buffer_bottom);
}

Pointer *
dump_and_reset_free_buffer(overflow, success)
     fast long overflow;
     Boolean *success;
{
  fast Pointer *into, *from;

  from = free_buffer_top;
  if (free_buffer_bottom == scan_buffer_bottom)
  {
    /* No need to dump now, it will be dumped when scan is dumped.
       Note that the next buffer may be dumped before this one,
       but there is no problem lseeking past the end of file.
     */
    free_position += GC_BUFFER_BYTES;
    free_buffer_bottom = ((scan_buffer_bottom == gc_disk_buffer_1) ?
			  gc_disk_buffer_2 :
			  gc_disk_buffer_1);
    free_buffer_top = free_buffer_bottom + GC_DISK_BUFFER_SIZE;
  }
  else
  {
    dump_buffer(free_buffer_bottom, &free_position, 1, "free", success);
  }

  for (into = free_buffer_bottom; --overflow >= 0; )
  {
    *into++ = *from++;
  }

  /* This need only be done when free_buffer_bottom was scan_buffer_bottom,
     but it does not hurt otherwise unless we were in the
     extend_scan_buffer/end_scan_buffer_extension window.
     It must also be done after the for loop above.
   */
  if (!extension_overlap_p)
  {
    *scan_buffer_top = Make_Pointer(TC_BROKEN_HEART, scan_buffer_top);
  }
  return (into);
}

void
dump_free_directly(from, nbuffers, success)
     Pointer *from;
     long nbuffers;
     Boolean *success;
{
  dump_buffer(from, &free_position, nbuffers, "free", success);
  return;
}

static long current_buffer_position;

void
initialize_new_space_buffer()
{
  current_buffer_position = -1;
  return;
}

void
flush_new_space_buffer()
{
  if (current_buffer_position == -1)
  {
    return;
  }
  dump_buffer(gc_disk_buffer_1, &current_buffer_position,
	      1, "weak pair buffer", NULL);
  current_buffer_position = -1;
  return;
}

Pointer *
guarantee_in_memory(addr)
     Pointer *addr;
{
  long position, offset;

  if (addr >= Constant_Space)
  {
    return (addr);
  }

  position = (addr - Heap_Bottom);
  offset = (position % GC_DISK_BUFFER_SIZE);
  position = (position / GC_DISK_BUFFER_SIZE);
  position *= GC_BUFFER_BYTES;
  if (position != current_buffer_position)
  {
    flush_new_space_buffer();
    load_buffer(position, gc_disk_buffer_1,
		GC_BUFFER_BYTES, "the weak pair buffer");
    current_buffer_position = position;
  }
  return (&gc_disk_buffer_1[offset]);
}

/* For a description of the algorithm, see memmag.c.
   This has been modified only to account for the fact that new space
   is on disk.  Old space is in memory.
*/

Pointer Weak_Chain;

void
Fix_Weak_Chain()
{
  fast Pointer *Old_Weak_Cell, *Scan, Old_Car, Temp, *Old, *Low_Constant;

  initialize_new_space_buffer();
  Low_Constant = Constant_Space;
  while (Weak_Chain != NIL)
  {
    Old_Weak_Cell = Get_Pointer(Weak_Chain);
    Scan = guarantee_in_memory(Get_Pointer(*Old_Weak_Cell++));
    Weak_Chain = *Old_Weak_Cell;
    Old_Car = *Scan;
    Temp = Make_New_Pointer(OBJECT_TYPE(Weak_Chain), Old_Car);
    Weak_Chain = Make_New_Pointer(TC_NULL, Weak_Chain);

    switch(GC_Type(Temp))
    {
      case GC_Non_Pointer:
        *Scan = Temp;
	continue;

      case GC_Special:
	if (OBJECT_TYPE(Temp) != TC_REFERENCE_TRAP)
	{
	  /* No other special type makes sense here. */
	  goto fail;
	}
	if (OBJECT_DATUM(Temp) <= TRAP_MAX_IMMEDIATE)
	{
	  *Scan = Temp;
	  continue;
	}
	/* Otherwise, it is a pointer.  Fall through */

      /* Normal pointer types, the broken heart is in the first word.
         Note that most special types are treated normally here.
	 The BH code updates *Scan if the object has been relocated.
	 Otherwise it falls through and we replace it with a full NIL.
	 Eliminating this assignment would keep old data (pl. of datum).
       */
      case GC_Cell:
      case GC_Pair:
      case GC_Triple:
      case GC_Quadruple:
      case GC_Vector:
	/* Old is still a pointer to old space */
	Old = Get_Pointer(Old_Car);
	if (Old >= Low_Constant)
	{
	  *Scan = Temp;
	  continue;
	}
	if (OBJECT_TYPE(*Old) == TC_BROKEN_HEART)
	{
	  *Scan = Make_New_Pointer(OBJECT_TYPE(Temp), *Old);
	  continue;
	}
	*Scan = NIL;
	continue;

      case GC_Compiled:
	/* Old is still a pointer to old space */
	Old = Get_Pointer(Old_Car);
	if (Old >= Low_Constant)
	{
	  *Scan = Temp;
	  continue;
	}
	Compiled_BH(false, continue);
	*Scan = NIL;
	continue;

      case GC_Undefined:
      default:			/* Non Marked Headers and Broken Hearts */
      fail:
        fprintf(stderr,
		"\nFix_Weak_Chain: Bad Object: Type = 0x%02x; Datum = %x\n",
		OBJECT_TYPE(Temp), OBJECT_DATUM(Temp));
	Microcode_Termination(TERM_INVALID_TYPE_CODE);
	/*NOTREACHED*/
    }
  }
  flush_new_space_buffer();
  return;
}

/* Here is the set up for the full garbage collection:

   - First it makes the constant space and stack into one large area
   by "hiding" the gap between them with a non-marked header.
   
   - Then it saves away all the relevant microcode registers into new
   space, making this the root for garbage collection.

   - Then it does the actual garbage collection in 4 steps:
     1) Trace constant space.
     2) Trace objects pointed out by the root and constant space.
     3) Trace the precious objects, remembering where consing started.
     4) Update all weak pointers.

   - Load new space to memory.

   - Finally it restores the microcode registers from the copies in
   new space.
*/

void
GC(initial_weak_chain)
     Pointer initial_weak_chain;
{
  Pointer
    *Root, *Result, *end_of_constant_area,
    The_Precious_Objects, *Root2, *free_buffer;

  free_buffer = initialize_free_buffer();
  Free = Heap_Bottom;
  SET_MEMTOP(Heap_Top - GC_Reserve);
  Weak_Chain = initial_weak_chain;

  /* Save the microcode registers so that they can be relocated */

  Terminate_Old_Stacklet();
  Terminate_Constant_Space(end_of_constant_area);
  Root = Free;
  The_Precious_Objects = Get_Fixed_Obj_Slot(Precious_Objects);
  Set_Fixed_Obj_Slot(Precious_Objects, NIL);
  Set_Fixed_Obj_Slot(Lost_Objects_Base, NIL);

  *free_buffer++ = Fixed_Objects;
  *free_buffer++ = Make_Pointer(UNMARKED_HISTORY_TYPE, History);
  *free_buffer++ = Undefined_Primitives;
  *free_buffer++ = Undefined_Primitives_Arity;
  *free_buffer++ = Get_Current_Stacklet();
  *free_buffer++ = ((Prev_Restore_History_Stacklet == NULL) ?
		    NIL :
		    Make_Pointer(TC_CONTROL_POINT,
				 Prev_Restore_History_Stacklet));
  *free_buffer++ = Current_State_Point;
  *free_buffer++ = Fluid_Bindings;
  Free += (free_buffer - free_buffer_bottom);
  if (free_buffer >= free_buffer_top)
  {
    free_buffer = dump_and_reset_free_buffer((free_buffer - free_buffer_top),
					     NULL);
  }

  /* The 4 step GC */

  Result = GCLoop(Constant_Space, &free_buffer, &Free);
  if (Result != end_of_constant_area)
  {
    fprintf(stderr, "\nGC: Constant Scan ended too early.\n");
    Microcode_Termination(TERM_EXIT);
    /*NOTREACHED*/
  }

  Result = GCLoop(initialize_scan_buffer(), &free_buffer, &Free);
  if (free_buffer != Result)
  {
    fprintf(stderr, "\nGC-1: Heap Scan ended too early.\n");
    Microcode_Termination(TERM_EXIT);
    /*NOTREACHED*/
  }

  Root2 = Free;
  *free_buffer++ = The_Precious_Objects;
  Free += (free_buffer - Result);
  if (free_buffer >= free_buffer_top)
    free_buffer = dump_and_reset_free_buffer((free_buffer - free_buffer_top), NULL);

  Result = GCLoop(Result, &free_buffer, &Free);
  if (free_buffer != Result)
  {
    fprintf(stderr, "\nGC-2: Heap Scan ended too early.\n");
    Microcode_Termination(TERM_EXIT);
    /*NOTREACHED*/
  }

  end_transport(NULL);

  Fix_Weak_Chain();

  /* Load new space into memory. */

  load_buffer(0, Heap_Bottom,
	      ((Free - Heap_Bottom) * sizeof(Pointer)),
	      "new space");

  /* Make the microcode registers point to the copies in new-space. */

  Fixed_Objects = *Root++;
  Set_Fixed_Obj_Slot(Precious_Objects, *Root2);
  Set_Fixed_Obj_Slot(Lost_Objects_Base, Make_Pointer(TC_ADDRESS, Root2));

  History = Get_Pointer(*Root++);
  Undefined_Primitives = *Root++;
  Undefined_Primitives_Arity = *Root++;

  /* Set_Current_Stacklet is sometimes a No-Op! */

  Set_Current_Stacklet(*Root);
  Root += 1;
  if (*Root == NIL)
  {
    Prev_Restore_History_Stacklet = NULL;
    Root += 1;
  }
  else
  {
    Prev_Restore_History_Stacklet = Get_Pointer(*Root++);
  }
  Current_State_Point = *Root++;
  Fluid_Bindings = *Root++;
  Free_Stacklets = NULL;
  return;
}

/* (GARBAGE-COLLECT SLACK)
   Requests a garbage collection leaving the specified amount of slack
   for the top of heap check on the next GC.  The primitive ends by invoking
   the GC daemon if there is one.
*/

DEFINE_PRIMITIVE ("GARBAGE-COLLECT", Prim_Garbage_Collect, 1)
{
  extern unsigned long gc_counter;
  Pointer GC_Daemon_Proc;
  Primitive_1_Arg();

  Arg_1_Type(TC_FIXNUM);
  if (Free > Heap_Top)
  {
    Microcode_Termination(TERM_GC_OUT_OF_SPACE);
    /*NOTREACHED*/
  }
  gc_counter += 1;
  GC_Reserve = Get_Integer(Arg1);
  GC(NIL);
  CLEAR_INTERRUPT(INT_GC);
  Pop_Primitive_Frame(1);
  GC_Daemon_Proc = Get_Fixed_Obj_Slot(GC_Daemon);
  if (GC_Daemon_Proc == NIL)
  {
   Will_Push(CONTINUATION_SIZE);
    Store_Return(RC_NORMAL_GC_DONE);
    Store_Expression(Make_Unsigned_Fixnum(MemTop - Free));
    Save_Cont();
   Pushed();
    PRIMITIVE_ABORT(PRIM_POP_RETURN);
    /*NOTREACHED*/
  }
 Will_Push(CONTINUATION_SIZE + (STACK_ENV_EXTRA_SLOTS + 1));
  Store_Return(RC_NORMAL_GC_DONE);
  Store_Expression(Make_Unsigned_Fixnum(MemTop - Free));
  Save_Cont();
  Push(GC_Daemon_Proc);
  Push(STACK_FRAME_HEADER);
 Pushed();
  PRIMITIVE_ABORT(PRIM_APPLY);
  /* The following comment is by courtesy of LINT, your friendly sponsor. */
  /*NOTREACHED*/
}
