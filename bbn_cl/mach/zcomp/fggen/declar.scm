#| -*-Scheme-*-

$Header: declar.scm,v 1.2 88/08/31 10:38:49 jinx Exp $
$MIT-Header: declar.scm,v 1.2 87/10/05 20:44:08 GMT jinx Exp $

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

;;;; Flow Graph Generation: Declarations

(declare (usual-integrations))

(define (process-top-level-declarations! block declarations)
  (process-declarations!
   block
   ;; Kludge!
   (if (assq 'UUO-LINK declarations)
       declarations
       (cons '(UUO-LINK ALL) declarations))))

(define (process-declarations! block declarations)
  (for-each (lambda (declaration)
	      (process-declaration! block declaration))
	    declarations))

(define (process-declaration! block declaration)
  (let ((entry (assq (car declaration) known-declarations)))
    (if entry
	((cdr entry) block (car declaration) (cdr declaration))
	(warn "Unknown declaration name" (car declaration)))))

(define known-declarations
  '())

(define (define-declaration keyword handler)
  (let ((entry (assq keyword known-declarations)))
    (if entry
	(set-cdr! entry handler)
	(set! known-declarations
	      (cons (cons keyword handler)
		    known-declarations))))
  keyword)

(package (boolean-variable-property)

(define-export (boolean-variable-property block keyword body)
  (if (and (pair? body) (null? (cdr body)))
      (for-each (lambda (variable)
		  (if (not (memq keyword (variable-declarations variable)))
		      (set-variable-declarations!
		       variable
		       (cons keyword (variable-declarations variable)))))
		(evaluate-variable-specification block (car body)))
      (warn "Misformed declaration" (cons keyword body))))

(define (evaluate-variable-specification block specification)
  (let loop ((specification specification))
    (cond ((eq? specification 'BOUND) (block-bound-variables block))
	  ((eq? specification 'FREE) (block-free-variables block))
	  ((eq? specification 'NONE) '())
	  ((eq? specification 'ALL)
	   (append (block-bound-variables block)
		   (block-free-variables block)))
	  ((and (pair? specification)
		(assq (car specification) binary-operators)
		(pair? (cdr specification))
		(pair? (cddr specification))
		(null? (cdddr specification)))
	   ((cdr (assq (car specification) binary-operators))
	    (loop (cadr specification))
	    (loop (caddr specification))))
	  ((and (pair? specification)
		(eq? (car specification) 'SET)
		(symbol-list? (cdr specification)))
	   (let loop ((symbols (cdr specification)))
	     (if (null? symbols)
		 '()
		 (let ((entry
			(or (variable-assoc (car symbols)
					    (block-bound-variables block))
			    (variable-assoc (car symbols)
					    (block-free-variables block)))))
		   (if entry
		       (cons entry (loop (cdr symbols)))
		       (loop (cdr symbols)))))))
	  (else
	   (warn "Misformed variable specification" specification)
	   '()))))

(define binary-operators
  `((DIFFERENCE . ,eq-set-difference)
    (INTERSECTION . ,eq-set-intersection)
    (UNION . ,eq-set-union)))

(define (symbol-list? object)
  (or (null? object)
      (and (pair? object)
	   (symbol? (car object))
	   (symbol-list? (cdr object)))))

)

(define-declaration 'UUO-LINK boolean-variable-property)
(define-declaration 'CONSTANT boolean-variable-property)
