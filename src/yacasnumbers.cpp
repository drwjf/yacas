

/* Implementation of the number classes (the functionality used
 * by yacas any way
 */

#include "yacasprivate.h"
#include "yacasbase.h"
#include "numbers.h"
#include "standard.h"
#include "anumber.h"
#include "platmath.h"
#include "lisperror.h"

#ifdef HAVE_CONFIG_H
  #include <config.h>
#endif

#ifdef HAVE_STDIO_H
  #include <stdio.h>
#endif

static LispStringPtr FloatToString(ANumber& aInt, LispHashTable& aHashTable
                                  , LispInt aBase = 10);

LispInt NumericSupportForMantissa()
{
  return LispTrue;
    // TODO make it a mission in life to support mantissa!
//    return LispFalse;
}

const LispCharPtr NumericLibraryName()
{
    return "Internal";
}


/* Converting between internal formats and ascii format.
 * It is best done as little as possible. Usually, during calculations,
 * the ascii version of a number will not be required, so only the
 * internal version needs to be stored.
 */
void* AsciiToNumber(LispCharPtr aString,LispInt aPrecision)
{
    Check(IsNumber(aString,LispTrue),KLispErrInvalidArg);
    return NEW ANumber(aString,aPrecision);
}
LispStringPtr NumberToAscii(void* aNumber,LispHashTable& aHashTable,
                           LispInt aBase)
{
    return FloatToString(*((ANumber*)aNumber),aHashTable,aBase);
}

void* NumberCopy(void* aOriginal)
{
    ANumber* orig = (ANumber*)aOriginal;
    return NEW ANumber(*orig);
}

void NumberDestroy(void* aNumber)
{
    ANumber* orig = (ANumber*)aNumber;
    delete orig;
}


LispStringPtr GcdInteger(LispCharPtr int1, LispCharPtr int2,
                         LispHashTable& aHashTable)
{
    ANumber i1(int1,10);
    ANumber i2(int2,10);
    Check(i1.iExp == 0, KLispErrNotInteger);
    Check(i2.iExp == 0, KLispErrNotInteger);
    ANumber res(10);
    BaseGcd(res,i1,i2);
    LispStringPtr result = FloatToString(res, aHashTable);
    return result;
}

LispStringPtr MultiplyFloat(LispCharPtr int1, LispCharPtr int2,
                            LispHashTable& aHashTable,LispInt aPrecision)
{
    ANumber i1(int1,aPrecision);
    ANumber i2(int2,aPrecision);
    ANumber res(aPrecision);
    Multiply(res,i1,i2);
    LispStringPtr result = FloatToString(res, aHashTable);
    return result;
}

LispStringPtr AddFloat(LispCharPtr int1, LispCharPtr int2,
                       LispHashTable& aHashTable,LispInt aPrecision)
{
    ANumber i1(int1,aPrecision);
    ANumber i2(int2,aPrecision);
    ANumber res(aPrecision);
    Add(res,i1,i2);
    LispStringPtr result = FloatToString(res, aHashTable);
    return result;
}

LispStringPtr PlusFloat(LispCharPtr int1,LispHashTable& aHashTable
                       ,LispInt aPrecision)
{
    ANumber i1(int1,aPrecision);
    LispStringPtr result = FloatToString(i1, aHashTable);
    return result;
}


LispStringPtr SubtractFloat(LispCharPtr int1, LispCharPtr int2,
                            LispHashTable& aHashTable,LispInt aPrecision)
{
    ANumber i1(int1,aPrecision);
    ANumber i2(int2,aPrecision);
    ANumber res(aPrecision);
    Subtract(res,i1,i2);
    LispStringPtr result = FloatToString(res, aHashTable);
    return result;
}

LispStringPtr NegateFloat(LispCharPtr int1, LispHashTable& aHashTable
                          ,LispInt aPrecision)
{
    ANumber i1(int1,aPrecision);
    Negate(i1);
    LispStringPtr result = FloatToString(i1, aHashTable);
    return result;
}

LispStringPtr DivideFloat(LispCharPtr int1, LispCharPtr int2,
                          LispHashTable& aHashTable,LispInt aPrecision)
{
    ANumber i1(int1,aPrecision);
    ANumber i2(int2,aPrecision);

    Check(!IsZero(i2),KLispErrDivideByZero);

    if (IsZero(i1))
    {
        return aHashTable.LookUp("0");
    }
    ANumber res(aPrecision);
    ANumber remainder(aPrecision);
    Divide(res,remainder,i1,i2);
    LispStringPtr result = FloatToString(res, aHashTable);
    return result;
}


static void Trigonometry(ANumber& x,ANumber& i,ANumber& sum,ANumber& term)
{
    ANumber x2(sum.iPrecision);
    Multiply(x2,x,x);
    ANumber one("1",sum.iPrecision);
    ANumber dummy(10);

    LispInt requiredDigits = WordDigits(sum.iPrecision, 10)+
        x2.NrItems()-x2.iExp+1;
//    printf("WordDigits=%d\n",requiredDigits);
//    printf("[%d,%d]:",x.NrItems()-x.iExp,x.iExp);

    // While (term>epsilon)
    while (Significant(term))      
    {
        ANumber orig(sum.iPrecision);

        //   term <- term*x^2/((i+1)(i+2))
        //   i <= i+2

        // added this: truncate digits to speed up the calculation
        {
            LispInt toDunk = term.iExp - requiredDigits;
            if (toDunk > 0)
            {
                term.Delete(0,toDunk);
                term.iExp = requiredDigits;
            }
        }

        orig.CopyFrom(term);

        Multiply(term,orig,x2);
//
        BaseAdd(i, one, WordBase);
        orig.CopyFrom(term);
        Divide(term, dummy, orig, i);
//
        BaseAdd(i, one, WordBase);
        orig.CopyFrom(term);
        Divide(term, dummy, orig, i);

        //   negate term
        Negate(term);
        //   sum <- sum+term
        orig.CopyFrom(sum);
        Add(sum, orig, term);
    }
//    printf("[%d,%d]:",sum.NrItems()-sum.iExp,sum.iExp);
}

