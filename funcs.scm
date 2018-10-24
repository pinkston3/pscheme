;; This function returns n! (n-factorial).
(define fact (lambda (n) (if (<= n 1) 1 (* n (fact (- n 1))))))


;; This function returns the n+1-th value of the Fibonacci sequence.
(define fib1 (lambda (n)
    (if (= n 0) 1
        (if (= n 1) 1
            (+ (fib1 (- n 2)) (fib1 (- n 1)))))))

(define fib2 (lambda (n)
    (cond ((= n 0) 1)
          ((= n 1) 1)
          (else (+ (fib2 (- n 2)) (fib2 (- n 1)))))))


(define (make-meter x)
    (lambda (op . args)
        (cond ((eq? op 'get) x)
              ((eq? op 'set) (set! x (car args)))
              (else "bad op"))))



