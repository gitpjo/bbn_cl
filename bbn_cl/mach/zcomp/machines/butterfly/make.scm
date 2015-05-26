#| -*-Scheme-*-

$Header: make.scm,v 1.5 89/02/08 15:08:11 las Exp $
$MIT-Header: make.scm,v 4.7 88/03/14 19:38:20 GMT jinx Exp $

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
MIT in each case. |#

;;;; Compiler Make File for MC68020

(declare (usual-integrations))

(load "base/pkging.bin" system-global-environment)

(in-package compiler-package

  (define compiler-system
    (make-environment
      (define :name "Liar (Butterfly 68020)")
      (define :version 4)
      (define :modification 7)
      (define :files)

      (define :rcs-header
	"$Header: make.scm,v 1.5 89/02/08 15:08:11 las Exp $")

      (define :files-lists
	(list
	 (cons touchifier-package
	       '("touchify.com"         ;future (touch) support
		 ))

	 (cons system-global-environment
	       '("base/pbs.bin"		;bit-string read/write syntax
		 "etc/direct.bin"	;directory reader
		 "etc/butils.bin"	;system building utilities
		 ))

	 (cons compiler-package
	       '("base/switch.bin"	;compiler option switches
		 "base/macros.bin"	;compiler syntax
		 "base/hashtb.com"	;hash tables
		 ))

	 (cons decls-package
	       '("base/decls.com"	;declarations
		 ))

	 (cons compiler-package
	       '("base/object.com"	;tagged object support
		 "base/enumer.com"	;enumerations
		 "base/queue.com"	;queue abstraction
		 "base/sets.com"	;set abstraction
		 "base/mvalue.com"	;multiple-value support
		 "base/scode.com"	;SCode abstraction
		 "base/pmlook.com"	;pattern matcher: lookup
		 "base/pmpars.com"	;pattern matcher: parser

		 "machines/butterfly/machin.com" ;machine dependent stuff
		 "base/toplev.com"	;top level
		 "base/debug.com"	;debugging support
		 "base/utils.com"	;odds and ends

		 "base/cfg1.com"	;control flow graph
		 "base/cfg2.com"
		 "base/cfg3.com"
		 "base/ctypes.com"	;CFG datatypes

		 "base/rvalue.com"	;Right hand values
		 "base/lvalue.com"	;Left hand values
		 "base/blocks.com"	;rvalue: blocks
		 "base/proced.com"	;rvalue: procedures
		 "base/contin.com"	;rvalue: continuations

		 "base/subprb.com"	;subproblem datatype

		 "rtlbase/rgraph.com"	;program graph abstraction
		 "rtlbase/rtlty1.com"	;RTL: type definitions
		 "rtlbase/rtlty2.com"	;RTL: type definitions
		 "rtlbase/rtlexp.com"	;RTL: expression operations
		 "rtlbase/rtlcon.com"	;RTL: complex constructors
		 "rtlbase/rtlreg.com"	;RTL: registers
		 "rtlbase/rtlcfg.com"	;RTL: CFG types
		 "rtlbase/rtlobj.com"	;RTL: CFG objects
		 "rtlbase/regset.com"	;RTL: register sets

		 "base/infutl.com"	;utilities for info generation, shared
		 "back/insseq.com"	;LAP instruction sequences
		 "machines/butterfly/dassm1.com" ;disassembler
		 ))

	 (cons disassembler-package
	       '("machines/butterfly/dassm2.com" ;disassembler
		 "machines/butterfly/dassm3.com"
		 ))

	 (cons fg-generator-package
	       '("fggen/fggen.com"	;SCode->flow-graph converter
		 "fggen/declar.com"	;Declaration handling
		 ))

	 (cons fg-optimizer-package
	       '("fgopt/simapp.com"	;simulate applications
		 "fgopt/outer.com"	;outer analysis
		 "fgopt/folcon.com"	;fold constants
		 "fgopt/operan.com"	;operator analysis
		 "fgopt/closan.com"	;closure analysis
		 "fgopt/blktyp.com"	;environment type assignment
		 "fgopt/contan.com"	;continuation analysis
		 "fgopt/simple.com"	;simplicity analysis
		 "fgopt/order.com"	;subproblem ordering
		 "fgopt/conect.com"	;connectivity analysis
		 "fgopt/desenv.com"	;environment design
		 "fgopt/offset.com"	;compute node offsets
		 ))

	 (cons rtl-generator-package
	       '("rtlgen/rtlgen.com"	;RTL generator
		 "rtlgen/rgproc.com"	;procedure headers
		 "rtlgen/rgstmt.com"	;statements
		 "rtlgen/rgrval.com"	;rvalues
		 "rtlgen/rgcomb.com"	;combinations
		 "rtlgen/rgretn.com"	;returns
		 "rtlgen/fndblk.com"	;find blocks and variables
		 "rtlgen/opncod.com"	;open-coded primitives
		 "machines/butterfly/rgspcm.com" ;special close-coded primitives
		 "rtlbase/rtline.com"	;linearizer
		 ))

	 (cons rtl-cse-package
	       '("rtlopt/rcse1.com"	;RTL common subexpression eliminator
		 "rtlopt/rcse2.com"
		 "rtlopt/rcseep.com"	;CSE expression predicates
		 "rtlopt/rcseht.com"	;CSE hash table
		 "rtlopt/rcserq.com"	;CSE register/quantity abstractions
		 "rtlopt/rcsesr.com"	;CSE stack references
		 ))

	 (cons rtl-optimizer-package
	       '("rtlopt/rlife.com"	;RTL register lifetime analyzer
		 "rtlopt/rdeath.com"	;RTL code compression
		 "rtlopt/rdebug.com"	;RTL optimizer debugging output
		 "rtlopt/ralloc.com"	;RTL register allocation
		 ))

	 (cons debugging-information-package
	       '("base/infnew.com"	;debugging information generation
		 ))

	 (cons lap-syntax-package
	       '("back/lapgn1.com"	;LAP generator.
		 "back/lapgn2.com"
		 "back/lapgn3.com"
		 "back/regmap.com"	;Hardware register allocator.
		 "back/linear.com"	;LAP linearizer.
		 "machines/butterfly/lapgen.com" ;code generation rules.
		 "machines/butterfly/rules1.com"
		 "machines/butterfly/rules2.com"
		 "machines/butterfly/rules3.com"
		 "machines/butterfly/rules4.com"
		 "back/syntax.com"	;Generic syntax phase
		 "machines/butterfly/coerce.com" ;Coercions: integer -> bit string
		 "back/asmmac.com"	;Macros for hairy syntax
		 "machines/butterfly/insmac.com" ;Macros for hairy syntax
		 "machines/butterfly/insutl.com" ;Utilities for instructions
		 "machines/butterfly/instr1.com" ;68000 Effective addressing
		 "machines/butterfly/instr2.com" ;68000 Instructions
		 "machines/butterfly/instr3.com" ;  "        "
		 "machines/butterfly/instr4.com" ;  "        "
		 ))

	 (cons bit-package
	       '("machines/butterfly/assmd.com" ;Machine dependent
		 "back/symtab.com"	;Symbol tables
		 "back/bitutl.com"	;Assembly blocks
		 "back/bittop.com"	;Assembler top level
		 ))

	 ))

      ))

  (load-system! compiler-system))

