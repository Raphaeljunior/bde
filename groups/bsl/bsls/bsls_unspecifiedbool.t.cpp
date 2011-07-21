// bsls_unspecifiedbool.t.cpp                                         -*-C++-*-

#include <bsls_unspecifiedbool.h>

#include <stdio.h>
#include <stdlib.h>     // atoi()

using namespace BloombergLP;

//=============================================================================
//                             TEST PLAN
//                             ---------
// The 'bsls_unspecifiedbool' component provides a class supplying the typedef
// for a type suitable for use in a conversion operator for types that must be
// convertible to bool, but without suffering the accidental integeral
// promotions that would result if a true 'operator bool' was defined.  It
// further provides two static member functions that return values of this type
// equivalant to 'true' and 'false'.  In order to completely test this
// component we must prove that the aliases type serves the advertized pupose
// of supporting only the desired boolean conversions, and not the broader set
// of integral conversions.  Then we must demonstrate that the two functions
// return values with the correct properties.  Finally, we will compile and
// run the usage example.
//
//-----------------------------------------------------------------------------
//                             Overview
//                             --------
// We are testing a proctor class that makes sure that only one instance holds
// a copy of an allocated pointer, along with the necessary information to
// deallocate it properly (the deleter).  The primary goal of this test program
// is to ascertain that no resource ever gets leaked, i.e., that when the
// proctor is re-assigned or destroyed, the managed pointer gets deleted
// properly.  In addition, we must also make sure that all the conversion and
// aliasing machinery works as documented.  At last, we must also check that
// a 'bdema_ManagedPtr' acts exactly as a pointer wherever one is expected.
//-----------------------------------------------------------------------------
// [ 3] BoolType falseValue();
// [ 3] BoolType trueValue();
// [ 2] typedef BoolType 
//-----------------------------------------------------------------------------
// [ 1] BREATHING TEST
// [ 4] USAGE EXAMPLE

