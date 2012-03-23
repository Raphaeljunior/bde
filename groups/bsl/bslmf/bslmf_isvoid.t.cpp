// bslmf_isvoid.t.cpp                                                 -*-C++-*-
#include <bslmf_isvoid.h>

#include <bsls_bsltestutil.h>

#include <stdio.h>      // sprintf()
#include <stdlib.h>     // atoi()

using namespace BloombergLP;

//=============================================================================
//                                TEST PLAN
//-----------------------------------------------------------------------------
//                                Overview
//                                --------
//-----------------------------------------------------------------------------
// [ 4] bslmf::IsVoid conversion to bslmf_MetaInt
// [ 3] bslmf::IsVoid::Type
// [ 2] bslmf::IsVoid::VALUE
//-----------------------------------------------------------------------------
// [ 1] BREATHING TEST
// [ 5] USAGE EXAMPLE
//=============================================================================
//                       STANDARD BDE ASSERT TEST MACRO
//-----------------------------------------------------------------------------
// NOTE: THIS IS A LOW-LEVEL COMPONENT AND MAY NOT USE ANY C++ LIBRARY
// FUNCTIONS, INCLUDING IOSTREAMS.
static int testStatus = 0;

void aSsErT(bool b, const char *s, int i)
{
    if (b) {
        printf("Error " __FILE__ "(%d): %s    (failed)\n", i, s);
        if (testStatus >= 0 && testStatus <= 100) ++testStatus;
    }
}

# define ASSERT(X) { aSsErT(!(X), #X, __LINE__); }

//=============================================================================
//                       STANDARD BDE TEST DRIVER MACROS
//-----------------------------------------------------------------------------
#define LOOP_ASSERT  BSLS_BSLTESTUTIL_LOOP_ASSERT
#define LOOP2_ASSERT BSLS_BSLTESTUTIL_LOOP2_ASSERT
#define LOOP3_ASSERT BSLS_BSLTESTUTIL_LOOP3_ASSERT
#define LOOP4_ASSERT BSLS_BSLTESTUTIL_LOOP4_ASSERT
#define LOOP5_ASSERT BSLS_BSLTESTUTIL_LOOP5_ASSERT
#define LOOP6_ASSERT BSLS_BSLTESTUTIL_LOOP6_ASSERT

#define Q   BSLS_BSLTESTUTIL_Q   // Quote identifier literally.
#define P   BSLS_BSLTESTUTIL_P   // Print identifier and value.
#define P_  BSLS_BSLTESTUTIL_P_  // P(X) without '\n'.
#define T_  BSLS_BSLTESTUTIL_T_  // Print a tab (w/o newline).
#define L_  BSLS_BSLTESTUTIL_L_  // current Line number

//=============================================================================
//                    GLOBAL CONSTANTS FOR TESTING
//-----------------------------------------------------------------------------

bool globalVerbose         = false;
bool globalVeryVerbose     = false;
bool globalVeryVeryVerbose = false;

//=============================================================================
//                  GLOBAL HELPER MACROS FOR TESTING
//-----------------------------------------------------------------------------
// These standard aliases will be defined below as part of the usage example.

//=============================================================================
//                  GLOBAL HELPER FUNCTIONS FOR TESTING
//-----------------------------------------------------------------------------
namespace
{
struct Empty {
    // This 'struct' is an empty dummy type to allow formation of pointer-to-
    // member types for testing.  Note that as all such uses expected for
    // testing this component will be initialized only with null values, there
    // is no need for any actual members to point to.
};

template<class TYPE>
struct Identity {
    // This metafunction provides a type-dependent context for testing the
    // behavior of type traits.

