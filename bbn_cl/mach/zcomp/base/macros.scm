#| -*-Scheme-*-

$Header: macros.scm,v 1.2 88/08/31 10:35:17 jinx Exp $
$MIT-Header: macros.scm,v 4.4 87/12/31 10:43:40 GMT cph Exp $

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

;;;; Compiler Macros

(declare (usual-integrations))

(define compiler-syntax-table
  (make-syntax-table system-global-syntax-table))

(define lap-generator-syntax-table
  (make-syntax-table compiler-syntax-table))

(define assembler-syntax-table
  (make-syntax-table compiler-syntax-table))

(define early-syntax-table
  (make-syntax-table compiler-syntax-table))

(syntax-table-define compiler-syntax-table 'PACKAGE
  (in-package system-global-environment
    (declare (usual-integrations))
    (lambda (expression)
      (apply (lambda (names . body)
	       (make-sequence
		`(,@(map (lambda (name)
			   (make-definition name (make-unassigned-object)))
			 names)
		  ,(make-combination
		    (let ((block (syntax* body)))
		      (if (open-block? block)
			  (open-block-components block
			    (lambda (names* declarations body)
			      (make-lambda lambda-tag:let '() '() false
					   (list-transform-negative names*
					     (lambda (name)
					       (memq name names)))
					   declarations
					   body)))
			  (make-lambda lambda-tag:let '() '() false '()
				       '() block)))
		    '()))))
	     (cdr expression)))))

(let ()

(define (parse-define-syntax pattern body if-variable if-lambda)
  (cond ((pair? pattern)
	 (let loop ((pattern pattern) (body body))
	   (cond ((pair? (car pattern))
		  (loop (car pattern) `((LAMBDA ,(cdr pattern) ,@body))))
		 ((symbol? (car pattern))
		  (if-lambda pattern body))
		 (else
		  (error "Illegal name" (car pattern))))))
	((symbol? pattern)
	 (if-variable pattern body))
	(else
	 (error "Illegal name" pattern))))

