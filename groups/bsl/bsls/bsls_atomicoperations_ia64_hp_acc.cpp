// bsls_atomicoperations_ia64_hp_acc.cpp                              -*-C++-*-

#include <bsls_atomicoperations_ia64_hp_acc.h>

#include <bsls_ident.h>
BSLS_IDENT("$Id$ $CSID$")

#if defined(BSLS_PLATFORM_CPU_IA64) && defined(BSLS_PLATFORM_OS_HPUX)

namespace BloombergLP {
namespace bsls {

// IMPLEMENTATION NOTES: due to a possible bug in HP compiler (version B3910B
// A.06.25.02) _Asm_ld_volatile operations are reordered above _Asm_sched_fence
// compiler fences.  The same is possible for _Asm_st_volatile.  Load and store
// operations are made out-of-line to prevent this from happening and provide
// the required memory consistency guarantees.

                     // -----------------------------------
                     // struct AtomicOperations_IA64_HP_ACC
                     // -----------------------------------

int AtomicOperations_IA64_HP_ACC::
    getInt(const AtomicTypes::Int *atomicInt)
{
    _Asm_sched_fence(d_fullfence);
    return int(_Asm_ld_volatile(_SZ_W, _LDHINT_NONE, &atomicInt->d_value));
}

int AtomicOperations_IA64_HP_ACC::
    getIntAcquire(const AtomicTypes::Int *atomicInt)
{
    _Asm_sched_fence(d_downfence);
    return int(_Asm_ld_volatile(_SZ_W, _LDHINT_NONE, &atomicInt->d_value));
}

void AtomicOperations_IA64_HP_ACC::
    setInt(AtomicTypes::Int *atomicInt, int value)
{
    _Asm_sched_fence(d_fullfence);
    _Asm_mf();
    _Asm_st_volatile(_SZ_W,
                     _STHINT_NONE,
                     &atomicInt->d_value,
                     (unsigned) value);
}

void AtomicOperations_IA64_HP_ACC::
    setIntRelease(AtomicTypes::Int *atomicInt, int value)
{
    _Asm_sched_fence(d_upfence);
    _Asm_st_volatile(_SZ_W,
                     _STHINT_NONE,
                     &atomicInt->d_value,
                     (unsigned) value);
}

Types::Int64 AtomicOperations_IA64_HP_ACC::
    getInt64(const AtomicTypes::Int64 *atomicInt)
{
    _Asm_sched_fence(d_fullfence);
    return _Asm_ld_volatile(_SZ_D, _LDHINT_NONE, &atomicInt->d_value);
}

Types::Int64 AtomicOperations_IA64_HP_ACC::
    getInt64Acquire(const AtomicTypes::Int64 *atomicInt)
{
    _Asm_sched_fence(d_downfence);
    return _Asm_ld_volatile(_SZ_D, _LDHINT_NONE, &atomicInt->d_value);
}

void AtomicOperations_IA64_HP_ACC::
    setInt64(AtomicTypes::Int64 *atomicInt, Types::Int64 value)
{
    _Asm_sched_fence(d_fullfence);
    _Asm_mf();
    _Asm_st_volatile(_SZ_D,
                     _STHINT_NONE,
                     &atomicInt->d_value,
                     value);
}

void AtomicOperations_IA64_HP_ACC::
    setInt64Release(AtomicTypes::Int64 *atomicInt,
                    Types::Int64 value)
{
    _Asm_sched_fence(d_upfence);
    _Asm_st_volatile(_SZ_D,
                     _STHINT_NONE,
                     &atomicInt->d_value,
                     value);
}

}  // close namespace bsls
}  // close namespace BloombergLP

#endif

// ---------------------------------------------------------------------------
// NOTICE:
//      Copyright (C) Bloomberg L.P., 2011
//      All Rights Reserved.
//      Property of Bloomberg L.P. (BLP)
//      This software is made available solely pursuant to the
//      terms of a BLP license agreement which governs its use.
// ----------------------------- END-OF-FILE ---------------------------------
