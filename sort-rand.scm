;; A function to generate random numbers.

(define (gen-random n)
  (if (zero? n) nil (cons (random 10000) (gen-random (- n 1)))))


;; Quick-sort algorithm, although with how much copying this function does,
;; there really isn't anything quick about it...

(define (quicksort lst)
  (cond ((null? lst) nil)         ;; zero elements in list
        ((null? (cdr lst)) lst)   ;; one element in list
        (else
          (let ((pivot (car lst))
                (rest (cdr lst)))
            (let ((lower (filter lst (lambda (x) (< x pivot))))
                  (higher (filter lst (lambda (x) (> x pivot)))))
              (set! lst (append! (quicksort lower)
                                 (cons pivot (quicksort higher))))
              lst
            )
          )
        )
  )
)

  
