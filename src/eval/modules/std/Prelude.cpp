#include <scm/eval/modules/std/Prelude.hpp>
#include <scm/eval/Data.hpp>
#include <scm/eval/Util.hpp>
#include <scm/eval/SExprReader.hpp>
#include <sstream>

/***************************
 NOTE: Do not edit this file, it is generated by the script in eval/tools/scm/PreludeGen.scm

   To change the prelude, edit eval/tools/scm/Prelude.scm
***************************/

namespace scm {

void InitStdPrelude(EnvironmentFrame* env) {
    std::istringstream ss
    (
        "(define (y f) ((lambda (x) (f (lambda (y) ((x x) y)))) (lambda (x) (f (lambda (y) ((x x) y))))))\n"
        "(define (id x) x)\n"
        "(define (, . fns) (lambda (v) (foldr v (lambda (f a) (f a)) fns)))\n"
        "(define (compose . fns) (lambda (v) (foldr v (lambda (f a) (f a)) fns)))\n"
        "(define (apply f args) (eval (pair f args)))\n"
        "(define (type-cmp tid val) (= tid (type val)))\n"
        "(define list? (type-cmp (quote pair)))\n"
        "(define string? (type-cmp (quote string)))\n"
        "(define symbol? (type-cmp (quote symbol)))\n"
        "(define number? (type-cmp (quote number)))\n"
        "(define (fixed-point fn iv) (define r iv) (while fn (set iv (fn r)) (if (= iv r) (set fn nil) (set r iv))) r)\n"
        "(define (for i f fn) (define v (+ 0 i)) (define r nil) (while (< v f) (set r (fn v)) (incr v 1)) r)\n"
        "(define (sort lt-pred lst) (if (<= (length lst) 1) lst (progn (define l1 (sort lt-pred (filter (lambda (v) (lt-pred v (head lst))) (tail lst)))) (define l2 (pair (head lst) (sort lt-pred (filter (lambda (v) (not (lt-pred v (head lst)))) (tail lst))))) (if l1 (dappend l1 l2) l2))))\n"
        "(define (group fn lst) (define r nil) (for-each (lambda (e) (define k (fn e)) (define kl (untag k r)) (set r (tag k (pair e kl) r))) lst) r)\n"
        "(define (any fn lst) (define r nil) (while (and lst (not r)) (set r (fn (head lst))) (set lst (tail lst))) r)\n"
        "(define (all fn lst) (define r t) (while (and lst r) (set r (fn (head lst))) (set lst (tail lst))) r)\n"
        "(define (map fn lst) (unfold not (compose fn head) tail lst))\n"
        "(define (filter fn lst) (foldr nil (lambda (a b) (if (fn a) (pair a b) b)) lst))\n"
        "(define (for-each fn lst) (foldl nil (lambda (r a) (fn a)) lst))\n"
        "(define (reverse lst) (foldl nil (flip pair) lst))\n"
        "(define (last lst) (nth lst (- (length lst) 1)))\n"
        "(define (scanl fn iv lst) (reverse (foldl (list iv) (lambda (vs a) (pair (fn (head vs) a) vs)) lst)))\n"
        "(define (zip v1 v2) (unfold (lambda (v) (not (and v (head v) (tail v)))) (lambda (v) (pair (head (head v)) (head (tail v)))) (lambda (v) (pair (tail (head v)) (tail (tail v)))) (pair v1 v2)))\n"
        "(define (zipwith v1 fn v2) (unfold (lambda (v) (not (and v (head v) (tail v)))) (lambda (v) (fn (head (head v)) (head (tail v)))) (lambda (v) (pair (tail (head v)) (tail (tail v)))) (pair v1 v2)))\n"
        "(define (dzip xs ys) (define xn (length xs)) (define yn (length ys)) (zip (append xs (repeat nil (- yn xn))) (append ys (repeat nil (- xn yn)))))\n"
        "(define (dzipwith f xs ys) (map (lambda (p) (f (head p) (tail p))) (dzip xs ys)))\n"
        "(define (transpose rs) (foldr nil (lambda (r cs) (dzipwith (lambda (c d) ((if c dappend append) c (pair d nil))) cs r)) rs))\n"
        "(define (counting-for-each f lst) (define i (+ 0 0)) (define r nil) (for-each (lambda (e) (set r (f e i)) (incr i 1)) lst) r)\n"
        "(define (counting-map f lst) (define i (+ 0 0)) (decr i 1) (map (lambda (e) (incr i 1) (f e i)) lst))\n"
        "(define (append v1 v2) (foldr v2 pair v1))\n"
        "(define (append-all lsts) (foldr nil append lsts))\n"
        "(define (append-vals . vs) (append-all vs))\n"
        "(define (flip fn) (lambda (a b) (fn b a)))\n"
        "(define (unique lst) (foldr nil (lambda (elem rest) (if (in elem rest) rest (pair elem rest))) lst))\n"
        "(define (flatten-step lst) (cond ((not (list? lst)) nil) ((not lst) nil) ((list? (head lst)) (append (head lst) (flatten-step (tail lst)))) (t (pair (head lst) (flatten-step (tail lst))))))\n"
        "(define (flatten lst) (cond ((not (list? lst)) nil) ((not lst) nil) ((list? (head lst)) (append (flatten (head lst)) (flatten (tail lst)))) (t (pair (head lst) (flatten (tail lst))))))\n"
        "(define (concat-all strs) (foldr \"\" dy/concat strs))\n"
        "(define (concat . strs) (concat-all strs))\n"
        "(define (nth lst n) (iterate 0 (= n) (dy+ 1) (lambda (i) (if lst (set lst (tail lst))))) (if lst (head lst) nil))\n"
        "(define (nth-tail lst n) (iterate 0 (= n) (dy+ 1) (lambda (i) (if lst (set lst (tail lst))))) lst)\n"
        "(define (sum lst) (foldl 0 dy+ lst))\n"
        "(define (+ . lst) (sum lst))\n"
        "(define (- v . lst) (foldl v dy- lst))\n"
        "(define (* . lst) (foldl 1 dy* lst))\n"
        "(define (/ v . lst) (foldl v dy/ lst))\n"
        "(define (max x y) (if (> x y) x y))\n"
        "(define (min x y) (if (< x y) x y))\n"
        "(define (seq-max min-val lst) (foldl min-val max lst))\n"
        "(define (seq-min max-val lst) (foldl max-val min lst))\n"
        "(define (up-to n) (unfold (= n) id (dy+ 1) 0))\n"
        "(define (s< s1 s2) (< (strcmp s1 s2) 0))\n"
        "(define (string v) (if (string? v) v (to-string v)))\n"
        "(define (throw . exprs) (raise (foldl \"\" (lambda (a expr) (concat a (string expr))) exprs)))\n"
        "(define (const a b) a)\n"
        "(define (make-zip h pfx sfx) (pair h (pair pfx sfx)))\n"
        "(define (open-zip xs) (if (= (length xs) 0) (throw \"Cannot open zipper on empty list.\") (make-zip (head xs) nil (tail xs))))\n"
        "(define (zip-head z) (head z))\n"
        "(define (zip-prefix z) (head (tail z)))\n"
        "(define (zip-suffix z) (tail (tail z)))\n"
        "(define (zip-set z x) (make-zip x (zip-prefix z) (zip-suffix z)))\n"
        "(define (zip-eof? z) (= nil (zip-suffix z)))\n"
        "(define (zip-bof? z) (= nil (zip-prefix z)))\n"
        "(define (zip-succ z) (if (zip-eof? z) (throw \"Cannot read past end of zipper\") (make-zip (head (zip-suffix z)) (pair (zip-head z) (zip-prefix z)) (tail (zip-suffix z)))))\n"
        "(define zip+ zip-succ)\n"
        "(define (zip-pred z) (if (zip-bof? z) (throw \"Cannot read past start of zipper\") (make-zip (head (zip-prefix z)) (tail (zip-prefix z)) (pair (zip-head z) (zip-suffix z)))))\n"
        "(define zip- zip-pred)\n"
        "(define (zip-n+ z n) (while (and (> n 0) (not (zip-eof? z))) (set z (zip+ z)) (set n (- n 1))) z)\n"
        "(define (zip-n- z n) (while (and (> n 0) (not (zip-bof? z))) (set z (zip- z)) (set n (- n 1))) z)\n"
        "(define (zip-length z) (+ 1 (length (zip-prefix z)) (length (zip-suffix z))))\n"
        "(define (zip-start z) (while (not (zip-bof? z)) (set z (zip- z))) z)\n"
        "(define (zip-end z) (while (not (zip-eof? z)) (set z (zip+ z))) z)\n"
        "(define (zip-close z) (append (reverse (zip-prefix z)) (pair (zip-head z) (zip-suffix z))))\n"
        "(define (dict-for-each fn d) (iterate (d (quote all)) (lambda (i) (i (quote eof?))) (lambda (i) (i (quote next)) i) (lambda (i) (fn (tail (i (quote value)))))))\n"
        "(define (dict-map fn d) (unfold (lambda (i) (i (quote eof?))) (lambda (i) (fn (tail (i (quote value))))) (lambda (i) (i (quote next)) i) (d (quote all))))\n"
        "(define (untag tag lst) (define r nil) (define lcell nil) (while (and (not r) lst) (set lcell (head lst)) (if (and lcell (list? lcell) (= (head lcell) tag)) (set r (tail lcell)) nil) (set lst (tail lst))) r)\n"
        "(define (tag tg v lst) (define found nil) (define pr (foldr nil (lambda (tent nlst) (if (= (head tent) tg) (progn (set found t) (pair (pair tg v) nlst)) (pair tent nlst))) lst)) (if found pr (pair (pair tg v) lst)))\n"
        "(define (append-tag tg v lst) (define found nil) (define pr (foldr nil (lambda (tent nlst) (if (= (head tent) tg) (progn (set found t) (pair (pair tg (pair v (tail tent))) nlst)) (pair tent nlst))) lst)) (if found pr (pair (pair tg (list v)) lst)))\n"
        "(define (cdelim strs delim) (foldl \"\" (lambda (a b) (if (= a \"\") b (concat a delim b))) strs))\n"
        "(define (explode str) (unfold (= \"\") (lambda (s) (substr s 0 1)) (lambda (s) (substr s 1 (- (length s) 1))) str))\n"
        "(define (csplit str delim) (unfold (= \"\") (lambda (s) (head (lsplit s delim))) (lambda (s) (tail (lsplit s delim))) str))\n"
        "(define (stdstream-printer suffix) (lambda vals (for-each (lambda (v) (print stdout (string v))) vals) (print stdout suffix)))\n"
        "(define (stream-printer stream suffix) (lambda vals (for-each (lambda (v) (print stream (string v))) vals) (print stream suffix)))\n"
        "(define with-file (lambda (fname fn) (define f (write-file fname)) (fn (stream-printer f \"\\n\")) (close-file f) (set f nil)))\n"
        "(define cprint (stdstream-printer \"\\n\"))\n"
        "(define (progn . exprs) (for-each id exprs))\n"
        "(define (memoize f) (define f-cache nil) (lambda args (define tv (untag args f-cache)) (if tv (tail tv) (progn (define r (apply f (map (lambda (a) (list (quote quote) a)) args))) (set f-cache (tag args (pair t r) f-cache)) r))))\n"
        "(define (split-choose stop-fn trans-fn vlst) (define r nil) (iterate vlst (lambda (v) (or (not v) r)) tail (lambda (vlt) (define tv (trans-fn (head vlt))) (if (stop-fn tv) (set r (pair tv (tail vlt))) nil))) r)\n"
        "(define (choose sfn tfn vlst) (define r (split-choose sfn tfn vlst)) (if r (head r) nil))\n"
        "(define (first-non-nil . vals) (choose id id vals))\n"
        "(define (cartesian-product a b) (map (lambda (a) (map (lambda (b) (pair a b)) b)) a))\n"
        "(define (set-difference a b) (filter (lambda (a) (not (in a b))) a))\n"
        "(define (set-intersection a b) (filter (lambda (a) (in a b)) a))\n"
        "(define (set-symmetric-difference a b) (set-union (set-difference a b) (set-difference b a)))\n"
        "(define (set-union a b) (unique (append a b)))\n"
        "(define (print-environment) (for-each (lambda (var) (print stdout (concat (to-string (head var)) \" :: \" (to-string (type (tail var))) \"\\n\"))) (head (tail (open-environment)))))\n"
        "(define (a-push a e) (a-set a (a-size a) e))\n"
        "(define (a-pop a) (if (> (a-size a) 0) (a-resize a (- (a-size a) 1))))\n"
        "(define (a-back a d) (if (= (a-size a) 0) d (a-get a (- (a-size a) 1))))\n"
        "(define (a-eachi f a) (define c (a-size a)) (define i (+ 0 0)) (while (< i c) (f i (a-get a i)) (incr i 1)))\n"
        "(define (a-mapi f a) (define c (a-size a)) (define i (+ 0 0)) (define r (a-from-size c)) (while (< i c) (a-set r i (f i (a-get a i))) (incr i 1)) r)\n"
        "(define (a-filteri f a) (define c (a-size a)) (define i (+ 0 0)) (define j (+ 0 0)) (define r (a-from-size c)) (define x nil) (while (< i c) (set x (a-get a i)) (if (f i x) (progn (a-set r j x) (incr j 1))) (incr i 1)) (a-resize r j) r)\n"
        "(define (a-foldli f x a) (define c (a-size a)) (define i (+ 0 0)) (while (< i c) (set x (f i x (a-get a i))) (incr i 1)) x)\n"
        "(define (bsearchi-span a f s e) (define spn (- e s)) (define mi (+ s (floor (/ spn 2)))) (define x (a-get a mi)) (define sdir (f x)) (cond ((= sdir 0) mi) ((<= spn 1) mi) ((< sdir 0) (bsearchi-span a f s mi)) (t (bsearchi-span a f mi e))))\n"
        "(define (bsearchi a f) (if (= (a-size a) 0) (throw \"Can\\\'t search empty array.\") (bsearchi-span a f 0 (a-size a))))\n"
        "(define (bsearch a f) (if (= (a-size a) 0) nil (a-get a (bsearchi a f))))\n"
        "(define (bsearch-range a sf ef) (define xi (bsearchi a sf)) (define xf (bsearchi a ef)) (a-cut a xi (+ xf 1)))\n"
        "(define (bs-eqv x v) (cond ((= x v) 0) ((< x v) -1) (t 1)))\n"
        "(define (print-bit-matrix m) (define c 0) (define r 0) (while (< r (bit-matrix-rows m)) (decr c c) (while (< c (bit-matrix-columns m)) (print stdout (if (bit-matrix-get m c r) \"1\" \"0\")) (incr c 1)) (print stdout \"\\n\") (incr r 1)) (decr c c) (decr r r) nil)\n"
        "(define (print-int-matrix m) (define c 0) (define r 0) (print stdout \"[\") (while (< r (int-matrix-rows m)) (if (> r 0) (print stdout \" \")) (print stdout \"[\") (decr c c) (while (< c (int-matrix-columns m)) (if (> c 0) (print stdout \" \")) (print stdout (to-string (int-matrix-get m c r))) (incr c 1)) (print stdout \"]\") (incr r 1)) (print stdout \"]\\n\") (decr c c) (decr r r) nil)\n"
        "(define (print-array a) (define i 0) (decr i i) (print stdout \"[\") (while (< i (array-size a)) (if (> i 0) (print stdout \" \")) (print stdout (to-string (array-get a i))) (incr i 1)) (print stdout \"]\\n\"))\n"
        "(define (print-byte-array ba) (define (print-nib n) (print stdout (nth (list (quote \"0\") (quote \"1\") (quote \"2\") (quote \"3\") (quote \"4\") (quote \"5\") (quote \"6\") (quote \"7\") (quote \"8\") (quote \"9\") (quote \"a\") (quote \"b\") (quote \"c\") (quote \"d\") (quote \"e\") (quote \"f\")) n))) (define (print-idx i) (define v (byte-array-read ba i)) (define ln (mod v 16)) (define hn (/ (- v ln) 16)) (print-nib hn) (print-nib ln)) (if (= (byte-array-size ba) 0) (sprintln \"0x0\") (progn (print stdout \"0x\") (for 0 (byte-array-size ba) (lambda (i) (print-idx i))) (print stdout \"\\n\"))))\n"
        "(define (list->byte-array lst) (define ba (make-byte-array)) (define i 0) (decr i i) (for-each (lambda (b) (byte-array-write ba i b) (incr i 1)) lst) ba)\n"
        "(define (byte-array->list ba) (define r nil) (define i 0) (decr i i) (while (< i (byte-array-size ba)) (set r (pair (byte-array-read ba i) r)) (incr i 1)) (reverse r))\n"
        "(define (pad val len) (define r nil) (define i 0) (decr i i) (while (< i len) (set r (pair val r)) (incr i 1)) r)\n"
        "(define (left lst len) (define r nil) (define i 0) (decr i i) (while (and lst (< i len)) (set r (pair (head lst) r)) (incr i 1) (set lst (tail lst))) (reverse r))\n"
        "(define (right lst len) (reverse (left (reverse lst) len)))\n"
        "(define sprint (stdstream-printer \"\"))\n"
        "(define sprintln (stdstream-printer \"\\n\"))\n"
        "(define (sprint-table vals cols) (define gvals (length-group vals cols)) (define zstrs (repeat \"\" cols)) (define cmaxlen (foldl (repeat 0 cols) (lambda (maxs row) (zipwith maxs max (map length (append row zstrs)))) gvals)) (for-each (lambda (row) (counting-for-each (lambda (c i) (sprint c) (sprint (concat-all (pad \" \" (- (nth cmaxlen i) (length c)))) \" \")) row) (sprintln)) gvals))\n"
        "(define (length-group lst n) (head (fixed-point (lambda (r-lst) (define r (head r-lst)) (define l (tail r-lst)) (if (not l) r-lst (pair (append r (list (left l n))) (nth-tail l n)))) (pair nil lst))))\n"
        "(define (repeat x n) (define r (list (quote rret))) (for 0 n (lambda (i) (dappend r (pair x nil)))) (tail r))\n"
        "(define (pprint e) (define (indent x) (if (= x 0) \"\" (concat \" \" (indent (- x 1))))) (define (ppe idt e) (case (type e) ((quote pair) (sprintln (indent idt) \"(\") (for-each (lambda (ne) (ppe (+ idt 1) ne)) e) (sprintln (indent idt) \")\")) ((quote ?else) (sprintln (indent idt) (to-string e))))) (ppe 0 e))\n"
        "(define (trace msg v) (print stdout (concat msg (to-string v) \"\\n\")) v)\n"
        "(define (time msg fn) (define st (tick)) (define r (fn)) (sprintln msg \": \" (- (tick) st) \"ms\") r)\n"
        "(define (dump-profile-info fname) (with-file fname (lambda (pf) (pf \"Expression,Invoke Count,Total Time,Average Time\") (for-each (lambda (pi) (pf (to-string (nth pi 0)) \",\" (to-string (nth pi 1)) \",\" (to-string (nth pi 2)) \",\" (to-string (if (= 0 (nth pi 2)) 0 (/ (nth pi 2) (nth pi 1)))))) (profile-info)))))\n"
        "(define (lunique lst) (reverse (foldl nil (lambda (rlst itm) (if (in itm rlst) rlst (pair itm rlst))) lst)))\n"
        "(define (align v b) (* b (ceil (/ v b))))\n"
        "(define (b/or . vals) (foldl 0 bit/or vals))\n"
        "(define (b/and . vals) (foldl (bit/not 0) bit/and vals))\n"
        "(define | b/or)\n"
        "(define & b/and)\n"
        "(define << bit/shl)\n"
        "(define >> bit/shr)\n"
        "(define (bit-set? b bv) (= b (bit/and b bv)))\n"
        "(define (byte-array-write-word ba i w) (define hb (floor (/ w 256))) (define lb (- w (* hb 256))) (byte-array-write ba i lb) (byte-array-write ba (+ i 1) hb))\n"
        "(define (byte-array-write-dword ba i dw) (define hw (floor (/ dw 65536))) (define lw (- dw (* hw 65536))) (byte-array-write-word ba i lw) (byte-array-write-word ba (+ i 2) hw))\n"
        "(define (byte-array-append-dword ba dw) (byte-array-write-dword ba (byte-array-size ba) dw))\n"
        "(define (byte-array-append-word ba w) (byte-array-write-word ba (byte-array-size ba) w))\n"
        "(define (byte-array-append-byte ba b) (byte-array-write ba (byte-array-size ba) b))\n"
        "(define (ensure-dir-exists dname) (define (eed-r prefix suffix) (define sp (lsplit suffix \"/\")) (make-directory (concat prefix (head sp))) (if (not (= (tail sp) \"\")) (eed-r (concat prefix (head sp) \"/\") (tail sp)))) (eed-r \"\" dname))\n"
        "(define (read-all istream) (define exprs nil) (while (not (eof? istream)) (set exprs (pair (read istream) exprs))) (reverse exprs))\n"
        "(define (slurp istream) (define s \"\") (while (not (eof? istream)) (set s (concat s (read-line istream) \"\\n\"))) s)\n"
        "(define (write-xml xml) (concat-all (map (lambda (xobj) (if (= (quote string) (type xobj)) xobj (write-xml-tag xobj))) xml)))\n"
        "(define (xml-symbol v) (case (type v) ((quote string) v) ((quote ?else) (to-string v))))\n"
        "(define (write-xml-tag xtag) (case xtag ((dappend (list (quote ?tn) (quote ())) (quote ?body)) (concat \"<\" (xml-symbol tn) \">\" (write-xml body) \"</\" (xml-symbol tn) \">\")) ((dappend (list (quote ?tn) (quote ?attrs)) (quote ?body)) (concat \"<\" (xml-symbol tn) \" \" (cdelim (map write-xml-attribute attrs) \" \") \">\" (write-xml body) \"</\" (xml-symbol tn) \">\")) ((quote ?else) (to-string xtag))))\n"
        "(define (write-xml-attribute attr) (concat (to-string (head attr)) \"=\\\"\" (tail attr) \"\\\"\"))\n"
        "(define (xpath blob stmt) (define (stmt-p s) (define p1 (lsplit s \"(\")) (define p2 (read (if (tail p1) (head (lsplit (tail p1) \")\")) \"0\"))) (pair (read (head p1)) (if p2 p2 0))) (define (xpath-e blob e) (define ep (stmt-p e)) (define tag (head ep)) (define inst (tail ep)) (define tags (filter (lambda (el) (and (= (quote pair) (type el)) (> (length el) 0) (= tag (head el)))) blob)) (nth tags inst)) (foldl blob xpath-e (csplit stmt \"/\")))\n"
        "(define (make-trie s-map) (define (dud kseq v ents) (cond (kseq (define appnd-seq (untag (head kseq) ents)) (cond (appnd-seq (dud (tail kseq) v appnd-seq)) (t (define nseq (list (quote trie) (quote ()))) (dud (tail kseq) v nseq) (dappend ents (list (pair (head kseq) (tail nseq))))))) (t (set-head (tail ents) v)))) (define r (list (quote trie))) (for-each (lambda (s-v) (dud (unpack (head s-v)) (tail s-v) r)) s-map) (tail r))\n"
        "(define (trie-search str trie) (define r (foldl trie (lambda (ntrie k) (untag k ntrie)) (unpack str))) (if r (head r) nil))\n"
        "(define (read-config fname) (define f (open-file fname)) (define r nil) (define cp nil) (define s \"\") (while (not (eof? f)) (set s (trim (read-line f))) (if (not (or (= s \"\") (= \"#\" (substr s 0 1)))) (progn (set cp (lsplit s \"=\")) (set r (pair (pair (read (head cp)) (read (tail cp))) r))))) r)\n"
    );

    while (ss) Eval(ReadSExpression(ss, *(env->allocator())), env);
}

}
