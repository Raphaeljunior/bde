// bslmf_issame.h                                                     -*-C++-*-
#ifndef INCLUDED_BSLMF_ISSAME
#define INCLUDED_BSLMF_ISSAME

#ifndef INCLUDED_BSLS_IDENT
#include <bsls_ident.h>
#endif
BSLS_IDENT("$Id: $")

//@PURPOSE: Provide an meta-function for checking if two types are the same.
//
//@CLASSES:
//  bslmf::IsSame: meta-function evaluating whether two types are the same
//
//@AUTHOR: Shawn Edwards (sedwards)
//
//@DESCRIPTION: This component defines a simple template structure used to
// evaluate whether it's two parameter have the same type.  'bslmf::IsSame'
// defines a member, 'value', whose value is initialized (at compile-time) to 1
// if the two parameters are the same type, and 0 if they are different types.
//
///Usage
///-----
// For example:
//..
//  typedef int    INT;
//  typedef double DOUBLE;
//
//  const int I = bslmf::IsSame<INT, INT>::value;            assert(1 == I);
//  const int J = bslmf::IsSame<INT, DOUBLE>::value;         assert(0 == J);
//..
// Note that a 'const'-qualified type is considered distinct from the
// non-'const' (but otherwise identical) type:
//..
//  typedef       short       SHORT;
//  typedef const short CONST_SHORT;
//  const int K = bslmf::IsSame<SHORT, CONST_SHORT>::value;  assert(0 == K);
//..
// Similarly, a 'TYPE' and a reference to 'TYPE' ('TYPE&') are distinct:
//..
//  typedef int  INT;
//  typedef int& INT_REF;
//  const int L = bslmf::IsSame<INT, INT_REF>::value;        assert(0 == L);
//..

#ifndef INCLUDED_BSLSCM_VERSION
#include <bslscm_version.h>
#endif

#ifndef INCLUDED_BSLMF_METAINT
#include <bslmf_metaint.h>
#endif

#ifndef INCLUDED_BSLMF_INTEGRALCONSTANT
#include <bslmf_integralconstant.h>
#endif

namespace bsl {

template <typename TYPE1, typename TYPE2>
struct is_same : false_type
{};

template <typename TYPE>
struct is_same<TYPE, TYPE> : true_type
{};

}

namespace BloombergLP {

namespace bslmf {

                            // =============
                            // struct IsSame
                            // =============

template <typename U, typename V>
struct IsSame : bsl::is_same<U, V>::type
{
    // Meta function with 'value == 0' unless 'U' and 'V' are the same type.
};

}  // close package namespace

#ifndef BDE_OMIT_TRANSITIONAL  // BACKWARD_COMPATIBILITY
// ===========================================================================
//                           BACKWARD COMPATIBILITY
// ===========================================================================

#ifdef bslmf_IsSame
#undef bslmf_IsSame
#endif
#define bslmf_IsSame bslmf::IsSame
    // This alias is defined for backward compatibility.
#endif  // BDE_OMIT_TRANSITIONAL -- BACKWARD_COMPATIBILITY

}  // close enterprise namespace

#endif

// ---------------------------------------------------------------------------
// NOTICE:
//      Copyright (C) Bloomberg L.P., 2002
//      All Rights Reserved.
//      Property of Bloomberg L.P. (BLP)
//      This software is made available solely pursuant to the
//      terms of a BLP license agreement which governs its use.
// ----------------------------- END-OF-FILE ---------------------------------
