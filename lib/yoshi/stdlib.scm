(define (void)
  (if #f #f))

;; sort of broken
(define (apply f args)
  (eval `(,f ,@args)))

(define (not x)
  (eq? x #f))

(define (null? x)
  (eq? x '()))

(define (boolean? x)
  (or (eq? x #t) (eq? x #f)))

;; doesn't work because apply doesn't really work
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

(define (map proc list)
  (cond
   ((null? list) '())
   (else (cons (proc (car list))
               (map proc (cdr list))))))

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

(define (vector-map proc vector)
  (define length (vector-length vector))
  (define new-vector (make-vector length))
  (define (iter k)
    (if (= k length)
        new-vector
        (begin
          (vector-set! new-vector k
                       (proc (vector-ref vector k)))
          (iter (+ k 1)))))
  (iter 0))