static void SinFloat(ANumber& aResult, ANumber& x)
{
    // i <- 1
    ANumber i("1",aResult.iPrecision);
    // sum <- x
    aResult.CopyFrom(x);
    // term <- x
    ANumber term(aResult.iPrecision);
    term.CopyFrom(x);
    Trigonometry(x,i,aResult,term);
}

static void SinFloat(ANumber& aResult, LispCharPtr int1)
{
    // Sin(x)=Sum(i=0 to Inf) (-1)^i x^(2i+1) /(2i+1)!
    // Which incrementally becomes the algorithm:
    //
    ANumber x(int1,aResult.iPrecision);
    SinFloat(aResult,x);
}

static void CosFloat(ANumber& aResult, ANumber& x)
{
    // i <- 0
    ANumber i("0",aResult.iPrecision);
    // sum <- 1
    aResult.SetTo("1");
    // term <- 1
    ANumber term("1",aResult.iPrecision);
    Trigonometry(x,i,aResult,term);
}

static void CosFloat(ANumber& aResult, LispCharPtr int1)
{
    // Cos(x)=Sum(i=0 to Inf) (-1)^i x^(2i) /(2i)!
    // Which incrementally becomes the algorithm:
    //
    ANumber x(int1,aResult.iPrecision);
    CosFloat(aResult,x);
}

LispStringPtr SinFloat(LispCharPtr int1, LispHashTable& aHashTable,LispInt aPrecision)
{
    ANumber sum(aPrecision);
    SinFloat(sum, int1);
    return FloatToString(sum, aHashTable);
}


LispStringPtr CosFloat(LispCharPtr int1, LispHashTable& aHashTable,LispInt aPrecision)
{
    ANumber sum(aPrecision);
    CosFloat(sum, int1);
    return FloatToString(sum, aHashTable);
}

LispStringPtr TanFloat(LispCharPtr int1, LispHashTable& aHashTable,LispInt aPrecision)
{
    // Tan(x) = Sin(x)/Cos(x)

    ANumber s(aPrecision);
    SinFloat(s, int1);

    ANumber c(aPrecision);
    CosFloat(c, int1);

    ANumber result(aPrecision);
    ANumber dummy(aPrecision);
    Divide(result,dummy,s,c);
    
    return FloatToString(result, aHashTable);

}

LispStringPtr ArcSinFloat(LispCharPtr int1, LispHashTable& aHashTable,LispInt aPrecision)
{
	// Use Newton's method to solve sin(x) = y by iteration:
    // x := x - (Sin(x) - y) / Cos(x)
	// this is similar to PiFloat()
	// we are using PlatArcSin() as the initial guess
	// maybe, for y very close to 1 or to -1 convergence will
	// suffer but seems okay in some tests
    LispStringPtr iResult = PlatArcSin(int1,  aHashTable, 0);
	ANumber result(iResult->String(), aPrecision);	// hack, hack, hack
	// how else do I get an ANumber from the result of PlatArcSin()?
    ANumber x(aPrecision);	// dummy variable
    ANumber q("10", aPrecision);	// initial value must be "significant"
    ANumber s(aPrecision);
    ANumber c(aPrecision);
	
	while (Significant(q))
    {
        x.CopyFrom(result);
        SinFloat(s, x);
		Negate(s);
        x.CopyFrom(s);
		ANumber y(int1, aPrecision);
		Add(s, x, y);
        // now s = y - Sin(x)
		x.CopyFrom(result);
        CosFloat(c, x);
        Divide(q,x,s,c);
		// now q = (y - Sin(x)) / Cos(x)

        // Calculate result:=result+q;
        x.CopyFrom(result);
        Add(result,x,q);
    }
    return FloatToString(result, aHashTable);
}

// ArcCosFloat should be defined in scripts through ArcSinFloat
LispStringPtr ArcCosFloat(LispCharPtr int1, LispHashTable& aHashTable,LispInt aPrecision)
{
    //TODO
    return PlatArcCos(int1,  aHashTable, 0);
}

LispStringPtr ArcTanFloat(LispCharPtr int1, LispHashTable& aHashTable,LispInt aPrecision)
{
    //TODO
    return PlatArcTan(int1,  aHashTable, 0);
}

static void ExpFloat(ANumber& aResult, ANumber& x)
{
    // Exp(x)=Sum(i=0 to Inf)  x^(i) /(i)!
    // Which incrementally becomes the algorithm:
    //
    ANumber one("1",aResult.iPrecision);
    // i <- 0
    ANumber i("0",aResult.iPrecision);     
    // sum <- 1
    aResult.SetTo("1");
    // term <- 1
    ANumber term("1",aResult.iPrecision);  
    ANumber dummy(10);

    LispInt requiredDigits = WordDigits(aResult.iPrecision, 10)+
        x.NrItems()-x.iExp+1;

    // While (term>epsilon)
    while (Significant(term))      
    {
        ANumber orig(aResult.iPrecision);

        {
            LispInt toDunk = term.iExp - requiredDigits;
            if (toDunk > 0)
            {
                term.Delete(0,toDunk);
                term.iExp = requiredDigits;
            }
        }

        
        //   i <- i+1
        BaseAdd(i, one, WordBase);     

        //   term <- term*x/(i)
        orig.CopyFrom(term);
        Multiply(term,orig,x);
        orig.CopyFrom(term);
        Divide(term, dummy, orig, i);

        //   sum <- sum+term
        orig.CopyFrom(aResult);
        Add(aResult, orig, term);
    }
}

