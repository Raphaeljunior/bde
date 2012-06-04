// bslmf_metaint.t.cpp                                                -*-C++-*-

#include <bslmf_metaint.h>

#include <cstdlib>      // atoi()
#include <cstring>      // strcmp()
#include <cstdio>
#include <climits>

using namespace BloombergLP;
using namespace std;
using namespace bslmf;

//=============================================================================
//                                TEST PLAN
//-----------------------------------------------------------------------------
//                                Overview
//                                --------
//-----------------------------------------------------------------------------
// [ 1] VALUE 
// [ 2] operator int() const;
// [ 2] operator bool() const;
// [ 3] implicit upcast to bsltt::integer_constant<int, INT_VALUE>
// [ 3] operator bsltt::false_type() const
// [ 3] operator bsltt::true_type() const
//-----------------------------------------------------------------------------
// [ 4] USAGE EXAMPLE
//=============================================================================

//==========================================================================
//                  STANDARD BDE ASSERT TEST MACRO
//--------------------------------------------------------------------------
// NOTE: THIS IS A LOW-LEVEL COMPONENT AND MAY NOT USE ANY C++ LIBRARY
// FUNCTIONS, INCLUDING IOSTREAMS.

namespace {

int verbose = 0;
int veryVerbose = 0;
int veryVeryVerbose = 0;
int testStatus = 0;

void aSsErT(int c, const char *s, int i) {
    if (c) {
        printf("Error " __FILE__ "(%d): %s    (failed)\n", i, s);
        if (testStatus >= 0 && testStatus <= 100) ++testStatus;
    }
}

}  // close unnamed namespace

# define ASSERT(X) { aSsErT(!(X), #X, __LINE__); }

//=============================================================================
//                  STANDARD BDE LOOP-ASSERT TEST MACROS
//-----------------------------------------------------------------------------
// NOTE: This implementation of LOOP_ASSERT macros must use printf since
//       cout uses new and must not be called during exception testing.