    typedef TYPE Type;
};

struct Incomplete;
    // A forward declaration for the type that shall remain incomplete for this
    // whole translation unit.  This is provided solely for the purpose of
    // testing the 'bslmf::IsVoid' metafunction with incomplete types.

typedef bslmf_MetaInt<0> FalseType;
typedef bslmf_MetaInt<1> TrueType;

template<int N>
bool dispatchFalseType(const bslmf_MetaInt<N>&) { return false; }
bool dispatchFalseType(FalseType)               { return true;  }

template<int N>
bool dispatchTrueType(const bslmf_MetaInt<N>&)  { return false; }
bool dispatchTrueType(TrueType)                 { return true;  }


template<class PREDICATE>
bool isFalseType() { return false; }

template<>
bool isFalseType<FalseType>() { return true; }

template<class PREDICATE>
bool isTrueType() { return false; }

template<>
bool isTrueType<TrueType>() { return true; }


template<class TYPE>
bool typeDependentTest()
{
    // Check that the 'bslmf::IsVoid' metafunction returns a consistent result
    // for template parameters, and when applied to the same type parameter in
    // a type-dependent context.  Return the diagnosed value for the specified
    // 'TYPE'.
    bool result = bslmf::IsVoid<TYPE>::VALUE;
    bool dependentCheck = bslmf::IsVoid<typename Identity<TYPE>::Type>::VALUE;
    LOOP2_ASSERT(result, dependentCheck, result == dependentCheck);
    return result;
}

template<class TYPE>
bool isDependentFalseType()
{
    // Check that the 'bslmf::IsVoid' metafunction returns a consistent result
    // for template parameters, and when applied to the same type parameter in
    // a type-dependent context.  Return the diagnosed value for the specified
    // 'TYPE'.
    ASSERT(isFalseType<typename bslmf::IsVoid<TYPE>::Type>());
    return isFalseType<
                typename bslmf::IsVoid<typename Identity<TYPE>::Type>::Type>();
}

template<class TYPE>
bool isDependentTrueType()
{
    // Check that the 'bslmf::IsVoid' metafunction returns a consistent result
    // for template parameters, and when applied to the same type parameter in
    // a type-dependent context.  Return the diagnosed value for the specified
    // 'TYPE'.
    ASSERT(isTrueType<typename bslmf::IsVoid<TYPE>::Type>());
    return isTrueType<
                typename bslmf::IsVoid<typename Identity<TYPE>::Type>::Type>();
}


template<class TYPE>
bool dispatchDependentFalseType()
{
    // Check that the 'bslmf::IsVoid' metafunction returns a consistent result
    // for template parameters, and when applied to the same type parameter in
    // a type-dependent context.  Return the diagnosed value for the specified
    // 'TYPE'.
    ASSERT(dispatchFalseType(typename bslmf::IsVoid<TYPE>()));
    return dispatchFalseType(
                      typename bslmf::IsVoid<typename Identity<TYPE>::Type>());
}

template<class TYPE>
bool dispatchDependentTrueType()
{
    // Check that the 'bslmf::IsVoid' metafunction returns a consistent result
    // for template parameters, and when applied to the same type parameter in
    // a type-dependent context.  Return the diagnosed value for the specified
    // 'TYPE'.
    ASSERT(dispatchTrueType(typename bslmf::IsVoid<TYPE>()));
    return dispatchTrueType(
                      typename bslmf::IsVoid<typename Identity<TYPE>::Type>());
}

}  // close unnamed namespace
//=============================================================================
//                                MAIN PROGRAM
//-----------------------------------------------------------------------------