LispStringPtr ExpFloat(LispCharPtr int1, LispHashTable& aHashTable,LispInt aPrecision)
{
    ANumber sum(aPrecision);
    ANumber x(int1,aPrecision);
    ExpFloat(sum, x);
    return FloatToString(sum, aHashTable);
}


static void LnFloat(ANumber& aResult, LispCharPtr int1)
{
    // Optimization for convergence: the following taylor
    // series converges faster when x is close to zero.
    // So a trick is to first take the square root a couple
    // of times, until x is sufficiently close to 1.
    

    // Ln(y) = Ln(1+x) = Sum(i=1 to inf) (-1)^(i+1) * x^(i) / i
    // thus y=1+x => x = y-1
    //

    LispInt shifts=0;
    LispBoolean smallenough=LispFalse;
    LispInt precision = 2*aResult.iPrecision;
    ANumber y(int1,precision);
    ANumber minusone("-1",precision);
    ANumber x(precision);

    ANumber delta("0.01",precision);
    while (!smallenough)
    {
        ANumber tosquare(precision);
        tosquare.CopyFrom(y);
        Sqrt(y,tosquare);
        shifts++;
        Add(x,y,minusone);
        if (BaseLessThan(x,delta))
            smallenough=LispTrue;
    }
    // i <- 0
    ANumber i("0",precision);
    // sum <- 1
    aResult.SetTo("0");
    // term <- 1
    ANumber term("-1",precision);
    ANumber dummy(10);

    ANumber one("1",precision);
    // While (term>epsilon)
    while (Significant(term))      
    {
        //   negate term
        Negate(term);

        ANumber orig(precision);

        orig.CopyFrom(term);
        Multiply(term,orig,x);
        //
        BaseAdd(i, one, WordBase);
        orig.CopyFrom(term);
        ANumber newTerm(precision);
        Divide(newTerm, dummy, orig, i);

        //   sum <- sum+term
        orig.CopyFrom(aResult);
        Add(aResult, orig, newTerm);
    }
    if (shifts)
        BaseShiftLeft(aResult,shifts);
}



LispStringPtr LnFloat(LispCharPtr int1, LispHashTable& aHashTable,LispInt aPrecision)
{
//TODO remove    return PlatLn(int1, aHashTable,aPrecision);
    ANumber sum(aPrecision);
    LnFloat(sum, int1);
    return FloatToString(sum, aHashTable);
}

LispStringPtr PowerFloat(LispCharPtr int1, LispCharPtr int2,
                         LispHashTable& aHashTable,LispInt aPrecision)
{
    // If is integer
    if (IsNumber(int2,LispFalse))
    {
        // Raising to the power of an integer can be done fastest by squaring
        // and bitshifting: x^(a+b) = x^a*x^b . Then, regarding each bit
        // in y (seen as a binary number) as added, the algorithm becomes:
        //
        ANumber x(int1,aPrecision);
        ANumber y(int2,aPrecision);
        LispBoolean neg = y.iNegative;
        y.iNegative=LispFalse;
        
        // result <- 1
        ANumber result("1",aPrecision);
        // base <- x
        ANumber base(aPrecision);
        base.CopyFrom(x);

        ANumber copy(aPrecision);

        // while (y!=0)
        while (!IsZero(y))
        {
            // if (y&1 != 0)
            if ( (y[0] & 1) != 0)
            {
                // result <- result*base
                copy.CopyFrom(result);
                Multiply(result,copy,base);
            }
            // base <- base*base
            copy.CopyFrom(base);
            Multiply(base,copy,copy);
            // y <- y>>1
            BaseShiftRight(y,1);
        }

        if (neg)
        {
            ANumber one("1",aPrecision);
            ANumber dummy(10);
            copy.CopyFrom(result);
            Divide(result,dummy,one,copy);
        }
        
        // result
        return FloatToString(result, aHashTable);
    }

    ANumber lnn(aPrecision);
    LnFloat(lnn, int1);

    ANumber exn(int2,aPrecision);

    ANumber x(aPrecision);
    Multiply(x,exn,lnn);
    ANumber result(aPrecision);
    ExpFloat(result, x);
    return FloatToString(result, aHashTable);
}



LispStringPtr SqrtFloat(LispCharPtr int1, LispHashTable& aHashTable,LispInt aPrecision)
{
    ANumber i1(int1,aPrecision);
    ANumber res(aPrecision);
    Sqrt(res,i1);
    LispStringPtr result = FloatToString(res, aHashTable);
    return result;
}

LispStringPtr AbsFloat( LispCharPtr int1, LispHashTable& aHashTable,LispInt aPrecision)
{
    ANumber i1(int1,aPrecision);
    i1.iNegative = LispFalse;
    LispStringPtr result = FloatToString(i1, aHashTable);
    return result;
}



LispBoolean LessThan(LispCharPtr int1, LispCharPtr int2,
                       LispHashTable& aHashTable,LispInt aPrecision)
{
    ANumber i1(int1,aPrecision);
    ANumber i2(int2,aPrecision);
    LispBoolean result = LessThan(i1,i2);
    return result;
}

LispBoolean GreaterThan(LispCharPtr int1, LispCharPtr int2,
                       LispHashTable& aHashTable,LispInt aPrecision)
{
    ANumber i1(int1,aPrecision);
    ANumber i2(int2,aPrecision);
    LispBoolean result = GreaterThan(i1,i2);
    return result;
}



LispStringPtr ShiftLeft( LispCharPtr int1, LispCharPtr int2, LispHashTable& aHashTable,LispInt aPrecision)
{
    ANumber i1(int1,aPrecision);
    LISPASSERT(i1.iExp == 0);

    LispInt bits = InternalAsciiToInt(int2);
    BaseShiftLeft(i1,bits);
    LispStringPtr result = FloatToString(i1, aHashTable);
    return result;
}