#define LOOP_ASSERT(I,X) { \
    if (!(X)) { printf("%s", #I ": "); dbg_print(I); printf("\n"); \
                fflush(stdout); aSsErT(1, #X, __LINE__); } }

#define LOOP2_ASSERT(I,J,X) { \
    if (!(X)) { printf("%s", #I ": "); dbg_print(I); printf("\t"); \
                printf("%s", #J ": "); dbg_print(J); printf("\n"); \
                fflush(stdout); aSsErT(1, #X, __LINE__); } }

#define LOOP3_ASSERT(I,J,K,X) {                    \
    if (!(X)) { printf("%s", #I ": "); dbg_print(I); printf("\t"); \
                printf("%s", #J ": "); dbg_print(J); printf("\t"); \
                printf("%s", #K ": "); dbg_print(K); printf("\n"); \
                fflush(stdout); aSsErT(1, #X, __LINE__); } }

#define LOOP4_ASSERT(I,J,K,L,X) {                  \
    if (!(X)) { printf("%s", #I ": "); dbg_print(I); printf("\t"); \
                printf("%s", #J ": "); dbg_print(J); printf("\t"); \
                printf("%s", #K ": "); dbg_print(K); printf("\t"); \
                printf("%s", #L ": "); dbg_print(L); printf("\n"); \
                fflush(stdout); aSsErT(1, #X, __LINE__); } }

#define LOOP5_ASSERT(I,J,K,L,M,X) {                \
    if (!(X)) { printf("%s", #I ": "); dbg_print(I); printf("\t"); \
                printf("%s", #J ": "); dbg_print(J); printf("\t"); \
                printf("%s", #K ": "); dbg_print(K); printf("\t"); \
                printf("%s", #L ": "); dbg_print(L); printf("\t"); \
                printf("%s", #M ": "); dbg_print(M); printf("\n"); \
                fflush(stdout); aSsErT(1, #X, __LINE__); } }

//=============================================================================
//                  SEMI-STANDARD TEST OUTPUT MACROS
//-----------------------------------------------------------------------------
#define Q(X) printf("<| " #X " |>\n");     // Quote identifier literally.
#define P(X) dbg_print(#X " = ", X, "\n")  // Print identifier and value.
#define P_(X) dbg_print(#X " = ", X, ", ") // P(X) without '\n'
#define L_ __LINE__                        // current Line number
#define T_ putchar('\t');                  // Print a tab (w/o newline)

//=============================================================================
//                  GLOBAL TYPEDEFS/CONSTANTS FOR TESTING
//-----------------------------------------------------------------------------

namespace {

struct AnyType
{
    // Type convertible from any other type.
    template <class TYPE>
    AnyType(const TYPE&) { }
};

template <int VALUE>
bool matchIntConstant(bsltt::integer_constant<int, VALUE>)
    // Return true when called with an 'integer_constant' of the specified
    // 'VALUE'.  Does not participate in overload resolution for
    // 'integer_constant's with a 'value' other than 'VALUE'.
{
    return true;
}

template <int VALUE>
bool matchIntConstant(AnyType)
    // Return false.  Overload resolution will select this function only when
    // the argument is other than 'integer_constant<int, VALUE>', e.g., when
    // called with an argument of type integer_constant<int, OTHER_VALUE>',
    // where 'Other_VALUE' is different from the specified 'VALUE' template
    // parameter.
{
    return false;
}

int dispatchOnIntConstant(int, const char*, bsltt::integer_constant<int, 0>)
{
    return 0;
}

int dispatchOnIntConstant(int, const char*, bsltt::integer_constant<int, 1>)
{
    return 1;
}

int dispatchOnIntConstant(int, const char*, bsltt::integer_constant<int, -1>)
{
    return -1;
}

int dispatchOnIntConstant(int, const char*, bsltt::integer_constant<int, 999>)
{
    return 999;
}

int dispatchOnBoolConstant(float, bsltt::false_type)
{
    return 1;
}

int dispatchOnBoolConstant(float, bsltt::true_type)
{
    return 2;
}

}

//=============================================================================
//                  CODE FOR TESTING USAGE EXAMPLES
//-----------------------------------------------------------------------------

///Example 1: Compile-Time Function Dispatching
/// - - - - - - - - - - - - - - - - - - - - - - 
// The most common use of this structure is to perform static function
// dispatching based on a compile-time calculation.  Often the calculation is
// nothing more than a simple predicate, allowing us to select one of two
// functions.  The following function, 'doSomething', uses a fast
// implementation (e.g., 'memcpy') if the parameterized type allows for such
// operations, otherwise it will use a more generic and slower implementation
// (e.g., copy constructor).
//..
    template <class T>
    void doSomethingImp(T *t, bslmf::MetaInt<0>)
    {
        // slow generic implementation
        (void) t;
        // ...
    }
  
    template <class T>
    void doSomethingImp(T *t, bslmf::MetaInt<1>)
    {
        // fast implementation (works only for some T's)
        (void) t;
        // ...
    }
  
    template <class T, bool IsFast>
    void doSomething(T *t)
    {
        doSomethingImp(t, bslmf::MetaInt<IsFast>());
    }
//..
// The power of this approach is that the compiler will compile only the
// implementation selected by the 'MetaInt' argument.  For some parameter
// types, the fast version of 'doSomethingImp' would be ill-formed.  This kind
// of compile-time dispatch prevents the ill-formed version from ever being
// instantiated.
//..
    int usageExample1()
    {
        int i;
        doSomething<int, true>(&i); // fast version selected for int
  
        double m;
        doSomething<double, false>(&m); // slow version selected for double
  
        return 0;
    }
//..
///Example 2: Reading the 'VALUE' member
/// - - - - - - - - - - - - - - - - - - 
// In addition to forming new types, the value of the integral paramameter to
// 'MetaInt' is "saved" in the enum member 'VALUE', and is accessible for use
// in compile-time or run-time operations.
//..
    template <int V>
    unsigned g()
    {
        bslmf::MetaInt<V> i;
        ASSERT(V == i.VALUE);
        ASSERT(V == bslmf::MetaInt<V>::VALUE);
        return bslmf::MetaInt<V>::VALUE;
    }
  
    int usageExample2()
    {
        int v = g<1>();
        ASSERT(1 == v);
        return 0;
    }
//..

//=============================================================================
//                              MAIN PROGRAM
//-----------------------------------------------------------------------------

int main(int argc, char *argv[])
{
    int test = argc > 1 ? atoi(argv[1]) : 0;
    verbose = argc > 2;
    veryVerbose = argc > 3;
    veryVeryVerbose = argc > 4;

    printf("TEST " __FILE__ " CASE %d\n", test);

    switch (test) { case 0:  // Zero is always the leading case.
      case 4: {
        // --------------------------------------------------------------------
        // TESTING USAGE EXAMPLE
        //   The usage example provided in the component header file must
        //   compile, link, and run on all platforms as shown.
        //
        // Plan:
        //   Incorporate usage example from header into driver, remove leading
        //   comment characters, and replace 'assert' with 'ASSERT'.
        //
        // Testing:
        //   USAGE EXAMPLE
        // --------------------------------------------------------------------

        if (verbose) printf("\nTesting USAGE EXAMPLE"
                            "\n=====================\n");

        usageExample1();
        usageExample2();

      } break;
      case 3: {
        // --------------------------------------------------------------------
        // TESTING CONVERSION TO integer_constant
        //
        // Concerns:
        //: 1 'MetaInt<V>' is convertible to 'integer_constant<int, V>'.
        //:
        //: 2 'MetaInt<V>' is NOT convertible to 'integer_constant<int, X>',
        //:   'X != V'.
        //:
        //: 3 Given several function overloads that take an argument of type
        //:   'integer_constant<int, V>' for different values of 'V' and
        //:   otherwise identical parameters, passing an argument of type
        //:   'MetaInt<X>' will dispatch to overload that takes
        //:   'integer_constant<int, X>'.
        //:
        //: 4 Given two function overloads with identical parameters
        //:   that the first takes an argument of type 'bsltt::false_type' and
        //:   the second takes an argument of type 'bsltt::true_type', passing
        //:   an argument of type 'MetaInt<0>' will dispatch to the first and
        //:   passing an argument of type 'MetaInt<1>' will dispatch t the
        //:   second.
        //
        // Plan:
        //: 1 Create a function template, 'matchIntConstant<V>' having two
        //:   overloads: one that takes an argument of type
        //:   'integer_constant<int, V>' and returns 'true', and another which
        //:   takes an argument of a type convertible from *any*
        //:   'integer_constant' and returns 'false'.  For various values 'V',
        //:   construct rvalues of type 'MetaInt<V>' and call
        //:   'matchIntConstant<V>', verifying that it returns
        //:   'true'. (C-1)
        //:
        //: 2 For various values 'V' and 'X' such that 'V != X', construct
        //:   rvalues of type 'MetaInt<V>' and call 'matchIntConstant<X>',
        //:   verifying that it returns 'false'. (C-2)
        //:
        //: 3 Create a set of overloaded functions,
        //:   'dispatchOnIntConstant' taking identical arguments except
        //:   that the last parameter is of type 'integer_constant<int, V>'
        //:   for several values of 'V'.  The return value of
        //:   'dispatchOnIntConstant' is an 'int' with value 'V'.  Call
        //:   'dispatchOnIntConstant' several times, each time passing a
        //:   different instantiation of 'integer_constant<int, V>' and
        //:   verifying that the return value is as expected (i.e., that the
        //:   call dispatched to the correct overload).  (C-3)
        //:
        //: 4 Create a pair of overloaded functions, 'dispatchOnBoolConstant'
        //:   taking identical arguments except that the last parameter of the
        //:   first overload is 'false_type' and the last parameter of the
        //:   second overload is 'true_type'.  The return value of
        //:   'dispatchOnBoolConstant' is an 'int' with value 1 for the first
        //:   overload and 2 for the second overload.  Call
        //:   'dispatchOnBoolConstant', passing it 'MetaInt<0>' as the last
        //:   argument and verify that it returns 1.  Call
        //:   'dispatchOnBoolConstant', passing it 'MetaInt<1>' as the last
        //:   argument and verify that it returns 2. (C-4)
        //
        // Testing:
        //      implicit upcast to bsltt::integer_constant<int, INT_VALUE>
        //      inheritence from bsltt::integer_constant<int, INT_VALUE>
        //      operator bsltt::false_type() const;  // MetaInt<0> only
        //      operator bsltt::true_type() const;   // MetaInt<1> only
        // --------------------------------------------------------------------

        if (verbose) printf("\nTESTING CONVERSION TO integer_constant"
                            "\n======================================\n");

        if (veryVerbose) printf("Testing good conversions\n");
        ASSERT(  (matchIntConstant<0>(MetaInt<0>())));
        ASSERT(  (matchIntConstant<1>(MetaInt<1>())));
        ASSERT(  (matchIntConstant<-99>(MetaInt<-99>())));
        ASSERT(  (matchIntConstant<1000>(MetaInt<1000>())));
        ASSERT(  (matchIntConstant<INT_MAX>(MetaInt<INT_MAX>())));

        if (veryVerbose) printf("Testing bad conversions\n");
        ASSERT(! (matchIntConstant<0>(MetaInt<1>())));
        ASSERT(! (matchIntConstant<1>(MetaInt<0>())));
        ASSERT(! (matchIntConstant<-99>(MetaInt<99>())));
        ASSERT(! (matchIntConstant<1000>(MetaInt<6>())));
        ASSERT(! (matchIntConstant<INT_MAX>(MetaInt<INT_MIN>())));

        if (veryVerbose) printf("Testing int dispatch\n");
        ASSERT(0 == dispatchOnIntConstant(9, "hello", MetaInt<0>()));
        ASSERT(1 == dispatchOnIntConstant(8, "world", MetaInt<1>()));
        ASSERT(-1 == dispatchOnIntConstant(7, "seven", MetaInt<-1>()));
        ASSERT(999 == dispatchOnIntConstant(8, "nine", MetaInt<999>()));

        if (veryVerbose) printf("Testing bool dispatch\n");
        ASSERT(1 == dispatchOnBoolConstant(0.3, MetaInt<0>()));
        ASSERT(2 == dispatchOnBoolConstant(5.2, MetaInt<1>()));

      } break;
      case 2: {
        // --------------------------------------------------------------------
        // TESTING CONVERSION TO int
        //
        // Concerns:
        //: 1 'MetaInt<V>' is convertible to 'int' with a resulting value of
        //:   'V'.
        //:
        //: 2 'MetaInt<0>' is convertible to 'bool' with a resulting value of
        //:   'false'.
        //:
        //: 3 'MetaInt<0>' is convertible to 'bool' with a resulting value of
        //:   'true'.
        //
        // Plan:
        //: 1 Define several 'int' variables, initializing each one with a
        //:   different 'MetaInt<V>' type.  Verify that the value of each
        //:   'int' is the corresponding 'V'. (C1)
        //:
        //: 2 Define a 'bool' variable, initializing it with a 'MetaInt<0>'
        //:   object. Verify that the resulting value is false. (C2)
        //:
        //: 3 Define a 'bool' variable, initializing it with a 'MetaInt<1>'
        //:   object. Verify that the resulting value is true. (C3)
        //
        // Testing:
        //      operator int() const;
        //      operator bool() const; // MetaInt<0> and MetaInt<1> only
        // --------------------------------------------------------------------

        if (verbose) printf("\nTESTING CONVERSION TO 'int'"
                            "\n===========================\n");

        int a = MetaInt<0>();
        ASSERT(0 == a);

        int b = MetaInt<1>();
        ASSERT(1 == b);

        int c = MetaInt<-1>();
        ASSERT(-1 == c);

        int d = MetaInt<INT_MAX>();
        ASSERT(INT_MAX == d);

        int e = MetaInt<INT_MIN>();
        ASSERT(INT_MIN == e);

        bool f = MetaInt<0>();
        ASSERT(!f);

        bool g = MetaInt<1>();
        ASSERT(g);
        
      } break;
      case 1: {
        // --------------------------------------------------------------------
        // TESTING VALUE
        //
        // Test Plan:
        //   Instantiate 'MetaInt' with various constant integral
        //   values and verify that their 'VALUE' member is initialized
        //   properly.
        // --------------------------------------------------------------------

        if (verbose) printf("\nTESTING VALUE"
                            "\n=============\n");

        // verify that the 'VALUE' member is evaluated at compile-time
        enum {
            C1 = bslmf::MetaInt<1>::VALUE,
            C2 = bslmf::MetaInt<2>::VALUE,
            C0 = bslmf::MetaInt<0>::VALUE
        };

        ASSERT(0 == C0);
        ASSERT(1 == C1);
        ASSERT(2 == C2);

        bslmf::MetaInt<0> i0;
        bslmf::MetaInt<1> i1;
        bslmf::MetaInt<2> i2;

        ASSERT(0 == i0.VALUE);
        ASSERT(1 == i1.VALUE);
        ASSERT(2 == i2.VALUE);

        // Check inherited value
        ASSERT(0 == i0.value);
        ASSERT(1 == i1.value);
        ASSERT(2 == i2.value);

        ASSERT(-5 == bslmf::MetaInt<(unsigned)-5>::VALUE);

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
//      Copyright (C) Bloomberg L.P., 2002-2012
//      All Rights Reserved.
//      Property of Bloomberg L.P. (BLP)
//      This software is made available solely pursuant to the
//      terms of a BLP license agreement which governs its use.
// ----------------------------- END-OF-FILE ---------------------------------
