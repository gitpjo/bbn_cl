#| -*-Scheme-*-

$Header: symtab.scm,v 1.2 88/08/31 10:33:14 jinx Exp $
$MIT-Header: symtab.scm,v 1.43 87/07/15 02:59:21 GMT jinx Exp $

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

;;;; Symbol Tables

(declare (usual-integrations))

(define-integrable (make-symbol-table)
  (symbol-hash-table/make 271))

(define (symbol-table-define! table key value)
  (symbol-hash-table/modify! table key
    (lambda (binding)
      (error "symbol-table-define!: Redefining" key)
      (set-binding-value! binding value)
      binding)
    (lambda ()
      (make-binding value))))

(define (symbol-table-value table key)
  (symbol-hash-table/lookup* table key
    (lambda (binding)
      (or (binding-value binding)
	  (error "SYMBOL-TABLE-VALUE: no value" key)))
    (lambda ()
      (error "SYMBOL-TABLE-VALUE: Undefined key" key))))

(define (symbol-table->assq-list table)
  (map (lambda (pair)
	 (cons (car pair) (binding-value (cdr pair))))
       (symbol-table-bindings table)))

(define-integrable (symbol-table-bindings table)
  (symbol-hash-table/bindings table))

(define-integrable (make-binding initial-value)
  (cons initial-value '()))

(define-integrable (binding-value binding)
  (car binding))

(define (set-binding-value! binding value)
  (set-car! binding value))
