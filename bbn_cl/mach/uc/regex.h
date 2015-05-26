/* -*-C-*-

$Header: regex.h,v 10.0 88/12/07 13:10:14 las Exp $
$MIT-Header: regex.h,v 1.1 87/07/14 03:00:23 GMT cph Rel $

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

/* Translated from GNU Emacs. */

/* Structure to represent a buffer of text to match against.
   This contains the information that an editor buffer would have
   to supply for the matching process to be executed.

   `translation' is an array of MAX_ASCII characters which is used to
   map each character before matching.  Both the pattern and the match
   text are mapped.  This is normally used to implement case
   insensitive searches.

   `syntax_table' describes the syntax of the match text.  See the
   syntax table primitives for more information.

   `text' points to the beginning of the match text.  It is used only
   for translating match text pointers into indices.

   `text_start' and `text_end' delimit the match text.  They define
   the buffer-start and buffer-end for those matching commands that
   refer to them.  Also, all matching must take place within these
   limits.

   `gap_start' and `gap_end' delimit a gap in the match text.  Editor
   buffers normally have such a gap.  For applications without a gap,
   it is recommended that these be set to the same value as
   `text_end'.

   Both `text_start' and `gap_start' are inclusive indices, while
   `text_end' and `gap_end' are exclusive.

   The following conditions must be true:

   (text <= text_start)
   (text_start <= text_end)
   (gap_start <= gap_end)
   (! ((text_start < text_end) &&
       (gap_start < gap_end) &&
       ((text_start == gap_start) || (text_end == gap_end))))

   */

struct re_buffer
  {
    unsigned char *translation;
    SYNTAX_TABLE_TYPE syntax_table;
    unsigned char *text;
    unsigned char *text_start;
    unsigned char *text_end;
    unsigned char *gap_start;
    unsigned char *gap_end;
  };

/* Structure to store "register" contents data in.

   Pass the address of such a structure as an argument to re_match,
   etc., if you want this information back.

   start[i] and end[i] record the string matched by \( ... \) grouping
   i, for i from 1 to RE_NREGS - 1.

   start[0] and end[0] record the entire string matched. */

#define RE_NREGS 10

struct re_registers
  {
    long start[RE_NREGS];
    long end[RE_NREGS];
  };

/* These are the command codes that appear in compiled regular
   expressions, one per byte.  Some command codes are followed by
   argument bytes.  A command code can specify any interpretation
   whatever for its arguments.  Zero-bytes may appear in the compiled
   regular expression. */

enum regexpcode
  {
    regexpcode_unused,
    regexpcode_exact_1,		/* Followed by 1 literal byte */

    /* Followed by one byte giving n, and then by n literal bytes. */
    regexpcode_exact_n,

    regexpcode_line_start,	/* Fails unless at beginning of line */
    regexpcode_line_end,	/* Fails unless at end of line */

    /* Followed by two bytes giving relative address to jump to. */
    regexpcode_jump,

    /* Followed by two bytes giving relative address of place to
       resume at in case of failure. */
    regexpcode_on_failure_jump,	

    /* Throw away latest failure point and then jump to address. */
    regexpcode_finalize_jump,

    /* Like jump but finalize if safe to do so.  This is used to jump
       back to the beginning of a repeat.  If the command that follows
       this jump is clearly incompatible with the one at the beginning
       of the repeat, such that we can be sure that there is no use
       backtracking out of repetitions already completed, then we
       finalize. */
    regexpcode_maybe_finalize_jump,

    /* jump, and push a dummy failure point.  This failure point will
       be thrown away if an attempt is made to use it for a failure.
       A + construct makes this before the first repeat. */
    regexpcode_dummy_failure_jump,

    regexpcode_any_char,	/* Matches any one character */

    /* Matches any one char belonging to specified set.  First
       following byte is # bitmap bytes.  Then come bytes for a
       bit-map saying which chars are in.  Bits in each byte are
       ordered low-bit-first.  A character is in the set if its bit is
       1.  A character too large to have a bit in the map is
       automatically not in the set. */
    regexpcode_char_set,

    /* Similar but match any character that is NOT one of those
       specified. */
    regexpcode_not_char_set,

    /* Starts remembering the text that is matched and stores it in a
       memory register.  Followed by one byte containing the register
       number.  Register numbers must be in the range 0 through
       (RE_NREGS - 1) inclusive.  */
    regexpcode_start_memory,

    /* Stops remembering the text that is matched and stores it in a
       memory register.  Followed by one byte containing the register
       number.  Register numbers must be in the range 0 through
       (RE_NREGS - 1) inclusive.  */
    regexpcode_stop_memory,

    /* Match a duplicate of something remembered.  Followed by one
       byte containing the index of the memory register. */
    regexpcode_duplicate,

    regexpcode_buffer_start,	/* Succeeds if at beginning of buffer */
    regexpcode_buffer_end,	/* Succeeds if at end of buffer */
    regexpcode_word_char,	/* Matches any word-constituent character */

    /* Matches any char that is not a word-constituent. */
    regexpcode_not_word_char,

    regexpcode_word_start,	/* Succeeds if at word beginning */
    regexpcode_word_end,	/* Succeeds if at word end */
    regexpcode_word_bound,	/* Succeeds if at a word boundary */
    regexpcode_not_word_bound,	/* Succeeds if not at a word boundary */

    /* Matches any character whose syntax is specified.  Followed by a
       byte which contains a syntax code, Sword or such like. */
    regexpcode_syntax_spec,

    /* Matches any character whose syntax differs from the specified. */
    regexpcode_not_syntax_spec
  };

extern void re_buffer_initialize ();
extern int re_compile_fastmap ();
extern int re_match ();
extern int re_search_forward ();
extern int re_search_backward ();