LispStringPtr ShiftRight( LispCharPtr int1, LispCharPtr int2, LispHashTable& aHashTable,LispInt aPrecision)
{
    ANumber i1(int1,aPrecision);
    LISPASSERT(i1.iExp == 0);

    LispInt bits = InternalAsciiToInt(int2);
    BaseShiftRight(i1,bits);
    LispStringPtr result = FloatToString(i1, aHashTable);
    return result;
}


LispStringPtr FromBase( LispCharPtr int1, LispCharPtr int2, LispHashTable& aHashTable,
                        LispInt aPrecision)
{
    LispInt base = InternalAsciiToInt(int1);
    ANumber i2(int2,aPrecision,base);
    LispStringPtr result = FloatToString(i2, aHashTable,10);
    return result;
}


LispStringPtr ToBase( LispCharPtr int1, LispCharPtr int2, LispHashTable& aHashTable,
                    LispInt aPrecision)
{
    LispInt base = InternalAsciiToInt(int1);
    ANumber i2(int2,aPrecision,10);
    LispStringPtr result = FloatToString(i2, aHashTable,base);
    return result;
}

LispStringPtr FloorFloat( LispCharPtr int1, LispHashTable& aHashTable,
                        LispInt aPrecision)
{
    ANumber i1(int1,aPrecision);
    LispInt i=0;
    LispInt fraciszero=LispTrue;

    while (i<i1.iExp && fraciszero)
    {
        if (i1[i] != 0)
            fraciszero=LispFalse;
        i++;
    }
    i1.Delete(0,i1.iExp);
    i1.iExp=0;
    if (i1.iNegative && !fraciszero)
    {
        ANumber orig(aPrecision);
        orig.CopyFrom(i1);
        ANumber minone("-1",10);
        Add(i1,orig,minone);
    }
    return FloatToString(i1, aHashTable,10);
}

LispStringPtr CeilFloat( LispCharPtr int1, LispHashTable& aHashTable,
                         LispInt aPrecision)
{
    ANumber i1(int1,aPrecision);
    LispInt i=0;
    LispInt fraciszero=LispTrue;
    while (i<i1.iExp && fraciszero)
    {
        if (i1[i] != 0)
            fraciszero=LispFalse;
        i++;
    }
    i1.Delete(0,i1.iExp);
    i1.iExp=0;
    if (!i1.iNegative && !fraciszero)
    {
        ANumber orig(aPrecision);
        orig.CopyFrom(i1);
        ANumber one("1",10);
        Add(i1,orig,one);
    }
    return FloatToString(i1, aHashTable,10);
}

static void DivideInteger( ANumber& aQuotient, ANumber& aRemainder,
                        LispCharPtr int1, LispCharPtr int2, 
                        LispInt aPrecision)
{
    ANumber a1(int1,aPrecision);
    ANumber a2(int2,aPrecision);
    
    Check(a1.iExp == 0, KLispErrNotInteger);
    Check(a2.iExp == 0, KLispErrNotInteger);
    Check(!IsZero(a2),KLispErrInvalidArg);
    IntegerDivide(aQuotient, aRemainder, a1, a2);
}

LispStringPtr ModFloat( LispCharPtr int1, LispCharPtr int2, LispHashTable& aHashTable,
                        LispInt aPrecision)
{
    ANumber quotient(static_cast<LispInt>(0));
    ANumber remainder(static_cast<LispInt>(0));
    DivideInteger( quotient, remainder, int1, int2, aPrecision);
    return FloatToString(remainder, aHashTable,10);

}

LispStringPtr DivFloat( LispCharPtr int1, LispCharPtr int2, LispHashTable& aHashTable,
                        LispInt aPrecision)
{
    ANumber quotient(static_cast<LispInt>(0));
    ANumber remainder(static_cast<LispInt>(0));
    DivideInteger( quotient, remainder, int1, int2, aPrecision);
    return FloatToString(quotient, aHashTable,10);
}

LispStringPtr PiFloat( LispHashTable& aHashTable,
                        LispInt aPrecision)
{
    // Newton's method for finding pi:
    // x[0] := 3.1415926
    // x[n+1] := x[n] + Sin(x[n])

	LispInt initial_prec = aPrecision;	// target precision of first iteration, will be computed below
    LispInt cur_prec = 40;  // precision of the initial guess
    ANumber result("3.141592653589793238462643383279502884197169399",cur_prec + 3);    // initial guess is stored with 3 guard digits
    ANumber x(cur_prec);
	ANumber s(cur_prec);

	// optimize precision sequence
	while (initial_prec > cur_prec*3)
		initial_prec = int((initial_prec+2)/3);
	cur_prec = initial_prec;
    while (cur_prec <= aPrecision)
    {
 		// start of iteration code
		result.ChangePrecision(cur_prec);	// result has precision cur_prec now
        // Get Sin(result)
        x.CopyFrom(result);
		s.ChangePrecision(cur_prec);
        SinFloat(s, x);
        // Calculate new result: result := result + Sin(result);
        x.CopyFrom(result);	// precision cur_prec
        Add(result,x,s);
		// end of iteration code
		// decide whether we are at end of loop now
		if (cur_prec == aPrecision)	// if we are exactly at full precision, it's the last iteration
			cur_prec = aPrecision+1;	// terminate loop
		else {
			cur_prec *= 3;	// precision triples at each iteration
			// need to guard against overshooting precision
 			if (cur_prec > aPrecision)
				cur_prec = aPrecision;	// next will be the last iteration
		}
    }
	
//    return aHashTable.LookUp("3.14"); // Just kidding, Serge ;-)
    return FloatToString(result, aHashTable);
}



static LispStringPtr FloatToString(ANumber& aInt,
                            LispHashTable& aHashTable, LispInt aBase)
{
    LispString result;
    ANumberToString(result, aInt, aBase);
    return aHashTable.LookUp(result.String());
}



