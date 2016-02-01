# Scanner testing
* \n
* \\\\\\\\\\\\\\\\\\\\\\\\\\\\
* \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\f{fnord}
* \{foo}
* {lol}
* \{}
* \f\o\r\formula{abgefahren}

# Formula testing
* \formula{\lim_{x \to \infty} \exp(-x) = 0}
* \formula{\cos (2\theta) = \cos^2 \theta - \sin^2 \theta}
* \begin{equation}
  x = a_0 + \cfrac{1}{a_1 
          + \cfrac{1}{a_2 
          + \cfrac{1}{a_3 
	  + \cfrac{1}{a_4} } } }
\end{equation}

fehler!!!

* If \formula{a \to b} then \formula{b \to c}!

# Highlighting testing

* \bash{
find -O3 / -mount -type d -name '.git' | \
    parallel --delimiter "\n" --will-cite \
        cd \$\(dirname {}\) \; git add -A \;}

* \haskell{
main = do
    let twoD = array ((0,0), (1, 2)) [((i, j), i + j) | i <- [0..1], j <- [0..2]]
    putStrLn $ "2d: " ++ show twoD}

* \c{
main(){
	int a,b;
	for (a=0; a<10; a++)
		for(b=0; b<100;b++)
			printf("a: %i, b: %i\n", a, b);
}}

# Graph testing
* \dot{
    digraph foo {
	    a->b->c;
	    a->c->b;
    }
}
