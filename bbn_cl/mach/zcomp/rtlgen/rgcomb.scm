#| -*-Scheme-*-

$Header: rgcomb.scm,v 1.3 88/08/31 10:43:11 jinx Exp $
$MIT-Header: rgcomb.scm,v 4.4 88/03/14 20:53:42 GMT jinx Exp $

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

;;;; RTL Generation: Combinations

(declare (usual-integrations))

(package (generate/combination)

(define (generate/combination combination)
  (if (combination/inline? combination)
      (combination/inline combination)
      (combination/normal combination)))

(define (combination/normal combination)
  (let ((block (combination/block combination))
	(operator (combination/operator combination))
	(frame-size (combination/frame-size combination))
	(continuation (combination/continuation combination))
	(offset (node/offset combination)))
    (let ((callee (rvalue-known-value operator)))
      (let ((finish
	     (lambda (invocation callee-external?)
	       (invocation operator
			   offset
			   frame-size
			   (and (return-operator/subproblem? continuation)
				(continuation/label continuation))
			   (generate/invocation-prefix block
						       callee
						       continuation
						       callee-external?)))))
	(cond ((not callee)
	       (finish (if (reference? operator)
			   invocation/reference
			   invocation/apply)
		       true))
	      ((rvalue/constant? callee)
	       (finish
		(if (normal-primitive-procedure? (constant-value callee))
		    invocation/primitive
		    invocation/apply)
		true))
	      ((rvalue/procedure? callee)
	       (case (procedure/type callee)
		 ((OPEN-EXTERNAL) (finish invocation/jump true))
		 ((OPEN-INTERNAL) (finish invocation/jump false))
		 ((CLOSURE)
		  ;; *** For the time being, known lexpr closures are invoked
		  ;; through apply.  This makes the code simpler and probably
		  ;; does not matter much. ***
		  (if (procedure-rest callee)
		      (finish invocation/apply true)
		      (finish invocation/jump true)))
		 ((IC) (finish invocation/ic true))
		 (else (error "Unknown procedure type" callee))))
	      (else
	       (finish invocation/apply true)))))))

;;;; Invocations

(define (invocation/jump operator offset frame-size continuation prefix)
  (let ((callee (rvalue-known-value operator)))
    (scfg*scfg->scfg!
     (prefix offset frame-size)
     (if (procedure-inline-code? callee)
	 (generate/procedure-entry/inline callee)
	 (begin
	   (enqueue-procedure! callee)
	   ((if (procedure-rest callee)
		#|
		;; BBN has this code instead.
		;; We should not have to special-case optionals, since the
		;; caller can set up the correct target frame directly
		;; at no extra cost.
		(or (procedure-rest callee)
		    (procedure-optional callee))
		|#
		rtl:make-invocation:lexpr
		rtl:make-invocation:jump)
	    frame-size
	    continuation
	    (procedure-label callee)))))))

(define (invocation/apply operator offset frame-size continuation prefix)
  (invocation/apply* offset frame-size continuation prefix))

(define (invocation/apply* offset frame-size continuation prefix)
  (scfg*scfg->scfg! (prefix offset frame-size)
		    (rtl:make-invocation:apply frame-size continuation)))

(define invocation/ic
  ;; For now, use apply.  Later we can optimize for the cases where
  ;; the callee's closing frame is easily available, such as calling a
  ;; sibling, self-recursion, or an ancestor.
  invocation/apply)

(define (invocation/primitive operator offset frame-size continuation prefix)
  (scfg*scfg->scfg!
   (prefix offset frame-size)
   (let ((primitive (constant-value (rvalue-known-value operator))))
     ((or (special-primitive-handler primitive)
	  rtl:make-invocation:primitive)
      (1+ frame-size)
      continuation
      primitive))))

(package (invocation/reference)

(define-export (invocation/reference operator offset frame-size continuation
				     prefix)
  (if (reference-to-known-location? operator)
      (invocation/apply* offset frame-size continuation prefix)
      (let ((block (reference-block operator))
	    (variable (reference-lvalue operator)))
	(find-variable block variable offset
	  (lambda (locative)
	    (scfg*scfg->scfg!
	     (rtl:make-push (rtl:make-fetch locative))
	     (invocation/apply* (1+ offset)
				(1+ frame-size)
				continuation
				prefix)))
	  (lambda (environment name)
	    (invocation/lookup frame-size
			       continuation
			       (prefix offset frame-size)
			       environment
			       (intern-scode-variable! block name)))
	  (lambda (name)
	    (if (memq 'UUO-LINK (variable-declarations variable))
		(invocation/uuo-link frame-size
				     continuation
				     (prefix offset frame-size)
				     name)
		(invocation/cache-reference offset
					    frame-size
					    continuation
					    prefix
					    name)))))))

(define (invocation/lookup frame-size continuation prefix environment variable)
  (let ((make-invocation
	 (lambda (environment)
	   (expression-simplify-for-statement environment
	     (lambda (environment)
	       (rtl:make-invocation:lookup (1+ frame-size)
					   continuation
					   environment
					   variable))))))
    (if (cfg-null? prefix)
	(make-invocation environment)
	(scfg-append!
	 (rtl:make-assignment register:environment environment)
	 prefix
	 (make-invocation (rtl:make-fetch register:environment))))))

(define (invocation/uuo-link frame-size continuation prefix name)
  (scfg*scfg->scfg! prefix
		    (rtl:make-invocation:uuo-link (1+ frame-size)
						  continuation
						  name)))

(define (invocation/cache-reference offset frame-size continuation prefix name)
  (let* ((temp (rtl:make-pseudo-register))
	 (cell (rtl:make-fetch temp))
	 (contents (rtl:make-fetch cell))
	 (n1 (rtl:make-assignment temp (rtl:make-variable-cache name))))
    ;; n1 MUST be bound before the rest.  It flags temp as a
    ;; register that contains an address.
    (let ((n2
	   (rtl:make-type-test (rtl:make-object->type contents)
			       (ucode-type reference-trap)))
	  (n3
	   (scfg*scfg->scfg!
	    (rtl:make-push contents)
	    (invocation/apply* (1+ offset)
			       (1+ frame-size)
			       continuation
			       prefix)))
	  (n4
	   (scfg*scfg->scfg!
	    (prefix offset frame-size)
	    (expression-simplify-for-statement cell
	      (lambda (cell)
		(rtl:make-invocation:cache-reference (1+ frame-size)
						     continuation
						     cell))))))
      (scfg-next-connect! n1 n2)
      (pcfg-consequent-connect! n2 n4)
      (pcfg-alternative-connect! n2 n3)
      (make-scfg (cfg-entry-node n1)
		 (hooks-union (scfg-next-hooks n3)
			      (scfg-next-hooks n4))))))

;;; end INVOCATION/REFERENCE
)

;;;; Prefixes

(package (generate/invocation-prefix)

(define-export (generate/invocation-prefix block
					   callee
					   continuation
					   callee-external?)
  (prefix-append
   (generate/link-prefix block callee continuation callee-external?)
   (let ((caller (block-procedure block)))
     (cond ((or (return-operator/subproblem? continuation)
		(not (rvalue/procedure? caller))
		(procedure/ic? caller))
	    prefix/null)
	   ((procedure/external? caller)
	    (if callee-external?
		(invocation-prefix/move-frame-up block block)
		prefix/null))
	   (callee-external?
	    (invocation-prefix/erase-to block
					continuation
					(stack-block/external-ancestor block)))
	   (else
	    (let ((block* (procedure-block callee)))
	      (if (block-child? block block*)
		  prefix/null
		  (invocation-prefix/erase-to block
					      continuation
					      (block-farthest-uncommon-ancestor
					       block
					       (block-parent block*))))))))))

(define (prefix-append prefix prefix*)
  (lambda (offset frame-size)
    (scfg*scfg->scfg! (prefix offset frame-size) (prefix* offset frame-size))))

(define (prefix/null offset frame-size)
  (make-null-cfg))

(define (generate/link-prefix block callee continuation callee-external?)
  (cond ((not (and (not callee-external?)
		   (internal-block/dynamic-link? (procedure-block callee))))
	 prefix/null)
	((return-operator/subproblem? continuation)
	 link-prefix/subproblem)
	((block/dynamic-link? block)
	 prefix/null)
	(else
	 (link-prefix/reduction
	  block
	  (reduction-continuation/popping-limit continuation)))))

(define (link-prefix/subproblem offset frame-size)
  (rtl:make-assignment
   register:dynamic-link
   (rtl:make-address
    (stack-locative-offset (rtl:make-fetch register:stack-pointer)
			   frame-size))))

(define (link-prefix/reduction block block*)
  (lambda (offset frame-size)
    (rtl:make-assignment register:dynamic-link
			 (popping-limit/locative block offset block* 0))))

(define (invocation-prefix/erase-to block continuation callee-limit)
  (let ((popping-limit (reduction-continuation/popping-limit continuation)))
    (if popping-limit
	(invocation-prefix/move-frame-up block
					 (if (block-ancestor? callee-limit
							      popping-limit)
					     callee-limit
					     popping-limit))
	(invocation-prefix/dynamic-link block callee-limit))))

(define (invocation-prefix/move-frame-up block block*)
  (lambda (offset frame-size)
    (expression-simplify-for-statement
     (popping-limit/locative block offset block* 0)
     (lambda (locative)
       (rtl:make-invocation-prefix:move-frame-up frame-size locative)))))

(define (invocation-prefix/dynamic-link block block*)
  (lambda (offset frame-size)
    (expression-simplify-for-statement
     (popping-limit/locative block offset block* 0)
     (lambda (locative)
       (expression-simplify-for-statement (interpreter-dynamic-link)
	 (lambda (dynamic-link)
	   (rtl:make-invocation-prefix:dynamic-link frame-size
						    locative
						    dynamic-link)))))))

;;; end GENERATE/INVOCATION-PREFIX
)

;;; end GENERATE/COMBINATION
)
