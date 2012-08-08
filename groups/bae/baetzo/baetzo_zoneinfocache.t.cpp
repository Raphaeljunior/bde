// baetzo_zoneinfocache.t.cpp                                         -*-C++-*-
#include <baetzo_zoneinfocache.h>

#include <baetzo_errorcode.h>
#include <baetzo_zoneinfo.h>

#include <bael_administration.h>
#include <bael_defaultobserver.h>
#include <bael_log.h>
#include <bael_loggermanager.h>
#include <bael_severity.h>

#include <bcema_sharedptr.h>
#include <bcema_testallocator.h>
#include <bcemt_threadutil.h>
#include <bcemt_barrier.h>
#include <bces_atomictypes.h>

#include <bslma_allocator.h>
#include <bslma_default.h>
#include <bslma_defaultallocatorguard.h>
#include <bslma_testallocator.h>
#include <bslma_testallocatorexception.h>
#include <bsls_assert.h>
#include <bsls_asserttest.h>

#include <bsl_iostream.h>
#include <bsl_map.h>

using namespace BloombergLP;
using namespace std;

//=============================================================================
//                              TEST PLAN
//-----------------------------------------------------------------------------
//                              Overview
//                              --------
// A 'baetzo_ZoneinfoCache' is a caching mechanism for 'baetzo_Zoneinfo'
// objects.  A Zoneinfo cache is provided a 'baetzo_Loader' object on
// construction that it uses to load 'baetzo_Zoneinfo' objects that are placed
// in the cache.  A 'baetzo_Zoneinfo' test loader provides a primary
// manipulator 'getZoneinfo', and an accessor 'lookupZoneinfo'.  To test the
// behavior of the cache, we utilize a test implementation of the
// 'baetzo_Loader' protocol, with which we can verify that the
// 'baetzo_ZoneinfoCache' only loads values it has not previously returned.
// 'baetzo_ZoneinfoCache' is also a thread-safe type, and therefore requires a
// concurrency test.  Note that the test for 'getZoneinfo' is split in two
// parts, a BOOTSTRAP' that tests basic features (to enable testing other
// methods), and a more complete test.
//
// Global Concerns:
//: o All methods are thread-safe.
//: o ACCESSOR methods are declared 'const'.
//: o CREATOR & MANIPULATOR pointer/reference parameters are declared 'const'.
//: o No memory is ever allocated from the global allocator.
//: o Any allocated memory is always from the object allocator.
//: o Injected exceptions are safely propagated during memory allocation.
//: o Precondition violations are detected in appropriate build modes.
//
// Global Assumptions:
//: o All explicit memory allocations are presumed to use the global, default,
//:   or object allocator.
//: o ACCESSOR methods are 'const' thread-safe.
//-----------------------------------------------------------------------------
// CREATORS
// [ 2] explicit baetzo_ZoneinfoCache(baetzo_Loader   *loader,
// [ 2] ~baetzo_ZoneinfoCache();
//
// MANIPULATORS
// [ 5] const baetzo_Zoneinfo *getZoneinfo(const char *timeZoneId);
// [ 4] const baetzo_Zoneinfo *getZoneinfo(int *rc, const char *timeZoneId);
//
// ACCESSORS
// [ 6] const baetzo_Zoneinfo *lookupZoneinfo(const char *timeZoneId) const;
// ----------------------------------------------------------------------------
// [ 1] BREATHING TEST
// [ 8] USAGE EXAMPLE
// [ 7] CONCERN: All methods are thread-safe
// [ 6] CONCERN: ACCESSOR methods are declared 'const'.
// [ 5] CONCERN: CREATOR & MANIPULATOR parameters are declared 'const'.
// [ 6] CONCERN: No memory is ever allocated from the global allocator.
// [ 5] CONCERN: Injected exceptions are safely propagated.
// [ 6] CONCERN: Precondition violations are detected.
//=============================================================================

//=============================================================================
//                    STANDARD BDE ASSERT TEST MACRO
//-----------------------------------------------------------------------------
static bces_AtomicInt testStatus = 0;
static void aSsErT(int c, const char *s, int i)
{
    if (c) {
        cout << "Error " << __FILE__ << "(" << i << "): " << s
             << "    (failed)" << endl;
        if (testStatus >= 0 && testStatus <= 100) ++testStatus;
    }
}
#define ASSERT(X) { aSsErT(!(X), #X, __LINE__); }

#define ASSERT_SAFE_PASS(EXPR) BSLS_ASSERTTEST_ASSERT_SAFE_PASS(EXPR)
#define ASSERT_SAFE_FAIL(EXPR) BSLS_ASSERTTEST_ASSERT_SAFE_FAIL(EXPR)
#define ASSERT_PASS(EXPR)      BSLS_ASSERTTEST_ASSERT_PASS(EXPR)
#define ASSERT_FAIL(EXPR)      BSLS_ASSERTTEST_ASSERT_FAIL(EXPR)
#define ASSERT_OPT_PASS(EXPR)  BSLS_ASSERTTEST_ASSERT_OPT_PASS(EXPR)
#define ASSERT_OPT_FAIL(EXPR)  BSLS_ASSERTTEST_ASSERT_OPT_FAIL(EXPR)

