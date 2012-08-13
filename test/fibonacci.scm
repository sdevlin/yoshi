(define fibonacci
  (lambda (n)
    (define fib
      (lambda (m a b)
        (if (= n m)
            b
            (fib (+ m 1) (+ a b) a))))
    (fib 0 1 0)))
