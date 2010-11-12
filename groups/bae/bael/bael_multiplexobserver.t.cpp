// bael_multiplexobserver.t.cpp         -*-C++-*-

#include <bael_multiplexobserver.h>

#include <bael_context.h>                       // for testing only
#include <bael_defaultobserver.h>               // for testing only
#include <bael_record.h>                        // for testing only
#include <bael_recordattributes.h>              // for testing only
#include <bael_testobserver.h>                  // for testing only
#include <bael_transmission.h>                  // for testing only

#include <bdema_bufferedsequentialallocator.h>  // for testing only
#include <bdetu_datetime.h>                     // for testing only

#include <bslma_testallocator.h>                // for testing only
#include <bslma_testallocatorexception.h>       // for testing only
#include <bsls_platformutil.h>                  // for testing only

#include <bsl_cstdlib.h>     // atoi()
#include <bsl_cstring.h>     // strlen(), memset(), memcpy(), memcmp()
#include <bsl_ctime.h>       // time()

#include <bsl_new.h>         // placement 'new' syntax
#include <bsl_iostream.h>
#include <bsl_strstream.h>

using namespace BloombergLP;
using namespace bsl;  // automatically added by script

//=============================================================================
//                              TEST PLAN
//-----------------------------------------------------------------------------
//                              Overview
//                              --------
// The component under test is a container of Observers that forwards records
// and their corresponding publication contexts to the contained observers.
// We must ensure that the component behaves correctly as both a container
// of Observers and a relayer of records and contexts.
//-----------------------------------------------------------------------------
// [ 2] bael_MultiplexObserver(bslma_Allocator *ba = 0);
// [ 2] ~bael_MultiplexObserver();
// [ 4] void publish(const bael_Record& rec, const bael_Context& ctxt);
// [ 3] int registerObserver(bael_Observer *observer);
// [ 3] int deregisterObserver(bael_Observer *observer);
// [ 3] int numRegisteredObservers() const;
//-----------------------------------------------------------------------------
// [ 1] BREATHING TEST
// [ 5] USAGE EXAMPLE

//=============================================================================
//                      STANDARD BDE ASSERT TEST MACRO
//-----------------------------------------------------------------------------
static int testStatus = 0;

void aSsErT(int c, const char *s, int i)
{
    if (c) {
        cout << "Error " << __FILE__ << "(" << i << "): " << s
             << "    (failed)" << endl;
        if (0 <= testStatus && testStatus <= 100) ++testStatus;
    }
}

#define ASSERT(X) { aSsErT(!(X), #X, __LINE__); }

