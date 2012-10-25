;; non-standard
(define (void)
  (if #f #f))

;; sort of broken
(define (apply proc args)
  (eval `(,proc ,@args)))

(define (not obj)
  (eq? obj #f))

(define (null? obj)
  (eq? obj '()))

(define (boolean? obj)
  (or (eq? obj #t) (eq? obj #f)))

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

(define (zero? z)
  (= z 0))

(define (< a b)
  (not (or (> a b) (= a b))))

(define (>= a b)
  (or (> a b) (= a b)))

(define (<= a b)
  (or (< a b) (= a b)))

(define (caar pair)
  (car (car pair)))

(define (cadr pair)
  (car (cdr pair)))

(define (cdar pair)
  (cdr (car pair)))

(define (cddr pair)
  (cdr (cdr pair)))

(define (caaar pair)
  (car (caar pair)))

(define (caadr pair)
  (car (cadr pair)))

(define (cadar pair)
  (car (cdar pair)))

(define (caddr pair)
  (car (cddr pair)))

(define (cdaar pair)
  (cdr (caar pair)))

(define (cdadr pair)
  (cdr (cadr pair)))

(define (cddar pair)
  (cdr (cdar pair)))

(define (cdddr pair)
  (cdr (cddr pair)))

(define (caaaar pair)
  (car (caaar pair)))

(define (caaadr pair)
  (car (caadr pair)))

(define (caadar pair)
  (car (cadar pair)))

(define (caaddr pair)
  (car (caddr pair)))

(define (cadaar pair)
  (car (cdaar pair)))

(define (cadadr pair)
  (car (cdadr pair)))

(define (caddar pair)
  (car (cddar pair)))

(define (cadddr pair)
  (car (cdddr pair)))

(define (cdaaar pair)
  (cdr (caaar pair)))

(define (cdaadr pair)
  (cdr (caadr pair)))

(define (cdadar pair)
  (cdr (cadar pair)))

(define (cdaddr pair)
  (cdr (caddr pair)))

(define (cddaar pair)
  (cdr (cdaar pair)))

(define (cddadr pair)
  (cdr (cdadr pair)))

(define (cdddar pair)
  (cdr (cddar pair)))

(define (cddddr pair)
  (cdr (cdddr pair)))

(define (length list)
  (cond
   ((null? list) 0)
   (else (+ 1 (length (cdr list))))))

(define (append l1 l2)
  (cond
   ((null? l1) l2)
   (else (cons (car l1)
               (append (cdr l1) l2)))))

(define (list . objs)
  objs)

(define (list? obj)
  (cond
   ((null? obj) #t)
   ((pair? obj) (list? (cdr obj)))
   (else #f)))

(define (map proc list)
  (cond
   ((null? list) '())
   (else (cons (proc (car list))
               (map proc (cdr list))))))

(define (for-each proc list)
  (cond
   ((null? list) (void))
   (else (begin
           (proc (car list))
           (for-each proc (cdr list))))))

(define (filter proc list)
  (cond
   ((null? list) '())
   ((proc (car list))
    (cons (car list) (filter proc (cdr list))))
   (else (filter proc (cdr list)))))

(define (reduce proc list acc)
  (cond
   ((null? list) acc)
   (else (reduce proc (cdr list) (proc (car list) acc)))))

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

(define (list->vector list)
  (define length (length list))
  (define vector (make-vector length))
  (define (iter k list)
    (if (null? list)
        vector
        (begin
          (vector-set! vector k (car list))
          (iter (+ k 1) (cdr list)))))
  (iter 0 list))

(define (vector . objs)
  (list->vector objs))

(define (call-with-current-continuation proc)
  "Maybe later.")

(define call/cc call-with-current-continuation)