//=============================================================================
//                  STANDARD BDE LOOP-ASSERT TEST MACROS
//-----------------------------------------------------------------------------
#define LOOP_ASSERT(I,X) { \
    if (!(X)) { bsl::cout << #I << ": " << I << "\n"; \
                aSsErT(1, #X, __LINE__); }}

#define LOOP2_ASSERT(I,J,X) { \
    if (!(X)) { bsl::cout << #I << ": " << I << "\t"  \
                          << #J << ": " << J << "\n"; \
                aSsErT(1, #X, __LINE__); } }

#define LOOP3_ASSERT(I,J,K,X) { \
   if (!(X)) { bsl::cout << #I << ": " << I << "\t" \
                         << #J << ": " << J << "\t" \
                         << #K << ": " << K << "\n";\
               aSsErT(1, #X, __LINE__); } }

//=============================================================================
//                  SEMI-STANDARD TEST OUTPUT MACROS
//-----------------------------------------------------------------------------
#define P(X) cout << #X " = " << (X) << endl; // Print identifier and value.
#define Q(X) cout << "<| " #X " |>" << endl;  // Quote identifier literally.
#define P_(X) cout << #X " = " << (X) << ", "<< flush; // P(X) without '\n'
#define T_()  cout << "\t" << flush;          // Print a tab (w/o newline)
#define L_ __LINE__                           // current Line number

//=============================================================================
//                  GLOBAL TYPEDEFS/CONSTANTS FOR TESTING
//-----------------------------------------------------------------------------
typedef baetzo_ZoneinfoCache       Obj;
typedef baetzo_Zoneinfo            Zone;
typedef baetzo_LocalTimeDescriptor Desc;
typedef bcema_SharedPtr<Desc>      DescPtr;

const int U = baetzo_ErrorCode::BAETZO_UNSUPPORTED_ID;

//=============================================================================
//                      GLOBAL CLASSES FOR TESTING
//-----------------------------------------------------------------------------

void executeInParallel(int                               numThreads,
                       bcemt_ThreadUtil::ThreadFunction  func,
                       void                             *threadArgs)
   // Create the specified 'numThreads', each executing the specified 'func'
   // on the specified 'threadArgs'.
{
    bcemt_ThreadUtil::Handle *threads =
                                      new bcemt_ThreadUtil::Handle[numThreads];
    ASSERT(threads);

    for (int i = 0; i < numThreads; ++i) {
        bcemt_ThreadUtil::create(&threads[i], func, threadArgs);
    }
    for (int i = 0; i < numThreads; ++i) {
        bcemt_ThreadUtil::join(threads[i]);
    }

    delete [] threads;
}

class ConcurrencyCounterGuard {
    // A guard that increments a counter, supplied at construction, during
    // construction, and decrements the same counter when it goes out of scope
    // and is destroyed.   If the counter is greater than 1 after being
    // incremented on construction, the guard will assert.

    bces_AtomicInt *d_counter;

  private:
    // NOT IMPLEMENTED
    ConcurrencyCounterGuard(const ConcurrencyCounterGuard&);
    ConcurrencyCounterGuard& operator=(const ConcurrencyCounterGuard);

  public:
    ConcurrencyCounterGuard(bces_AtomicInt *concurrenyCallCounter)
        // Increment the specified 'concurrentCallCounter' and assert if the
        // resulting count is greater than 1.
    : d_counter(concurrenyCallCounter)
    {
        int concurrentCount = ++(*d_counter);
        ASSERT(1 >= concurrentCount);
    }

    ~ConcurrencyCounterGuard()
        // Decrement the concurrent call counter supplied at construction.
    {
        --(*d_counter);
    }
};

class TestDriverTestLoader : public baetzo_Loader {
    // This class provides a concrete test implementation of the
    // 'baetzo_Loader' protocol (an abstract interface) for obtaining a time
    // zone.

  private:
    typedef bsl::map<bsl::string, baetzo_Zoneinfo> TimeZoneMap;
        // A 'TimeZoneMap' is a type that maps a time zone identifier to
        // information about that time zone.

    // DATA
    TimeZoneMap      d_timeZones;    // set of time zones that this test
                                     // loader will return

    bsl::string      d_lastRequestedTimeZone;
                                     // most recently requested time zone id

    bces_AtomicInt   d_concurrentCallCount;
                                     // number of concurrent function calls
                                     // currently being made to 'loadTimeZone'
                                     // (must be <= 1).

    int              d_delayMicroseconds;
                                     // number of seconds to sleep on each
                                     // call to 'loadTimeZone'

    bslma_Allocator *d_allocator_p;  // allocator (held, not owned)

  private:
    // NOT IMPLEMENTED
    TestDriverTestLoader(const TestDriverTestLoader&);
    TestDriverTestLoader operator=(const TestDriverTestLoader&);

  public:
    // PUBLIC CONSTANTS
    static const char *NO_REQUESTS;

    // CREATORS
    explicit TestDriverTestLoader(bslma_Allocator *basicAllocator = 0);
    explicit TestDriverTestLoader(int              loadDelayMicroseconds,
                                  bslma_Allocator *basicAllocator = 0);
        // Create a 'TestLoader' object.  Optionally specify
        // 'loadDelayMicroseconds' indicating the period of time to sleep on
        // each call to 'loadTimeZone'.  If 'loadDelayMicroseconds' is not
        // supplied, calls to 'loadTimeZone' will not sleep.  Optionally
        // specify a 'basicAllocator' used to supply memory.  If
        // 'basicAllocator' is 0, the currently installed default allocator is
        // used.  By default the test loader will return
        // 'baetzo_ErrorCode::BAETZO_UNSUPPORTED_ID' for all time zone
        // identifiers.  Note that providing a delay for 'loadTimeZone' in
        // order to verify thread safe access to that method.

    virtual ~TestDriverTestLoader();
        // Destroy this time zone data-source.

    // MANIPULATORS
    void addTimeZone(const char *identifier,
                     int         utcOffset,
                     bool        dstFlag,
                     const char *name);
        // If 'name' is not 0, add to this test loader a time zone having the
        // specified 'identifier', and containing a single local time
        // descriptor having the specified  'utcOffset', 'dstFlag', and
        // 'name', and add an invalid time zone otherwise.

    void addInvalidTimeZone(const char *identifier);
        // Add to this test load a time zone that is not well-defined with the
        // specified 'identifier'.

    virtual int loadTimeZone(baetzo_Zoneinfo *result, const char *timeZoneId);
        // Load into the specified 'result' the olson time zone information
        // for the time zone identified by the specified 'timeZoneId'.  Return
        // 0 on success, 'baetzo_ErrorCode::BAETZO_UNSUPPORTED_ID' if the
        // identified time zone is not supported, and a negative value if any
        // other error occurs.

    // ACCESSORS
    const std::string& lastRequestedTimeZone() const;
        // Return the time zone identifier supplied on the most recent call to
        // 'loadTimeZone' or 'NO_REQUESTS' if 'loadTimeZone' has not been
        // called.

};

// PUBLIC CONSTANTS
const char *TestDriverTestLoader::NO_REQUESTS = "loadTimeZone not called";

// CREATORS
TestDriverTestLoader::TestDriverTestLoader(bslma_Allocator *basicAllocator)
    : d_timeZones(basicAllocator)
    , d_lastRequestedTimeZone(NO_REQUESTS, basicAllocator)
    , d_concurrentCallCount(0)
    , d_delayMicroseconds(0)
    , d_allocator_p(bslma_Default::allocator(basicAllocator)) { }

TestDriverTestLoader::TestDriverTestLoader(int loadDelayMicroseconds,
                                           bslma_Allocator *basicAllocator)
    : d_timeZones(basicAllocator)
    , d_lastRequestedTimeZone(NO_REQUESTS, basicAllocator)
    , d_concurrentCallCount(0)
    , d_delayMicroseconds(loadDelayMicroseconds)
    , d_allocator_p(bslma_Default::allocator(basicAllocator)) { }

TestDriverTestLoader::~TestDriverTestLoader() { }

// MANIPULATORS
void TestDriverTestLoader::addTimeZone(const char *timeZone,
                                       int         utcOffset,
                                       bool        dstFlag,
                                       const char *name)
{
    baetzo_Zoneinfo zoneinfo; zoneinfo.setIdentifier(timeZone);

    if (0 == name) {
        d_timeZones[timeZone] = zoneinfo;
        return;                                                       // RETURN
    }

    baetzo_LocalTimeDescriptor type(utcOffset, dstFlag, name, d_allocator_p);
    bdet_Datetime firstTime(1, 1, 1);
    bsls_Types::Int64 firstTimeT = bdetu_Epoch::convertToTimeT64(firstTime);

    zoneinfo.addTransition(firstTimeT, type);
    d_timeZones[timeZone] = zoneinfo;
}

int TestDriverTestLoader::loadTimeZone(baetzo_Zoneinfo *result,
                                       const char      *timeZoneId)
{
    // Ensure that this method is not invoked concurrently.

    ConcurrencyCounterGuard guard(&d_concurrentCallCount);

    d_lastRequestedTimeZone = timeZoneId;

    if (d_delayMicroseconds > 0) {
        bcemt_ThreadUtil::microSleep(d_delayMicroseconds); }

    TimeZoneMap::const_iterator it = d_timeZones.find(timeZoneId);

    if (it == d_timeZones.end()) {
        return baetzo_ErrorCode::BAETZO_UNSUPPORTED_ID;               // RETURN
    }

    *result = it->second;

    return 0;
}

// ACCESSORS
const std::string& TestDriverTestLoader::lastRequestedTimeZone() const
{
    return d_lastRequestedTimeZone;
}

//=============================================================================
//                      CONCURRENCY CONCERNS RELATED ENTRIES
//-----------------------------------------------------------------------------

namespace BAETZO_ZONEINFOCACHE_CONCURRENCY {

struct TimeZoneData {
        int         d_line;       // line number
        const char *d_id;         // time zone id
        int         d_utcOffset;  // utc offset in seconds for descriptor
        bool        d_dstFlag;    // dst flag for descriptor
        const char *d_abbrev;     // abbreviation for descriptor, or 0 for
                                  // invalid time zones

        bces_AtomicPointer<const Zone>
                    d_expectedAddress_p;
                                  // expected cache address of this id
} VALUES[] = {
    { L_,  "ID_A",  1, true,  "A" , 0 },
    { L_,  "ID_B",  2, false, "B" , 0 },
    { L_,  "ID_C",  3, true,  0   , 0 },
    { L_,  "ID_D",  4, false, "A" , 0 },
    { L_,  "ID_E",  5, true,  "B" , 0 },
    { L_,  "ID_F",  6, false, 0   , 0 },
    { L_,  "ID_G",  7, true,  "A" , 0 },
    { L_,  "ID_H",  8, true,  "A" , 0 },
    { L_,  "ID_I",  9, false, "B" , 0 },
    { L_,  "ID_J", 10, true,  0   , 0 },
    { L_,  "ID_K", 11, false, "A" , 0 },
    { L_,  "ID_L", 12, true,  "B" , 0 },
    { L_,  "ID_M", 13, false, 0   , 0 },
    { L_,  "ID_N", 14, true,  "A" , 0 }
};
const int NUM_VALUES = sizeof(VALUES) / sizeof(*VALUES);

struct ThreadData {
    Obj           *d_cache_p;    // cache under test
    bcemt_Barrier *d_barrier_p;  // testing barrier
};

extern "C" void *workerThread(void *arg)
{
    ThreadData *p = (ThreadData*)arg;
    bcemt_Barrier& barrier = *p->d_barrier_p;

    Obj &mX = *p->d_cache_p; const Obj &X = mX;

    // Begin the test.

    barrier.wait();
    for (int i = 0; i < NUM_VALUES; ++i) {
        // Add all the test values to the cache.

        const char *ID = VALUES[i].d_id;
        bces_AtomicPointer<const Zone>& addr = VALUES[i].d_expectedAddress_p;

        // 'result' should either be 0, or the previously returned value (*IF*
        // the previously returned value has been set).

        const Zone *result   = X.lookupZoneinfo(ID);
        const Zone *EXPECTED = addr.relaxedLoad();
        ASSERT(0 == result || EXPECTED == 0 || EXPECTED == result);

        result = mX.getZoneinfo(ID);
        const Zone *prevAddress = addr.testAndSwap(0, result);
        ASSERT( 0 == prevAddress || prevAddress == result);

        ASSERT(result == X.lookupZoneinfo(ID));
    }

    // At this point we are guaranteed all the values that can be loaded are
    // already loaded.  Simply verify that repeated calls to 'getZoneinfo' and
    // 'lookupZoneinfo' return the same value.

    for (int testRun = 0; testRun < 5; ++testRun) {
        for (int i = 0; i < NUM_VALUES; ++i) {
            const char *ID       = VALUES[i].d_id;
            const Zone *EXPECTED = VALUES[i].d_expectedAddress_p.relaxedLoad();
            ASSERT(EXPECTED == X.lookupZoneinfo(ID));
            ASSERT(EXPECTED == mX.getZoneinfo(ID));
            ASSERT(EXPECTED == X.lookupZoneinfo(ID));
        }
    }

    barrier.wait();
    return 0;
}

}

//=============================================================================
//                                USAGE EXAMPLE
//-----------------------------------------------------------------------------

// Note that the following was cut & pasted from 'baetzo_testloader' for the
// purposes of the usage example (replacing 'TestLoader' for
// 'baetzo_TestLoader'):

                        // ================
                        // class TestLoader
                        // ================

class TestLoader : public baetzo_Loader {
    // This class provides a concrete test implementation of the
    // 'baetzo_Loader' protocol (an abstract interface) for obtaining a time
    // zone.  This test implementation maintains a mapping of time zone
    // identifiers to 'baetzo_Zoneinfo' objects.  Olson time zone objects are
    // associated with a time zone identifier using the 'setTimeZone'
    // method, and can be subsequently accessed by calling the protocol method
    // 'loadTimeZone' with the same identifier.

  private:
    typedef bsl::map<bsl::string, baetzo_Zoneinfo> TimeZoneMap;
        // A 'TimeZoneMap' is a type that maps a time zone identifier to
        // information about that time zone.

    // DATA
    TimeZoneMap d_timeZones;  // set of time zones that this test loader
                              // will return

  public:
    // CREATORS
    explicit TestLoader(bslma_Allocator *basicAllocator = 0);
        // Create a 'TestLoader' object.  Optionally specify a
        // 'basicAllocator' used to supply memory.  If 'basicAllocator' is 0,
        // the currently installed default allocator is used.  By default the
        // test loader will return 'baetzo_ErrorCode::BAETZO_UNSUPPORTED_ID'
        // for all time zone identifiers.

    virtual ~TestLoader();
        // Destroy this time zone data-source.

    void print();

    // MANIPULATORS
    void setTimeZone(const baetzo_Zoneinfo&  timeZone);
        // Set, to the specified 'timeZone', the time zone information that
        // will be returned by 'loadTimeZone' for the identifier
        // 'timeZone.identifier()'.

    virtual int loadTimeZone(baetzo_Zoneinfo *timeZone,
                             const char      *timeZoneId);
        // Load into the specified 'result' the olson time zone information
        // for the time zone identified by the specified 'timeZoneId'.  Return
        // 0 on success, 'baetzo_ErrorCode::BAETZO_UNSUPPORTED_ID' if the
        // identified time zone is not supported, and a negative value if any
        // other error occurs.
};

                        // ----------------
                        // class TestLoader
                        // ----------------

// CREATORS
inline
TestLoader::TestLoader(bslma_Allocator *basicAllocator)
: d_timeZones(basicAllocator)
{
}

TestLoader::~TestLoader()
{
}

// MANIPULATORS
void TestLoader::setTimeZone(const baetzo_Zoneinfo&  timeZone)
{
    TimeZoneMap::iterator it = d_timeZones.find(timeZone.identifier());
    if (it == d_timeZones.end()) {
        d_timeZones.insert(TimeZoneMap::value_type(timeZone.identifier(),
                                                   timeZone));
    }
    else {
        it->second = timeZone;
    }
}

int TestLoader::loadTimeZone(baetzo_Zoneinfo *result,
                             const char      *timeZoneId)
{
    TimeZoneMap::const_iterator it = d_timeZones.find(timeZoneId);

    if (it == d_timeZones.end()) {
        return baetzo_ErrorCode::BAETZO_UNSUPPORTED_ID;               // RETURN
    }

    *result = it->second;
    return 0;
}

//=============================================================================
//                                 MAIN PROGRAM
//-----------------------------------------------------------------------------

int main(int argc, char *argv[])
{
    int             test = argc > 1 ? atoi(argv[1]) : 0;
    bool         verbose = argc > 2;
    bool     veryVerbose = argc > 3;
    bool veryVeryVerbose = argc > 4;

    cout << "TEST " << __FILE__ << " CASE " << test << endl;

    bael_DefaultObserver            observer(&bsl::cout);
    bael_LoggerManagerConfiguration configuration;
    configuration.setDefaultThresholdLevelsIfValid(bael_Severity::BAEL_OFF,
                                                   bael_Severity::BAEL_OFF,
                                                   bael_Severity::BAEL_OFF,
                                                   bael_Severity::BAEL_OFF);
    bael_LoggerManager& manager =
                   bael_LoggerManager::initSingleton(&observer, configuration);

    bslma_TestAllocator defaultAllocator;  // To be used to make sure the
                                           // allocator is always passed down
                                           // where necessary.

    bslma_TestAllocator testAllocator;
    bslma_TestAllocator *Z = &testAllocator;  // To be used to allocate
                                              // everything in this code.

    bslma_DefaultAllocatorGuard guard(&defaultAllocator);

    if (veryVeryVerbose) {
        defaultAllocator.setVerbose(true);
    }

    switch (test) { case 0:
      case 8: {
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

        if (verbose) cout << endl << "TESTING USAGE EXAMPLE" << endl
                                  << "=====================" << endl;
///Usage
///-----
// In this section, we demonstrate creating a 'baetzo_ZoneinfoCache' object
// and using it to access time zone information.
//
///Example 1: Creating a Concrete 'baetzo_Loader'
/// - - - - - - - - - - - - - - - - - - - - - - -
// A 'baetzo_ZoneinfoCache' object is provided a 'baetzo_Loader' on
// construction.  The 'loader' is used to populate the cache per user requests
// via the 'getZoneinfo' method.  In this example, we use a 'TestLoader'
// implementation of the 'baetzo_Loader' protocol, based on the
// 'baetzo_testloader' component.  In this example, our test loader is
// explicitly primed with responses for requests for certain time-zone
// identifiers.  Note that, in practice, a 'loader' typically obtains time-zone
// information from some external data store (e.g., see
// 'baetzo_datafileloader').
//
// We start by creating and initializing a couple of example time zone
// information objects.  Note that the 'baetzo_Zoneinfo' objects below are
// illustrative, and contain no actual time zone information:
//..
    baetzo_Zoneinfo newYorkZoneinfo;
    newYorkZoneinfo.setIdentifier("America/New_York");
//
    baetzo_Zoneinfo londonZoneinfo;
    londonZoneinfo.setIdentifier("Europe/London");
//..
// Next we create a description of Eastern Standard Time (EST) and Greenwich
// Mean Time (GMT):
//..
    baetzo_LocalTimeDescriptor est(-5 * 60 * 60, false, "EST");
    baetzo_LocalTimeDescriptor gmt(           0, false, "GMT");
//..
// Then we set the initial transition for 'newYorkZoneinfo' to Eastern Standard
// Time, and the initial transition for 'londonZoneinfo' to Greenwich Mean
// Time.  Note that such an initial transition is required for a
// 'baetzo_Zoneinfo' object to be considered Well-Defined (see
// 'isWellFormed'):
//..
    bsls_Types::Int64 firstTime = bdetu_Epoch::convertToTimeT64(
                                                       bdet_Datetime(1, 1, 1));
    newYorkZoneinfo.addTransition(firstTime, est);
    londonZoneinfo.addTransition(firstTime, gmt);
//..
// Next we create a 'TestLoader', and then populate it with our example time
// zone information objects:
//..
    TestLoader testLoader;
    testLoader.setTimeZone(newYorkZoneinfo);
    testLoader.setTimeZone(londonZoneinfo);
//..
// Finally, we verify that 'testLoader' contains the configured
// 'baetzo_Zoneinfo' objects for New York and London:
//..
    baetzo_Zoneinfo newYorkResult;
    int rc = testLoader.loadTimeZone(&newYorkResult, "America/New_York");
    ASSERT(0 == rc);
    ASSERT(newYorkZoneinfo == newYorkResult);

    baetzo_Zoneinfo londonResult;
    rc = testLoader.loadTimeZone(&londonResult, "Europe/London");
    ASSERT(0 == rc);
    ASSERT(londonZoneinfo == londonResult);
//..
//
///Example 2: Creating and Using a 'baetzo_ZoneinfoCache'
/// - - - - - - - - - - - - - - - - - - - - - - - - - - -
// In this example, we create a 'baetzo_ZoneinfoCache', and use it to access
// time zone information for several time zones.
//
// We start by creating a 'baetzo_ZoneinfoCache' object supplied with the
// address of the 'TestLoader' we populated in the preceding example:
//..
    baetzo_ZoneinfoCache cache(&testLoader);
//..
// Next, we verify the newly constructed cachedoes not contain either New York
// or London:
//..
    ASSERT(0 == cache.lookupZoneinfo("America/New_York"));
    ASSERT(0 == cache.lookupZoneinfo("Europe/London"));
//..
// Then, we call 'getZoneinfo' to obtain the data for the New York time zone.
// Note that, because this is the first 'getZoneinfo' operation on the class,
// the time-zone data has not previously been retrieved, and the data must be
// loaded using the loader supplied at construction:
//..
    const baetzo_Zoneinfo *newYork = cache.getZoneinfo(&rc,
                                                       "America/New_York");
//
    ASSERT(0 == rc);
    ASSERT(0 != newYork);
    ASSERT("America/New_York" == newYork->identifier());
//..
// Next, we verify that a subsequent call 'lookupZoneinfo' for New York,
// returns the previously cached value.  However, a call to 'lookupZoneinfo'
// for London will return 0 because the value has not been cached:
//..
    ASSERT(newYork == cache.lookupZoneinfo("America/New_York"));
    ASSERT(0       == cache.lookupZoneinfo("Europe/London"));
//..
// Next, we call 'getZoneinfo' for London and verify that it returns the
// expected value:
//..
    const baetzo_Zoneinfo *london = cache.getZoneinfo(&rc, "Europe/London");
    ASSERT(0 == rc);
    ASSERT(0 != london);
    ASSERT("Europe/London" == london->identifier());
//..
// Finally, we call 'getZoneinfo' with time zone identifier unknown to our
// 'TestLoader'.  The call to 'getZoneinfo' returns 0 because the time zone
// information cannot be loaded.  Examination of 'rc' shows indicates that the
// identifier is not supported:
//..
    ASSERT(0 == cache.getZoneinfo(&rc, "badId"));
    ASSERT(baetzo_ErrorCode::BAETZO_UNSUPPORTED_ID == rc);
//..

      } break;
      case 7: {
        // --------------------------------------------------------------------
        // TESTING CONCURRENT ACCESS
        //
        // Concerns:
        //: 1 That simultaneous calls to 'getZoneinfo' for the same time-zone
        //:   id initialize and load a *single* 'baetzo_Zoneinfo' object, whose
        //:   address is returned for all calls.
        //:
        //: 2 That a call to 'lookupZoneinfo', made concurrently to a call
        //:   to either 'lookupZoneinfo' or 'loadTimeZone' for the same time
        //:   zone id, returns the address of the unique 'Zoneinfo' object that
        //:   has been loaded for that id, or 0.
        //:
        //: 3 That the cache serializes calls to the 'loadTimeZone' method of
        //:   the 'baetzo_Loader' object supplied at construction
        //
        // Plan:
        //: 1 Define a 'TestDriverTestLoader' implementation of 'baetzo_Loader'
        //:   that will delay on calls to 'loadTimeZone', maintain a count of
        //:   conccurent invocations, and ASSERT if that conccurent call count
        //:   is greater than 1.
        //:
        //: 2 Create an instance of 'TestDriverTestLoader' and configure that
        //:   instance with a set of sample data.
        //:
        //: 3 Concurrently invoke 'lookupZoneinfo' and 'getZoneinfo' for the
        //:   set of sample time zone ids.  Verify that each call to
        //:   'getZoneinfo' for the same time zone identifier returns the same
        //:   'baetzo_Zoneinfo' address.  Verify that each call to
        //:   'lookupZoneinfo' either returns 0 (if it is possible
        //:   'getZoneinfo' has not been called), or the same 'baetzo_Zoneinfo'
        //:   address that was previously returned for that time zone
        //:   identifier.
        // --------------------------------------------------------------------

        if (verbose) cout << endl << "TESTING CONCURRENCY" << endl
                                  << "===================" << endl;

        using namespace BAETZO_ZONEINFOCACHE_CONCURRENCY;

        // A 'bcema_TestAllocator' is required for thread safe allocations.
        bcema_TestAllocator testAllocator;

        enum {
            ONE_MS_IN_MICROS = 1000
        };
        TestDriverTestLoader testLoader(ONE_MS_IN_MICROS, &testAllocator);
        for (int i = 0; i < NUM_VALUES; ++i) {
            testLoader.addTimeZone(VALUES[i].d_id,
                                   VALUES[i].d_utcOffset,
                                   VALUES[i].d_dstFlag,
                                   VALUES[i].d_abbrev);
        }

        enum {
            NUM_THREADS = 5
        };

        bcemt_Barrier barrier(NUM_THREADS);
        Obj mX(&testLoader, &testAllocator); const Obj& X = mX;
        ThreadData args = { &mX, &barrier };
        executeInParallel(NUM_THREADS, workerThread, &args);

      } break;
      case 6: {
        // --------------------------------------------------------------------
        // TESTING: 'lookupZoneinfo'
        //
        // Concerns:
        //: 1 'lookupZoneinfo' returns 0 if 'getZoneinfo' has not successfully
        //:    been invoked for the supplied id.
        //:
        //: 2 If 'getZoneinfo' has been successfully called for the supplied
        //:   time-zone id, 'lookupZoneinfo' returns the address to the same
        //:   object as the call to 'getZoneinfo'.
        //
        // Plan:
        //: 1 Using a table-driven approach
        //:   1 Create descriptions for a varied set of time zone
        //:     values, some of which are not well-defined.  Initialize a test
        //:     loader with these values.  Note that each time zone, for the
        //:     purpose of this test, is uniquely determined by an identifier
        //:     and a single transition.
        //:
        //:   2 For each row in the test table
        //:     1 Call 'lookupZoneinfo' and verify it returns 0
        //:     2 Call 'getZoneinfo' and record the results
        //:     3 Call 'lookupZoneinfo' and verify it returns the same address
        //:       as that returned from 'getZoneinfo'.
        //:
        //: 2 Use ASSERT_PASS and ASSERT_FAIL to test assertions for null
        //:   pointers.
        //
        // Testing:
        //   const baetzo_Zoneinfo *lookupZoneinfo(const char *) const;
        // --------------------------------------------------------------------

        if (verbose) cout << endl << "TEST: 'lookupZoneinfo'" << endl
                                  << "======================" << endl;

        struct TimeZoneData {
            int         d_line;       // line number
            const char *d_id;         // time zone id
            int         d_utcOffset;  // utc offset in seconds for descriptor
            bool        d_dstFlag;    // dst flag for descriptor
            const char *d_abbrev;     // abbreviation for descriptor, or 0 for
                                      // invalid time zones

        } VALUES[] = {
            { L_,  "ID_A",  1, true,  "A" },
            { L_,  "ID_B",  2, false, "B" },
            { L_,  "ID_BA", 3, true,  0   },
            { L_,  "ID_C",  4, false, "A" },
            { L_,  "ID_D",  5, true,  "B" },
            { L_,  "ID_E",  6, false, 0   },
            { L_,  "ID_BB", 7, true,  "A" },
        };
        const int NUM_VALUES = sizeof(VALUES) / sizeof(*VALUES);

        TestDriverTestLoader testLoader(Z);
        bsl::map<bsl::string, const Zone *> addressMap(Z);
        for (int i = 0; i < NUM_VALUES; ++i) {
            testLoader.addTimeZone(VALUES[i].d_id,
                                   VALUES[i].d_utcOffset,
                                   VALUES[i].d_dstFlag,
                                   VALUES[i].d_abbrev);
            addressMap[VALUES[i].d_id] = 0;
        }

        Obj mX(&testLoader, Z); const Obj& X = mX;
        for (int i = 0; i < NUM_VALUES; ++i) {
            const int   LINE    = VALUES[i].d_line;
            const char *ID      = VALUES[i].d_id;
            const bool  SUCCESS = VALUES[i].d_abbrev != 0;

            LOOP_ASSERT(LINE, 0 == X.lookupZoneinfo(ID));

            const Zone *EXPECTED = mX.getZoneinfo(ID);

            LOOP_ASSERT(LINE, EXPECTED == X.lookupZoneinfo(ID));
            addressMap[ID] = EXPECTED;

            for (int j = 0; j < NUM_VALUES; ++j) {
                const char *ID = VALUES[j].d_id;
                LOOP_ASSERT(LINE, addressMap[ID] == X.lookupZoneinfo(ID));
            }
        }
        {
            bsls_AssertFailureHandlerGuard hG(bsls_AssertTest::failTestDriver);
            if (veryVerbose) cout << "\tTest assertions." << endl;

            TestDriverTestLoader testLoader(Z);
            Obj mX(&testLoader, Z); const Obj& X = mX;

            int rc;
            ASSERT_FAIL(X.lookupZoneinfo((const char *)0));
            ASSERT_PASS(X.lookupZoneinfo("abc"));
        }
      } break;
      case 5: {
        // --------------------------------------------------------------------
        // TESTING: 'getZoneinfo(const char *)'
        //
        // Concerns:
        //: 1 White Box: This method trivially delegates to the primary
        //:   manipulator.  Verify that the time zone identifier is correctly
        //:   passed to the primary manipulator, and that the primary
        //:   manipulators return value is correctly propagated back.
        //
        // Plan:
        //: 1 Using a table-driven approach
        //:   1 Create descriptions for a varied set of time zone
        //:     values, some of which are not well-defined.  Initialize a test
        //:     loader with these values.  Note that each time zone, for the
        //:     purpose of this test, is uniquely determined by an identifier
        //:     and a single transition.
        //:
        //: 2 Use the 'BSLMA_EXCEPTION_TEST' macro, and for each row in the
        //:   test-table:
        //:    1 Call 'getZoneinfo' and verify the results match the expected
        //:     Zoneinfo value.
        //:
        //: 3 Use ASSERT_PASS and ASSERT_FAIL to test assertions for null
        //:   pointers.
        //
        // Testing:
        //   const baetzo_Zoneinfo *getZoneinfo(const char *);
        // --------------------------------------------------------------------

        if (verbose) cout << endl
                          << "TEST: 'getZoneinfo(const char *)'" << endl
                          << "==============================" << endl;

        struct TimeZoneData {
            int         d_line;       // line number
            const char *d_id;         // time zone id
            int         d_utcOffset;  // utc offset in seconds for descriptor
            bool        d_dstFlag;    // dst flag for descriptor
            const char *d_abbrev;     // abbreviation for descriptor, or 0 for
                                      // invalid time zones

        } VALUES[] = {
            { L_,  "ID_A",  1, true,  "A" },
            { L_,  "ID_B",  2, false, "B" },
            { L_,  "ID_BA", 3, true,  0   },
            { L_,  "ID_C",  4, false, "A" },
            { L_,  "ID_D",  5, true,  "B" },
            { L_,  "ID_E",  6, false, 0   },
            { L_,  "ID_BB", 7, true,  "A" },
        };
        const int NUM_VALUES = sizeof(VALUES) / sizeof(*VALUES);

        TestDriverTestLoader testLoader(Z);
        for (int i = 0; i < NUM_VALUES; ++i) {
            testLoader.addTimeZone(VALUES[i].d_id,
                                   VALUES[i].d_utcOffset,
                                   VALUES[i].d_dstFlag,
                                   VALUES[i].d_abbrev);
        }

        {
            if (veryVerbose) cout << "\tTest basic behavior" << endl;

            Obj mX(&testLoader, Z); const Obj& X = mX;
            BEGIN_BSLMA_EXCEPTION_TEST {
                for (int i = 0; i < NUM_VALUES; ++i) {
                    const int   LINE    = VALUES[i].d_line;
                    const char *ID      = VALUES[i].d_id;
                    const bool  SUCCESS = VALUES[i].d_abbrev != 0;

                    Zone expected(Z); const Zone& EXPECTED = expected;
                    testLoader.loadTimeZone(&expected, ID);

                    const Zone *result = mX.getZoneinfo(ID);
                    ASSERT(ID == testLoader.lastRequestedTimeZone());
                    if (SUCCESS) {
                        LOOP_ASSERT(LINE, 0 != result);
                        LOOP_ASSERT(LINE, EXPECTED == *result);
                        LOOP_ASSERT(LINE, result   == mX.getZoneinfo(ID));
                    }
                    else {
                        LOOP_ASSERT(LINE, 0 == result);
                    }
                    LOOP_ASSERT(LINE, 0 == defaultAllocator.numBytesInUse());
                    LOOP_ASSERT(LINE, 0 <  testAllocator.numBytesInUse());
                }
            } END_BSLMA_EXCEPTION_TEST
        }

        {
            bsls_AssertFailureHandlerGuard hG(bsls_AssertTest::failTestDriver);
            if (veryVerbose) cout << "\tTest assertions." << endl;

            TestDriverTestLoader testLoader(Z);
            Obj mX(&testLoader, Z); const Obj& X = mX;

            int rc;
            ASSERT_FAIL(mX.getZoneinfo((const char *)0));
            ASSERT_PASS(mX.getZoneinfo("abc"));
        }
      } break;
      case 4: {
        // --------------------------------------------------------------------
        // TESTING: 'getZoneinfo(int *, const char *)'
        //
        // Concerns:
        //  Address concerns for 'getZoneinfo' not directly tested by the
        //  bootstrap tests:
        //
        //: 1 'getZoneinfo' will return an 'BAETZO_UNSPECIFIED_ID' if the time
        //:   zone loader does not know the specified id.
        //:
        //: 2 'getZoneinfo' will return an error (other than
        //:   'BAETZO_UNSPECIED_ID') if the loaded time zone is not
        //:   well-defined.
        //:
        //: 3 'getZoneinfo' will return an error code, other than
        //:   ' BAETZO_UNSPECIFIED_ID' it the loader fails to load the time
        //:    zone.
        //:
        //: 4 'getZoneinfo' uses the appropriate allocator.
        //:
        //: 5 'getZoneinfo' is exception neutral
        //
        // Plan:
        //: 1 Using a table-driven approach
        //:   1 Create descriptions for a varied set of time zone
        //:     values, some of which are not well-defined.  Initialize a test
        //:     loader with these values.  Note that each time zone, for the
        //:     purpose of this test, is uniquely determined by an identifier
        //:     and a single transition.
        //:
        //:   2 For each row in the test table
        //:     1 Create an empty ZoneinfoCache.
        //:
        //:     2 Call 'getZoneinfo' and verify it returns an error and
        //:       'UNSUPPORTED_ID' error code.
        //:
        //:     3 Configure the test loader with the Zoneinfo
        //:
        //:     4 Verify a subsequent call to 'getZoneinfo' returns success.
        //:
        //: 2 Create a implementation of 'baetzo_Loader' that returns a
        //:    specific error code that is not 'UNSUPPORTED_ID'  For a series
        //:    of test identifiers, ensure that the cache propagates the
        //:    specific error code.
        //:
        //: 3 For each row in the test-table:
        //:    1 Verify that loading a time zone does not allocate from the
        //:      default allocator, and does allocate from the object allocator
        //:
        //:    2 Once the object is destroyed, all memory is released.
        //:
        //: 4 Use the 'BSLMA_EXCEPTION_TEST' macro, and for each row in the
        //:   test-table:
        //:    1 Call 'getZoneinfo' and verify the results match the expected
        //:     Zoneinfo value.
        //:
        //: 5 Use ASSERT_PASS and ASSERT_FAIL to test assertions for null
        //:   pointers.
        //
        // Testing:
        //   const baetzo_Zoneinfo *getZoneinfo(int *, const char *);
        // --------------------------------------------------------------------

        if (verbose) cout << endl
                          << "TEST: 'getZoneinfo(int *, const char *)'" << endl
                          << "=====================================" << endl;

        struct TimeZoneData {
            int         d_line;       // line number
            const char *d_id;         // time zone id
            int         d_utcOffset;  // utc offset in seconds for descriptor
            bool        d_dstFlag;    // dst flag for descriptor
            const char *d_abbrev;     // abbreviation for descriptor, or 0 for
                                      // invalid time zones

        } VALUES[] = {
            { L_,  "ID_A",  1, true,  "A" },
            { L_,  "ID_B",  2, false, "B" },
            { L_,  "ID_BA", 3, true,  0   },
            { L_,  "ID_C",  4, false, "A" },
            { L_,  "ID_D",  5, true,  "B" },
            { L_,  "ID_E",  6, false, 0   },
            { L_,  "ID_BB", 7, true,  "A" },
        };
        const int NUM_VALUES = sizeof(VALUES) / sizeof(*VALUES);

        if (veryVerbose) cout << "\tTest returning 'UNSPECIFIED_ID'" << endl;
        TestDriverTestLoader testLoader(Z);
        for (int i = 0; i < NUM_VALUES; ++i) {
            const int   LINE    = VALUES[i].d_line;
            const char *ID      = VALUES[i].d_id;
            const bool  SUCCESS = VALUES[i].d_abbrev != 0;

            Obj mX(&testLoader, Z); const Obj& X = mX;

            int rc = INT_MIN;
            LOOP_ASSERT(LINE, 0 == mX.getZoneinfo(&rc, ID));
            LOOP_ASSERT(LINE, U == rc);

            testLoader.addTimeZone(VALUES[i].d_id,
                                   VALUES[i].d_utcOffset,
                                   VALUES[i].d_dstFlag,
                                   VALUES[i].d_abbrev);
            if (SUCCESS) {
                LOOP_ASSERT(LINE, 0 != mX.getZoneinfo(&rc, ID));
                LOOP_ASSERT(LINE, 0 == rc);
            }
        }
        {
            if (veryVerbose)
                cout << "\tTest time zones that are not well defined "
                     << "produce an error." << endl;

            Obj mX(&testLoader, Z); const Obj& X = mX;

            for (int i = 0; i < NUM_VALUES; ++i) {
                const int   LINE    = VALUES[i].d_line;
                const char *ID      = VALUES[i].d_id;
                const bool  SUCCESS = VALUES[i].d_abbrev != 0;

                int         rc     = INT_MIN;
                const Zone *result = mX.getZoneinfo(&rc, ID);
                if (SUCCESS) {
                    LOOP_ASSERT(LINE, 0 != result);
                    LOOP_ASSERT(LINE, 0 == rc);
                }
                else {
                    LOOP_ASSERT(LINE, 0 == result);
                    LOOP_ASSERT(LINE, 0 != rc);
                    LOOP_ASSERT(LINE, U != rc);
                    LOOP_ASSERT(LINE, INT_MIN != rc);
                }
                LOOP_ASSERT(LINE, 0 == defaultAllocator.numBytesInUse());
                LOOP_ASSERT(LINE, 0 <  testAllocator.numBytesInUse());
            }
        }
        {
            if (veryVerbose)
                cout << "\tTest 'getZoneinfo' propagates error codes from "
                     << "'baetzo_Loader'. " << endl;

            class ErrorLoader : public baetzo_Loader {
                // DATA
                int d_code;

              public:
                // CREATORS
                explicit ErrorLoader(int code): d_code(code) {}
                virtual ~ErrorLoader() {}
                // MANIPULATORS
                virtual int loadTimeZone(baetzo_Zoneinfo *result,
                                         const char      *getZoneinfo)
                { return d_code; }
            };

            enum { ERROR_CODE = 61 };
            ErrorLoader errorLoader(ERROR_CODE);
            Obj mX(&errorLoader, Z); const Obj& X = mX;

            for (int i = 0; i < NUM_VALUES; ++i) {
                const int   LINE    = VALUES[i].d_line;
                const char *ID      = VALUES[i].d_id;

                int         rc     = INT_MIN;

                const Zone *result = mX.getZoneinfo(&rc, ID);
                LOOP_ASSERT(LINE, 0 == result);
                LOOP_ASSERT(LINE, 0 != rc);
                LOOP_ASSERT(LINE, U != rc);
                LOOP_ASSERT(LINE, ERROR_CODE == rc);
            }
        }

        const int EXP_NUM_BYTES = testAllocator.numBytesInUse();
        {
            if (veryVerbose) cout << "\tTesting allocation." << endl;

            Obj mX(&testLoader, Z); const Obj& X = mX;

            ASSERT(0 == defaultAllocator.numBytesInUse());
            ASSERT(0 <  testAllocator.numBytesInUse());

            int lastNumBytes = testAllocator.numBytesInUse();
            for (int i = 0; i < NUM_VALUES; ++i) {
                const int   LINE    = VALUES[i].d_line;
                const char *ID      = VALUES[i].d_id;
                const bool  SUCCESS = VALUES[i].d_abbrev != 0;

                if (!SUCCESS) {
                    continue;
                }

                int         rc     = INT_MIN;
                const Zone *result = mX.getZoneinfo(&rc, ID);

                LOOP_ASSERT(LINE, 0 == defaultAllocator.numBytesInUse());
                LOOP_ASSERT(LINE,
                            lastNumBytes < testAllocator.numBytesInUse());
                lastNumBytes = testAllocator.numBytesInUse();
            }
        }
        ASSERT(0 == defaultAllocator.numBytesInUse());
        ASSERT(EXP_NUM_BYTES == testAllocator.numBytesInUse());

        {
            if (veryVerbose) cout << "\tTesting exception neutrality." << endl;

            bslma_TestAllocator  testAllocator;
            Obj mX(&testLoader, Z); const Obj& X = mX;
            BEGIN_BSLMA_EXCEPTION_TEST {
                for (int i = 0; i < NUM_VALUES; ++i) {
                    const int   LINE    = VALUES[i].d_line;
                    const char *ID      = VALUES[i].d_id;
                    const bool  SUCCESS = VALUES[i].d_abbrev != 0;

                    int rc;
                    const Zone *result = mX.getZoneinfo(&rc, ID);

                    if (SUCCESS) {
                        LOOP_ASSERT(LINE, 0 != result);
                        LOOP_ASSERT(LINE, 0 == rc);
                    }
                    else {
                        LOOP_ASSERT(LINE, 0 == result);
                        LOOP_ASSERT(LINE, 0 != rc);
                        LOOP_ASSERT(LINE, U != rc);
                        LOOP_ASSERT(LINE, INT_MIN != rc);
                    }
                }
                ASSERT(0 == defaultAllocator.numBytesInUse());
            } END_BSLMA_EXCEPTION_TEST
        }
        {
            bsls_AssertFailureHandlerGuard hG(bsls_AssertTest::failTestDriver);
            if (veryVerbose) cout << "\tTest assertions." << endl;

            TestDriverTestLoader testLoader(Z);
            Obj mX(&testLoader, Z); const Obj& X = mX;

            int rc;
            ASSERT_FAIL(mX.getZoneinfo(&rc, (const char *)0));
            ASSERT_FAIL(mX.getZoneinfo((int *)0, "abc"));
            ASSERT_PASS(mX.getZoneinfo(&rc, "abc"));
        }
      } break;
      case 3: {
        // --------------------------------------------------------------------
        // TESTING PRIMARY MANIPULATOR (BOOSTRAP)
        //
        // Concerns:
        //: 1 A constructed 'baetzo_ZoneinfoCache' object is initialized
        //:   without any time zones in the cache.
        //:
        //: 2 A first call to 'getZoneinfo' for a time zone identifier, returns
        //:   a newly loaded description using the 'baetzo_Loader' protocol
        //:   supplied at construction.
        //:
        //: 3 Subsequenct calls to 'getZoneinfo' for a successfully loaded time
        //:   zone, return that previously cached value.
        //
        // Plan:
        //: 1 Create a default 'baetzo_ZoneinfoCache' object with a
        //:   test-loader, and verify it makes no attempts to load data.
        //:
        //: 2 Using a table-driven approach
        //:   1 Create descriptions for a varied set of time zone
        //:     values, some of which are not well-defined.  Note that each
        //:     time zone, for the purpose of this test, is uniquely
        //:     determined by an identifier and a single transition.
        //:
        //:   2 Initialize a TestDriverTestLoader with the valid time zones
        //:
        //:   3 Initialize a 'baetzo_Zoneinfo' object to test.
        //:
        //:   4 For each row in the test table:
        //:     1 Call 'getZoneinfo' and verify it returns the either an
        //:       error, if the time zone is not well-formed, or the correct
        //:       Zoneinfo value.
        //:
        //:     2 That a subsequent call to 'getZoneinfo' returns the same
        //:       address.
        //
        // Testing:
        //   explicit baetzo_ZoneinfoCache(baetzo_Loader *, *ba = 0);
        //   ~baetzo_ZoneinfoCache();
        //   const baetzo_Zoneinfo *getZoneinfo(int *, const char *);
        // --------------------------------------------------------------------

        if (verbose) cout << endl << "BOOTSTRAP 'getZoneinfo'" << endl
                                  << "====================" << endl;

        const char *NO_REQ = TestDriverTestLoader::NO_REQUESTS;
        {
            if (veryVerbose) cout << "\tTest default construction" << endl;

            TestDriverTestLoader testLoader(Z);
            Obj mX(&testLoader, Z); const Obj& X = mX;

            ASSERT(0 == defaultAllocator.numBytesInUse());

            ASSERT(NO_REQ == testLoader.lastRequestedTimeZone());
        }

        struct TimeZoneData {
            int         d_line;       // line number
            const char *d_id;         // time zone id
            int         d_utcOffset;  // utc offset in seconds for descriptor
            bool        d_dstFlag;    // dst flag for descriptor
            const char *d_abbrev;     // abbreviation for descriptor, or 0 for
                                      // invalid time zones

        } VALUES[] = {
            { L_,  "ID_A",  1, true,  "A" },
            { L_,  "ID_B",  2, false, "B" },
            { L_,  "ID_BA", 3, true,  0   },
            { L_,  "ID_C",  4, false, "A" },
            { L_,  "ID_D",  5, true,  "B" },
            { L_,  "ID_E",  6, false, 0   },
            { L_,  "ID_BB", 7, true,  "A" },
        };
        const int NUM_VALUES = sizeof(VALUES) / sizeof(*VALUES);

        TestDriverTestLoader testLoader(Z);
        for (int i = 0; i < NUM_VALUES; ++i) {
            testLoader.addTimeZone(VALUES[i].d_id,
                                   VALUES[i].d_utcOffset,
                                   VALUES[i].d_dstFlag,
                                   VALUES[i].d_abbrev);
        }

        {
            if (veryVerbose) cout << "\tTest basic behavior" << endl;

            Obj mX(&testLoader, Z); const Obj& X = mX;

            ASSERT(NO_REQ == testLoader.lastRequestedTimeZone());
            for (int i = 0; i < NUM_VALUES; ++i) {
                const int   LINE    = VALUES[i].d_line;
                const char *ID      = VALUES[i].d_id;
                const bool  SUCCESS = VALUES[i].d_abbrev != 0;

                Zone expected(Z); const Zone& EXPECTED = expected;
                testLoader.loadTimeZone(&expected, ID);

                int         rc     = INT_MIN;
                const Zone *result = mX.getZoneinfo(&rc, ID);
                ASSERT(ID == testLoader.lastRequestedTimeZone());
                if (SUCCESS) {
                    LOOP_ASSERT(LINE, 0 != result);
                    LOOP_ASSERT(LINE, 0 == rc);
                    LOOP_ASSERT(LINE, EXPECTED == *result);
                    LOOP_ASSERT(LINE, result   == mX.getZoneinfo(&rc, ID));
                }
                else {
                    LOOP_ASSERT(LINE, 0 == result);
                    LOOP_ASSERT(LINE, 0 != rc);
                    LOOP_ASSERT(LINE, U != rc);
                    LOOP_ASSERT(LINE, INT_MIN != rc);
                }
                LOOP_ASSERT(LINE, 0 == defaultAllocator.numBytesInUse());
                LOOP_ASSERT(LINE, 0 <  testAllocator.numBytesInUse());
            }
        }
      } break;
      case 2: {
        // --------------------------------------------------------------------
        // TESTING: TEST APPARATUS (TestDriverTestLoader)
        //
        // Concerns:
        //: 1 The 'TestDriverTestLoader' correctly adds and returns valid and
        //:   invalid 'baetzo_Zoneinfo' objects.  For valid Zoneinfo objects,
        //:   they contain a single local-time descriptor with the properties
        //:   provided.
        //:
        //: 2 That 'loadTimeZone' returns
        //:   'baetzo_ErrorCode::BAETZO_UNSUPPORTED_ID' for time zone ids that
        //:   have not been added.
        //:
        //: 3 That 'lastReturnedTimeZone' correctly returns the last time zone
        //:   requested using the 'loadTimeZone' method, and
        //:   'TestDriverTestLoader::NO_REQUESTS' otherwise.
        //
        // Plan:
        //: 1 Verify a default constructed TestDriverTestLoader correctly
        //:   returns status.
        //:
        //: 2 Using a table-driven approach
        //:   1 Create descriptions for a varied set of valid time zone
        //:     values.  Note that each time zone, for the purpose of this
        //:     test, is uniquely determined by an identifier and a single
        //:     transition.
        //:
        //:   2 Create a TestDriverTestLoader to test.
        //:
        //:   3 For each row in the test table:
        //:     1 Add the Zoneinfo to the TestDriverTestLoader
        //:
        //:     2 For each row in the test table:
        //:       1 If the row has been added to the TestDriverTestLoader,
        //:         verify that it exists and is returned correction.
        //:
        //:       2 If the row does not exist, verify that 'loadTimeZone'
        //:         returns an error status.
        //
        // Testing:
        //   explicit TestDriverTestLoader(bslma_Allocator *);
        //   virtual ~TestDriverTestLoader();
        //   void addTimeZone(const char *, int , bool, const char *);
        //   void addInvalidTimeZone(const char *);
        //   virtual int loadTimeZone(baetzo_Zoneinfo *, const bsl::string&);
        //   const std::string& lastRequestedTimeZone() const;
        // --------------------------------------------------------------------

        if (verbose) cout << endl << "TESTING: TEST APARATUS" << endl
                                  << "======================" << endl;

        typedef TestDriverTestLoader TDTL;
        {
            if (veryVerbose) cout << "\tDefault constructor" << endl;

            TDTL mX(Z); const TDTL& X = mX;
            Zone zone(Z);

            ASSERT(TDTL::NO_REQUESTS == X.lastRequestedTimeZone());
            ASSERT(                U == mX.loadTimeZone(&zone, "A"));

            ASSERT(0 == defaultAllocator.numBytesInUse());
        }
        ASSERT(0 == defaultAllocator.numBytesInUse());
        ASSERT(0 == testAllocator.numBytesInUse());

        struct TimeZoneData {
            int         d_line;       // line number
            const char *d_id;         // time zone id
            int         d_utcOffset;  // utc offset in seconds for descriptor
            bool        d_dstFlag;    // dst flag for descriptor
            const char *d_abbrev;     // abbreviation for descriptor, or 0 for
                                      // invalid time zones

        } VALUES[] = {
            { L_,  "ID_A",  1, true,  "A" },
            { L_,  "ID_B",  2, false, "B" },
            { L_,  "ID_BA", 3, true,  0   },
            { L_,  "ID_C",  4, false, "A" },
            { L_,  "ID_D",  5, true,  "B" },
            { L_,  "ID_E",  6, false, 0   },
            { L_,  "ID_BB", 7, true,  "A" },
        };
        const int NUM_VALUES = sizeof(VALUES) / sizeof(*VALUES);
        if (veryVerbose) cout << "\tManipulators & accessors" << endl;
        {
            TDTL mX(Z); const TDTL& X = mX;
            for (int i = 0; i < NUM_VALUES; ++i) {
                const int   LINE    = VALUES[i].d_line;
                const char *ID      = VALUES[i].d_id;
                bool        INVALID = VALUES[i].d_abbrev == 0;
                bool        DST     = VALUES[i].d_dstFlag;
                const char *ABBREV  = VALUES[i].d_abbrev;
                const int   OFFSET  = VALUES[i].d_utcOffset;

                mX.addTimeZone(ID, OFFSET, DST, ABBREV);

                for (int j = 0; j < NUM_VALUES; ++j) {
                    const char *ID      = VALUES[j].d_id;
                    bool        INVALID = VALUES[j].d_abbrev == 0;
                    bool        DST     = VALUES[j].d_dstFlag;
                    const char *ABBREV  = VALUES[j].d_abbrev;
                    const int   OFFSET  = VALUES[j].d_utcOffset;

                    Zone result(Z);
                    int rc = mX.loadTimeZone(&result, ID);
                    LOOP_ASSERT(LINE, ID == X.lastRequestedTimeZone());

                    if (veryVeryVerbose) {
                        P(result);
                    }
                    if (j > i) {
                        LOOP_ASSERT(LINE,
                                    U == rc);
                    }
                    else if (INVALID) {
                        LOOP_ASSERT(LINE, 0 == rc);
                        LOOP_ASSERT(LINE, 0 == result.numTransitions());
                    }
                    else {
                        LOOP_ASSERT(LINE, 0 == rc);
                        LOOP_ASSERT(LINE, 1 == result.numTransitions());

                        bdet_Datetime firstTime(1, 1, 1);
                        bsls_Types::Int64 firstTimeT =
                                    bdetu_Epoch::convertToTimeT64(firstTime);

                        baetzo_Zoneinfo::TransitionConstIterator itT =
                                                     result.beginTransitions();
                        LOOP_ASSERT(LINE, result.endTransitions() != itT);

                        const baetzo_LocalTimeDescriptor& D =
                                                             itT->descriptor();
                        LOOP_ASSERT(LINE, DST    == D.dstInEffectFlag());
                        LOOP_ASSERT(LINE, OFFSET == D.utcOffsetInSeconds());
                        LOOP_ASSERT(LINE, ABBREV == D.description());
                    }
                    ASSERT(0 == defaultAllocator.numBytesInUse());
                 }
            }
        }
      } break;
      case 1: {
        // --------------------------------------------------------------------
        // BREATHING TEST:
        //   Developers' Sandbox.
        //
        // Plan:
        //   Perform and ad-hoc test of the primary modifiers and accessors.
        //
        // Testing:
        //   This "test" *exercises* basic functionality, but *tests* nothing.
        // --------------------------------------------------------------------

        if (verbose) cout << endl << "BREATHING TEST" << endl
                                  << "==============" << endl;

        TestLoader testLoader;

        bsls_Types::Int64 NB = defaultAllocator.numBytesInUse();
        Obj mX(&testLoader, Z); const Obj& X = mX;
        ASSERT(defaultAllocator.numBytesInUse() == NB);

        // Verify the cache returns 0 when the loader is not populated.
        const char *IDA = "America/New_York";
        const char *IDB = "Europe/London";
        const char *IDC = "Asia/Beijing";

        int rcA; const int& RCA = rcA;
        int rcB; const int& RCB = rcB;
        int rcC; const int& RCC = rcC;

        NB = defaultAllocator.numBytesInUse();
        const Zone *TZA = mX.getZoneinfo(&rcA, IDA);
        const Zone *TZB = mX.getZoneinfo(&rcB, IDB);
        const Zone *TZC = mX.getZoneinfo(&rcC, IDC);
        ASSERT(defaultAllocator.numBytesInUse() == NB);

        ASSERT(0 == TZA);  ASSERT(0 == TZB);  ASSERT(0 == TZC);
        ASSERT(U == RCA);  ASSERT(U == RCB);  ASSERT(U == RCC);

        // Fill the loader with sample data.
        const char *ID_ARRAY[] = { IDA, IDB, IDC };
        const int   NUM_TIME_ZONES = sizeof(ID_ARRAY)/sizeof(*ID_ARRAY);

        bdet_Datetime firstTime(1, 1, 1);
        bsls_Types::Int64 firstTimeT =
                                      bdetu_Epoch::convertToTimeT64(firstTime);

        bsl::vector<Zone> timeZones(Z);
        for (int i = 0; i < 3; ++i) {
            Zone tz(Z);

            tz.setIdentifier(ID_ARRAY[i]);
            baetzo_LocalTimeDescriptor desc(i * 60 * 60, true,  "dummy", Z);
            tz.addTransition(firstTimeT, desc);
            testLoader.setTimeZone(tz);
            timeZones.push_back(tz);
        }

        // Verify that 'getZoneinfo' correctly returns the sample data, but
        // 'lookupZoneinfo' only returns the sample data if it had been
        // previously populated.
        for (int i = 0; i < 3; ++i) {
            int rc;
            ASSERT(0 == X.lookupZoneinfo(ID_ARRAY[i]));
            const Zone *TZ = mX.getZoneinfo(&rc, ID_ARRAY[i]);
            ASSERT(0           == rc);
            ASSERT(0           != TZ);
            ASSERT(ID_ARRAY[i] == TZ->identifier());

            ASSERT(timeZones[i] == *TZ);
            ASSERT(TZ           == X.lookupZoneinfo(ID_ARRAY[i]));
            ASSERT(TZ           == mX.getZoneinfo(&rc, ID_ARRAY[i]));
        }
        {
            if (verbose) {
                cout << "\tVerify only well-defined time zones are returned."
                     << endl;
            }
            Zone tz(Z), result(Z);

            tz.setIdentifier("testId");
            testLoader.setTimeZone(tz);
            ASSERT(0 == testLoader.loadTimeZone(&result, "testId"));

            // Verify a badly defined time zone is not loaded
            int rc;
            ASSERT(0 == mX.getZoneinfo(&rc, "testId"));
            ASSERT(0 != rc);
            ASSERT(0 == X.lookupZoneinfo("testId"));

            baetzo_LocalTimeDescriptor desc(0, true,  "dummy", Z);
            tz.addTransition(firstTimeT, desc);

            testLoader.setTimeZone(tz);
            ASSERT(0 == testLoader.loadTimeZone(&result, "testId"));

            // Verify that if the loader value is fixed, the time zone will be
            // loaded.
            ASSERT(tz == *mX.getZoneinfo(&rc, "testId"));
            ASSERT(0 == rc);
            ASSERT(tz == *X.lookupZoneinfo("testId"));
        }
      } break;
      default: {
        cerr << "WARNING: CASE `" << test << "' NOT FOUND." << endl;
        testStatus = -1;
      }
    }

    ASSERT(0 == Z->numBytesInUse());

    if (testStatus > 0) {
        cerr << "Error, non-zero test status = " << testStatus << "." << endl;
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