int main(int argc, char *argv[])
{
    int test = argc > 1 ? atoi(argv[1]) : 0;
    int verbose = argc > 2;
    int veryVerbose = argc > 3;

    setbuf(stdout, 0);    // Use unbuffered output

    printf("TEST " __FILE__ " CASE %d\n", test);

    switch (test) { case 0:
      case 5: {
        // --------------------------------------------------------------------
        // USAGE EXAMPLE
        //
        // Concerns:
        //: 1 The usage example provided in the component header file compiles,
        //:   links, and runs as shown.
        //
        // Plan:
        //: 1 Incorporate usage example from header into test driver, remove
        //:   leading comment characters, and replace 'assert' with 'ASSERT'.
        //:   (C-1)
        //
        // Testing:
        //   USAGE EXAMPLE
        // --------------------------------------------------------------------

        if (verbose) printf("\nUSAGE EXAMPLE"
                            "\n=============\n");

      } break;
      case 4: {
        // --------------------------------------------------------------------
        // TESTING type conversion:
        // Concerns:
        //: 1 ...
        //
        // Plan:
        //
        // Testing:
        //   conversion to 'bslmf_MetaInt<>'
        // --------------------------------------------------------------------

        if (verbose) printf("\nTESTING conversion to bslmf_MetaInt"
                            "\n====================================\n");

        // Basic test dataset
        ASSERT(dispatchTrueType(bslmf::IsVoid<void>()));
        ASSERT(dispatchTrueType(bslmf::IsVoid<const void>()));
        ASSERT(dispatchTrueType(bslmf::IsVoid<volatile void>()));
        ASSERT(dispatchTrueType(bslmf::IsVoid<const volatile void>()));
        ASSERT(dispatchFalseType(bslmf::IsVoid<void *>()));
        ASSERT(dispatchFalseType(bslmf::IsVoid<void *&>()));
        ASSERT(dispatchFalseType(bslmf::IsVoid<void()>()));
        ASSERT(dispatchFalseType(bslmf::IsVoid<void(*)()>()));
        ASSERT(dispatchFalseType(bslmf::IsVoid<void *Empty::*>()));
        ASSERT(dispatchFalseType(bslmf::IsVoid<bslmf::IsVoid<void> >()));
        ASSERT(dispatchFalseType(bslmf::IsVoid<Incomplete>()));

        // Test nested template typenames with the same dataset
        ASSERT(dispatchTrueType(bslmf::IsVoid<Identity<void>::Type>()));
        ASSERT(dispatchTrueType(bslmf::IsVoid<Identity<const void>::Type>()));
        ASSERT(dispatchTrueType(bslmf::IsVoid<
                                            Identity<volatile void>::Type>()));
        ASSERT(dispatchTrueType(bslmf::IsVoid<
                                      Identity<const volatile void>::Type>()));
        ASSERT(dispatchFalseType(bslmf::IsVoid<Identity<void *>::Type>()));
        ASSERT(dispatchFalseType(bslmf::IsVoid<Identity<void *&>::Type>()));
        ASSERT(dispatchFalseType(bslmf::IsVoid<Identity<void()>::Type>()));
        ASSERT(dispatchFalseType(bslmf::IsVoid<Identity<void(*)()>::Type>()));
        ASSERT(dispatchFalseType(bslmf::IsVoid<
                                           Identity<void *Empty::*>::Type>()));
        ASSERT(dispatchFalseType(bslmf::IsVoid<
                                     Identity<bslmf::IsVoid<void> >::Type>()));
        ASSERT(dispatchFalseType(bslmf::IsVoid<Identity<Incomplete>::Type>()));

        // Test type-dependent typenames with the same dataset
        ASSERT(dispatchDependentTrueType<void>());
        ASSERT(dispatchDependentTrueType<const void>());
        ASSERT(dispatchDependentTrueType<volatile void>());
        ASSERT(dispatchDependentTrueType<const volatile void>());
        ASSERT(dispatchDependentFalseType<void *>());
        ASSERT(dispatchDependentFalseType<void *&>());
        ASSERT(dispatchDependentFalseType<void()>());
        ASSERT(dispatchDependentFalseType<void(*)()>());
        ASSERT(dispatchDependentFalseType<void *Empty::*>());
        ASSERT(dispatchDependentFalseType<bslmf::IsVoid<void> >());
        ASSERT(dispatchDependentFalseType<Incomplete>());
      } break;
      case 3: {
        // --------------------------------------------------------------------
        // TESTING bslmf::IsVoid metafunction:
        // Concerns:
        //: 1 ...
        //
        // Plan:
        //
        // Testing:
        //   bslmf::IsVoid::Type
        // --------------------------------------------------------------------

        if (verbose) printf("\nTESTING bslmf::IsVoid::Type"
                            "\n===========================\n");

        // Basic test dataset
        ASSERT(isTrueType<bslmf::IsVoid<void>::Type>());
        ASSERT(isTrueType<bslmf::IsVoid<const void>::Type>());
        ASSERT(isTrueType<bslmf::IsVoid<volatile void>::Type>());
        ASSERT(isTrueType<bslmf::IsVoid<const volatile void>::Type>());
        ASSERT(isFalseType<bslmf::IsVoid<void *>::Type>());
        ASSERT(isFalseType<bslmf::IsVoid<void *&>::Type>());
        ASSERT(isFalseType<bslmf::IsVoid<void()>::Type>());
        ASSERT(isFalseType<bslmf::IsVoid<void(*)()>::Type>());
        ASSERT(isFalseType<bslmf::IsVoid<void *Empty::*>::Type>());
        ASSERT(isFalseType<bslmf::IsVoid<bslmf::IsVoid<void> >::Type>());
        ASSERT(isFalseType<bslmf::IsVoid<Incomplete>::Type>());

        // Test nested template typenames with the same dataset
        ASSERT(isTrueType<bslmf::IsVoid<Identity<void>::Type>::Type>());
        ASSERT(isTrueType<bslmf::IsVoid<Identity<const void>::Type>::Type>());
        ASSERT(isTrueType<bslmf::IsVoid<
                                      Identity<volatile void>::Type>::Type>());
        ASSERT(isTrueType<bslmf::IsVoid<
                                Identity<const volatile void>::Type>::Type>());
        ASSERT(isFalseType<bslmf::IsVoid<Identity<void *>::Type>::Type>());
        ASSERT(isFalseType<bslmf::IsVoid<Identity<void *&>::Type>::Type>());
        ASSERT(isFalseType<bslmf::IsVoid<Identity<void()>::Type>::Type>());
        ASSERT(isFalseType<bslmf::IsVoid<Identity<void(*)()>::Type>::Type>());
        ASSERT(isFalseType<bslmf::IsVoid<
                                     Identity<void *Empty::*>::Type>::Type>());
        ASSERT(isFalseType<bslmf::IsVoid<
                               Identity<bslmf::IsVoid<void> >::Type>::Type>());
        ASSERT(isFalseType<bslmf::IsVoid<Identity<Incomplete>::Type>::Type>());

        // Test type-dependent typenames with the same dataset
        ASSERT(isDependentTrueType<void>());
        ASSERT(isDependentTrueType<const void>());
        ASSERT(isDependentTrueType<volatile void>());
        ASSERT(isDependentTrueType<const volatile void>());
        ASSERT(isDependentFalseType<void *>());
        ASSERT(isDependentFalseType<void *&>());
        ASSERT(isDependentFalseType<void()>());
        ASSERT(isDependentFalseType<void(*)()>());
        ASSERT(isDependentFalseType<void *Empty::*>());
        ASSERT(isDependentFalseType<bslmf::IsVoid<void> >());
        ASSERT(isDependentFalseType<Incomplete>());
      } break;
      case 2: {
        // --------------------------------------------------------------------
        // TESTING bslmf::IsVoid::VALUE:
        // Concerns:
        //: 1 The metafunction returns 'true' for any 'void' type, regardless
        //:   of its cv-qualification.
        //: 2 The metafunction returns 'false' for every other type.
        //: 3 The metafunction returns the correct result, even when the
        //:   potentially 'void' type it is diagnosing is a type alias, such as
        //:   a dependent type name in a template.
        //
        // Plan:
        //
        // Testing:
        //   bslmf::IsVoid::VALUE
        // --------------------------------------------------------------------

        if (verbose) printf("\nTESTING bslmf::IsVoid::VALUE"
                            "\n============================\n");

        // Basic test dataset
        ASSERT(bslmf::IsVoid<void>::VALUE);
        ASSERT(bslmf::IsVoid<const void>::VALUE);
        ASSERT(bslmf::IsVoid<volatile void>::VALUE);
        ASSERT(bslmf::IsVoid<const volatile void>::VALUE);
        ASSERT(!bslmf::IsVoid<void *>::VALUE);
        ASSERT(!bslmf::IsVoid<void *&>::VALUE);
        ASSERT(!bslmf::IsVoid<void()>::VALUE);
        ASSERT(!bslmf::IsVoid<void(*)()>::VALUE);
        ASSERT(!bslmf::IsVoid<void *Empty::*>::VALUE);
        ASSERT(!bslmf::IsVoid<bslmf::IsVoid<void> >::VALUE);
        ASSERT(!bslmf::IsVoid<Incomplete>::VALUE);

        // Test nested template typenames with the same dataset
        ASSERT(bslmf::IsVoid<Identity<void>::Type>::VALUE);
        ASSERT(bslmf::IsVoid<Identity<const void>::Type>::VALUE);
        ASSERT(bslmf::IsVoid<Identity<volatile void>::Type>::VALUE);
        ASSERT(bslmf::IsVoid<Identity<const volatile void>::Type>::VALUE);
        ASSERT(!bslmf::IsVoid<Identity<void *>::Type>::VALUE);
        ASSERT(!bslmf::IsVoid<Identity<void *&>::Type>::VALUE);
        ASSERT(!bslmf::IsVoid<Identity<void()>::Type>::VALUE);
        ASSERT(!bslmf::IsVoid<Identity<void(*)()>::Type>::VALUE);
        ASSERT(!bslmf::IsVoid<Identity<void *Empty::*>::Type>::VALUE);
        ASSERT(!bslmf::IsVoid<Identity<bslmf::IsVoid<void> >::Type>::VALUE);
        ASSERT(!bslmf::IsVoid<Identity<Incomplete>::Type>::VALUE);

        // Test type-dependent typenames with the same dataset
        ASSERT(typeDependentTest<void>());
        ASSERT(typeDependentTest<const void>());
        ASSERT(typeDependentTest<volatile void>());
        ASSERT(typeDependentTest<const volatile void>());
        ASSERT(!typeDependentTest<void *>());
        ASSERT(!typeDependentTest<void *&>());
        ASSERT(!typeDependentTest<void()>());
        ASSERT(!typeDependentTest<void(*)()>());
        ASSERT(!typeDependentTest<void *Empty::*>());
        ASSERT(!typeDependentTest<bslmf::IsVoid<void> >());
        ASSERT(!typeDependentTest<Incomplete>());

      } break;
      case 1: {
        // --------------------------------------------------------------------
        // BREATHING TEST
        //   This case exercises (but does not fully test) basic functionality.
        //
        // Concerns:
        //: 1 The class is sufficiently functional to enable comprehensive
        //:   testing in subsequent test cases.
        //
        // Plan:
        //
        // Testing:
        //   BREATHING TEST
        // --------------------------------------------------------------------

        if (verbose) printf("\nBREATHING TEST"
                            "\n==============\n");

        ASSERT(bslmf::IsVoid<void>::VALUE);
        ASSERT(!bslmf::IsVoid<int>::VALUE);

      } break;
      default: {
        fprintf(stderr, "WARNING: CASE `%d' NOT FOUND.\n", test);
        testStatus = -1;
      }
    }

    if (testStatus > 0) {
        fprintf(stderr, "Error, non-zero test status = %d.\n", testStatus);
    }

    return testStatus;
}
// ---------------------------------------------------------------------------
// NOTICE:
//      Copyright (C) Bloomberg L.P., 2012
//      All Rights Reserved.
//      Property of Bloomberg L.P. (BLP)
//      This software is made available solely pursuant to the
//      terms of a BLP license agreement which governs its use.
// ----------------------------- END-OF-FILE ---------------------------------
