;;============================================================================
;; SCHEME MEMORY-USAGE TESTING EXAMPLES
;;
;; This file contains some simple snippets of Scheme code that will allow you
;; to verify that your garbage collector properly collects all necessary kinds
;; of Scheme objects.  Comments will describe the various details below.


;;============================================================================
;; These examples are excellent for ensuring that both Value and Environment
;; objects are properly collected.  They are various recursive computations
;; that generate both.

; This function returns n! (n-factorial).
(define fact (lambda (n) (if (<= n 1) 1 (* n (fact (- n 1))))))

; Compute factorial of 7, which should be 5040.
(fact 7)


; These functions each return the n+1-th value of the Fibonacci sequence,
; using slightly different flow-control mechanisms.  DO NOT USE ON LARGE
; INPUTS!  This Scheme interpreter can't handle it!
(define fib1 (lambda (n)
    (if (= n 0) 1
        (if (= n 1) 1
            (+ (fib1 (- n 2)) (fib1 (- n 1)))))))

(define fib2 (lambda (n)
    (cond ((= n 0) 1)
          ((= n 1) 1)
          (else (+ (fib2 (- n 2)) (fib2 (- n 1)))))))

; These will generate *lots* of values and environments...
(fib1 18)
(fib2 18)


;;============================================================================
;; These examples allow you to test whether Lambdas are reclaimed, and also to
;; ensure that trapped frames are NOT reclaimed until they are no longer
;; accessible.  They use a basic mechanism to implement Scheme objects.


(define (make-point x y)
    (lambda (op . args)
        (cond ((eq? op 'get-x) x)
              ((eq? op 'get-y) y)

              ((eq? op 'set-x) (set! x (car args)))
              ((eq? op 'set-y) (set! y (car args)))

              ((eq? op 'distance-to)
               (let ((p (car args)))  ;; This is the passed-in point object
                   (let ((dx (- x (p 'get-x)))     ;; Deltas between coords
                         (dy (- y (p 'get-y))))
                       (sqrt (+ (* dx dx) (* dy dy))))))

              (else "bad op"))))


(define p1 (make-point 5 21))
(define p2 (make-point 8 25))

(display "p1 = (" (p1 'get-x) ", " (p1 'get-y) ")")
(display "p2 = (" (p2 'get-x) ", " (p2 'get-y) ")")
(display "distance between p1 and p2 = " (p1 'distance-to p2))

; This is a bit gross, but it will allow the lambdas and trapped frames
; that p1 and p2 to be reclaimed.
(set! p1 0)
(set! p2 0)


