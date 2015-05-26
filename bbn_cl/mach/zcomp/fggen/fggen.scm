#| -*-Scheme-*-

$Header: fggen.scm,v 1.5 88/12/15 10:16:25 las Exp $
$MIT-Header: fggen.scm,v 4.4 88/03/14 20:48:00 GMT jinx Exp $

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

;;;; Flow Graph Generation

(declare (usual-integrations))

(define (construct-graph scode)
  (fluid-let ((*virtual-continuations* '()))
    (let ((block (make-block false 'EXPRESSION)))
      (let ((continuation (make-continuation-variable block)))
	(let ((expression
	       (make-expression
		block
		continuation
		(transmit-values
		    (if (scode/open-block? scode)
			(scode/open-block-components scode
			  (lambda (names declarations body)
			    (return-3 (make-variables block names)
				      declarations
				      (unscan-defines names '() body))))
			(return-3 '() '() scode))
		  (lambda (variables declarations scode)
		    (set-block-bound-variables! block variables)
		    (generate/body block continuation declarations scode))))))
	  ;; Delete as many noop nodes as possible.
	  (for-each
	   (lambda (procedure)
	     (if (procedure-continuation? procedure)
		 (let ((next (snode-next (procedure-entry-node procedure))))
		   (if next
		       (set-procedure-entry-node! procedure next)))))
	   *procedures*)
	  (for-each (lambda (continuation)
		      (set-virtual-continuation/parent! continuation false))
		    *virtual-continuations*)
	  expression)))))

(define (make-variables block names)
  (map (lambda (name) (make-variable block name)) names))

(define (generate/body block continuation declarations expression)
  ;; The call to `process-declarations!' must come after the
  ;; expression is generated because it can refer to the set of free
  ;; variables in the expression.
  (let ((node (generate/expression block continuation expression)))
    (process-top-level-declarations! block declarations)
    node))

;;;; Continuations

(define (continuation/case continuation unknown effect predicate value)
  (cond ((variable? continuation)
	 (let ((type (continuation-variable/type continuation)))
	   (cond ((not type) unknown)
		 ((eq? type continuation-type/effect) effect)
		 ((eq? type continuation-type/predicate) unknown)
		 ((eq? type continuation-type/value) unknown)
		 (else (error "Illegal continuation type" type)))))
	((virtual-continuation? continuation)
	 (if (virtual-continuation/reified? continuation)
	     (continuation/case (virtual-continuation/reification continuation)
				unknown
				effect
				predicate
				value)
	     (let ((type (virtual-continuation/type continuation)))
	       (cond ((eq? type continuation-type/effect) effect)
		     ((eq? type continuation-type/predicate) predicate)
		     ((eq? type continuation-type/value) value)
		     (else
		      (error "Illegal virtual continuation type" type))))))
	((procedure? continuation)
	 (let ((type (continuation/type continuation)))
	   (cond ((eq? type continuation-type/effect) effect)
		 ((eq? type continuation-type/predicate) predicate)
		 ((eq? type continuation-type/value) value)
		 (else (error "Illegal continuation type" type)))))
	(else (error "Illegal continuation" continuation))))

(define (continuation/known-type continuation)
  (cond ((variable? continuation)
	 (continuation-variable/type continuation))
	((virtual-continuation? continuation)
	 (virtual-continuation/type continuation))
	((procedure? continuation)
	 (continuation/type continuation))
	(else
	 (error "Illegal continuation" continuation))))

(define (continuation/type? continuation type)
  (cond ((variable? continuation)
	 (eq? (continuation-variable/type continuation) type))
	((virtual-continuation? continuation)
	 (eq? (virtual-continuation/type continuation) type))
	((procedure? continuation)
	 (eq? (continuation/type continuation) type))
	(else
	 (error "Illegal continuation" continuation))))

(define-integrable (continuation/effect? continuation)
  (continuation/type? continuation continuation-type/effect))

(define-integrable (continuation/predicate? continuation)
  (continuation/type? continuation continuation-type/predicate))

(define (continuation/rvalue continuation)
  (make-reference (continuation/block continuation)
		  (continuation/parameter continuation)
		  true))

(define-integrable (continuation/next-hooks continuation)
  (list (make-hook (continuation/entry-node continuation)
		   set-snode-next-edge!)))

(define-integrable (continuation-reference block continuation)
  (cond ((variable? continuation) (make-reference block continuation true))
	((procedure? continuation) continuation)
	(else (error "Illegal continuation" continuation))))

;;;; Subproblems

(define (with-reified-continuation block
				   continuation
				   scfg*value->value!
				   generator)
  (if (virtual-continuation? continuation)
      (let ((continuation (virtual-continuation/reify! continuation)))
	(scfg*value->value! (make-push block continuation)
			    (generator continuation)))
      (generator continuation)))

(define (make-subproblem/canonical prefix continuation)
  (make-subproblem prefix
		   continuation
		   (continuation/rvalue continuation)))

(define (scfg*subproblem->subproblem! scfg subproblem)
  (make-subproblem (scfg*scfg->scfg! scfg (subproblem-prefix subproblem))
		   (subproblem-continuation subproblem)
		   (subproblem-rvalue subproblem)))

(define *virtual-continuations*)

(define (virtual-continuation/make block parent type)
  (let ((continuation (virtual-continuation/%make block parent type)))
    (set! *virtual-continuations* (cons continuation *virtual-continuations*))
    continuation))

(define (wrapper/subproblem type)
  (lambda (block continuation generator)
    (generator (virtual-continuation/make block continuation type))))

(define wrapper/subproblem/effect
  (wrapper/subproblem continuation-type/effect))

(define wrapper/subproblem/predicate
  (wrapper/subproblem continuation-type/predicate))

(define wrapper/subproblem/value
  (wrapper/subproblem continuation-type/value))

(define (generator/subproblem wrapper)
  (lambda (block continuation expression)
    (wrapper block continuation
      (lambda (continuation)
	(generate/expression block continuation expression)))))

(define generate/subproblem/effect
  (generator/subproblem wrapper/subproblem/effect))

(define generate/subproblem/predicate
  (generator/subproblem wrapper/subproblem/predicate))

(define generate/subproblem/value
  (generator/subproblem wrapper/subproblem/value))

;;;; Values

(define (generate/constant block continuation expression)
  (continue/rvalue-constant block continuation (make-constant expression)))

(define (generate/the-environment block continuation expression)
  (continue/rvalue-constant block continuation block))

(define (continue/rvalue-constant block continuation rvalue)
  ((continuation/case continuation
		      continue/unknown
		      continue/effect
		      continue/predicate-constant
		      continue/value)
   block
   continuation
   rvalue))

(define (continue/predicate-constant block continuation rvalue)
  (if (and (rvalue/constant? rvalue)
	   (false? (constant-value rvalue)))
      (snode->pcfg-false (make-fg-noop))
      (snode->pcfg-true (make-fg-noop))))

(define (continue/rvalue block continuation rvalue)
  ((continuation/case continuation
		      continue/unknown
		      continue/effect
		      continue/predicate
		      continue/value)
   block
   continuation
   rvalue))

(define (continue/unknown block continuation rvalue)
  (make-return block (make-reference block continuation true) rvalue))

(define (continue/effect block continuation rvalue)
  (if (variable? continuation)
      (continue/unknown block continuation (make-constant false))
      (make-null-cfg)))

(define-integrable (continue/predicate block continuation rvalue)
  (make-true-test rvalue))

(define (continue/value block continuation rvalue)
  (if (virtual-continuation? continuation)
      (make-subproblem (make-null-cfg) continuation rvalue)
      (make-subproblem/canonical (make-return block continuation rvalue)
				 continuation)))

(define (generate/variable block continuation expression)
  (continue/rvalue block
		   continuation
		   (make-reference block
				   (find-name block
					      (scode/variable-name expression))
				   false)))

(define (generate/safe-variable block continuation expression)
  (continue/rvalue
   block
   continuation
   (make-reference block
		   (find-name block (scode/safe-variable-name expression))
		   true)))

(define-integrable (scode/make-safe-variable name)
  (cons safe-variable-tag name))

(define-integrable (scode/safe-variable-name reference)
  (cdr reference))

(define safe-variable-tag
  "safe-variable")

(define (generate/unassigned? block continuation expression)
  (if (continuation/predicate? continuation)
      (continue/rvalue block
		       continuation
		       (make-unassigned-test
			block
			(find-name block (scode/unassigned?-name expression))))
      (generate/conditional block
			    continuation
			    (scode/make-conditional expression #T #F))))

(define (find-name block name)
  (define (search block)
    (or (variable-assoc name (block-bound-variables block))
	(variable-assoc name (block-free-variables block))
	(let ((variable
	       (if (block-parent block)
		   (search (block-parent block))
		   (make-variable block name))))
	  (set-block-free-variables! block
				     (cons variable
					   (block-free-variables block)))
	  variable)))
  (search block))

(define (generate/lambda block continuation expression)
  (generate/lambda* block continuation expression false))

(define (generate/lambda* block continuation expression continuation-type)
  (continue/rvalue-constant
   block
   continuation
   (scode/lambda-components expression
     (lambda (name required optional rest auxiliary declarations body)
       (transmit-values (parse-procedure-body auxiliary body)
	 (lambda (names values body)
	   (let ((block (make-block block 'PROCEDURE)))
	     (let ((continuation (make-continuation-variable block))
		   (required (make-variables block required))
		   (optional (make-variables block optional))
		   (rest (and rest (make-variable block rest)))
		   (names (make-variables block names)))
	       (set-continuation-variable/type! continuation continuation-type)
	       (set-block-bound-variables! block
					   `(,continuation
					     ,@required
					     ,@optional
					     ,@(if rest (list rest) '())
					     ,@names))
	       (make-procedure
		continuation-type/procedure
		block name (cons continuation required) optional rest names
		(map (lambda (value)
		       ;; The other parts of this subproblem are not
		       ;; interesting since `value' is guaranteed to
		       ;; be either a constant or a procedure.
		       (subproblem-rvalue
			(generate/subproblem/value block continuation value)))
		     values)
		(generate/body block continuation declarations body))))))))))

(define (parse-procedure-body auxiliary body)
  (transmit-values
      (parse-procedure-body* auxiliary (scode/sequence-actions body))
    (lambda (names values auxiliary actions)
      (if (null? auxiliary)
	  (return-3 names values (scode/make-sequence actions))
	  (return-3 '() '()
		    (scode/make-combination
		     (scode/make-lambda
		      lambda-tag:let auxiliary '() false names '()
		      (scode/make-sequence
		       (map* actions scode/make-assignment names values)))
		     (map (lambda (name) (scode/make-unassigned-object))
			  auxiliary)))))))

(define (parse-procedure-body* names actions)
  ;; Extract any definitions that do not depend on the order of
  ;; events.
  (cond ((null? names)
	 (return-4 '() '() '() actions))
	((null? actions)
	 (error "Extraneous auxiliaries" names))

	;; Because `scan-defines' returns the auxiliary names in a
	;; particular order, we can expect to encounter them in that
	;; same order when looking through the body's actions.

	((and (scode/assignment? (car actions))
	      (eq? (scode/assignment-name (car actions)) (car names)))
	 (transmit-values (parse-procedure-body* (cdr names) (cdr actions))
	   (let ((value (scode/assignment-value (car actions))))
	     (if (or (scode/lambda? value)
		     (scode/delay? value)
		     (scode/constant? value))
		 (lambda (names* values auxiliary actions*)
		   (return-4 (cons (car names) names*)
			     (cons value values)
			     auxiliary
			     (if (null? actions*)
				 (list undefined-conditional-branch)
				 actions*)))
		 (lambda (names* values auxiliary actions*)
		   (return-4 names*
			     values
			     (cons (car names) auxiliary)
			     (cons (car actions) actions*)))))))
	(else
	 (transmit-values (parse-procedure-body* names (cdr actions))
	   (lambda (names* values auxiliary actions*)
	     (return-4 names*
		       values
		       auxiliary
		       (cons (car actions) actions*)))))))

;;;; Combinators

(define (generate/sequence block continuation expression)
  (let ((join
	 (continuation/case continuation
			    scfg*scfg->scfg!
			    scfg*scfg->scfg!
			    scfg*pcfg->pcfg!
			    scfg*subproblem->subproblem!)))
    (let loop ((actions (scode/sequence-actions expression)))
      (if (null? (cdr actions))
	  (generate/expression block continuation (car actions))
	  (join (generate/subproblem/effect block continuation (car actions))
		(loop (cdr actions)))))))

(define (generate/conditional block continuation expression)
  (scode/conditional-components expression
    (lambda (predicate consequent alternative)
      (let ((predicate
	     (generate/subproblem/predicate block continuation predicate)))
	(let ((simple
	       (lambda (hooks branch)
		 ((continuation/case continuation
				     scfg*scfg->scfg!
				     scfg*scfg->scfg!
				     scfg*pcfg->pcfg!
				     scfg*subproblem->subproblem!)
		  (make-scfg (cfg-entry-node predicate) hooks)
		  (generate/expression block continuation branch)))))
	  (cond ((hooks-null? (pcfg-consequent-hooks predicate))
		 (simple (pcfg-alternative-hooks predicate) alternative))
		((hooks-null? (pcfg-alternative-hooks predicate))
		 (simple (pcfg-consequent-hooks predicate) consequent))
		(else
		 (let ((finish
			(lambda (continuation combiner)
			  (combiner
			   predicate
			   (generate/expression block
						continuation
						consequent)
			   (generate/expression block
						continuation
						alternative)))))
		   ((continuation/case continuation
		      (lambda () (finish continuation pcfg*scfg->scfg!))
		      (lambda () (finish continuation pcfg*scfg->scfg!))
		      (lambda () (finish continuation pcfg*pcfg->pcfg!))
		      (lambda ()
			(with-reified-continuation block
						   continuation
						   scfg*subproblem->subproblem!
			  (lambda (continuation)
			    (finish continuation
			      (lambda (predicate consequent alternative)
				(make-subproblem/canonical
				 (pcfg*scfg->scfg!
				  predicate
				  (subproblem-prefix consequent)
				  (subproblem-prefix alternative))
				 continuation))))))))))))))))

(define (generate/combination block continuation expression)
  (scode/combination-components expression
    (lambda (operator operands)
      (if (and (scode/variable? operator)
	       (let ((name (scode/variable-name operator)))
		 (or (eq? name 'structure-ref)
		     (eq? name 'structure-set!))))
	  (let* ((name (scode/variable-name operator))
		 (ref? (eq? name 'structure-ref))
		 (op-type (if ref? 'accessor 'setter))
		 (prim-name (if ref? system-vector-ref cl-system-vector-set!))
		 (dd (if ref? (second operands) (third operands)))
		 (dsd (if ref? (third operands) (fourth operands))))
	    (structure-operation-info (list name op-type dd dsd)
	      (lambda (op-type slot-type index dd dsd)
		(generate/combination 
		 block continuation
		 (scode/make-combination
		  prim-name
		  (if ref?
		      (list (car operands) index)
		      (list (car operands) index (cadr operands))))))))
	  (let ((make-combination
		 (lambda (continuation)
		   (make-combination
		    block
		    (continuation-reference block continuation)
		    (generate/operator block continuation operator)
		    (map (lambda (expression)
			   (generate/subproblem/value block
						      continuation
						      expression))
			 operands)))))
	    ((continuation/case continuation
              (lambda () (make-combination continuation))
	      (lambda ()
		(if (variable? continuation)
		    (make-combination continuation)
		    (with-reified-continuation block
                      continuation
		      scfg*scfg->scfg!
		      (lambda (continuation)
			(make-scfg
			 (cfg-entry-node (make-combination continuation))
			 (continuation/next-hooks continuation))))))
                           (lambda ()
			     (if (eq? not operator)
				 (pcfg-invert
				  (generate/expression block continuation (car operands)))
				 (with-reified-continuation block
                                   continuation
				   scfg*pcfg->pcfg!
				   (lambda (continuation)
				     (scfg*pcfg->pcfg!
				      (make-scfg
				       (cfg-entry-node (make-combination continuation))
				       (continuation/next-hooks continuation))
				      (make-true-test (continuation/rvalue continuation)))))))
				(lambda ()
				  (with-reified-continuation block
                                    continuation
				    scfg*subproblem->subproblem!
				    (lambda (continuation)
				      (make-subproblem/canonical (make-combination continuation)
								 continuation)))))))))))

(define (generate/operator block continuation operator)
  (wrapper/subproblem/value block continuation
    (lambda (continuation*)
      (if (scode/lambda? operator)
	  (generate/lambda* block
			    continuation*
			    operator
			    (continuation/known-type continuation))
	  (generate/expression block
			       continuation*
			       operator)))))

;;;; Assignments

(define (generate/assignment* maker find-name block continuation name value)
  (let ((subproblem (generate/subproblem/value block continuation value)))
    (scfg-append!
     (if (subproblem-canonical? subproblem)
	 (make-scfg
	  (cfg-entry-node (subproblem-prefix subproblem))
	  (continuation/next-hooks (subproblem-continuation subproblem)))
	 (subproblem-prefix subproblem))
     (maker block (find-name block name) (subproblem-rvalue subproblem))
     (continue/effect block continuation false))))

(define (generate/assignment block continuation expression)
  (scode/assignment-components expression
    (lambda (name value)
      (if (continuation/effect? continuation)
	  (generate/assignment* make-assignment find-name
				block continuation name value)
	  (generate/combination
	   block
	   continuation
	   (let ((old-value-temp (generate-uninterned-symbol))
		 (new-value-temp (generate-uninterned-symbol)))
	     (scode/make-let (list old-value-temp new-value-temp)
			     (list (scode/make-safe-variable name) value)
			     (scode/make-assignment
			      name
			      (scode/make-variable new-value-temp))
			     (scode/make-variable old-value-temp))))))))

(define (generate/definition block continuation expression)
  (scode/definition-components expression
    (lambda (name value)
      (if (continuation/effect? continuation)
	  (generate/assignment* make-definition make-definition-variable
				block continuation name
				(insert-letrec name value))
	  (generate/sequence block
			     continuation
			     (scode/make-sequence
			      (list (scode/make-definition name value)
				    name)))))))

(define (make-definition-variable block name)
  (let ((bound (block-bound-variables block)))
    (or (variable-assoc name bound)
	(let ((variable (make-variable block name)))
	  (set-block-bound-variables! block (cons variable bound))
	  variable))))

(define (insert-letrec name value)
  (if (and compiler:implicit-self-static?
	   (scode/lambda? value))
      (scode/make-let '() '()
		      (scode/make-definition name value)
		      (scode/make-variable name))
      value))

;;;; Rewrites

(define (generate/disjunction block continuation expression)
  ((continuation/case continuation
		      generate/disjunction/value
		      generate/disjunction/control
		      generate/disjunction/control
		      generate/disjunction/value)
   block continuation expression))

(define (generate/disjunction/control block continuation expression)
  (scode/disjunction-components expression
    (lambda (predicate alternative)
      (generate/conditional
       block
       continuation
       (scode/make-conditional predicate (make-constant true) alternative)))))

(define (generate/disjunction/value block continuation expression)
  (scode/disjunction-components expression
    (lambda (predicate alternative)
      (generate/combination
       block
       continuation
       (let ((temp (generate-uninterned-symbol)))
	 (scode/make-let (list temp)
			 (list predicate)
			 (let ((predicate (scode/make-variable temp)))
			   (scode/make-conditional predicate
						   predicate
						   alternative))))))))

(define (generate/access block continuation expression)
  (scode/access-components expression
    (lambda (environment name)
      (generate/combination
       block
       continuation
       (scode/make-combination (ucode-primitive lexical-reference)
			       (list environment name))))))

(define (generate/comment block continuation expression)
  (generate/expression block
		       continuation
		       (scode/comment-expression expression)))

(define (generate/delay block continuation expression)
  (generate/combination
   block
   continuation
   (scode/make-combination
    (ucode-primitive system-pair-cons)
    (list (ucode-type delayed)
	  0
	  (scode/make-lambda lambda-tag:unnamed '() '() false '() '()
			     (scode/delay-expression expression))))))

(define (generate/error-combination block continuation expression)
  (scode/error-combination-components expression
    (lambda (message irritants)
      (generate/combination
       block
       continuation
       (scode/make-combination compiled-error-procedure
			       (cons message irritants))))))

;; For now

(define (compile-recursively expression block)
  (error "compile-recursively: invoked!" expression))

(define (compile-recursively? block)
  false)

(define (generate/in-package block continuation expression)
  (let ((recursive? (compile-recursively? block)))
    (if (not recursive?)
	(warn "dynamic IN-PACKAGE not supported; body will be interpreted"
	      expression))
    (scode/in-package-components expression
     (lambda (environment expression)
       (generate/combination
	block
	continuation
	(scode/make-combination
	 (ucode-primitive scode-eval)
	 (list (if recursive?
		   (scode/make-constant
		    (compile-recursively expression false))
		   (scode/make-quotation expression))
	       environment)))))))

(define (generate/quotation block continuation expression)
  (generate/combination
   block
   continuation
   (scode/make-combination
    (ucode-primitive car)
    (list (list (scode/quotation-expression expression))))))

;;;; Dispatcher

(define generate/expression
  (let ((dispatch-vector
	 (make-vector number-of-microcode-types generate/constant))
	(generate/combination
	 (lambda (block continuation expression)
	   (let ((operator (scode/combination-operator expression))
		 (operands (scode/combination-operands expression)))
	     (cond ((and (eq? operator (ucode-primitive lexical-unassigned?))
			 (the-environment? (car operands))
			 (scode/symbol? (cadr operands)))
		    (generate/unassigned? block continuation expression))
		   ((or (eq? operator (ucode-primitive error-procedure))
			(and (scode/absolute-reference? operator)
			     (eq? (scode/absolute-reference-name operator)
				  'ERROR-PROCEDURE)))
		    (generate/error-combination block continuation expression))
		   (else
		    (generate/combination block continuation expression))))))
	(generate/pair
	 (lambda (block continuation expression)
	   (if (eq? (car expression) safe-variable-tag)
	       (generate/safe-variable block continuation expression)
	       (generate/constant block continuation expression)))))

    (let-syntax
	((dispatch-entry
	  (macro (type handler)
	    `(VECTOR-SET! DISPATCH-VECTOR ,(microcode-type type) ,handler)))
	 (dispatch-entries
	  (macro (types handler)
	    `(BEGIN ,@(map (lambda (type)
			     `(DISPATCH-ENTRY ,type ,handler))
			   types))))
	 (standard-entry
	  (macro (name)
	    `(DISPATCH-ENTRY ,name ,(symbol-append 'GENERATE/ name)))))
      (standard-entry access)
      (standard-entry assignment)
      (standard-entry conditional)
      (standard-entry definition)
      (standard-entry delay)
      (standard-entry disjunction)
      (standard-entry in-package)
      (standard-entry pair)
      (standard-entry quotation)
      (standard-entry the-environment)
      (standard-entry variable)
      (dispatch-entries (lambda lexpr extended-lambda) generate/lambda)
      (dispatch-entries (sequence-2 sequence-3) generate/sequence)
      (dispatch-entries (combination-1 combination-2 combination
				       primitive-combination-0
				       primitive-combination-1
				       primitive-combination-2
				       primitive-combination-3)
			generate/combination)
      (dispatch-entry comment generate/comment))
    (named-lambda (generate/expression block continuation expression)
      ((vector-ref dispatch-vector (primitive-type expression))
       block continuation expression))))
