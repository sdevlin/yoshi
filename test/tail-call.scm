(define (odd? n)
  (if (= n 0)
      #f
      (even? (- n 1))))

(define (even? n)
  (if (= n 0)
      #t
      (odd? (- n 1))))

(even? 1000000)
;; #t
