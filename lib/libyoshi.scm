(define (zero? n)
  (= n 0))

(define (length l)
  (cond
   ((null? l) 0)
   (else (+ 1 (length (cdr l))))))

(define (append l1 l2)
  (cond
   ((null? l1) l2)
   (else (cons (car l1)
               (append (cdr l1) l2)))))

(define (list . xs)
  xs)

(define (list? x)
  (cond
   ((null? x) #t)
   ((pair? x) (list? (cdr x)))
   (else #f)))

(define (map f l)
  (cond
   ((null? l) ())
   (else (cons (f (car l))
               (map f (cdr l))))))

(define (filter f l)
  (cond
   ((null? l) '())
   ((f (car l))
    (cons (car l) (filter f (cdr l))))
   (else (filter f (cdr l)))))

(define (reduce f l acc)
  (cond
   ((null? l) acc)
   (else (reduce f (cdr l) (f (car l) acc)))))

(define (range n)
  (define (iter m)
    (if (= m n)
        '()
        (cons m (iter (+ m 1)))))
  (iter 0))