;;;
;;; Special loading of bbn-inline-code. Each piece of the file
;;; must be evaluated in the appropriate environment.
;;;

(let ((load-compiled? 
       (begin
	 (newline)
	 (write-string "Load bbn inline code files compiled? ")
	 (eq? (read) 'y))))
  (let ((scode-list (fasload-multiple 
		     (if load-compiled?
			 "bbn-inline-code.com"
			 "bbn-inline-code.bin"))))
    (for-each (lambda (scode env)
		(scode-eval scode env))
	      scode-list
	      (list
	       compiler-package
	       (access rtl-cse-package compiler-package)
	       (access rtl-method-package compiler-package)
	       (access open-coder-package
		       (access rtl-generator-package compiler-package))
	       (access lap-syntax-package compiler-package)))
    (load (if load-compiled?
	      "fltasm.com"
	      "fltasm.bin")
	  (access lap-syntax-package compiler-package))))

;; This does not use system-global-environment so that multiple
;; versions of the compiler can coexist in different environments.
;; This file must therefore be loaded into system-global-environment
;; when the names below must be exported everywhere.

(let ((top-level-env system-global-environment)) ;; was (the-environment)
  (for-each (lambda (name)
	    (local-assignment top-level-env name
			      (lexical-reference compiler-package name)))
	    '(CF
	      COMPILE-BIN-FILE
	      COMPILE-PROCEDURE
	      COMPILER:RESET!
	      COMPILER:WRITE-LAP-FILE
	      COMPILER:WRITE-RTL-FILE)))