(define lambda-list->bound-names
  (letrec ((lambda-list->bound-names
	    (lambda (lambda-list)
	      (cond ((null? lambda-list)
		     '())
		    ((pair? lambda-list)
		     (if (eq? (car lambda-list)
			      (access lambda-optional-tag lambda-package))
			 (if (pair? (cdr lambda-list))
			     (accumulate (cdr lambda-list))
			     (error "Missing optional variable" lambda-list))
			 (accumulate lambda-list)))
		    ((symbol? lambda-list)
		     (list lambda-list))
		    (else
		     (error "Illegal rest variable" lambda-list)))))
	   (accumulate
	    (lambda (lambda-list)
	      (cons (let ((parameter (car lambda-list)))
		      (if (pair? parameter) (car parameter) parameter))
		    (lambda-list->bound-names (cdr lambda-list))))))
    lambda-list->bound-names))

(syntax-table-define compiler-syntax-table 'DEFINE-EXPORT
  (macro (pattern . body)
    (parse-define-syntax pattern body
      (lambda (name body)
	`(SET! ,pattern ,@body))
      (lambda (pattern body)
	`(SET! ,(car pattern)
	       (NAMED-LAMBDA ,pattern ,@body))))))

(syntax-table-define compiler-syntax-table 'DEFINE-INTEGRABLE
  (macro (pattern . body)
    (if compiler:enable-integration-declarations?
	(parse-define-syntax pattern body
	  (lambda (name body)
	    `(BEGIN (DECLARE (INTEGRATE ,pattern))
		    (DEFINE ,pattern ,@body)))
	  (lambda (pattern body)
	    `(BEGIN (DECLARE (INTEGRATE-OPERATOR ,(car pattern)))
		    (DEFINE ,pattern
		      ,@(if (list? (cdr pattern))
			    `((DECLARE
			       (INTEGRATE
				,@(lambda-list->bound-names (cdr pattern)))))
			    '())
		      ,@body))))
	`(DEFINE ,pattern ,@body))))

)

(syntax-table-define compiler-syntax-table 'DEFINE-VECTOR-SLOTS
  (macro (class index . slots)
    (define (loop slots n)
      (if (null? slots)
	  '()
	  (let ((make-defs
		 (lambda (slot)
		   (let ((ref-name (symbol-append class '- slot)))
		     `(BEGIN
			(DEFINE-INTEGRABLE (,ref-name ,class)
			  (VECTOR-REF ,class ,n))
			(DEFINE-INTEGRABLE (,(symbol-append 'SET- ref-name '!)
					    ,class ,slot)
			  (VECTOR-SET! ,class ,n ,slot))))))
		(rest (loop (cdr slots) (1+ n))))
	    (if (pair? (car slots))
		(map* rest make-defs (car slots))
		(cons (make-defs (car slots)) rest)))))
    (if (null? slots)
	'*THE-NON-PRINTING-OBJECT*
	`(BEGIN ,@(loop slots index)))))

(syntax-table-define compiler-syntax-table 'DEFINE-ROOT-TYPE
  (macro (type . slots)
    (let ((tag-name (symbol-append type '-TAG)))
      `(BEGIN (DEFINE ,tag-name
		(MAKE-VECTOR-TAG FALSE ',type FALSE))
	      (DEFINE ,(symbol-append type '?)
		(TAGGED-VECTOR/SUBCLASS-PREDICATE ,tag-name))
	      (DEFINE-VECTOR-SLOTS ,type 1 ,@slots)
	      (SET-VECTOR-TAG-DESCRIPTION!
	       ,tag-name
	       (LAMBDA (,type)
		 (DESCRIPTOR-LIST ,type ,@slots)))))))

(syntax-table-define compiler-syntax-table 'DESCRIPTOR-LIST
  (macro (type . slots)
    (let ((ref-name (lambda (slot) (symbol-append type '- slot))))
      `(LIST ,@(map (lambda (slot)
		      (if (pair? slot)
			  (let ((ref-names (map ref-name slot)))
			    ``(,',ref-names ,(,(car ref-names) ,type)))
			  (let ((ref-name (ref-name slot)))
			    ``(,',ref-name ,(,ref-name ,type)))))
		    slots)))))

(let-syntax
 ((define-type-definition
    (macro (name reserved enumeration)
      (let ((parent (symbol-append name '-TAG)))
	`(SYNTAX-TABLE-DEFINE COMPILER-SYNTAX-TABLE
			      ',(symbol-append 'DEFINE- name)
	   (macro (type . slots)
	     (let ((tag-name (symbol-append type '-TAG)))
	       `(BEGIN (DEFINE ,tag-name
			 (MAKE-VECTOR-TAG ,',parent ',type ,',enumeration))
		       (DEFINE ,(symbol-append type '?)
			 (TAGGED-VECTOR/PREDICATE ,tag-name))
		       (DEFINE-VECTOR-SLOTS ,type ,,reserved ,@slots)
		       (SET-VECTOR-TAG-DESCRIPTION!
			,tag-name
			(LAMBDA (,type)
			  (APPEND!
			   ((VECTOR-TAG-DESCRIPTION ,',parent) ,type)
			   (DESCRIPTOR-LIST ,type ,@slots))))))))))))
 (define-type-definition snode 5 false)
 (define-type-definition pnode 6 false)
 (define-type-definition rvalue 2 rvalue-types)
 (define-type-definition lvalue 10 false))

;;; Kludge to make these compile efficiently.

(syntax-table-define compiler-syntax-table 'MAKE-SNODE
  (macro (tag . extra)
    `((ACCESS VECTOR ,system-global-environment)
      ,tag FALSE '() '() FALSE ,@extra)))

(syntax-table-define compiler-syntax-table 'MAKE-PNODE
  (macro (tag . extra)
    `((ACCESS VECTOR ,system-global-environment)
      ,tag FALSE '() '() FALSE FALSE ,@extra)))

(syntax-table-define compiler-syntax-table 'MAKE-RVALUE
  (macro (tag . extra)
    `((ACCESS VECTOR ,system-global-environment)
      ,tag FALSE ,@extra)))

(syntax-table-define compiler-syntax-table 'MAKE-LVALUE
  (macro (tag . extra)
    (let ((result (generate-uninterned-symbol)))
      `(let ((,result
	      ((ACCESS VECTOR ,system-global-environment)
	       ,tag '() '() '() 'NOT-CACHED FALSE '() FALSE FALSE '()
	       ,@extra)))
	 (SET! *LVALUES* (CONS ,result *LVALUES*))
	 ,result))))

(let ((rtl-common
       (lambda (type prefix components wrap-constructor)
	 `(BEGIN
	    (DEFINE-INTEGRABLE
	      (,(symbol-append prefix 'MAKE- type) ,@components)
	      ,(wrap-constructor `(LIST ',type ,@components)))
	    (DEFINE-INTEGRABLE (,(symbol-append 'RTL: type '?) EXPRESSION)
	      (EQ? (CAR EXPRESSION) ',type))
	    ,@(let loop ((components components)
			 (ref-index 6)
			 (set-index 2))
		(if (null? components)
		    '()
		    (let* ((slot (car components))
			   (name (symbol-append type '- slot)))
		      `((DEFINE-INTEGRABLE (,(symbol-append 'RTL: name) ,type)
			  (GENERAL-CAR-CDR ,type ,ref-index))
			(DEFINE-INTEGRABLE (,(symbol-append 'RTL:SET- name '!)
					    ,type ,slot)
			  (SET-CAR! (GENERAL-CAR-CDR ,type ,set-index) ,slot))
			,@(loop (cdr components)
				(* ref-index 2)
				(* set-index 2))))))))))
  (syntax-table-define compiler-syntax-table 'DEFINE-RTL-EXPRESSION
    (macro (type prefix . components)
      (rtl-common type prefix components identity-procedure)))

  (syntax-table-define compiler-syntax-table 'DEFINE-RTL-STATEMENT
    (macro (type prefix . components)
      (rtl-common type prefix components
		  (lambda (expression) `(STATEMENT->SRTL ,expression)))))

  (syntax-table-define compiler-syntax-table 'DEFINE-RTL-PREDICATE
    (macro (type prefix . components)
      (rtl-common type prefix components
		  (lambda (expression) `(PREDICATE->PRTL ,expression))))))

(syntax-table-define compiler-syntax-table 'UCODE-TYPE
  (macro (name)
    (microcode-type name)))

(syntax-table-define compiler-syntax-table 'UCODE-PRIMITIVE
  (macro (name)
    (make-primitive-procedure name)))

(syntax-table-define lap-generator-syntax-table 'DEFINE-RULE
  (macro (type pattern . body)
    (parse-rule pattern body
      (lambda (pattern variables qualifier actions)
	`(,(case type
	     ((STATEMENT) 'ADD-STATEMENT-RULE!)
	     ((PREDICATE) 'ADD-STATEMENT-RULE!)
	     (else (error "Unknown rule type" type)))
	  ',pattern
	  ,(rule-result-expression variables qualifier
				   `(BEGIN ,@actions)))))))

;;;; Lap instruction sequences.

;; The effect of unquote and unquote-splicing is the same since
;; syntax-instruction actually returns a bit-level instruction sequence.
;; Kept separate for clarity and because it does not have to be like that.

(syntax-table-define compiler-syntax-table 'LAP
  (macro some-instructions
    (define (handle current remaining)
      (let ((processed
	     (cond ((eq? (car current) 'UNQUOTE)
		    (cadr current))
		   ((eq? (car current) 'UNQUOTE-SPLICING)
		    (cadr current))
		   (else `(INST ,current)))))
	(if (null? remaining)
	    processed
	    `(APPEND-INSTRUCTION-SEQUENCES!
	      ,processed
	      ,(handle (car remaining) (cdr remaining))))))
    (if (null? some-instructions)
	`EMPTY-INSTRUCTION-SEQUENCE
	(handle (car some-instructions) (cdr some-instructions)))))

(syntax-table-define compiler-syntax-table 'INST
  (macro (the-instruction)
    `(LAP:SYNTAX-INSTRUCTION
      ,(list 'QUASIQUOTE the-instruction))))

;; This is a NOP for now.

(syntax-table-define compiler-syntax-table 'INST-EA
  (macro (ea)
    (list 'QUASIQUOTE ea)))

(syntax-table-define compiler-syntax-table 'DEFINE-ENUMERATION
  (macro (name elements)
    (let ((enumeration (symbol-append name 'S)))
      `(BEGIN (DEFINE ,enumeration
		(MAKE-ENUMERATION ',elements))
	      ,@(map (lambda (element)
		       `(DEFINE ,(symbol-append name '/ element)
			  (ENUMERATION/NAME->INDEX ,enumeration ',element)))
		     elements)))))

(define (macros/case-macro expression clauses predicate default)
  (let ((need-temp? (not (symbol? expression))))
    (let ((expression*
	   (if need-temp?
	       (generate-uninterned-symbol)
	       expression)))
      (let ((body
	     `(COND
	       ,@(let loop ((clauses clauses))
		   (cond ((null? clauses)
			  (default expression*))
			 ((eq? (caar clauses) 'ELSE)
			  (if (null? (cdr clauses))
			      clauses
			      (error "ELSE clause not last" clauses)))
			 (else
			  `(((OR ,@(map (lambda (element)
					  (predicate expression* element))
					(caar clauses)))
			     ,@(cdar clauses))
			    ,@(loop (cdr clauses)))))))))
	(if need-temp?
	    `(LET ((,expression* ,expression))
	       ,body)
	    body)))))

(syntax-table-define compiler-syntax-table 'ENUMERATION-CASE
  (macro (name expression . clauses)
    (macros/case-macro expression
		       clauses
		       (lambda (expression element)
			 `(EQ? ,expression ,(symbol-append name '/ element)))
		       (lambda (expression)
			 '()))))

(syntax-table-define compiler-syntax-table 'CFG-NODE-CASE
  (macro (expression . clauses)
    (macros/case-macro expression
		       clauses
		       (lambda (expression element)
			 `(EQ? ,expression ,(symbol-append element '-TAG)))
		       (lambda (expression)
			 `((ELSE (ERROR "Unknown node type" ,expression)))))))
