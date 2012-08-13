(define fib
  (lambda (n)
    (define iter
      (lambda (m a b)
        (if (= n m)
            b
            (iter (+ m 1) (+ a b) a))))
    (iter 0 1 0)))

(fib 30)
