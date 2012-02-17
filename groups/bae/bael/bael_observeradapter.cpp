// bael_observeradapter.cpp                                           -*-C++-*-
#include <bael_observeradapter.h>

#include <bdes_ident.h>
BDES_IDENT_RCSID(bael_observeradapter_cpp,"$Id$ $CSID$")

#ifdef BDE_FOR_TESTING_ONLY
#include <bael_context.h>              // for testing only
#include <bael_countingallocator.h>    // for testing only
#include <bael_record.h>               // for testing only
#include <bael_transmission.h>         // for testing only
#endif

namespace BloombergLP {

                        // --------------------------
                        // class bael_ObserverAdapter
                        // --------------------------

// CREATORS
bael_ObserverAdapter::~bael_ObserverAdapter()
{
}

// MANIPULATORS
void bael_ObserverAdapter::publish(const bael_Record&  record,
                                   const bael_Context& context) 
{
    BSLS_ASSERT_OPT(false);  // Should not be called
}

void bael_ObserverAdapter::publish(
                            const bcema_SharedPtr<const bael_Record>& record,
                            const bael_Context&                       context) 
{
    publish(*record, context);
}

void bael_ObserverAdapter::clear()
{
}

}  // close namespace BloombergLP

// ---------------------------------------------------------------------------
// NOTICE:
//      Copyright (C) Bloomberg L.P., 2012
//      All Rights Reserved.
//      Property of Bloomberg L.P. (BLP)
//      This software is made available solely pursuant to the
//      terms of a BLP license agreement which governs its use.
// ----------------------------- END-OF-FILE ---------------------------------
