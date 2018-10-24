;; Standard functions provided in the Scheme interpreter, that we don't have to
;; define as native functions.

(define (not x) (if x #f #t))

(define (null? x) (eq? x nil))

(define (zero? x) (= x 0))
(define (positive? x) (> x 0))
(define (negative? x) (< x 0))

;; Returns the trailing portion of the input list x, starting with the k-th
;; element.  (list-tail x 0) returns the entire list.
(define (list-tail x k) (if (zero? k) x (list-tail (cdr x) (- k 1))))

;; Returns the k-th element of the list.  The first element has index 0.
(define (list-ref x k) (car (list-tail x k)))

;; Appends a list onto another list.  The result contains a copy of the
;; first list, although the second list is referenced by the result.
(define (append a b)
  (cond ((null? a) b)
        ((null? b) a)
        ((null? (cdr a)) (cons (car a) b))
        (else (cons (car a) (append (cdr a) b)))))

(define (append! a b)
  (define (helper a b) (if (null? (cdr a)) (set-cdr! a b) (helper (cdr a) b)))

  (if (null? a)
      b
      (begin
        (helper a b)
        a
      )
  )
)

;; Reverses a list, returning the result as a separate copy.
(define (reverse x)
  (define (helper lst result)
    (cond ((null? lst) result)
          (else (helper (cdr lst) (cons (car lst) result)))))
  (helper x nil))

;; Filters a list using the specified predicate.
(define (filter x pred?)
   (cond ((null? x) x)
         ((pred? (car x)) (cons (car x) (filter (cdr x) pred?)))
         (else (filter (cdr x) pred?))))

