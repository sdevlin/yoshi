;; The fixed point combinator

(define Y
  (lambda (f)
    ((lambda (x) (f (lambda (v) ((x x) v))))
     (lambda (x) (f (lambda (v) ((x x) v)))))))

(define fact
  (Y (lambda (f)
       (lambda (n)
         (if (= n 0)
             1
             (* n (f (- n 1))))))))

(fact 10)
;; 3628800
