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

;;;; History Manipulation

(declare (usual-integrations))

(define max-subproblems 10)
(define max-reductions 5)
(define with-new-history)

(define history-package
  (let ((set-current-history!
	 (make-primitive-procedure 'SET-CURRENT-HISTORY!))
	(return-address-pop-from-compiled-code
	 (make-return-address
	  (microcode-return 'POP-FROM-COMPILED-CODE)))
	(hunk:make (make-primitive-procedure 'HUNK3-CONS))
	(type-code:unmarked-history (microcode-type 'unmarked-history))
	(type-code:marked-history (microcode-type 'marked-history))

	;; VERTEBRA abstraction.
	(vertebra-rib system-hunk3-cxr0)
	(shallower-vertebra system-hunk3-cxr2)
	(set-vertebra-rib! system-hunk3-set-cxr0!)
	(set-deeper-vertebra! system-hunk3-set-cxr1!)
	(set-shallower-vertebra! system-hunk3-set-cxr2!)

	;; REDUCTION abstraction.
	(reduction-expression system-hunk3-cxr0)
	(reduction-environment system-hunk3-cxr1)
	(set-reduction-expression! system-hunk3-set-cxr0!)
	(set-reduction-environment! system-hunk3-set-cxr1!)
	(set-next-reduction! system-hunk3-set-cxr2!))

(declare (integrate-primitive-procedures
	  (hunk:make hunk3-cons)
	  (vertebra-rib system-hunk3-cxr0)
	  (shallower-vertebra system-hunk3-cxr2)
	  (set-vertebra-rib! system-hunk3-set-cxr0!)
	  (set-deeper-vertebra! system-hunk3-set-cxr1!)
	  (set-shallower-vertebra! system-hunk3-set-cxr2!)
	  (reduction-expression system-hunk3-cxr0)
	  (reduction-environment system-hunk3-cxr1)
	  (set-reduction-expression! system-hunk3-set-cxr0!)
	  (set-reduction-environment! system-hunk3-set-cxr1!)
	  (set-next-reduction! system-hunk3-set-cxr2!))

	 (integrate-operator history:mark history:unmark history:marked?))

(define (history:unmark object)
  (declare (integrate object))
  (primitive-set-type type-code:unmarked-history object))

(define (history:mark object)
  (declare (integrate object))
  (primitive-set-type type-code:marked-history object))

(define (history:marked? object)
  (declare (integrate object))
  (primitive-type? type-code:marked-history object))

;;; Vertebra operations

(declare (integrate-operator make-vertebra same-vertebra?))

(define (make-vertebra rib deeper shallower)
  (declare (integrate rib deeper shallower))
  (history:unmark (hunk:make rib deeper shallower)))

(define (deeper-vertebra vertebra)
  (system-hunk3-cxr1 vertebra))

(define (marked-vertebra? vertebra)
  (history:marked? (system-hunk3-cxr1 vertebra)))

(define (mark-vertebra! vertebra)
  (system-hunk3-set-cxr1!
   vertebra
   (history:mark (system-hunk3-cxr1 vertebra))))

(define (unmark-vertebra! vertebra)
  (system-hunk3-set-cxr1! vertebra
			  (history:unmark (system-hunk3-cxr1 vertebra))))

(define (same-vertebra? x y)
  (declare (integrate x y))
  (= (primitive-datum x) (primitive-datum y)))

(define (link-vertebrae previous next)
  (set-deeper-vertebra! previous next)
  (set-shallower-vertebra! next previous))

;;; Reduction operations

(declare (integrate-operator make-reduction same-reduction?))

(define (make-reduction expression environment next)
  (declare (integrate expression environment next))
  (history:unmark (hunk:make expression environment next)))

(define (next-reduction reduction)
  (system-hunk3-cxr2 reduction))

(define (marked-reduction? reduction)
  (history:marked? (system-hunk3-cxr2 reduction)))

(define (mark-reduction! reduction)
  (system-hunk3-set-cxr2!
   reduction
   (history:mark (system-hunk3-cxr2 reduction))))

(define (unmark-reduction! reduction)
  (system-hunk3-set-cxr2! reduction
			  (history:unmark (system-hunk3-cxr2 reduction))))

(define (same-reduction? x y)
  (declare (integrate x y))
  (= (primitive-datum x) (primitive-datum y)))

;;;; History Initialization

(define (create-history depth width)
  (define (new-vertebra)
    (let ((head (make-reduction false false '())))
      (set-next-reduction!
       head
       (let reduction-loop ((n (-1+ width)))
	 (if (zero? n)
	     head
	     (make-reduction false false (reduction-loop (-1+ n))))))
      (make-vertebra head '() '())))

  (cond ((or (not (integer? depth))
	     (negative? depth))
	 (error "Invalid Depth" 'CREATE-HISTORY depth))
	((or (not (integer? width))
	     (negative? width))
	 (error "Invalid Width" 'CREATE-HISTORY width))
	(else
	 (if (or (zero? depth) (zero? width))
	     (begin (set! depth 1) (set! width 1)))
	 (let ((head (new-vertebra)))
	   (let subproblem-loop ((n (-1+ depth))
				 (previous head))
	     (if (zero? n)
		 (link-vertebrae previous head)
		 (let ((next (new-vertebra)))
		   (link-vertebrae previous next)
		   (subproblem-loop (-1+ n) next))))
	   head))))

;;; The PUSH-HISTORY! accounts for the pop which happens after
;;; SET-CURRENT-HISTORY! is run.

(set! with-new-history
  (named-lambda (with-new-history thunk)
    (set-current-history!
     (let ((history
	    (push-history! (create-history max-subproblems
					   max-reductions))))
       (if (zero? max-subproblems)

	   ;; In this case, we want the history to appear empty,
	   ;; so when it pops up, there is nothing in it.
	   history

	   ;; Otherwise, record a dummy reduction, which will appear
	   ;; in the history.
	   (begin
	    (record-evaluation-in-history! history
					   (scode-quote #F)
					   system-global-environment)
	    (push-history! history)))))
    (thunk)))

;;;; Primitive History Operations
;;;  These operations mimic the actions of the microcode.
;;;  The history motion operations all return the new history.

(define (record-evaluation-in-history! history expression environment)
  (let ((current-reduction (vertebra-rib history)))
    (set-reduction-expression! current-reduction expression)
    (set-reduction-environment! current-reduction environment)))

(define (set-history-to-next-reduction! history)
  (let ((next-reduction (next-reduction (vertebra-rib history))))
    (set-vertebra-rib! history next-reduction)
    (unmark-reduction! next-reduction)
    history))

(define (push-history! history)
  (let ((deeper-vertebra (deeper-vertebra history)))
    (mark-vertebra! deeper-vertebra)
    (mark-reduction! (vertebra-rib deeper-vertebra))
    deeper-vertebra))

(define (pop-history! history)
  (unmark-vertebra! history)
  (shallower-vertebra history))

;;;; Side-Effectless Examiners

(define (history-transform history)
  (let loop ((current history))
    (cons current
	  (if (marked-vertebra? current)
	      (cons (delay (unfold-and-reverse-rib (vertebra-rib current)))
		    (delay (let ((next (shallower-vertebra current)))
			     (if (same-vertebra? next history)
				 the-empty-history
				 (loop next)))))
	      '()))))

(define (unfold-and-reverse-rib rib)
  (let loop ((current (next-reduction rib)) (output 'WRAP-AROUND))
    (let ((step
	   (let ((tail
		  (if (marked-reduction? current)
		      '()
		      output)))
	     (if (dummy-compiler-reduction? current)
		 tail
		 (cons (list (reduction-expression current)
			     (reduction-environment current))
		       tail)))))
      (if (same-reduction? current rib)
	  step
	  (loop (next-reduction current) step)))))

(define (dummy-compiler-reduction? reduction)
  (and (null? (reduction-expression reduction))
       (eq? return-address-pop-from-compiled-code
	    (reduction-environment reduction))))

(define the-empty-history
  (cons (vector-ref (get-fixed-objects-vector)
		    (fixed-objects-vector-slot 'DUMMY-HISTORY))
	'()))

(define (history-superproblem history)
  (if (null? (cdr history))
      history
      (force (cddr history))))

(define (history-reductions history)
  (if (null? (cdr history))
      '()
      (force (cadr history))))

(define (history-untransform history)
  (car history))

;;; end HISTORY-PACKAGE.
(the-environment)))
