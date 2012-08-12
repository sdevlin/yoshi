(define map
  (lambda (f l)
    (cond
     ((null? l) ())
     (else (cons (f (car l))
                 (map f (cdr l)))))))

(map (lambda (x) (* x x x)) '(1 2 3 4 5))
;; (1 8 27 64 125)

(define filter
  (lambda (f l)
    (cond
     ((null? l) '())
     ((f (car l))
      (cons (car l) (filter f (cdr l))))
     (else (filter f (cdr l))))))

(filter (lambda (x) (= (mod x 2) 0)) '(1 2 3 4 5 6 7))
;; (2 4 6)

(define reduce
  (lambda (f l acc)
    (cond
     ((null? l) acc)
     (else (reduce f (cdr l) (f (car l) acc))))))

(reduce + '(1 2 3 4 5) 0)
;; 15
