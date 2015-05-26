#| -*-Scheme-*-

$Header: rules4.scm,v 1.2 88/08/31 10:46:19 jinx Exp $
$MIT-Header: rules4.scm,v 4.2 88/03/14 20:18:11 GMT jinx Exp $

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

;;;; LAP Generation Rules: Interpreter Calls

(declare (usual-integrations))

;;;; Interpreter Calls

(define-rule statement
  (INTERPRETER-CALL:ACCESS (? environment) (? name))
  (lookup-call entry:compiler-access environment name))

(define-rule statement
  (INTERPRETER-CALL:LOOKUP (? environment) (? name) (? safe?))
  (lookup-call (if safe? entry:compiler-safe-lookup entry:compiler-lookup)
	       environment name))

(define-rule statement
  (INTERPRETER-CALL:UNASSIGNED? (? environment) (? name))
  (lookup-call entry:compiler-unassigned? environment name))

(define-rule statement
  (INTERPRETER-CALL:UNBOUND? (? environment) (? name))
  (lookup-call entry:compiler-unbound? environment name))

(define (lookup-call entry environment name)
  (let ((set-environment (expression->machine-register! environment a0)))
    (let ((clear-map (clear-map!)))
      (LAP ,@set-environment
	   ,@clear-map
	   ,(load-constant name (INST-EA (A 1)))
	   (JSR ,entry)
	   ,@(make-external-label continuation-code-word (generate-label))))))

(define-rule statement
  (INTERPRETER-CALL:DEFINE (? environment) (? name) (? value))
  (QUALIFIER (not (eq? 'CONS-POINTER (car value))))
  (assignment-call:default entry:compiler-define environment name value))

(define-rule statement
  (INTERPRETER-CALL:SET! (? environment) (? name) (? value))
  (QUALIFIER (not (eq? 'CONS-POINTER (car value))))
  (assignment-call:default entry:compiler-set! environment name value))

(define (assignment-call:default entry environment name value)
  (let ((set-environment (expression->machine-register! environment a0)))
    (let ((set-value (expression->machine-register! value a2)))
      (let ((clear-map (clear-map!)))
	(LAP ,@set-environment
	     ,@set-value
	     ,@clear-map
	     ,(load-constant name (INST-EA (A 1)))
	     (JSR ,entry)
	     ,@(make-external-label continuation-code-word (generate-label)))))))

(define-rule statement
  (INTERPRETER-CALL:DEFINE (? environment) (? name)
			   (CONS-POINTER (CONSTANT (? type))
					 (REGISTER (? datum))))
  (assignment-call:cons-pointer entry:compiler-define environment name type
				datum))

(define-rule statement
  (INTERPRETER-CALL:SET! (? environment) (? name)
			 (CONS-POINTER (CONSTANT (? type))
				       (REGISTER (? datum))))
  (assignment-call:cons-pointer entry:compiler-set! environment name type
				datum))

(define (assignment-call:cons-pointer entry environment name type datum)
  (let ((set-environment (expression->machine-register! environment a0)))
    (let ((datum (coerce->any datum)))
      (let ((clear-map (clear-map!)))
	(LAP ,@set-environment
	     (MOV L ,datum ,reg:temp)
	     (MOV B (& ,type) ,reg:temp)
	     ,@clear-map
	     (MOV L ,reg:temp (A 2))
	     ,(load-constant name (INST-EA (A 1)))
	     (JSR ,entry)
	     ,@(make-external-label continuation-code-word (generate-label)))))))

(define-rule statement
  (INTERPRETER-CALL:DEFINE (? environment) (? name)
			   (CONS-POINTER (CONSTANT (? type))
					 (ENTRY:PROCEDURE (? label))))
  (assignment-call:cons-pointer entry:compiler-define environment name type
				label))

(define-rule statement
  (INTERPRETER-CALL:SET! (? environment) (? name)
			 (CONS-POINTER (CONSTANT (? type))
				       (ENTRY:PROCEDURE (? label))))
  (assignment-call:cons-pointer entry:compiler-set! environment name type
				label))

(define (assignment-call:cons-pointer entry environment name type label)
  (let ((set-environment (expression->machine-register! environment a0)))
    (LAP ,@set-environment
	 ,@(clear-map!)
	 (PEA (@PCR ,(rtl-procedure/external-label (label->object label))))
	 (MOV B (& ,type) (@A 7))
	 (MOV L (@A+ 7) (A 2))
	 ,(load-constant name (INST-EA (A 1)))
	 (JSR ,entry)
	 ,@(make-external-label continuation-code-word (generate-label)))))

(define-rule statement
  (INTERPRETER-CALL:CACHE-REFERENCE (? extension) (? safe?))
  (let ((set-extension (expression->machine-register! extension a0)))
    (let ((clear-map (clear-map!)))
      (LAP ,@set-extension
	   ,@clear-map
	   (JSR ,(if safe?
		     entry:compiler-safe-reference-trap
		     entry:compiler-reference-trap))
	   ,@(make-external-label continuation-code-word (generate-label))))))

(define-rule statement
  (INTERPRETER-CALL:CACHE-ASSIGNMENT (? extension) (? value))
  (QUALIFIER (not (eq? 'CONS-POINTER (car value))))
  (let ((set-extension (expression->machine-register! extension a0)))
    (let ((set-value (expression->machine-register! value a1)))
      (let ((clear-map (clear-map!)))
	(LAP ,@set-extension
	     ,@set-value
	     ,@clear-map
	     (JSR ,entry:compiler-assignment-trap)
	     ,@(make-external-label continuation-code-word (generate-label)))))))

(define-rule statement
  (INTERPRETER-CALL:CACHE-ASSIGNMENT (? extension)
				     (CONS-POINTER (CONSTANT (? type))
						   (REGISTER (? datum))))
  (let ((set-extension (expression->machine-register! extension a0)))
    (let ((datum (coerce->any datum)))
      (let ((clear-map (clear-map!)))
	(LAP ,@set-extension
	     (MOV L ,datum ,reg:temp)
	     (MOV B (& ,type) ,reg:temp)
	     ,@clear-map
	     (MOV L ,reg:temp (A 1))
	     (JSR ,entry:compiler-assignment-trap)
	     ,@(make-external-label continuation-code-word (generate-label)))))))

(define-rule statement
  (INTERPRETER-CALL:CACHE-ASSIGNMENT (? extension)
				     (CONS-POINTER (CONSTANT (? type))
						   (ENTRY:PROCEDURE (? label))))
  (let ((set-extension (expression->machine-register! extension a0)))
    (LAP ,@set-extension
	 ,@(clear-map!)
	 (PEA (@PCR ,(rtl-procedure/external-label (label->object label))))
	 (MOV B (& ,type) (@A 7))
	 (MOV L (@A+ 7) (A 1))
	 (JSR ,entry:compiler-assignment-trap)
	 ,@(make-external-label continuation-code-word (generate-label)))))

(define-rule statement
  (INTERPRETER-CALL:CACHE-UNASSIGNED? (? extension))
  (let ((set-extension (expression->machine-register! extension a0)))
    (let ((clear-map (clear-map!)))
      (LAP ,@set-extension
	   ,@clear-map
	   (JSR ,entry:compiler-unassigned?-trap)
	   ,@(make-external-label continuation-code-word (generate-label))))))
