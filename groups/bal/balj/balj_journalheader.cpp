// balj_journalheader.cpp  -*-C++-*-
#include <balj_journalheader.h>

#include <ball_log.h>
#include <bdlt_epochutil.h>
#include <bdlt_currenttime.h>
#include <bsls_timeinterval.h>
#include <bsls_assert.h>
#include <bsls_types.h>

const char LOG_CATEGORY[] = "BAECS.JOURNALHEADER";

namespace BloombergLP {

namespace {

inline
bsls::Types::Int64 nowAsInt64GMT()
{
    bsls::TimeInterval now = bdlt::CurrentTime::now();
    return now.seconds() * 1000000000 + now.nanoseconds();
}

inline
bdlt::Datetime int64ToDatetime(bsls::Types::Int64 ns)
{
    bdlt::Datetime result = bdlt::EpochUtil::epoch();
    result.addMilliseconds(ns / 1000000);
    return result;
}

}

void balj::JournalHeader::HeaderState::copyState(
                                  balj::JournalHeader::HeaderState *dst,
                                  const balj::JournalHeader::HeaderState& src)
{
    dst->d_numPages           = src.d_numPages;
    dst->d_modificationTime   = src.d_modificationTime;
    dst->d_confirmedRecords   = src.d_confirmedRecords;
    dst->d_unconfirmedRecords = src.d_unconfirmedRecords;
    dst->d_fillPages          = src.d_fillPages;
    dst->d_preFillPages       = src.d_preFillPages;
}

namespace balj {
JournalHeader::JournalHeader()
: d_currentTransactionId(0)
, d_committedTransactionId(0)
, d_activeStateIndex(0)
, d_persistent_p(NULL)
{

}

JournalHeader::~JournalHeader()
{
    d_persistent_p = (OnDisk*)NULL;
}

void JournalHeader::init(int headerSize,
                               int pageHeaderSize,
                               int pageDataSize,
                               int alignment,
                               int userDataSize,
                               const JournalParameters&  param)
{
    BSLS_ASSERT(d_persistent_p);

    d_persistent_p->d_magic               = k_JOURNAL_MAGIC_NUMBER;
    d_persistent_p->d_version             = k_JOURNAL_VERSION_CURRENT;
    d_persistent_p->d_headerSize          = headerSize;
    d_persistent_p->d_blockSize           = param.blockSize();
    d_persistent_p->d_freeBlockThreshold  = param.freeBlockThreshold();
    d_persistent_p->d_blocksPerPage       = bsl::min(param.blocksPerPage(),
            (int)JournalPageHeader::k_MAX_BLOCKS_PER_PAGE);
    d_persistent_p->d_pagesPerSet         = param.pagesPerSet();
    d_persistent_p->d_pageHeaderSize      = pageHeaderSize;
    d_persistent_p->d_pageDataSize        = pageDataSize;
    d_persistent_p->d_alignment           = alignment;
    d_persistent_p->d_creationTime        = nowAsInt64GMT(); // Break in two?
    d_persistent_p->d_userDataSize        = userDataSize;
    d_activeStateIndex    = 0;

    for (int wi=0; wi < OnDisk::k_NUM_STATES; wi++) {
        HeaderState *state = &d_persistent_p->d_state[wi];
        state->d_committedTransactionId.setValue(wi);
        state->d_numPages = 0;
        state->d_modificationTime = d_persistent_p->d_creationTime;
        state->d_confirmedRecords.init();
        state->d_unconfirmedRecords.init();
        state->d_fillPages.init();
        state->d_preFillPages.init();
    }
    d_currentTransactionId = 1001;
    d_committedTransactionId = 0;
}

void JournalHeader::commitCurrentTransaction()
{
    BALL_LOG_SET_CATEGORY(LOG_CATEGORY);
    BALL_LOG_TRACE << "Committing current transaction "
        << d_currentTransactionId
        << " at index " << d_activeStateIndex
        << BALL_LOG_END;

    d_persistent_p->d_state[d_activeStateIndex].d_modificationTime
                                                             = nowAsInt64GMT();
    d_persistent_p->d_state[
        d_activeStateIndex
    ].d_committedTransactionId.setValue(d_currentTransactionId);
}

int JournalHeader::recoverTransaction(bool offset)
{
    BSLS_ASSERT(d_persistent_p);
    bsls::Types::Int64 tid[OnDisk::k_NUM_STATES];

    if (d_persistent_p->d_state[0].d_committedTransactionId.getValue(tid+0)) {
        return 1;
    }
    if (d_persistent_p->d_state[1].d_committedTransactionId.getValue(tid+1)) {
        return 1;
    }
    int index = 0;
    if (tid[1] > tid[0]) {
        index = 1;
    }
    d_activeStateIndex = index;
    d_committedTransactionId = tid[index];

    if (offset)
    {
        d_currentTransactionId = d_committedTransactionId + 1000;
                                                   // sufficiently large number
                                                   // for ease of debugging
    } else
    {
        d_currentTransactionId = d_committedTransactionId;
    }
    return 0;
}

bsl::ostream& operator<<(bsl::ostream& stream,
                         const balj::JournalHeaderPageList& pList)
{
    stream << "(" << pList.d_numElements << ": " << pList.d_firstPage << ", "
           << pList.d_last << ")";
    return stream;
}

bsl::ostream& operator<<(bsl::ostream& stream,
                         const balj::JournalHeaderRecordList& pList)
{
    stream << "{" << pList.d_numElements << ": " << pList.d_first << ", "
           << pList.d_last << "}";
    return stream;
}

bsl::ostream& operator<<(bsl::ostream& stream,
                         const balj::JournalHeader::HeaderState& state)
{
#define PRINTOUT(X, S) stream << #X " = " << (X) << (S);
    stream << " [";
    PRINTOUT(state.d_committedTransactionId, ',');
    PRINTOUT(state.d_numPages, ", ");
    PRINTOUT(state.d_modificationTime, '(');
    PRINTOUT(int64ToDatetime(state.d_modificationTime), "), " );
    PRINTOUT(state.d_confirmedRecords, ", ");
    PRINTOUT(state.d_unconfirmedRecords, ", ");
    PRINTOUT(state.d_fillPages, ", ");
    PRINTOUT(state.d_preFillPages, " ]");
    return stream;
#undef PRINTOUT
}

}  // close package namespace

namespace balj {
bsl::ostream& JournalHeader::print(bsl::ostream& stream) const
{
    if (!d_persistent_p)
    {
        stream << "[ unmapped ]" << bsl::endl;
        return stream;
    }
    BSLS_ASSERT(d_persistent_p);
#define PRINTOUT(X, S) stream << #X " = " << (X) << (S);
    stream << " [";
    PRINTOUT(d_persistent_p->d_magic, ", ");
    PRINTOUT(d_persistent_p->d_version, ", ");
    PRINTOUT(d_persistent_p->d_headerSize, ", ");
    PRINTOUT(d_persistent_p->d_pagesPerSet, ", ");
    PRINTOUT(d_persistent_p->d_blocksPerPage, ", ");
    PRINTOUT(d_persistent_p->d_blockSize, ", ");
    PRINTOUT(d_persistent_p->d_freeBlockThreshold, ", ");
    PRINTOUT(d_persistent_p->d_pageHeaderSize, ", ");
    PRINTOUT(d_persistent_p->d_pageDataSize, ", ");
    PRINTOUT(d_persistent_p->d_alignment, ", ");
    PRINTOUT(int64ToDatetime(d_persistent_p->d_creationTime), ", ");
    PRINTOUT(d_persistent_p->d_userDataSize, ", ");
    PRINTOUT(d_persistent_p->d_state[0], ", ");
    PRINTOUT(d_persistent_p->d_state[1], "] ");
    return stream;
#undef PRINTOUT
}

unsigned JournalHeaderPageList::addPage(unsigned page)
{
    unsigned last = d_last;
    if (d_last == k_INVALID_PAGE) {
        BSLS_ASSERT(d_firstPage == k_INVALID_PAGE);
        BSLS_ASSERT(d_numElements == 0);
        d_firstPage = page;
        d_last  = page;
    } else {
        BSLS_ASSERT(d_firstPage != k_INVALID_PAGE);
        BSLS_ASSERT(d_numElements > 0);
        if (d_numElements == 1) {
            BSLS_ASSERT(d_firstPage == d_last);
        }
        BSLS_ASSERT(d_last != page);
        d_last = page;
    }
    d_numElements = d_numElements + 1;
    return last;
}
}  // close package namespace

}  // close namespace BloombergLP

// ---------------------------------------------------------------------------
// NOTICE:
//      Copyright (C) Bloomberg L.P., 2008
//      All Rights Reserved.
//      Property of Bloomberg L.P. (BLP)
//      This software is made available solely pursuant to the
//      terms of a BLP license agreement which governs its use.
// ----------------------------- END-OF-FILE ---------------------------------
