#| -*-Scheme-*-

$Header: rgproc.scm,v 1.2 88/08/31 10:43:14 jinx Exp $
$MIT-Header: rgproc.scm,v 4.2 88/03/14 20:54:09 GMT jinx Exp $

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

;;;; RTL Generation: Procedure Headers

(declare (usual-integrations))

(package (generate/procedure-header)

(define-export (generate/procedure-header procedure body inline?)
  (scfg*scfg->scfg!
   (if (procedure/ic? procedure)
       (scfg*scfg->scfg!
	(if inline?
	    (make-null-cfg)
	    (rtl:make-ic-procedure-header (procedure-label procedure)))
	(setup-ic-frame procedure))
       (scfg*scfg->scfg!
	(cond ((procedure/closure? procedure)
	       (if (procedure/trivial-closure? procedure)
		   (with-procedure-arity-encoding
		    procedure
		    (lambda (min max)
		      (rtl:make-procedure-header (procedure-label procedure)
						 min max)))
		   (rtl:make-closure-header (procedure-label procedure))))
	      (inline?
	       (make-null-cfg))
	      ((procedure-rest procedure)
	       (with-procedure-arity-encoding
		procedure
		(lambda (min max)
		  (rtl:make-procedure-header (procedure-label procedure)
					     min max))))
	      (else
	       (rtl:make-open-procedure-header (procedure-label procedure))))
	(setup-stack-frame procedure)))
   body))

(define (setup-ic-frame procedure)
  (scfg*->scfg!
   (map (let ((block (procedure-block procedure)))
	  (lambda (name value)
	    (generate/rvalue value 0 scfg*scfg->scfg!
	      (lambda (expression)
		(rtl:make-interpreter-call:set!
		 (rtl:make-fetch register:environment)
		 (intern-scode-variable! block (variable-name name))
		 expression)))))
	(procedure-names procedure)
	(procedure-values procedure))))

(define (setup-stack-frame procedure)
  (let ((block (procedure-block procedure)))
    (define (cellify-variables variables)
      (scfg*->scfg! (map cellify-variable variables)))

    (define (cellify-variable variable)
      (if (variable-in-cell? variable)
	  (let ((locative
		 (stack-locative-offset (rtl:make-fetch register:stack-pointer)
					(variable-offset block variable))))
	    (rtl:make-assignment
	     locative
	     (rtl:make-cell-cons (rtl:make-fetch locative))))
	  (make-null-cfg)))

    (let ((names (procedure-names procedure))
	  (values (procedure-values procedure)))
      (scfg-append!
       (setup-bindings names values '())
       (cellify-variables (procedure-required-arguments procedure))
       (cellify-variables (procedure-optional procedure))
       (let ((rest (procedure-rest procedure)))
	 (if rest
	     (cellify-variable rest)
	     (make-null-cfg)))
       (scfg*->scfg!
	(map (lambda (name value)
	       (if (and (procedure? value)
			(procedure/closure? value)
			(not (procedure/trivial-closure? value)))
		   (letrec-close block name value)
		   (make-null-cfg)))
	     names values))))))

(define (setup-bindings names values pushes)
  (if (null? names)
      (scfg*->scfg! pushes)
      (setup-bindings (cdr names)
		      (cdr values)
		      (letrec-value (car values)
		       (lambda (scfg expression)
			 (cons (scfg*scfg->scfg!
				scfg
				(make-auxiliary-push (car names) expression))
			       pushes))))))

(define (make-auxiliary-push variable value)
  (rtl:make-push (if (variable-in-cell? variable)
		     (rtl:make-cell-cons value)
		     value)))

(define (letrec-value value recvr)
  (cond ((constant? value)
	 (recvr (make-null-cfg)
		(rtl:make-constant (constant-value value))))
	((procedure? value)
	 (enqueue-procedure! value)
	 (case (procedure/type value)
	   ((CLOSURE)
	    (if (procedure/trivial-closure? value)
		(recvr (make-null-cfg)
		       (make-trivial-closure-cons value))
		(recvr (make-non-trivial-closure-cons value)
		       (rtl:interpreter-call-result:enclose))))
	   ((IC)
	    (recvr (make-null-cfg)
		   (make-ic-cons value)))
	   ((OPEN-EXTERNAL OPEN-INTERNAL)
	    (error "Letrec value is open procedure" value))
	   (else
	    (error "Unknown procedure type" value))))
	(else
	 (error "Unknown letrec binding value" value))))

(define (letrec-close block variable value)
  (load-closure-environment
   value 0
   (find-variable block variable 0
		  rtl:make-fetch
		  (lambda (nearest-ic-locative name)
		    (error "Missing closure variable" variable))
		  (lambda (name)
		    (error "Missing closure variable" variable)))))

;;; end GENERATE/PROCEDURE-HEADER
)
