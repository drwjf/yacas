
#ifndef __standard_h__
#define __standard_h__

#include "yacasbase.h"
#include "lispobject.h"
#include "lispenvironment.h"
#include "lisphash.h"
#include "lispatom.h"
#include "numbers.h"


// Prototypes
class LispHashTable;

LispBoolean InternalIsList(LispPtr& aPtr);
LispBoolean InternalIsString(LispStringPtr aOriginal);
void InternalUnstringify(LispString& aResult, LispStringPtr aOriginal);
void InternalStringify(LispString& aResult, LispStringPtr aOriginal);
void InternalIntToAscii(LispCharPtr aTrg,LispInt aInt);
LispInt InternalAsciiToInt(LispCharPtr aString);
LispBoolean IsNumber(LispCharPtr ptr,LispBoolean aAllowFloat);

void InternalNth(LispPtr& aResult, LispPtr& aArg, LispInt n);
void InternalTail(LispPtr& aResult, LispPtr& aArg);
void InternalAssociate(LispPtr& aResult, LispPtr& aKey,
                      LispPtr& aAssociationList);

void InternalReverseList(LispPtr& aResult, LispPtr& aOriginal);
void InternalFlatCopy(LispPtr& aResult, LispPtr& aOriginal);
LispInt InternalListLength(LispPtr& aOriginal);

LispBoolean InternalEquals(LispEnvironment& aEnvironment,
                           LispPtr& aExpression1,
                           LispPtr& aExpression2);


inline LispPtr& Argument(LispPtr& cur, LispInt n);
inline void InternalTrue(LispEnvironment& aEnvironment, LispPtr& aResult);
inline void InternalFalse(LispEnvironment& aEnvironment, LispPtr& aResult);
inline void InternalBoolean(LispEnvironment& aEnvironment, LispPtr& aResult,
                            LispBoolean aValue);
inline LispBoolean IsTrue(LispEnvironment& aEnvironment, LispPtr& aExpression);
inline LispBoolean IsFalse(LispEnvironment& aEnvironment, LispPtr& aExpression);
inline void InternalNot(LispPtr& aResult, LispEnvironment& aEnvironment, LispPtr& aExpression);

void DoInternalLoad(LispEnvironment& aEnvironment,LispInput* aInput);
void InternalLoad(LispEnvironment& aEnvironment,LispStringPtr aFileName);
void InternalUse(LispEnvironment& aEnvironment,LispStringPtr aFileName);
void InternalApplyString(LispEnvironment& aEnvironment, LispPtr& aResult,
                         LispStringPtr aOperator,LispPtr& aArgs);
void InternalApplyPure(LispPtr& oper,LispPtr& args2,LispPtr& aResult,LispEnvironment& aEnvironment);

void InternalEvalString(LispEnvironment& aEnvironment, LispPtr& aResult,
                        LispCharPtr aString);

LispInt StrCompare(LispCharPtr f1, LispCharPtr f2);

#define ATOML(_s) LispAtom::New(aEnvironment,aEnvironment.HashTable().LookUp(_s))
#define LIST(_c) LispSubList::New(_c)
class LispObjectAdder : public YacasBase
{
public:
    LispObjectAdder(LispObject* aPtr)
        : iPtr(aPtr) {};
   LispObject* iPtr;
};
#define LA(_o) LispObjectAdder(_o)

LispObject* operator+(const LispObjectAdder& left, const LispObjectAdder& right);

#define PARSE(_r,_s) ParseExpression(_r,_s,aEnvironment)
void ParseExpression(LispPtr& aResult,LispCharPtr aString,LispEnvironment& aEnvironment);

void ReturnUnEvaluated(LispPtr& aResult,LispPtr& aArguments,
                       LispEnvironment& aEnvironment);

/** PrintExpression : print an expression into a string,
 limiting it to a maximum number of characters. If aMaxChars
 is less than zero, the result is not truncated.
 */
void PrintExpression(LispString& aResult, LispPtr& aExpression,
                     LispEnvironment& aEnvironment,
                     LispInt aMaxChars);

LispStringPtr SymbolName(LispEnvironment& aEnvironment,LispCharPtr aSymbol);



#include "standard.inl"


#endif
