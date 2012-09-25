// strstream.h                                                        -*-C++-*-

//@PURPOSE: Provide backwards compatibility for STLPort header
//
//@SEE_ALSO: package bsl+stdhdrs
//
//@AUTHOR: Arthur Chiu (achiu21)
//
//@DESCRIPTION: This component provides backwards compatibility for Bloomberg
// libraries that depend on non-standard STLPort header files.  This header
// includes the corresponding C++ header and provides any additional symbols
// that the STLPort header would define via transitive includes.

#include <bsl_strstream.h>

#ifndef BDE_OMIT_INTERNAL_DEPRECATED
    // Permit reliance on transitive includes within robo.
#include <iostream.h>
#endif // BDE_OMIT_INTERNAL_DEPRECATED

using std::strstreambuf;
using std::istrstream;
using std::ostrstream;
using std::strstream;

// ---------------------------------------------------------------------------
// NOTICE:
//      Copyright (C) Bloomberg L.P., 2009
//      All Rights Reserved.
//      Property of Bloomberg L.P. (BLP)
//      This software is made available solely pursuant to the
//      terms of a BLP license agreement which governs its use.
// ----------------------------- END-OF-FILE ---------------------------------
