;; 0

(define test-num 0)

(define (test? exp)
  (set! test-num (+ test-num 1))
  (cons test-num exp))

(define atom?
  (lambda (x)
    (and (not (pair? x)) (not (null? x)))))

(test? (atom? 'a))
(test? (not (atom? '())))
(test? (not (atom? '(a))))

(define add1
  (lambda (n)
    (+ n 1)))

(test? (= (add1 3) 4))

(define sub1
  (lambda (n)
    (- n 1)))

(test? (= (sub1 4) 3))

;; 1

;; 2

(define lat?
  (lambda (l)
    (cond
     ((null? l) #t)
     ((atom? (car l)) (lat? (cdr l)))
     (else #f))))

(test? (lat? '(Jack Sprat could eat no chicken fat)))
(test? (not (lat? '(Jack (Sprat could) eat no chicken fat))))

(define member?
  (lambda (a lat)
    (cond
     ((null? lat) #f)
     (else (or (eq? (car lat) a)
               (member? a (cdr lat)))))))

(test? (member? 'meat '(mashed potatoes and meat gravy)))
(test? (not (member? 'liver '(bagels and lox))))

;; 3

(define rember
  (lambda (a lat)
    (cond
     ((null? lat) '())
     ((eq? (car lat) a) (cdr lat))
     (else (cons (car lat)
                 (rember a (cdr lat)))))))

(test? (equal? (rember 'mint '(lamb chops and mint flavored mint jelly))
               '(lamb chops and flavored mint jelly)))
(test? (equal? (rember 'toast '(bacon lettuce and tomato))
               '(bacon lettuce and tomato)))

(define firsts
  (lambda (l)
    (cond
     ((null? l) '())
     (else (cons (car (car l))
                 (firsts (cdr l)))))))

(test? (equal? (firsts '((apple peach pumpkin)
                         (plum pear cherry)
                         (grape raisin pea)
                         (bean carrot eggplant)))
               '(apple plum grape bean)))
(test? (equal? (firsts '((a b) (c d) (e f)))
               '(a c e)))

(define insertR
  (lambda (new old lat)
    (cond
     ((null? lat) '())
     ((eq? (car lat) old)
      (cons old (cons new (cdr lat))))
     (else (cons (car lat)
                 (insertR new old (cdr lat)))))))

(test? (equal? (insertR 'topping 'fudge '(ice cream with fudge for dessert))
               '(ice cream with fudge topping for dessert)))
(test? (equal? (insertR 'e 'd '(a b c d f g d h))
               '(a b c d e f g d h)))

(define insertL
  (lambda (new old lat)
    (cond
     ((null? lat) '())
     ((eq? (car lat) old)
      (cons new (cons old (cdr lat))))
     (else (cons (car lat)
                 (insertL new old (cdr lat)))))))

(test? (equal? (insertL 'topping 'fudge '(ice cream with fudge for dessert))
               '(ice cream with topping fudge for dessert)))

(define subst
  (lambda (new old lat)
    (cond
     ((null? lat) '())
     ((eq? (car lat) old)
      (cons new (cdr lat)))
     (else (cons (car lat)
                 (subst new old (cdr lat)))))))

(test? (equal? (subst 'topping 'fudge '(ice cream with fudge for dessert))
               '(ice cream with topping for dessert)))

(define subst2
  (lambda (new o1 o2 lat)
    (cond
     ((null? lat) '())
     ((or (eq? (car lat) o1)
          (eq? (car lat) o2))
      (cons new (cdr lat)))
     (else (cons (car lat)
                 (subst2 new o1 o2 (cdr lat)))))))

(test? (equal? (subst2 'vanilla 'chocolate 'banana
                       '(banana ice cream with chocolate topping))
               '(vanilla ice cream with chocolate topping)))

(define multirember
  (lambda (a lat)
    (cond
     ((null? lat) '())
     ((eq? (car lat) a)
      (multirember a (cdr lat)))
     (else (cons (car lat)
                 (multirember a (cdr lat)))))))

(test? (equal? (multirember 'cup '(coffee cup tea cup and hick cup))
               '(coffee tea and hick)))

(define multiinsertR
  (lambda (new old lat)
    (cond
     ((null? lat) '())
     ((eq? (car lat) old)
      (cons old (cons new (multiinsertR new old (cdr lat)))))
     (else (cons (car lat) (multiinsertR new old (cdr lat)))))))

(test? (equal? (multiinsertR 'chili 'cup '(coffee cup tea cup and hick cup))
               '(coffee cup chili tea cup chili and hick cup chili)))

(define multiinsertL
  (lambda (new old lat)
    (cond
     ((null? lat) '())
     ((eq? (car lat) old)
      (cons new (cons old (multiinsertL new old (cdr lat)))))
     (else (cons (car lat) (multiinsertL new old (cdr lat)))))))

(test? (equal? (multiinsertL 'chili 'cup '(coffee cup tea cup and hick cup))
               '(coffee chili cup tea chili cup and hick chili cup)))

(define multisubst
  (lambda (new old lat)
    (cond
     ((null? lat) '())
     ((eq? (car lat) old)
      (cons new (multisubst new old (cdr lat))))
     (else (cons (car lat) (multisubst new old (cdr lat)))))))

(test? (equal? (multisubst 'chili 'cup '(coffee cup tea cup and hick cup))
               '(coffee chili tea chili and hick chili)))

;; 4

(define o+
  (lambda (n m)
    (cond
     ((zero? m) n)
     (else (add1 (o+ n (sub1 m)))))))

(test? (= (o+ 46 12) 58))

(define o-
  (lambda (n m)
    (cond
     ((zero? m) n)
     (else (sub1 (o- n (sub1 m)))))))

(test? (= (o- 14 3) 11))
(test? (= (o- 17 9) 8))

(define addtup
  (lambda (tup)
    (cond
     ((null? tup) 0)
     (else (o+ (car tup)
               (addtup (cdr tup)))))))

(test? (= (addtup '(3 5 2 8)) 18))
(test? (= (addtup '(15 6 7 12 3)) 43))

(define o*
  (lambda (n m)
    (cond
     ((zero? m) 0)
     (else (o+ n (o* n (sub1 m)))))))

(test? (= (o* 5 3) 15))
(test? (= (o* 13 4) 52))

(define tup+
  (lambda (tup1 tup2)
    (cond
     ((and (null? tup1) (null? tup2)) '())
     (else (cons (o+ (car tup1) (car tup2))
                 (tup+ (cdr tup1) (cdr tup2)))))))

(test? (equal? (tup+ '(3 6 9 11 4) '(8 5 2 0 7))
               '(11 11 11 11 11)))
(test? (equal? (tup+ '(2 3) '(4 6))
               '(6 9)))

(define tup+
  (lambda (tup1 tup2)
    (cond
     ((null? tup1) tup2)
     ((null? tup2) tup1)
     (else (cons (o+ (car tup1) (car tup2))
                 (tup+ (cdr tup1) (cdr tup2)))))))

(test? (equal? (tup+ '(3 7) '(4 6 8 1))
               '(7 13 8 1)))

(define o>
  (lambda (n m)
    (cond
     ((zero? n) #f)
     ((zero? m) #t)
     (else (o> (sub1 n) (sub1 m))))))

(test? (not (o> 12 133)))
(test? (o> 120 11))

(define o<
  (lambda (n m)
    (cond
     ((zero? m) #f)
     ((zero? n) #t)
     (else (o< (sub1 n) (sub1 m))))))

(define o=
  (lambda (n m)
    (cond
     ((zero? m) (zero? n))
     ((zero? n) #f)
     (else (o= (sub1 n) (sub1 m))))))

(define o=
  (lambda (n m)
    (cond
     ((o> n m) #f)
     ((o< n m) #f)
     (else #t))))

(define o^
  (lambda (n m)
    (cond
     ((zero? m) 1)
     (else (o* n (o^ n (sub1 m)))))))

(define o/
  (lambda (n m)
    (cond
     ((o< n m) 0)
     (else (add1 (o/ (o- n m) m))))))

(define length
  (lambda (lat)
    (cond
     ((null? lat) 0)
     (else (add1 (length (cdr lat)))))))

(define pick
  (lambda (n lat)
    (cond
     ((zero? (sub1 n)) (car lat))
     (else (pick (sub1 n) (cdr lat))))))

(define rempick
  (lambda (n lat)
    (cond
     ((zero? (sub1 n)) (cdr lat))
     (else (cons (car lat) (rempick (sub1 n) (cdr lat)))))))

(define no-nums
  (lambda (lat)
    (cond
     ((null? lat) '())
     ((number? (car lat))
      (no-nums (cdr lat)))
     (else (cons (car lat) (no-nums (cdr lat)))))))

(define all-nums
  (lambda (lat)
    (cond
     ((null? lat) '())
     ((number? (car lat))
      (cons (car lat) (all-nums (cdr lat))))
     (else (all-nums (cdr lat))))))

(define eqan?
  (lambda (a1 a2)
    (cond
     ((and (number? a1) (number? a2))
      (o= a1 a2))
     ((or (number? a1) (number? a2))
      #f)
     (else (eq? a1 a2)))))

(define occur
  (lambda (a lat)
    (cond
     ((null? lat) 0)
     ((eq? (car lat) a)
      (add1 (occur a (cdr lat))))
     (else (occur a (cdr lat))))))

(define one?
  (lambda (n)
    (o= n 1)))

(define rempick
  (lambda (n lat)
    (cond
     ((one? n) (cdr lat))
     (else (cons (car lat)
                 (rempick (sub1 n) (cdr lat)))))))

;; 5

(define rember*
  (lambda (a l)
    (cond
     ((null? l) '())
     ((atom? (car l))
      (cond
       ((eq? (car l) a) (rember* a (cdr l)))
       (else (cons (car l) (rember* a (cdr l))))))
     (else (cons (rember* a (car l))
                 (rember* a (cdr l)))))))

(define insertR*
  (lambda (new old l)
    (cond
     ((null? l) '())
     ((atom? (car l))
      (cond
       ((eq? (car l) old)
        (cons old (cons new (insertR* new old (cdr l)))))
       (else (cons (car l) (insertR* new old (cdr l))))))
     (else (cons (insertR* new old (car l))
                 (insertR* new old (cdr l)))))))

(define occur*
  (lambda (a l)
    (cond
     ((null? l) 0)
     ((atom? (car l))
      (cond
       ((eq? (car l) a)
        (add1 (occur* a (cdr l))))
       (else (occur* a (cdr l)))))
     (else (o+ (occur* a (car l))
               (occur* a (cdr l)))))))

(define subst*
  (lambda (new old l)
    (cond
     ((null? l) '())
     ((atom? (car l))
      (cond
       ((eq? (car l) old)
        (cons new (subst* new old (cdr l))))
       (else (cons (car l) (subst* new old (cdr l))))))
     (else (cons (subst* new old (car l))
                 (subst* new old (cdr l)))))))

(define insertL*
  (lambda (new old l)
    (cond
     ((null? l) '())
     ((atom? (car l))
      (cond
       ((eq? (car l) old)
        (cons new (cons old (insertL* new old (cdr l)))))
       (else (cons (car l) (insertL* new old (cdr l))))))
     (else (cons (insertL* new old (car l))
                 (insertL* new old (cdr l)))))))

(define member*
  (lambda (a l)
    (cond
     ((null? l) #f)
     ((atom? (car l))
      (or (eq? (car l) a)
          (member* a (cdr l))))
     (else (or (member* a (car l))
               (member* a (cdr l)))))))

(define leftmost
  (lambda (l)
    (cond
     ((atom? (car l)) (car l))
     (else (leftmost (car l))))))

(define eqlist?
  (lambda (l1 l2)
    (cond
     ((and (null? l1) (null? l2)) #t)
     ((or (null? l1) (null? l2)) #f)
     ((and (atom? (car l1)) (atom? (car l2)))
      (and (eqan? (car l1) (car l2))
           (eqlist? (cdr l1) (cdr l2))))
     ((or (atom? (car l1)) (atom? (car l2))) #f)
     (else (and (eqlist? (car l1) (car l2))
                (eqlist? (cdr l1) (cdr l2)))))))

(define equal?
  (lambda (s1 s2)
    (cond
     ((and (atom? s1) (atom? s2))
      (eqan? s1 s2))
     ((or (atom? s1) (atom? s2)) #f)
     (else (eqlist? s1 s2)))))

(define eqlist?
  (lambda (l1 l2)
    (cond
     ((and (null? l1) (null? l2)) #t)
     ((or (null? l1) (null? l2)) #f)
     (else (and (equal? (car l1) (car l2))
                (equal? (cdr l1) (cdr l2)))))))

(define rember
  (lambda (s l)
    (cond
     ((null? l) '())
     ((equal? (car l) s) (cdr l))
     (else (cons (car l) (rember s (cdr l)))))))

;; 6

(define numbered?
  (lambda (aexp)
    (cond
     ((atom? aexp) (number? aexp))
     (else (and (numbered? (car aexp))
                (numbered? (car (cdr (cdr aexp)))))))))

(define value
  (lambda (nexp)
    (cond
     ((atom? nexp) nexp)
     ((eq? (car (cdr nexp)) '+)
      (o+ (value (car nexp))
          (value (car (cdr (cdr nexp))))))
     ((eq? (car (cdr nexp)) '*)
      (o* (value (car nexp))
          (value (car (cdr (cdr nexp))))))
     (else
      (o^ (value (car nexp))
          (value (car (cdr (cdr nexp)))))))))

(define value
  (lambda (nexp)
    (cond
     ((atom? nexp) nexp)
     ((eq? (car nexp) '+)
      (o+ (value (car (cdr nexp)))
          (value (car (cdr (cdr nexp))))))
     ((eq? (car nexp) '*)
      (o* (value (car (cdr nexp)))
          (value (car (cdr (cdr nexp))))))
     (else
      (o^ (value (car (cdr nexp)))
          (value (car (cdr (cdr nexp)))))))))

(define 1st-sub-exp
  (lambda (aexp)
    (car (cdr aexp))))

(define 2nd-sub-exp
  (lambda (aexp)
    (car (cdr (cdr aexp)))))

(define operator
  (lambda (aexp)
    (car aexp)))

(define value
  (lambda (nexp)
    (cond
     ((atom? nexp) nexp)
     ((eq? (operator nexp) '+)
      (o+ (value (1st-sub-exp nexp))
          (value (2nd-sub-exp nexp))))
     ((eq? (operator nexp) '*)
      (o* (value (1st-sub-exp nexp))
          (value (2nd-sub-exp nexp))))
     (else
      (o^ (value (1st-sub-exp nexp))
          (value (2nd-sub-exp nexp)))))))

(define 1st-sub-exp
  (lambda (aexp)
    (car aexp)))

(define operator
  (lambda (aexp)
    (car (cdr aexp))))

(define sero?
  (lambda (n)
    (null? n)))

(define edd1
  (lambda (n)
    (cons '() n)))

(define zub1
  (lambda (n)
    (cdr n)))

(define old-o+ o+)

(define o+
  (lambda (n m)
    (cond
     ((sero? m) n)
     (else (edd1 (o+ n (zub1 m)))))))

(define o+ old-o+)

;; 7
