(define (apply f args)
  (eval `(,f ,@args)))

(define (not x)
  (eq? x #f))

(define (null? x)
  (eq? x '()))

(define (boolean? x)
  (or (eq? x #t) (eq? x #f)))

(define (equal? . args)
  (define (eql? a b)
    (cond
     ((and (pair? a) (pair? b))
      (and (equal? (car a) (car b))
           (equal? (cdr a) (cdr b))))
     ((or (pair? a) (pair? b)) #f)
     (else (eq? a b))))
  (or (< (length args) 2)
      (and (eql? (car args) (car (cdr args)))
           (apply equal? (cdr args)))))

(define (zero? n)
  (= n 0))

(define (< a b)
  (not (or (> a b) (= a b))))

(define (>= a b)
  (or (> a b) (= a b)))

(define (<= a b)
  (or (< a b) (= a b)))

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
   ((null? l) '())
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
