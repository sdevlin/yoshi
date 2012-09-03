This is yoshi, a LISP-1 interpreter written in C. (The syntax and semantics are Scheme-like.)

The eval/apply definitions are based on SICP 4.1. The list of built-in functions I chose to include is mostly based on Peter Norvig's lispy. (Most diversions are due to fixnums being the sole numeric type I support.)

This implementation performs proper tail call optimizations for most relevant forms. (The `and` and `or` special forms are not optimized correctly.)

I wrote a garbage collector. I think it works, but I have been known to make mistakes from time to time.

RIP Dennis Ritchie and John McCarthy.