//=============================================================================
//                  STANDARD BDE LOOP-ASSERT TEST MACROS
//-----------------------------------------------------------------------------
#define LOOP_ASSERT(I,X) { \
   if (!(X)) { cout << #I << ": " << I << "\n"; aSsErT(1, #X, __LINE__); }}

#define LOOP2_ASSERT(I,J,X) { \
   if (!(X)) { cout << #I << ": " << I << "\t" << #J << ": " \
              << J << "\n"; aSsErT(1, #X, __LINE__); } }

#define LOOP3_ASSERT(I,J,K,X) { \
   if (!(X)) { cout << #I << ": " << I << "\t" << #J << ": " << J << "\t" \
              << #K << ": " << K << "\n"; aSsErT(1, #X, __LINE__); } }

//=============================================================================
//                  SEMI-STANDARD TEST OUTPUT MACROS
//-----------------------------------------------------------------------------
#define P(X) cout << #X " = " << (X) << endl; // Print identifier and value.
#define Q(X) cout << "<| " #X " |>" << endl;  // Quote identifier literally.
#define P_(X) cout << #X " = " << (X) << ", "<< flush; // P(X) without '\n'
#define L_ __LINE__                           // current Line number
#define T_()  cout << "\t" << flush;          // Print tab w/o newline

//=============================================================================
//                             USAGE EXAMPLE
//-----------------------------------------------------------------------------

// my_ostreamobserver.h

class my_OstreamObserver : public bael_Observer {
    ostream& d_stream;

  public:
    my_OstreamObserver(ostream& stream) : d_stream(stream) { }
    ~my_OstreamObserver() { }
    void publish(const bael_Record&  record,
                 const bael_Context& context);
};

// my_ostreamobserver.cpp

void my_OstreamObserver::publish(const bael_Record&  record,
                                 const bael_Context& context)
{

    switch (context.transmissionCause()) {
      case bael_Transmission::BAEL_PASSTHROUGH: {
        d_stream << "Single Passthrough Message:" << endl;
      } break;
      case bael_Transmission::BAEL_TRIGGER_ALL: {
        d_stream << "Remotely ";      // no 'break'; concatenated output
      }
      case bael_Transmission::BAEL_TRIGGER: {
        d_stream << "Triggered Publication Sequence: Message ";
        d_stream << context.recordIndex() + 1  // Account for 0-based index.
                 << " of " << context.sequenceLength() << endl;
      } break;
      default: {
        d_stream << "***ERROR*** Unknown Message Cause:" << endl;
      } break;
    }

    d_stream << "\tTimestamp:  " << record.fixedFields().timestamp() << endl;
    d_stream << "\tProcess ID: " << record.fixedFields().processID() << endl;
    d_stream << "\tThread ID:  " << record.fixedFields().threadID()  << endl
             << endl;
}

//=============================================================================
//                  GLOBAL TYPEDEFS/CONSTANTS FOR TESTING
//-----------------------------------------------------------------------------

typedef bael_MultiplexObserver Obj;

typedef bael_Context           Ctxt;
typedef bael_Record            Rec;
typedef bael_RecordAttributes  Attr;
typedef bael_TestObserver      TestObs;

typedef bdem_List              List;

const int SEQUENCE_LENGTH = 99;

//=============================================================================
//                  GLOBAL HELPER FUNCTIONS FOR TESTING
//-----------------------------------------------------------------------------

// The following three helper functions are used to test 'publish' (case 4).

static int nextRecord(bael_Record& record)
    // Modify the specified 'record' to have a value distinct from any other
    // record transmitted so far in the publication sequence and return the
    // 0-based sequence number of 'record'.
{
    static int sequenceNumber = 0;

    bael_RecordAttributes attr;
    attr.setLineNumber(sequenceNumber);
    attr.setProcessID(sequenceNumber);
    attr.setSeverity(sequenceNumber);
    attr.setThreadID(sequenceNumber);
    record.setFixedFields(attr);

    return sequenceNumber++;
}

static int isNthRecord(const bael_Record& record, int nth)
    // Return 1 if the specified 'record' is the specified 'nth' in the
    // publication sequence, and 0 otherwise.
{
    const bael_RecordAttributes& attr = record.fixedFields();

    if (nth == attr.lineNumber()
     && nth == attr.processID()
     && nth == attr.severity()
     && nth == attr.threadID()) {
        return 1;
    }
    else {
        return 0;
    }
}

static int isNthContext(const bael_Context& context, int nth)
    // Return 1 if the specified 'context' is the specified 'nth' in the
    // publication sequence, and 0 otherwise.
{
    if (bael_Transmission::BAEL_TRIGGER == context.transmissionCause()
        &&                     nth == context.recordIndex()
        &&         SEQUENCE_LENGTH == context.sequenceLength()) {
        return 1;
    }
    else {
        return 0;
    }
}

//=============================================================================
//                              MAIN PROGRAM
//-----------------------------------------------------------------------------

int main(int argc, char *argv[])
{
    int test = argc > 1 ? atoi(argv[1]) : 0;
    int verbose = argc > 2;
    //     int veryVerbose = argc > 3; // unused
    int veryVeryVerbose = argc > 4;

    cout << "TEST " << __FILE__ << " CASE " << test << endl;;

    bslma_TestAllocator testAllocator(veryVeryVerbose);
    bslma_TestAllocator *Z = &testAllocator;

    switch (test) { case 0:  // Zero is always the leading case.
      case 5: {
        // --------------------------------------------------------------------
        // TESTING USAGE EXAMPLE
        //
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

        if (verbose) cout << endl << "Testing Usage Example" << endl
                                  << "=====================" << endl;

        {
            typedef bael_DefaultObserver my_LogfileObserver;
            typedef bael_DefaultObserver my_EncryptingObserver;

            bael_MultiplexObserver multiplexor;
            ASSERT(0 == multiplexor.numRegisteredObservers());

            bael_DefaultObserver   defaultObserver(bsl::cout);
            my_LogfileObserver     logfileObserver(bsl::cout);
            my_EncryptingObserver  encryptingObserver(bsl::cout);

            ASSERT(0 == multiplexor.registerObserver(&defaultObserver));
            ASSERT(0 == multiplexor.registerObserver(&logfileObserver));
            ASSERT(0 == multiplexor.registerObserver(&encryptingObserver));
            ASSERT(3 == multiplexor.numRegisteredObservers());

            // Do *not* do this.  It significantly increases the level #
            // of this component.
            //bael_LoggerManager::initSingleton(&multiplexor);
        }
      } break;
      case 4: {
        // --------------------------------------------------------------------
        // TESTING PUBLISH
        //
        // Concerns:
        //   The primary concern is that 'publish' correctly forwards records
        //   and their corresponding publication contexts to all registered
        //   observers that should received them.
        //
        // Plan:
        //   Construct a pair of multiplex observers x and y, and a trio of
        //   test observers o1, o2, and o3.  Arrange x, y, o1, o2, and o3 into
        //   various configurations with o1, o2, and o3 registered with at most
        //   one of the multiplex observers.  Also register y with x for a
        //   portion of the test.  Publish several distinct records to x and
        //   y and verify that all registered observers correctly receive the
        //   records and corresponding publication contexts.  The helper
        //   functions 'nextRecord', 'isNthRecord', and 'isNthContext' are
        //   used to verify that the records and contexts are properly
        //   transmitted.
        //
        // Testing:
        //   void publish(const bael_Record& rec, const bael_Context& ctxt);
        // --------------------------------------------------------------------

        if (verbose) cout << endl << "Testing 'publish'" << endl
                                  << "=================" << endl;

        {
            //  X
            Obj mX;  const Obj& X = mX;
            ASSERT(0 == X.numRegisteredObservers());

            //  Y
            Obj mY;  const Obj& Y = mY;
            ASSERT(0 == Y.numRegisteredObservers());

            TestObs mO1(bsl::cout);  const TestObs& O1 = mO1;
            TestObs mO2(bsl::cout);  const TestObs& O2 = mO2;
            TestObs mO3(bsl::cout);  const TestObs& O3 = mO3;

            Rec mR;  const Rec& R = mR;

            Ctxt mC(bael_Transmission::BAEL_TRIGGER, 0, SEQUENCE_LENGTH);
            const Ctxt& C = mC;

            //   X
            //   |
            //   O1
            ASSERT(0 == mX.registerObserver(&mO1));
            ASSERT(1 == X.numRegisteredObservers());
            ASSERT(0 == O1.numPublishedRecords());

            const int I1 = nextRecord(mR);  ASSERT(1 == isNthRecord(R, I1));
            mC.setRecordIndexRaw(I1);       ASSERT(1 == isNthContext(C, I1));
            mX.publish(R, C);
            ASSERT(1 == O1.numPublishedRecords());
            ASSERT(1 == isNthRecord(O1.lastPublishedRecord(), I1));
            ASSERT(1 == isNthContext(O1.lastPublishedContext(), I1));

            //   X
            ASSERT(0 == mX.deregisterObserver(&mO1));
            ASSERT(0 == X.numRegisteredObservers());

            const int I2 = nextRecord(mR);  ASSERT(1 == isNthRecord(R, I2));
            mC.setRecordIndexRaw(I2);       ASSERT(1 == isNthContext(C, I2));
            mX.publish(R, C);
            ASSERT(1 == O1.numPublishedRecords());
            ASSERT(1 == isNthRecord(O1.lastPublishedRecord(), I1));
            ASSERT(1 == isNthContext(O1.lastPublishedContext(), I1));

            //   X
            //   |
            //   O1
            ASSERT(0 == mX.registerObserver(&mO1));
            ASSERT(1 == X.numRegisteredObservers());
            ASSERT(1 == O1.numPublishedRecords());

            const int I3 = nextRecord(mR);  ASSERT(1 == isNthRecord(R, I3));
            mC.setRecordIndexRaw(I3);       ASSERT(1 == isNthContext(C, I3));
            mX.publish(R, C);
            ASSERT(2 == O1.numPublishedRecords());
            ASSERT(1 == isNthRecord(O1.lastPublishedRecord(), I3));
            ASSERT(1 == isNthContext(O1.lastPublishedContext(), I3));

            //     X
            //    / |
            //  O1  O2
            ASSERT(0 == mX.registerObserver(&mO2));
            ASSERT(2 == X.numRegisteredObservers());
            ASSERT(0 == O2.numPublishedRecords());

            const int I4 = nextRecord(mR);  ASSERT(1 == isNthRecord(R, I4));
            mC.setRecordIndexRaw(I4);       ASSERT(1 == isNthContext(C, I4));
            mX.publish(R, C);
            ASSERT(3 == O1.numPublishedRecords());
            ASSERT(1 == isNthRecord(O1.lastPublishedRecord(), I4));
            ASSERT(1 == isNthContext(O1.lastPublishedContext(), I4));
            ASSERT(1 == O2.numPublishedRecords());
            ASSERT(1 == isNthRecord(O2.lastPublishedRecord(), I4));
            ASSERT(1 == isNthContext(O2.lastPublishedContext(), I4));

            //     --X--
            //    /  |  |
            //  O1   O2 Y
            ASSERT(0 == mX.registerObserver(&mY));
            ASSERT(3 == X.numRegisteredObservers());
            ASSERT(0 == Y.numRegisteredObservers());

            const int I5 = nextRecord(mR);  ASSERT(1 == isNthRecord(R, I5));
            mC.setRecordIndexRaw(I5);       ASSERT(1 == isNthContext(C, I5));
            mX.publish(R, C);
            ASSERT(4 == O1.numPublishedRecords());
            ASSERT(1 == isNthRecord(O1.lastPublishedRecord(), I5));
            ASSERT(1 == isNthContext(O1.lastPublishedContext(), I5));
            ASSERT(2 == O2.numPublishedRecords());
            ASSERT(1 == isNthRecord(O2.lastPublishedRecord(), I5));
            ASSERT(1 == isNthContext(O2.lastPublishedContext(), I5));

            //     --X--
            //    /  |  |
            //  O1   O2 Y
            //          |
            //          O3
            ASSERT(0 == mY.registerObserver(&mO3));
            ASSERT(3 == X.numRegisteredObservers());
            ASSERT(1 == Y.numRegisteredObservers());
            ASSERT(0 == O3.numPublishedRecords());

            const int I6 = nextRecord(mR);  ASSERT(1 == isNthRecord(R, I6));
            mC.setRecordIndexRaw(I6);       ASSERT(1 == isNthContext(C, I6));
            mX.publish(R, C);
            ASSERT(5 == O1.numPublishedRecords());
            ASSERT(1 == isNthRecord(O1.lastPublishedRecord(), I6));
            ASSERT(1 == isNthContext(O1.lastPublishedContext(), I6));
            ASSERT(3 == O2.numPublishedRecords());
            ASSERT(1 == isNthRecord(O2.lastPublishedRecord(), I6));
            ASSERT(1 == isNthContext(O2.lastPublishedContext(), I6));
            ASSERT(1 == O3.numPublishedRecords());
            ASSERT(1 == isNthRecord(O3.lastPublishedRecord(), I6));
            ASSERT(1 == isNthContext(O3.lastPublishedContext(), I6));

            //     X
            //    / |
            //  O1  Y
            //      |
            //      O3
            ASSERT(0 == mX.deregisterObserver(&mO2));
            ASSERT(2 == X.numRegisteredObservers());
            ASSERT(1 == Y.numRegisteredObservers());

            const int I7 = nextRecord(mR);  ASSERT(1 == isNthRecord(R, I7));
            mC.setRecordIndexRaw(I7);       ASSERT(1 == isNthContext(C, I7));
            mX.publish(R, C);
            ASSERT(6 == O1.numPublishedRecords());
            ASSERT(1 == isNthRecord(O1.lastPublishedRecord(), I7));
            ASSERT(1 == isNthContext(O1.lastPublishedContext(), I7));
            ASSERT(3 == O2.numPublishedRecords());
            ASSERT(1 == isNthRecord(O2.lastPublishedRecord(), I6));
            ASSERT(1 == isNthContext(O2.lastPublishedContext(), I6));
            ASSERT(2 == O3.numPublishedRecords());
            ASSERT(1 == isNthRecord(O3.lastPublishedRecord(), I7));
            ASSERT(1 == isNthContext(O3.lastPublishedContext(), I7));

            //     X
            //    / |
            //  O1  Y
            //     / |
            //   O3  O2
            ASSERT(0 == mY.registerObserver(&mO2));
            ASSERT(2 == X.numRegisteredObservers());
            ASSERT(2 == Y.numRegisteredObservers());
            ASSERT(3 == O2.numPublishedRecords());

            const int I8 = nextRecord(mR);  ASSERT(1 == isNthRecord(R, I8));
            mC.setRecordIndexRaw(I8);       ASSERT(1 == isNthContext(C, I8));
            mX.publish(R, C);
            ASSERT(7 == O1.numPublishedRecords());
            ASSERT(1 == isNthRecord(O1.lastPublishedRecord(), I8));
            ASSERT(1 == isNthContext(O1.lastPublishedContext(), I8));
            ASSERT(4 == O2.numPublishedRecords());
            ASSERT(1 == isNthRecord(O2.lastPublishedRecord(), I8));
            ASSERT(1 == isNthContext(O2.lastPublishedContext(), I8));
            ASSERT(3 == O3.numPublishedRecords());
            ASSERT(1 == isNthRecord(O3.lastPublishedRecord(), I8));
            ASSERT(1 == isNthContext(O3.lastPublishedContext(), I8));

            //     X
            //    / |
            //  O1  Y
            //      |
            //      O2
            ASSERT(0 == mY.deregisterObserver(&mO3));
            ASSERT(2 == X.numRegisteredObservers());
            ASSERT(1 == Y.numRegisteredObservers());

            const int I9 = nextRecord(mR);  ASSERT(1 == isNthRecord(R, I9));
            mC.setRecordIndexRaw(I9);       ASSERT(1 == isNthContext(C, I9));
            mX.publish(R, C);
            ASSERT(8 == O1.numPublishedRecords());
            ASSERT(1 == isNthRecord(O1.lastPublishedRecord(), I9));
            ASSERT(1 == isNthContext(O1.lastPublishedContext(), I9));
            ASSERT(5 == O2.numPublishedRecords());
            ASSERT(1 == isNthRecord(O2.lastPublishedRecord(), I9));
            ASSERT(1 == isNthContext(O2.lastPublishedContext(), I9));
            ASSERT(3 == O3.numPublishedRecords());
            ASSERT(1 == isNthRecord(O3.lastPublishedRecord(), I8));
            ASSERT(1 == isNthContext(O3.lastPublishedContext(), I8));

            //   X   Y
            //   |   |
            //   O1  O2
            ASSERT(0 == mX.deregisterObserver(&mY));
            ASSERT(1 == X.numRegisteredObservers());
            ASSERT(1 == Y.numRegisteredObservers());

            const int I10 = nextRecord(mR);  ASSERT(1 == isNthRecord(R, I10));
            mC.setRecordIndexRaw(I10);       ASSERT(1 == isNthContext(C, I10));
            mX.publish(R, C);
            ASSERT(9 == O1.numPublishedRecords());
            ASSERT(1 == isNthRecord(O1.lastPublishedRecord(), I10));
            ASSERT(1 == isNthContext(O1.lastPublishedContext(), I10));
            ASSERT(5 == O2.numPublishedRecords());
            ASSERT(1 == isNthRecord(O2.lastPublishedRecord(), I9));
            ASSERT(1 == isNthContext(O2.lastPublishedContext(), I9));
            ASSERT(3 == O3.numPublishedRecords());
            ASSERT(1 == isNthRecord(O3.lastPublishedRecord(), I8));
            ASSERT(1 == isNthContext(O3.lastPublishedContext(), I8));

            const int I11 = nextRecord(mR);  ASSERT(1 == isNthRecord(R, I11));
            mC.setRecordIndexRaw(I11);       ASSERT(1 == isNthContext(C, I11));
            mY.publish(R, C);
            ASSERT(9 == O1.numPublishedRecords());
            ASSERT(1 == isNthRecord(O1.lastPublishedRecord(), I10));
            ASSERT(1 == isNthContext(O1.lastPublishedContext(), I10));
            ASSERT(6 == O2.numPublishedRecords());
            ASSERT(1 == isNthRecord(O2.lastPublishedRecord(), I11));
            ASSERT(1 == isNthContext(O2.lastPublishedContext(), I11));
            ASSERT(3 == O3.numPublishedRecords());
            ASSERT(1 == isNthRecord(O3.lastPublishedRecord(), I8));
            ASSERT(1 == isNthContext(O3.lastPublishedContext(), I8));

            //   X   Y
            //       |
            //       O2
            ASSERT(0 == mX.deregisterObserver(&mO1));
            ASSERT(0 == X.numRegisteredObservers());
            ASSERT(1 == Y.numRegisteredObservers());

            const int I12 = nextRecord(mR);  ASSERT(1 == isNthRecord(R, I12));
            mC.setRecordIndexRaw(I12);       ASSERT(1 == isNthContext(C, I12));
            mX.publish(R, C);
            ASSERT(9 == O1.numPublishedRecords());
            ASSERT(1 == isNthRecord(O1.lastPublishedRecord(), I10));
            ASSERT(1 == isNthContext(O1.lastPublishedContext(), I10));
            ASSERT(6 == O2.numPublishedRecords());
            ASSERT(1 == isNthRecord(O2.lastPublishedRecord(), I11));
            ASSERT(1 == isNthContext(O2.lastPublishedContext(), I11));
            ASSERT(3 == O3.numPublishedRecords());
            ASSERT(1 == isNthRecord(O3.lastPublishedRecord(), I8));
            ASSERT(1 == isNthContext(O3.lastPublishedContext(), I8));

            const int I13 = nextRecord(mR);  ASSERT(1 == isNthRecord(R, I13));
            mC.setRecordIndexRaw(I13);       ASSERT(1 == isNthContext(C, I13));
            mY.publish(R, C);
            ASSERT(9 == O1.numPublishedRecords());
            ASSERT(1 == isNthRecord(O1.lastPublishedRecord(), I10));
            ASSERT(1 == isNthContext(O1.lastPublishedContext(), I10));
            ASSERT(7 == O2.numPublishedRecords());
            ASSERT(1 == isNthRecord(O2.lastPublishedRecord(), I13));
            ASSERT(1 == isNthContext(O2.lastPublishedContext(), I13));
            ASSERT(3 == O3.numPublishedRecords());
            ASSERT(1 == isNthRecord(O3.lastPublishedRecord(), I8));
            ASSERT(1 == isNthContext(O3.lastPublishedContext(), I8));

            //   X   Y
            ASSERT(0 == mY.deregisterObserver(&mO2));
            ASSERT(0 == X.numRegisteredObservers());
            ASSERT(0 == Y.numRegisteredObservers());

            const int I14 = nextRecord(mR);  ASSERT(1 == isNthRecord(R, I14));
            mC.setRecordIndexRaw(I14);       ASSERT(1 == isNthContext(C, I14));
            mX.publish(R, C);
            mY.publish(R, C);
            ASSERT(9 == O1.numPublishedRecords());
            ASSERT(1 == isNthRecord(O1.lastPublishedRecord(), I10));
            ASSERT(1 == isNthContext(O1.lastPublishedContext(), I10));
            ASSERT(7 == O2.numPublishedRecords());
            ASSERT(1 == isNthRecord(O2.lastPublishedRecord(), I13));
            ASSERT(1 == isNthContext(O2.lastPublishedContext(), I13));
            ASSERT(3 == O3.numPublishedRecords());
            ASSERT(1 == isNthRecord(O3.lastPublishedRecord(), I8));
            ASSERT(1 == isNthContext(O3.lastPublishedContext(), I8));
        }
      } break;
      case 3: {
        // --------------------------------------------------------------------
        // TESTING OBSERVER REGISTRATION
        //
        // Concerns:
        //   Our concerns are that 'registerObserver' and 'deregisterObserver'
        //   produce the expected side-effects on the container of observers
        //   and that 'numRegisteredObservers' returns the current number of
        //   registered observers.
        //
        // Plan:
        //   Construct a pair of multiplex observers x and y, and a trio of
        //   test observers o1, o2, and o3.  Arrange x, y, o1, o2, and o3 into
        //   various configurations with o1, o2, and o3 registered with at most
        //   one of the multiplex observers.  Also register y with x for a
        //   portion of the test.  Test the return value of the three methods
        //   under test to infer that they fulfill their contractual
        //   obligations.
        //
        // Testing:
        //   int registerObserver(bael_Observer *observer);
        //   int deregisterObserver(bael_Observer *observer);
        //   int numRegisteredObservers() const;
        // --------------------------------------------------------------------

        if (verbose) cout << endl << "Testing Observer Registration" << endl
                                  << "=============================" << endl;

        if (verbose)
            cout << "Testing invalid registration and deregistration." << endl;
        {
            Obj mX;  const Obj& X = mX;
            ASSERT(0 == X.numRegisteredObservers());

            TestObs mO1(bsl::cout);
            TestObs mO2(bsl::cout);

            ASSERT(0 != mX.registerObserver(0));
            ASSERT(0 == X.numRegisteredObservers());

            ASSERT(0 != mX.deregisterObserver(&mO1));
            ASSERT(0 != mX.deregisterObserver(0));
            ASSERT(0 == X.numRegisteredObservers());

            ASSERT(0 == mX.registerObserver(&mO1));
            ASSERT(1 == X.numRegisteredObservers());

            ASSERT(0 != mX.registerObserver(0));
            ASSERT(0 != mX.registerObserver(&mO1));
            ASSERT(1 == X.numRegisteredObservers());

            ASSERT(0 != mX.deregisterObserver(&mO2));
            ASSERT(0 != mX.deregisterObserver(0));
            ASSERT(1 == X.numRegisteredObservers());

            ASSERT(0 == mX.deregisterObserver(&mO1));
            ASSERT(0 == X.numRegisteredObservers());

            ASSERT(0 != mX.deregisterObserver(&mO1));
            ASSERT(0 != mX.deregisterObserver(0));
            ASSERT(0 == X.numRegisteredObservers());
        }

        if (verbose)
            cout << "Testing valid registration and deregistration." << endl;
        {
            //  X
            Obj mX;  const Obj& X = mX;
            ASSERT(0 == X.numRegisteredObservers());

            //  Y
            Obj mY;  const Obj& Y = mY;
            ASSERT(0 == Y.numRegisteredObservers());

            TestObs mO1(bsl::cout);
            TestObs mO2(bsl::cout);
            TestObs mO3(bsl::cout);

            //   X
            //   |
            //   O1
            ASSERT(0 == mX.registerObserver(&mO1));
            ASSERT(1 == X.numRegisteredObservers());

            //   X
            ASSERT(0 == mX.deregisterObserver(&mO1));
            ASSERT(0 == X.numRegisteredObservers());

            //   X
            //   |
            //   O1
            ASSERT(0 == mX.registerObserver(&mO1));
            ASSERT(1 == X.numRegisteredObservers());

            //     X
            //    / |
            //  O1  O2
            ASSERT(0 == mX.registerObserver(&mO2));
            ASSERT(2 == X.numRegisteredObservers());

            //     --X--
            //    /  |  |
            //  O1   O2 Y
            ASSERT(0 == mX.registerObserver(&mY));
            ASSERT(3 == X.numRegisteredObservers());
            ASSERT(0 == Y.numRegisteredObservers());

            //     --X--
            //    /  |  |
            //  O1   O2 Y
            //          |
            //          O3
            ASSERT(0 == mY.registerObserver(&mO3));
            ASSERT(3 == X.numRegisteredObservers());
            ASSERT(1 == Y.numRegisteredObservers());

            //     X
            //    / |
            //  O1  Y
            //      |
            //      O3
            ASSERT(0 == mX.deregisterObserver(&mO2));
            ASSERT(2 == X.numRegisteredObservers());
            ASSERT(1 == Y.numRegisteredObservers());

            //     X
            //    / |
            //  O1  Y
            //     / |
            //   O3  O2
            ASSERT(0 == mY.registerObserver(&mO2));
            ASSERT(2 == X.numRegisteredObservers());
            ASSERT(2 == Y.numRegisteredObservers());

            //     X
            //    / |
            //  O1   Y
            //       |
            //       O2
            ASSERT(0 == mY.deregisterObserver(&mO3));
            ASSERT(2 == X.numRegisteredObservers());
            ASSERT(1 == Y.numRegisteredObservers());

            //   X   Y
            //   |   |
            //   O1  O2
            ASSERT(0 == mX.deregisterObserver(&mY));
            ASSERT(1 == X.numRegisteredObservers());
            ASSERT(1 == Y.numRegisteredObservers());

            //   X   Y
            //       |
            //       O2
            ASSERT(0 == mX.deregisterObserver(&mO1));
            ASSERT(0 == X.numRegisteredObservers());
            ASSERT(1 == Y.numRegisteredObservers());

            //   X   Y
            ASSERT(0 == mY.deregisterObserver(&mO2));
            ASSERT(0 == X.numRegisteredObservers());
            ASSERT(0 == Y.numRegisteredObservers());
        }
      } break;
      case 2: {
        // --------------------------------------------------------------------
        // TESTING CTOR AND DTOR
        //
        // Concerns:
        //   The basic concern is that the constructor and destructor operate
        //   as expected.  We have the following specific concerns:
        //     1. The constructor:
        //        a. Creates the correct initial state (empty).
        //        b. Has the internal memory management system hooked up
        //           properly so that *all* internally allocated memory draws
        //           from the same user-supplied allocator whenever one is
        //           specified.
        //     2. The destructor properly deallocates all allocated memory to
        //        its corresponding allocator.
        //
        // Plan:
        //   To address concerns 1a & 1b, create an object:
        //     - With and without passing in an allocator to the constructor.
        //     - Where the object is constructed entirely in static memory
        //       (using a 'bdema_BufferedSequentialAllocator') and never
        //       destroyed.
        //
        //   To address concern 2, allow each object to leave scope, so that
        //   the destructor asserts internal object invariants appropriately.
        //   Use a 'bslma_TestAllocator' to verify memory usage.
        //
        // Testing:
        //   bael_MultiplexObserver(bslma_Allocator *ba = 0);
        //   ~bael_MultiplexObserver();
        // --------------------------------------------------------------------

        if (verbose) cout << endl << "Testing 'ctor' and 'dtor'" << endl
                                  << "=========================" << endl;

        if (verbose) cout << "\tWithout passing in an allocator." << endl;
        {
            Obj mX;  const Obj& X = mX;
            ASSERT(0 == X.numRegisteredObservers());
        }
        {
            Obj mX;  const Obj& X = mX;
            ASSERT(0 == X.numRegisteredObservers());

            TestObs mO1(bsl::cout);
            TestObs mO2(bsl::cout);
            ASSERT(0 == mX.registerObserver(&mO1));
            ASSERT(0 == mX.registerObserver(&mO2));
            ASSERT(2 == X.numRegisteredObservers());
        }

        if (verbose) cout << "\tPassing in an allocator." << endl;
        const int NUM_BLOCKS_IN_USE = testAllocator.numBlocksInUse();
        const int NUM_BYTES_IN_USE  = testAllocator.numBytesInUse();
        {
            Obj mX(&testAllocator);  const Obj& X = mX;
            ASSERT(0 == X.numRegisteredObservers());
        }
        ASSERT(NUM_BLOCKS_IN_USE == testAllocator.numBlocksInUse());
        ASSERT(NUM_BYTES_IN_USE  == testAllocator.numBytesInUse());

        {
            Obj mX(&testAllocator);  const Obj& X = mX;
            ASSERT(0 == X.numRegisteredObservers());

            TestObs mO1(bsl::cout);
            TestObs mO2(bsl::cout);
            ASSERT(0 == mX.registerObserver(&mO1));
            ASSERT(0 == mX.registerObserver(&mO2));
            ASSERT(2 == X.numRegisteredObservers());

            // White-box test: 'publish' does not utilize the supplied
            // allocator.

            const int NUM_BLOCKS_TOTAL = testAllocator.numBlocksTotal();
            const int NUM_BYTES_TOTAL  = testAllocator.numBytesTotal();
            Rec  mR;  const Rec&  R = mR;
            Ctxt mC;  const Ctxt& C = mC;
            mX.publish(R, C);
            ASSERT(NUM_BLOCKS_TOTAL == testAllocator.numBlocksTotal());
            ASSERT(NUM_BYTES_TOTAL  == testAllocator.numBytesTotal());
        }
        ASSERT(NUM_BLOCKS_IN_USE == testAllocator.numBlocksInUse());
        ASSERT(NUM_BYTES_IN_USE  == testAllocator.numBytesInUse());

        if (verbose) cout << "\tIn place using a buffer allocator." << endl;
        {
            char memory[2048];
            bdema_BufferedSequentialAllocator a(memory, sizeof memory);
            Obj *doNotDelete = new(a.allocate(sizeof(Obj))) Obj(&a);
            Obj& mX = *doNotDelete;  const Obj& X = mX;
            ASSERT(0 == X.numRegisteredObservers());

            TestObs mO1(bsl::cout);
            TestObs mO2(bsl::cout);
            ASSERT(0 == mX.registerObserver(&mO1));
            ASSERT(0 == mX.registerObserver(&mO2));
            ASSERT(2 == X.numRegisteredObservers());

            // No destructor is called; will produce memory leak in purify
            // if internal allocators are not hooked up properly.
        }
      } break;
      case 1: {
        // --------------------------------------------------------------------
        // BREATHING TEST
        //
        // Concerns:
        //   We want to exercise basic functionality.  In particular, we want
        //   to demonstrate a base-line level of correct operation for all
        //   methods.
        //
        // Plan:
        //   1.  Construct an object x.
        //   2.  Register an observer o1.
        //   3.  Deregister observer o1.
        //   4.  Reregister observer o1.
        //   4a. Publish a record r1.
        //   5.  Register an observer o2.
        //   5a. Publish a record r2.
        //   6.  Deregister observer o1.
        //   6a. Publish a record r3.
        //   7.  Deregister observer o2.
        //   7a. Publish a record r4.
        //   8.  Destroy x.
        //
        // Testing:
        //   This "test" *exercises* basic functionality, but *tests* nothing.
        // --------------------------------------------------------------------

        if (verbose) cout << endl << "BREATHING TEST" << endl
                                  << "==============" << endl;
        {
            // ----------------------------------------------------------------

            if (verbose) cout << "\t1.  Construct an object x." << endl;

            Obj mX;  const Obj& X = mX;
            ASSERT(0 == X.numRegisteredObservers());

            // ----------------------------------------------------------------

            if (verbose) cout << "\t2.  Register an observer o1." << endl;

            TestObs mO1(bsl::cout);  const TestObs& O1 = mO1;
            ASSERT(0 == mX.registerObserver(&mO1));
            ASSERT(1 == X.numRegisteredObservers());

            // ----------------------------------------------------------------

            if (verbose) cout << "\t3.  Deregister observer o1." << endl;

            ASSERT(0 == mX.deregisterObserver(&mO1));
            ASSERT(0 == X.numRegisteredObservers());

            // ----------------------------------------------------------------

            if (verbose) cout << "\t4.  Reregister observer o1." << endl;

            ASSERT(0 == mX.registerObserver(&mO1));
            ASSERT(1 == X.numRegisteredObservers());
            ASSERT(0 == O1.numPublishedRecords());

            if (verbose) cout << "\t4a. Publish a record r1." << endl;

            Rec  mR1;  const Rec&  R1 = mR1;
            Ctxt mC1;  const Ctxt& C1 = mC1;

            mX.publish(R1, C1);
            ASSERT(1 == O1.numPublishedRecords());

            // ----------------------------------------------------------------

            if (verbose) cout << "\t5.  Register an observer o2." << endl;

            TestObs mO2(bsl::cout);  const TestObs& O2 = mO2;
            ASSERT(0 == mX.registerObserver(&mO2));
            ASSERT(2 == X.numRegisteredObservers());
            ASSERT(0 == O2.numPublishedRecords());

            if (verbose) cout << "\t5a. Publish a record r2." << endl;

            Rec  mR2;  const Rec&  R2 = mR2;
            Ctxt mC2;  const Ctxt& C2 = mC2;

            mX.publish(R2, C2);
            ASSERT(2 == O1.numPublishedRecords());
            ASSERT(1 == O2.numPublishedRecords());

            // ----------------------------------------------------------------

            if (verbose) cout << "\t6.  Deregister observer o1." << endl;

            ASSERT(0 == mX.deregisterObserver(&mO1));
            ASSERT(1 == X.numRegisteredObservers());

            if (verbose) cout << "\t6a. Publish a record r3." << endl;

            Rec  mR3;  const Rec&  R3 = mR3;
            Ctxt mC3;  const Ctxt& C3 = mC3;

            mX.publish(R3, C3);
            ASSERT(2 == O1.numPublishedRecords());
            ASSERT(2 == O2.numPublishedRecords());

            // ----------------------------------------------------------------

            if (verbose) cout << "\t7.  Deregister observer o2." << endl;

            ASSERT(0 == mX.deregisterObserver(&mO2));
            ASSERT(0 == X.numRegisteredObservers());

            if (verbose) cout << "\t7a. Publish a record r4." << endl;

            Rec  mR4;  const Rec&  R4 = mR4;
            Ctxt mC4;  const Ctxt& C4 = mC4;

            mX.publish(R4, C4);
            ASSERT(2 == O1.numPublishedRecords());
            ASSERT(2 == O2.numPublishedRecords());

            // ----------------------------------------------------------------

            if (verbose) cout << "\t8.  Destroy x." << endl;
        }
      } break;
      default: {
        cerr << "WARNING: CASE `" << test << "' NOT FOUND." << endl;
        testStatus = -1;
      }
    }

    if (testStatus > 0) {
        cerr << "Error, non-zero test status = " << testStatus << "." << endl;
    }
    return testStatus;
}

// ---------------------------------------------------------------------------
// NOTICE:
//      Copyright (C) Bloomberg L.P., 2004
//      All Rights Reserved.
//      Property of Bloomberg L.P. (BLP)
//      This software is made available solely pursuant to the
//      terms of a BLP license agreement which governs its use.
// ----------------------------- END-OF-FILE ---------------------------------
