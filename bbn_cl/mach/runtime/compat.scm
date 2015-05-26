;;; -*-Scheme-*-
;;;
;;;
;;;	Copyright (c) 1987 Massachusetts Institute of Technology
;;;
;;;	This material was developed by the Scheme project at the
;;;	Massachusetts Institute of Technology, Department of
;;;	Electrical Engineering and Computer Science.  Permission to
;;;	copy this software, to redistribute it, and to use it for any
;;;	purpose is granted, subject to the following restrictions and
;;;	understandings.
;;;
;;;	1. Any copy made of this software must include this copyright
;;;	notice in full.
;;;
;;;	2. Users of this software agree to make their best efforts (a)
;;;	to return to the MIT Scheme project any improvements or
;;;	extensions that they make, so that these may be included in
;;;	future releases; and (b) to inform MIT of noteworthy uses of
;;;	this software.
;;;
;;;	3. All materials developed as a consequence of the use of this
;;;	software shall duly acknowledge such use, in accordance with
;;;	the usual standards of acknowledging credit in academic
;;;	research.
;;;
;;;	4. MIT has made no warrantee or representation that the
;;;	operation of this software will be error-free, and MIT is
;;;	under no obligation to provide any services, by way of
;;;	maintenance, update, or otherwise.
;;;
;;;	5. In conjunction with products arising from the use of this
;;;	material, there shall be no use of the name of the
;;;	Massachusetts Institute of Technology nor of any adaptation
;;;	thereof in any advertising, promotional, or sales literature
;;;	without prior written consent from MIT in each case.
;;;

;;;; 6.001 Compatibility Definitions

(declare (usual-integrations))

(syntax-table-define system-global-syntax-table 'CONJUNCTION
		     (syntax-table-ref system-global-syntax-table 'AND))

(syntax-table-define system-global-syntax-table 'DISJUNCTION
		     (syntax-table-ref system-global-syntax-table 'OR))

(define (and* . args)
  (define (and-loop args)
    (or (null? args)
	(and (car args)
	     (and-loop (cdr args)))))
  (and-loop args))

(define (or* . args)
  (define (or-loop args)
    (and (not (null? args))
	 (or (car args)
	     (or-loop (cdr args)))))
  (or-loop args))

(define (applicable? object)
  (or (procedure? object)
      (continuation? object)))

(define (atom? object)
  (not (pair? object)))

(define nil false)
(define t true)

(define (nth n l)
  (list-ref l n))

(define (nthcdr n l)
  (list-tail l n))

(define user-memq (member-procedure eq?))
(define user-assq (association-procedure eq? car))

(define (close-channel port)
  (cond ((input-port? port) (close-input-port port))
	((output-port? port) (close-output-port port))
	(else (error "CLOSE-CHANNEL: Wrong type argument" port))))

(define (print object #!optional port)
  (cond ((unassigned? port) (set! port (current-output-port)))
	((not (output-port? port)) (error "Bad output port" port)))
  (if (not (eq? object *the-non-printing-object*))
      (begin ((access :write-char port) char:newline)
	     ((access unparse-object unparser-package) object port true)
	     ((access :write-char port) #\Space)))
  *the-non-printing-object*)

(define (tyi #!optional port)
  (if (unassigned? port) (set! port (current-input-port)))
  (let ((char (read-char port)))
    (if (eof-object? char)
	char
	(char->ascii char))))

(define (tyipeek #!optional port)
  (if (unassigned? port) (set! port (current-input-port)))
  (let ((char (peek-char port)))
    (if (eof-object? char)
	char
	(char->ascii char))))

(define (tyo ascii #!optional port)
  (if (unassigned? port) (set! port (current-output-port)))
  (write-char (ascii->char ascii) port))

(define (print-depth #!optional newval)
  (if (unassigned? newval) (set! newval false))
  (if (or (not newval)
	  (and (integer? newval)
	       (positive? newval)))
      (set! *unparser-list-depth-limit* newval)
      (error "PRINT-DEPTH: Wrong type argument" newval)))

(define (print-breadth #!optional newval)
  (if (unassigned? newval) (set! newval false))
  (if (or (not newval)
	  (and (integer? newval)
	       (positive? newval)))
      (set! *unparser-list-breadth-limit* newval)
      (error "PRINT-BREADTH: Wrong type argument" newval)))






