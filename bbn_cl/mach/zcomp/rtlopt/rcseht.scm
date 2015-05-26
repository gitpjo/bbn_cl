#| -*-Scheme-*-

$Header: rcseht.scm,v 1.2 88/08/31 10:44:25 jinx Exp $
$MIT-Header: rcseht.scm,v 4.2 87/12/30 07:13:28 GMT cph Exp $

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

;;;; RTL Common Subexpression Elimination: Hash Table Abstraction
;;;  Based on the GNU C Compiler

(declare (usual-integrations))

(define (make-hash-table)
  (make-vector 31 false))

(define *hash-table*)

(define-integrable (hash-table-size)
  (vector-length *hash-table*))

(define-integrable (hash-table-ref hash)
  (vector-ref *hash-table* hash))

(define-integrable (hash-table-set! hash element)
  (vector-set! *hash-table* hash element))

(define-structure (element
		   (constructor %make-element)
		   (constructor make-element (expression))
		   (print-procedure (standard-unparser 'ELEMENT false)))
  (expression false read-only true)
  (cost false)
  (in-memory? false)
  (next-hash false)
  (previous-hash false)
  (next-value false)
  (previous-value false)
  (first-value false)
  (copy-cache false))

(set-type-object-description!
 element
 (lambda (element)
   `((ELEMENT-EXPRESSION ,(element-expression element))
     (ELEMENT-COST ,(element-cost element))
     (ELEMENT-IN-MEMORY? ,(element-in-memory? element))
     (ELEMENT-NEXT-HASH ,(element-next-hash element))
     (ELEMENT-PREVIOUS-HASH ,(element-previous-hash element))
     (ELEMENT-NEXT-VALUE ,(element-next-value element))
     (ELEMENT-PREVIOUS-VALUE ,(element-previous-value element))
     (ELEMENT-FIRST-VALUE ,(element-first-value element))
     (ELEMENT-COPY-CACHE ,(element-copy-cache element)))))

(define (hash-table-lookup hash expression)
  (define (loop element)
    (and element
	 (if (let ((expression* (element-expression element)))
	       (or (eq? expression expression*)
		   (expression-equivalent? expression expression* true)))
	     element
	     (loop (element-next-hash element)))))
  (loop (hash-table-ref hash)))

(define (hash-table-insert! hash expression class)
  (let ((element (make-element expression))
	(cost (rtl:expression-cost expression)))
    (set-element-cost! element cost)
    (let ((next (hash-table-ref hash)))
      (set-element-next-hash! element next)
      (if next (set-element-previous-hash! next element)))
    (hash-table-set! hash element)
    (cond ((not class)
	   (set-element-first-value! element element))
	  ((< cost (element-cost class))
	   (set-element-next-value! element class)
	   (set-element-previous-value! class element)
	   (let loop ((x element))
	     (if x
		 (begin (set-element-first-value! x element)
			(loop (element-next-value x))))))
	  (else
	   (set-element-first-value! element class)
	   (let loop ((previous class)
		      (next (element-next-value class)))
	     (cond ((not next)
		    (set-element-next-value! element false)
		    (set-element-next-value! previous element)
		    (set-element-previous-value! element previous))
		   ((<= cost (element-cost next))
		    (set-element-next-value! element next)
		    (set-element-previous-value! next element)
		    (set-element-next-value! previous element)
		    (set-element-previous-value! element previous))
		   (else
		    (loop next (element-next-value next)))))))
    element))

(define (hash-table-delete! hash element)
  (if element
      (begin
       ;; **** Mark this element as removed.  [ref crock-1]
       (set-element-first-value! element false)
       (let ((next (element-next-value element))
	     (previous (element-previous-value element)))
	 (if next (set-element-previous-value! next previous))
	 (if previous
	     (set-element-next-value! previous next)
	     (let loop ((element next))
	       (if element
		   (begin (set-element-first-value! element next)
			  (loop (element-next-value element)))))))
       (let ((next (element-next-hash element))
	     (previous (element-previous-hash element)))
	 (if next (set-element-previous-hash! next previous))
	 (if previous
	     (set-element-next-hash! previous next)
	     (hash-table-set! hash next))))))

(define (hash-table-delete-class! predicate)
  (let table-loop ((i 0))
    (if (< i (hash-table-size))
	(let bucket-loop ((element (hash-table-ref i)))
	  (if element
	      (begin (if (predicate element)
			 (hash-table-delete! i element))
		     (bucket-loop (element-next-hash element)))
	      (table-loop (1+ i)))))))

(define hash-table-copy
  (let ()
    (define (copy-loop elements)
      (define (per-element element previous)
	(and element
	     (let ((element*
		    (%make-element (element-expression element)
				   (element-cost element)
				   (element-in-memory? element)
				   (per-element (element-next-hash element)
						element)
				   previous
				   (element-next-value element)
				   (element-previous-value element)
				   (element-first-value element)
				   element)))
	       (set-element-copy-cache! element element*)
	       element*)))
      (if (null? elements)
	  '()
	  (cons (per-element (car elements) false)
		(copy-loop (cdr elements)))))

    (define (update-values! elements)
      (define (per-element element)
	(if element
	    (begin (if (element-first-value element)
		       (set-element-first-value!
			element
			(element-copy-cache (element-first-value element))))
		   (if (element-previous-value element)
		       (set-element-previous-value!
			element
			(element-copy-cache (element-previous-value element))))
		   (if (element-next-value element)
		       (set-element-next-value!
			element
			(element-copy-cache (element-next-value element))))
		   (per-element (element-next-hash element)))))
      (if (not (null? elements))
	  (begin (per-element (car elements))
		 (update-values! (cdr elements)))))

    (define (reset-caches! elements)
      (define (per-element element)
	(if element
	    (begin (set-element-copy-cache! element false)
		   (per-element (element-next-hash element)))))
      (if (not (null? elements))
	  (begin (per-element (car elements))
		 (reset-caches! (cdr elements)))))

    (named-lambda (hash-table-copy table)
      (let ((elements (vector->list table)))
	(let ((elements* (copy-loop elements)))
	  (update-values! elements*)
	  (reset-caches! elements)
	  (list->vector elements*))))))
