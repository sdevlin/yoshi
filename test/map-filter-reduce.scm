(define map
  (lambda (f l)
    (if (null? l)
        ()
        (cons (f (car l))
              (map f (cdr l))))))

(print (map (lambda (x) (* x x x))
            (list 1 2 3 4 5)))

(define filter
  (lambda (f l)
    (if (null? l)
        ()
        (if (f (car l))
            (cons (car l) (filter f (cdr l)))
            (filter f (cdr l))))))

(print (filter (lambda (x) (= (mod x 2) 0))
               (list 1 2 3 4 5 6 7)))

(define reduce
  (lambda (f l acc)
    (if (null? l)
        acc
        (reduce f (cdr l) (f (car l) acc)))))

(print (reduce + (list 1 2 3 4 5) 0))