LispStringPtr BitAnd(LispCharPtr int1, LispCharPtr int2,
                     LispHashTable& aHashTable,LispInt aPrecision)
{
    ANumber i1(int1,aPrecision);
    ANumber i2(int2,aPrecision);
    Check(i1.iExp == 0, KLispErrNotInteger);
    Check(i2.iExp == 0, KLispErrNotInteger);

    ANumber res(aPrecision);
    LispInt len1=i1.NrItems(), len2=i2.NrItems();
    LispInt min=len1,max=len2;
    if (min>max)
    {
        LispInt swap=min;
        min=max;
        max=swap;
    }
    res.GrowTo(min);
    LispInt i;
    for (i=0;i<len1 && i<len2;i++)
    {
        res[i] = i1[i] & i2[i];
    }

    LispStringPtr result = FloatToString(res, aHashTable);
    return result;
}

LispStringPtr BitOr(LispCharPtr int1, LispCharPtr int2,
                     LispHashTable& aHashTable,LispInt aPrecision)
{
    ANumber i1(int1,aPrecision);
    ANumber i2(int2,aPrecision);
    Check(i1.iExp == 0, KLispErrNotInteger);
    Check(i2.iExp == 0, KLispErrNotInteger);
    ANumber res(aPrecision);
    LispInt len1=i1.NrItems(), len2=i2.NrItems();
    LispInt min=len1,max=len2;
    if (min>max)
    {
        LispInt swap=min;
        min=max;
        max=swap;
    }
    
    res.GrowTo(max);

    LispInt i;
    for (i=0;i<len1 && i<len2;i++)
    {
        res[i] = i1[i] | i2[i];
    }
    for (i=len1;i<len2;i++)
    {
        res[i] = i2[i];
    }
    for (i=len2;i<len1;i++)
    {
        res[i] = i1[i];
    }
    
    LispStringPtr result = FloatToString(res, aHashTable);
    return result;
}

LispStringPtr BitXor(LispCharPtr int1, LispCharPtr int2,
                     LispHashTable& aHashTable,LispInt aPrecision)
{
    ANumber i1(int1,aPrecision);
    ANumber i2(int2,aPrecision);
    Check(i1.iExp == 0, KLispErrNotInteger);
    Check(i2.iExp == 0, KLispErrNotInteger);
    ANumber res(aPrecision);
    LispInt len1=i1.NrItems(), len2=i2.NrItems();
    LispInt min=len1,max=len2;
    if (min>max)
    {
        LispInt swap=min;
        min=max;
        max=swap;
    }
    
    res.GrowTo(max);

    LispInt i;
    for (i=0;i<len1 && i<len2;i++)
    {
        res[i] = i1[i] ^ i2[i];
    }
    for (i=len1;i<len2;i++)
    {
        res[i] = i2[i];
    }
    for (i=len2;i<len1;i++)
    {
        res[i] = i1[i];
    }
    
    LispStringPtr result = FloatToString(res, aHashTable);
    return result;
}

LispStringPtr LispFactorial(LispCharPtr int1, LispHashTable& aHashTable,LispInt aPrecision)
{
    LispInt nr = InternalAsciiToInt(int1);
    Check(nr>=0,KLispErrInvalidArg);
    ANumber fac("1",aPrecision);
    LispInt i;
    for (i=2;i<=nr;i++)
    {
        BaseTimesInt(fac,i, WordBase);
    }
/*
    for (i=2;i<nr;i+=2)
    {
        BaseTimesInt(fac,i*(i+1), WordBase);
    }
    if (i==nr) BaseTimesInt(fac, i, WordBase);
*/
    return FloatToString(fac, aHashTable);
}

/* This code will compute factorials faster when multiplication becomes better than quadratic time

// return old result*product of all integers from iLeft to iRight
void tree_factorial(ANumber& result, LispInt iLeft, LispInt iRight, LispInt aPrecision)
{
	if (iRight == iLeft) BaseTimesInt(result, iLeft, WordBase);
	else if (iRight == iLeft + 1) BaseTimesInt(result, iLeft*iRight, WordBase);
	else if (iRight == iLeft + 2) BaseTimesInt(result, iLeft*iRight*(iLeft+1), WordBase);
	else
	{
	    ANumber fac1("1", aPrecision), fac2("1", aPrecision);
	    LispInt i = (iLeft+iRight)>>1;
		tree_factorial(fac1, iLeft, i, aPrecision);
		tree_factorial(fac2, i+1, iRight, aPrecision);
		Multiply(result, fac1, fac2);
	}
}

LispStringPtr LispFactorial(LispCharPtr int1, LispHashTable& aHashTable,LispInt aPrecision)
{
    LispInt nr = InternalAsciiToInt(int1);
    Check(nr>=0,KLispErrInvalidArg);
	ANumber fac("1",aPrecision);
	tree_factorial(fac, 1, nr, aPrecision);
    return FloatToString(fac, aHashTable);
}

*/






#ifndef USE_NATIVE


BigNumber::BigNumber(const LispCharPtr aString,LispInt aPrecision,LispInt aBase)
{
  iNumber = NULL;
  SetTo(aString, aPrecision, aBase);
}
BigNumber::BigNumber(const BigNumber& aOther)
{
  iNumber = NEW ANumber(aOther.GetPrecision());
  SetTo(aOther);
}
BigNumber::BigNumber(LispInt aPrecision)
{
  iPrecision = aPrecision;
  iNumber = NEW ANumber(aPrecision);
  SetIsInteger(LispTrue);
}

BigNumber::~BigNumber()
{
  delete iNumber;
}

