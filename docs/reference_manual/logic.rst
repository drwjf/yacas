==================================
Propositional logic theorem prover
==================================

.. function:: CanProve(proposition)

   try to prove statement

   :param proposition: an expression with logical operations

   Yacas has a small built-in propositional logic theorem prover.  It
   can be invoked with a call to {CanProve}.    An example of a
   proposition is: "if a implies b and b implies c then  a implies c".
   Yacas supports the following logical operations:    {Not} :
   negation, read as "not"    {And}  :   conjunction, read as "and"
   {Or}  :   disjunction, read as "or"    {=>}  : implication, read as
   "implies"    The abovementioned proposition would be represented by
   the following expression,           ( (a=>b) And (b=>c) ) => (a=>c)
   Yacas can prove that is correct by applying {CanProve}  to it:
   In> CanProve(( (a=>b) And (b=>c) ) => (a=>c))         Out> True;
   It does this in the following way: in order to prove a proposition
   $p$, it  suffices to prove that  $Not p$ is false. It continues to
   simplify  $Not p$  using the rules:           Not  ( Not x)
   --> x  (eliminate double negation),         x=>y  -->  Not x  Or  y
   (eliminate implication),         Not (x And y)  -->  Not x  Or  Not
   y  (De Morgan's law),         Not (x Or y) -->  Not x  And  Not y
   (De Morgan's law),         (x And y) Or z --> (x Or z) And (y Or z)
   (distribution),         x Or (y And z) --> (x Or y) And (x Or z)
   (distribution),  and the obvious other rules, such as,         True
   Or x --> True  etc.  The above rules will translate a proposition
   into a form           (p1  Or  p2  Or  ...)  And  (q1  Or  q2
   Or  ...)  And ...  If any of the clauses is false, the entire
   expression will be false.  In the next step, clauses are scanned
   for situations of the form:           (p Or Y)  And  ( Not p Or Z)
   --> (Y Or Z)  If this combination {(Y Or Z)} is empty, it is false,
   and  thus the entire proposition is false.    As a last step, the
   algorithm negates the result again. This has the  added advantage
   of simplifying the expression further.

   :Example:

   ::

      In> CanProve(a  Or   Not a)
      Out> True;
      In> CanProve(True  Or  a)
      Out> True;
      In> CanProve(False  Or  a)
      Out> a;
      In> CanProve(a  And   Not a)
      Out> False;
      In> CanProve(a  Or b Or (a And b))
      Out> a Or b;
      

   .. seealso:: :func:`True`, :func:`False`, :func:`And`, :func:`Or`, :func:`Not`