namespace {

//=============================================================================
//                  STANDARD BDE ASSERT TEST MACRO
//-----------------------------------------------------------------------------
// NOTE: THIS IS A LOW-LEVEL COMPONENT AND MAY NOT USE ANY C++ LIBRARY
// FUNCTIONS, INCLUDING IOSTREAMS.
static int testStatus = 0;

static void aSsErT(bool b, const char *s, int i) {
    if (b) {
        printf("Error " __FILE__ "(%d): %s    (failed)\n", i, s);
        if (testStatus >= 0 && testStatus <= 100) ++testStatus;
    }
}

# define ASSERT(X) { aSsErT(!(X), #X, __LINE__); }

//=============================================================================
//                  STANDARD BDE LOOP-ASSERT TEST MACROS
//-----------------------------------------------------------------------------
# define LOOP_ASSERT(I,X) { \
    if (!(X)) { P_(I); aSsErT(!(X), #X, __LINE__); } }

# define LOOP2_ASSERT(I,J,X) { \
    if (!(X)) { P(I) P_(J);   \
                aSsErT(!(X), #X, __LINE__); } }

#define LOOP3_ASSERT(I,J,K,X) { \
    if (!(X)) { P(I) P(J) P_(K) \
                aSsErT(!(X), #X, __LINE__); } }

//=============================================================================
//                  SEMI-STANDARD TEST OUTPUT MACROS
//-----------------------------------------------------------------------------
#define Q(X) printf("<| " #X " |>\n");      // Quote identifier literally.
#define P(X) dbg_print(#X " = ", X, "\n");  // Print identifier and value.
#define P_(X) dbg_print(#X " = ", X, ", "); // P(X) without '\n'
#define L_ __LINE__                         // current Line number
#define T_ putchar('\t');                   // Print a tab (w/o newline)

//=============================================================================
//                  GLOBAL HELPER FUNCTIONS FOR TESTING
//-----------------------------------------------------------------------------

// Fundamental-type-specific print functions.
inline void dbg_print(bool b) { printf(b ? "true" : "false"); fflush(stdout); }
inline void dbg_print(char c) { printf("%c", c); fflush(stdout); }
inline void dbg_print(unsigned char c) { printf("%c", c); fflush(stdout); }
inline void dbg_print(signed char c) { printf("%c", c); fflush(stdout); }
inline void dbg_print(short val) { printf("%d", (int)val); fflush(stdout); }
inline void dbg_print(unsigned short val) {
    printf("%d", (int)val); fflush(stdout);
}
inline void dbg_print(int val) { printf("%d", val); fflush(stdout); }
inline void dbg_print(unsigned int val) { printf("%u", val); fflush(stdout); }
inline void dbg_print(long val) { printf("%ld", val); fflush(stdout); }
inline void dbg_print(unsigned long val) {
    printf("%lu", val); fflush(stdout);
}
inline void dbg_print(long long val) { printf("%lld", val); fflush(stdout); }
inline void dbg_print(unsigned long long val) {
    printf("%llu", val); fflush(stdout);
}
inline void dbg_print(float val) {
    printf("'%f'", (double)val); fflush(stdout);
}
inline void dbg_print(double val) { printf("'%f'", val); fflush(stdout); }
inline void dbg_print(long double val) {
    printf("'%Lf'", val); fflush(stdout);
}
inline void dbg_print(const char* s) { printf("\"%s\"", s); fflush(stdout); }
inline void dbg_print(char* s) { printf("\"%s\"", s); fflush(stdout); }
inline void dbg_print(void* p) { printf("%p", p); fflush(stdout); }

// Generic debug print function (3-arguments).
template <typename T>
void dbg_print(const char* s, const T& val, const char* nl) {
    printf("%s", s); dbg_print(val);
    printf("%s", nl);
    fflush(stdout);
}

//=============================================================================
//                  GLOBAL TYPEDEFS/CONSTANTS FOR TESTING
//-----------------------------------------------------------------------------

//=============================================================================
//                         HELPER CLASSES FOR TESTING
//-----------------------------------------------------------------------------

}  // close unnamed namespace

//=============================================================================
//                                USAGE EXAMPLE
//-----------------------------------------------------------------------------
namespace USAGE_EXAMPLE {

// A common requirement for "smart pointer" types is to emulate the native
// pointer types, and in particular support testing for "null" or "empty" 
// pointer values as a simple boolean conversion in 'if' and 'while' clauses.
// We will demonstrate with a simple smart pointer type, 'SimplePtr', that
// holds a pointer value but does not claim ownership or any responsibility for
// the lifetime of the pointed-to object.  It will act as a "simple" native
// pointer.
//..
    template <class T>
    class SimplePtr
    {
        // This class holds a pointer to a single object, and provides a subset
        // of the regular pointer operatiors.  For example, it can be
        // dereferenced with 'operator*' and tested as a boolean value to see
        // if it is null.  Conversely, it does not support pointer arithmetic.
      private:
        // DATA
        T *d_ptr;  // address of the referenced object

      public:
        // CREATORS
        explicit SimplePtr(T *ptr = 0) : d_ptr(ptr) {}
            // Create a 'SimplePtr' what the value of the specified 'ptr'.

        //! ~SimplePtr() = default;
            // Destroy this object.  Note that this trivial destructor is
            // generated by the compiler.

        // ACCESSORS
        T & operator*() const  { return *d_ptr; }
            // Return a reference to the object pointed to by this
            // 'SimplePtr'.

        T * operator->() const { return d_ptr; }
            // Return the held 'd_ptr'.
//..
// Next we define, for conventience, an alias for a unique type that is
// implcititly convertible to bool.  Note that we pass the current template
// instantiation to the 'bsls_UnspecefiedBool' template in order to guarantee
// a unique name, even for different instantiation of this same 'SimplePtr'
// template.
//..
        typedef typename bsls_UnspecifiedBool<SimplePtr>::BoolType BoolType;
//..
// Now we can define a boolean conversion operator that tests whether or not
// this 'SimplePtr' object is holding a null pointer, or a valid address.  Note
// that we do not need to define 'operator!' as this single boolean conversion
// operator will be invoked with the correct semantics when the user tries that
// operator.
//..
        operator BoolType() const {
            return d_ptr
                 ? bsls_UnspecifiedBool<SimplePtr>::trueValue()
                 : bsls_UnspecifiedBool<SimplePtr>::falseValue();
        }
    }; // class simple_pointer
//..
// Next we will write a simple test function, creating a couple of 'SimplePtr'
// objects, one "null" and the other with a well-defined address.
//..
    void runTests() {
        int i = 3;
        SimplePtr<int> p1;
        SimplePtr<int> p2(&i);
//..
// Finally we test each 'SimplePtr' value, as if they were raw pointers.
//..
        ASSERT(!p1);
        ASSERT(p2);
        if (p2) {
            ASSERT(3 == *p2);
        }
    }
//..
} // namespace USAGE_EXAMPLE

//=============================================================================
//                  TEST PROGRAM
//-----------------------------------------------------------------------------
int main(int argc, char *argv[])
{
    int test = argc > 1 ? atoi(argv[1]) : 0;
    int verbose = argc > 2;
    int veryVerbose = argc > 3;

    (void) verbose;
    (void) veryVerbose;

    setbuf(stdout, 0);    // Use unbuffered output

    printf("TEST " __FILE__ " CASE %d\n", test);

    switch (test) { case 0:
      case 4: {
        // --------------------------------------------------------------------
        // TESTING USAGE EXAMPLE
        // Concerns:
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

        if (verbose) printf("\nTESTING usage example"
                            "\n---------------------\n");

          USAGE_EXAMPLE::runTests();
      } break;
      case 3: {
        // --------------------------------------------------------------------
        // TESTING true and false values
        // Concerns:
        //: 1 The static member functions 'trueValue' and 'falseValue' each
        //:   return a value of type BoolType.
        //: 2 'falseValue' returns a value that converts to the 'bool' value
        //:   'false'
        //: 3 'trueValue' returns a value that converts to the 'bool' value
        //:   'true'
        //:
        // Plan:
        //   blah... Needs to be spelled out.
        //
        // Testing:
        //  BoolType falseValue()
        //  BoolType trueValue()
        // --------------------------------------------------------------------

        if (verbose) printf("\nTESTING falseValue() and trueValue()"
                            "\n------------------------------------\n");

        if (verbose) printf("\t1. Functions must return BoolType values\n");

        typedef bsls_UnspecifiedBool<int(int)> HostType;
        typedef HostType::BoolType        BoolType;

        typedef BoolType (*FuncType)();

        FuncType falseFunc = &HostType::falseValue;
        FuncType trueFunc  = &HostType::trueValue;

        // Silence unused variable warnings.
        (void)falseFunc;
        (void)trueFunc;

        // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

        if (verbose) printf("\t2. falseValue() converts to false\n");

        ASSERT(!HostType::falseValue());

        // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

        if (verbose) printf("\t3. trueValue() converts to true\n");

        ASSERT(HostType::trueValue());

      } break;
      case 2: {
        // --------------------------------------------------------------------
        // TESTING bsls_UnspecifiedBool::BoolType
        //
        // Concerns:
        //: 1 Objects of type 'bsls_UnspecifiedBool::BoolType' must be
        //:   implicitly convertible to bool.
        //: 2 Objects of type 'bsls_UnspecifiedBool::BoolType' must not
        //:   promote to type 'int'.
        //: 3 A default constructed 'bsls_UnspecifiedBool::BoolType'
        //:   should convert to 'false' when converted to a boolean value, the
        //:   same value as a default constructed 'bool'.
        //: 4 A 'bsls_UnspecifiedBool::BoolType' object initialized
        //:   with the literal '0' should produce the value 'false' when
        //:   converted to a boolean value.
        //: 5 A class with a conversion operator to type
        //:   'bsls_UnspecifiedBool::BoolType' should be implicitly
        //:   convertible to bool.
        //: 6 Two classes that are implicitly convertible to type
        //:   'bsls_UnspecifiedBool::BoolType' should not accidentally
        //:   be comparable to each other using 'operator=='.
        //
        // Plan:
        //: 1 blah ...  Needs to be spelled out.
        //
        // Testing:
        //   typedef bsls_UnspecifiedBool::BoolType
        // --------------------------------------------------------------------

        if (verbose) printf("\nTESTING bsls_UnspecifiedBool"
                            "\n----------------------------\n");

        if (verbose) printf("\t1. BoolType implicitly conversts to bool\n");

        typedef bsls_UnspecifiedBool<int>::BoolType BoolType;

        const BoolType bt = BoolType();
        if (bt) {
            ASSERT(false);
        }

        while (bt) {
            ASSERT(false);
        }

        for ( ; bt; ) {
            ASSERT(false);
        }

        do {} while (bt);

        if (!bt) {
            ASSERT(true);
        }
        else {
            ASSERT(false);
        }

        const bool b = bt;
        ASSERT(!b);

        // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

        if (verbose) printf("\t2. BoolType does not promote to int\n");

        struct TestPromoteToInt {
            static bool call(...) { return false; }
            static bool call(int) { return true;  }
        };

        ASSERT(TestPromoteToInt::call(0));
        ASSERT(!TestPromoteToInt::call(bt));

        // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

        if (verbose) printf("\t3. BoolType default value converts to false\n");

        ASSERT(bt == false);
        ASSERT(false == bt);
        ASSERT(!bt);

        // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

        if (verbose) printf("\t4. BoolType(0) converts to false\n");

        const BoolType b0 = 0;
        ASSERT(b0 == false);
        ASSERT(false == b0);
        ASSERT(!b0);

        // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

        if (verbose) printf("\t5. Testing BoolType conversion operator\n");

        struct Booleable {
            typedef bsls_UnspecifiedBool<Booleable>::BoolType BoolType;

            operator BoolType() const { return false; }
        };

        const Booleable babel;
        ASSERT(babel == false);
        ASSERT(false == babel);
        ASSERT(!babel);

        if (babel) {
            ASSERT(false);
        }

        while (babel) {
            ASSERT(false);
        }

        for ( ; babel; ) {
            ASSERT(false);
        }

        do {} while (babel);

        if (!babel) {
            ASSERT(true);
        }
        else {
            ASSERT(false);
        }

        const bool bb = babel;
        ASSERT(!bb);
      } break;
      case 1: {
        // --------------------------------------------------------------------
        // BREATHING TEST
        //
        // Concerns:
        //: 1 That the functions exist with the documented signatures.
        //: 2 That the basic functionality works as documented.
        //
        // Plan:
        //   Exercise each function in turn and devise an elementary test
        //   sequence to ensure that the basic functionality is as documented.
        //
        // Testing:
        //   This test exercises basic functionality but *tests* *nothing*.
        // --------------------------------------------------------------------

        if (verbose) printf("\nBREATHING TEST"
                            "\n==============\n");

        typedef bsls_UnspecifiedBool<void> HostType;
        ASSERT(!HostType::falseValue());
        if (HostType::trueValue()) {
            ASSERT(true);
        }
        else {
            ASSERT(false);
        }
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
//      Copyright (C) Bloomberg L.P., 2011
//      All Rights Reserved.
//      Property of Bloomberg L.P. (BLP)
//      This software is made available solely pursuant to the
//      terms of a BLP license agreement which governs its use.
// ----------------------------- END-OF-FILE ---------------------------------