void BigNumber::SetTo(const BigNumber& aOther)
{
  iPrecision = aOther.GetPrecision();
  if (iNumber == NULL) iNumber = NEW ANumber(aOther.GetPrecision());
  iNumber->CopyFrom(*aOther.iNumber);
  SetIsInteger(aOther.IsInt());
}
void BigNumber::ToString(LispString& aResult, LispInt aPrecision, LispInt aBase) const
{
  ANumber num(aPrecision);
  num.CopyFrom(*iNumber);
  if (num.iExp > 1)
    num.RoundBits();
  num.ChangePrecision(aPrecision);

#define ENABLE_SCI_NOTATION
#ifdef ENABLE_SCI_NOTATION
  if (!IsInt())
  {
    for(;;)
    {
      LispInt i;
      LispBoolean greaterOne = LispFalse;
      if (num.iExp >= num.NrItems()) break;
      for (i=num.iExp;i<num.NrItems();i++)
      {
        if (num[i] != 0) 
        {
          if (!(i==num.iExp && num[i]<10000 && num.iTensExp == 0))
          {
            greaterOne=LispTrue;
            break;
          }
        }
      }
      if (!greaterOne) break;
      PlatDoubleWord carry=0;
      BaseDivideInt(num,10, WordBase, carry);
      num.iTensExp++;
    }
  }
#endif

  ANumberToString(aResult, num, aBase,(iType == KFloat));
}
double BigNumber::Double() const
{
// There are platforms that don't have strtod
#ifdef HAVE_STRTOD
  LispString str;
  ANumber num(iNumber->iPrecision);
  num.CopyFrom(*iNumber);
  ANumberToString(str, num, 10);
  char* endptr;
  return strtod(str.String(),&endptr);
#else
  //FIXME
  LISPASSERT(0);
  return 0.0;
#endif
}

const LispCharPtr BigNumber::NumericLibraryName()
{
  return "Internal Yacas numbers";
}

void BigNumber::Multiply(const BigNumber& aX, const BigNumber& aY, LispInt aPrecision)
{
  SetIsInteger(aX.IsInt() && aY.IsInt());
  iNumber->ChangePrecision(aPrecision);
  ANumber a1(aPrecision);
  a1.CopyFrom(*aX.iNumber);
  ANumber a2(aPrecision);
  a2.CopyFrom(*aY.iNumber);
  :: Multiply(*iNumber,a1,a2);
}
void BigNumber::MultiplyAdd(const BigNumber& aX, const BigNumber& aY, LispInt aPrecision)
{//FIXME
  BigNumber mult;
  mult.Multiply(aX,aY,aPrecision);
  Add(*this,mult,aPrecision);
}
void BigNumber::Add(const BigNumber& aX, const BigNumber& aY, LispInt aPrecision)
{
  SetIsInteger(aX.IsInt() && aY.IsInt());
  ANumber a1(aPrecision);
  a1.CopyFrom(*aX.iNumber);
  ANumber a2(aPrecision);
  a2.CopyFrom(*aY.iNumber);
	::Add(*iNumber, a1, a2);
}
void BigNumber::Negate(const BigNumber& aX)
{
  if (aX.iNumber != iNumber)
  {
    iNumber->CopyFrom(*aX.iNumber);
  }
  ::Negate(*iNumber);
  SetIsInteger(aX.IsInt());
}
void BigNumber::Divide(const BigNumber& aX, const BigNumber& aY, LispInt aPrecision)
{
  ANumber a1(aPrecision);
  a1.CopyFrom(*aX.iNumber);
  ANumber a2(aPrecision);
  a2.CopyFrom(*aY.iNumber);
  ANumber remainder(aPrecision);

  if (aX.IsInt() && aY.IsInt())
  { 
    Check(a1.iExp == 0, KLispErrNotInteger);
    Check(a2.iExp == 0, KLispErrNotInteger);
    Check(!IsZero(a2),KLispErrInvalidArg);
    SetIsInteger(LispTrue);
    ::IntegerDivide(*iNumber, remainder, a1, a2);
  }
  else
  {
    SetIsInteger(LispFalse);
    ::Divide(*iNumber,remainder,a1,a2);
  }
}
void BigNumber::ShiftLeft(const BigNumber& aX, LispInt aNrToShift)
{
  if (aX.iNumber != iNumber)
  {
    iNumber->CopyFrom(*aX.iNumber);
  }
  ::BaseShiftLeft(*iNumber,aNrToShift);
}
void BigNumber::ShiftRight(const BigNumber& aX, LispInt aNrToShift)
{
  if (aX.iNumber != iNumber)
  {
    iNumber->CopyFrom(*aX.iNumber);
  }
  ::BaseShiftRight(*iNumber,aNrToShift);
}
void BigNumber::BitAnd(const BigNumber& aX, const BigNumber& aY)
{
  LispInt len1=aX.iNumber->NrItems(), len2=aY.iNumber->NrItems();
  LispInt min=len1,max=len2;
  if (min>max)
  {
    LispInt swap=min;
    min=max;
    max=swap;
  }
  iNumber->GrowTo(min);
  LispInt i;
  for (i=0;i<len1 && i<len2;i++)
  {
    (*iNumber)[i] = (*aX.iNumber)[i] & (*aY.iNumber)[i];
  }
}
void BigNumber::BitOr(const BigNumber& aX, const BigNumber& aY)
{
  LispInt len1=(*aX.iNumber).NrItems(), len2=(*aY.iNumber).NrItems();
  LispInt min=len1,max=len2;
  if (min>max)
  {
    LispInt swap=min;
    min=max;
    max=swap;
  }
  
  iNumber->GrowTo(max);

  LispInt i;
  for (i=0;i<len1 && i<len2;i++)
  {
    (*iNumber)[i] = (*aX.iNumber)[i] | (*aY.iNumber)[i];
  }
  for (i=len1;i<len2;i++)
  {
    (*iNumber)[i] = (*aY.iNumber)[i];
  }
  for (i=len2;i<len1;i++)
  {
    (*iNumber)[i] = (*aX.iNumber)[i];
  }
}
void BigNumber::BitXor(const BigNumber& aX, const BigNumber& aY)
{
  LispInt len1=(*aX.iNumber).NrItems(), len2=(*aY.iNumber).NrItems();
  LispInt min=len1,max=len2;
  if (min>max)
  {
    LispInt swap=min;
    min=max;
    max=swap;
  }
  
  iNumber->GrowTo(max);

  LispInt i;
  for (i=0;i<len1 && i<len2;i++)
  {
    (*iNumber)[i] = (*aX.iNumber)[i] ^ (*aY.iNumber)[i];
  }
  for (i=len1;i<len2;i++)
  {
    (*iNumber)[i] = (*aY.iNumber)[i];
  }
  for (i=len2;i<len1;i++)
  {
    (*iNumber)[i] = (*aX.iNumber)[i];
  }
}

void BigNumber::BitNot(const BigNumber& aX)
{// FIXME?
  LispInt len=(*aX.iNumber).NrItems();
  
  iNumber->GrowTo(len);

  LispInt i;
  for (i=0;i<len;i++)
  {
    (*iNumber)[i] = ~((*aX.iNumber)[i]);
  }
}

/// return true if the bit count fits into signed long
LispBoolean BigNumber::BitCountIsSmall() const
{//FIXME
	return LispTrue;
}

/// Bit count operation: return the number of significant bits if integer, return the binary exponent if float (shortcut for binary logarithm)
void BigNumber::BitCount(const BigNumber& aX)
{
  if (aX.BitCountIsSmall())
  {
    SetTo(aX.BitCount());
  }
  else
  {// FIXME
    LISPASSERT(0);
  }
}

// give BitCount as platform integer
signed long BigNumber::BitCount() const
{
  ANumber num(iPrecision);
  num.CopyFrom(*iNumber);
  while (num.iTensExp < 0)
  {
    PlatDoubleWord carry=0;
    BaseDivideInt(num,10, WordBase, carry);
    num.iTensExp++;
  }
  while (num.iTensExp > 0)
  {
    BaseTimesInt(num,10, WordBase);
    num.iTensExp--;
  }

  LispInt i,nr=num.NrItems();
  for (i=nr-1;i>=0;i--) 
  {
    if (num[i] != 0) break;
  }
  LispInt bits=(i-num.iExp)*sizeof(PlatWord)*8;
  if (i>=0)
  {
    PlatWord w=num[i];
    while (w) 
    {
      w>>=1;
      bits++;
    }
  }
  return (bits);
}
LispInt BigNumber::Sign() const
{
  if (iNumber->iNegative) return -1;
  if (IsZero(*iNumber)) return 0;
  return 1;
}




/// integer operation: *this = y mod z
void BigNumber::Mod(const BigNumber& aY, const BigNumber& aZ)
{
    ANumber a1(iPrecision);
    ANumber a2(iPrecision);
    a1.CopyFrom(*aY.iNumber);
    a2.CopyFrom(*aZ.iNumber);
    Check(a1.iExp == 0, KLispErrNotInteger);
    Check(a2.iExp == 0, KLispErrNotInteger);
    Check(!IsZero(a2),KLispErrInvalidArg);

    ANumber quotient(static_cast<LispInt>(0));
    ::IntegerDivide(quotient, *iNumber, a1, a2);

    if (iNumber->iNegative)
    {
      ANumber a3(iPrecision);
      a3.CopyFrom(*iNumber);
      ::Add(*iNumber, a3, a2);
    }
    SetIsInteger(LispTrue);
}

void BigNumber::Floor(const BigNumber& aX)
{
//TODO FIXME slow code! But correct
    LispString str;
    iNumber->CopyFrom(*aX.iNumber);
    if (iNumber->iExp>1)
      iNumber->RoundBits();

//    aX.ToString(str,aX.GetPrecision());
//    iNumber->SetTo(str.String());

    if (iNumber->iTensExp > 0)
    {
      while (iNumber->iTensExp > 0)
      {
        BaseTimesInt(*iNumber,10, WordBase);
        iNumber->iTensExp--;
      }
    }
    else if (iNumber->iTensExp < 0)
    {
      while (iNumber->iTensExp < 0)
      {
        PlatDoubleWord carry;
        BaseDivideInt(*iNumber,10, WordBase, carry);
        iNumber->iTensExp++;
      }
    }
    iNumber->ChangePrecision(iNumber->iPrecision);
    LispInt i=0;
    LispInt fraciszero=LispTrue;
    while (i<iNumber->iExp && fraciszero)
    {
        PlatWord digit = (*iNumber)[i];
        if (digit != 0)
            fraciszero=LispFalse;
        i++;
    }
    iNumber->Delete(0,iNumber->iExp);
    iNumber->iExp=0;

    if (iNumber->iNegative && !fraciszero)
    {
        ANumber orig(iPrecision);
        orig.CopyFrom(*iNumber);
        ANumber minone("-1",10);
        ::Add(*iNumber,orig,minone);
    }
    SetIsInteger(LispTrue);
}


void BigNumber::Precision(LispInt aPrecision)
{//FIXME
  if (aPrecision < iNumber->iPrecision)
  {
  }
  else
  {
    iNumber->ChangePrecision(aPrecision);
  }
  SetIsInteger(iNumber->iExp == 0 && iNumber->iTensExp == 0);
  iPrecision = aPrecision;
}


//basic object manipulation
LispBoolean BigNumber::Equals(const BigNumber& aOther) const
{
/*Not working yet? Not good rounding...
  LispInt lowest = iNumber->iPrecision;
  if (aOther.iNumber->iPrecision<lowest)
    lowest = aOther.iNumber->iPrecision;
  ANumber a1(lowest);
  a1.CopyFrom(*iNumber);
  ANumber a2(lowest);
  a2.CopyFrom(*aOther.iNumber);
  a1.ChangePrecision(lowest);
  a2.ChangePrecision(lowest);
  ANumber diff(lowest);
  ANumber otherNeg(lowest);
  ::Negate(a2);
	::Add(diff, a1, a2);
  return !Significant(diff);
 */
/*TODO remove, old? */

  BigNumber diff;
  BigNumber otherNeg;
  otherNeg.Negate(aOther);
  diff.Add(*this,otherNeg,GetPrecision());
  diff.iNumber->ChangePrecision(iPrecision);

  return !Significant(*diff.iNumber);
/* */
}


LispBoolean BigNumber::IsInt() const
{
  return (iType == KInt);
}


LispBoolean BigNumber::IsIntValue() const
{
//FIXME I need to round first to get more reliable results.
  if (IsInt()) return LispTrue;
  if (iNumber->iExp == 0 && iNumber->iTensExp == 0) return LispTrue;
  
  BigNumber num(iPrecision);
  num.Floor(*this);
  return Equals(num);

}


LispBoolean BigNumber::IsSmall() const
{
  if (IsInt())
  {
    PlatWord* ptr = &((*iNumber)[iNumber->NrItems()-1]);
    LispInt nr=iNumber->NrItems();
    while (nr>1 && *ptr == 0) {ptr--;nr--;}
    return (nr <= iNumber->iExp+1);
  }
  else
  // a function to test smallness of a float is not present in ANumber, need to code a workaround to determine whether a mpf_t fits into double.
  {
    LispInt tensExp = iNumber->iTensExp;
    if (tensExp<0)tensExp = -tensExp;
    return
    (
      iPrecision <= 53	// standard float is 53 bits
      && tensExp<1021
    );
    // standard range of double precision is about 53 bits of mantissa and binary exponent of about 1021
  }
}


void BigNumber::BecomeInt()
{
  iNumber->ChangePrecision(0);
  SetIsInteger(LispTrue);
}

/// Transform integer to float, setting a given bit precision.
/// Note that aPrecision=0 means automatic setting (just enough digits to represent the integer).
void BigNumber::BecomeFloat(LispInt aPrecision)
{//FIXME: need to specify precision explicitly
  if (IsInt())
  {
    LispInt precision = aPrecision;
    if (iNumber->iPrecision>aPrecision)
      precision = iNumber->iPrecision;
    iNumber->ChangePrecision(precision);	// is this OK or ChangePrecision means floating-point precision?
    SetIsInteger(LispFalse);
  }
}


LispBoolean BigNumber::LessThan(const BigNumber& aOther) const
{
  ANumber a1(iPrecision);
  a1.CopyFrom(*this->iNumber);
  ANumber a2(iPrecision);
  a2.CopyFrom(*aOther.iNumber);
	return ::LessThan(a1, a2);
}

// assign from a platform type
void BigNumber::SetTo(long aValue)
{
#ifdef HAVE_STDIO_H
  char dummy[150];
  //FIXME platform code
  sprintf(dummy,"%ld",aValue);
  SetTo(dummy,iPrecision,10);
  SetIsInteger(LispTrue);
#else
  //FIXME
  LISPASSERT(0);
#endif
}


void BigNumber::SetTo(double aValue)
{
#ifdef HAVE_STDIO_H
  iPrecision = 53;	// standard double has 53 bits
  char dummy[150];
  //FIXME platform code
  char format[20];
  sprintf(format,"%%.%dg",iPrecision);
  sprintf(dummy,format,aValue);
  SetTo(dummy,iPrecision,10);
  SetIsInteger(LispFalse);
#else
  //FIXME
  LISPASSERT(0);
#endif
}


// assign from string
void BigNumber::SetTo(const LispCharPtr aString,LispInt aPrecision,LispInt aBase)
{//FIXME
  iPrecision = aPrecision;
  LispBoolean isFloat = 0;
  const LispCharPtr ptr = aString;
  while (*ptr && *ptr != '.') ptr++;
  if (*ptr == '.')
  {
    isFloat = 1;
    ptr++;
    const LispCharPtr start = ptr;
    while (IsDigit(*ptr)) ptr++;
    LispInt digits = ptr-start;
    if (digits>aPrecision) 
      aPrecision = digits;
  }
  if (iNumber == NULL)   iNumber = NEW ANumber(aPrecision);
  iNumber->SetPrecision(aPrecision);
  iNumber->SetTo(aString,aBase);
  
//TODO remove old  iNumber = NEW ANumber(aString,aPrecision,aBase);
  SetIsInteger(!isFloat && iNumber->iExp == 0 && iNumber->iTensExp == 0);
}


void BigNumber::ShiftLeft(const BigNumber& aX, const BigNumber& aNrToShift)
{
  // first, see if we can use short numbers
  if (aNrToShift.IsInt() && aNrToShift.Sign()>=0)
  {
    if (aNrToShift.IsSmall())
    {
      long shift_amount=(*aNrToShift.iNumber)[0];
      ShiftLeft(aX, shift_amount);
    }
    else
    {
      // only floats can be shifted by a non-small number, so convert to float and use exponent_
      // FIXME: make this work for large shift amounts
    }
  }	// do nothing if shift amount is not integer or negative
}



void BigNumber::ShiftRight(const BigNumber& aX, const BigNumber& aNrToShift)
{
  // first, see if we can use short numbers
  if (aNrToShift.IsInt() && aNrToShift.Sign()>=0)
  {
    if (aNrToShift.IsSmall())
    {
      long shift_amount=(*aNrToShift.iNumber)[0];
      ShiftRight(aX, shift_amount);
    }
    else
    {
      // only floats can be shifted by a non-small number, so convert to float and use exponent_
      // FIXME: make this work for large shift amounts
    }
  }	// do nothing if shift amount is not integer or negative
}



#endif
