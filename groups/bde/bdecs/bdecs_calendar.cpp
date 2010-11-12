// bdecs_calendar.cpp            -*-C++-*-
#include <bdecs_calendar.h>

#include <bdes_ident.h>
BDES_IDENT_RCSID(bdecs_calendar_cpp,"$Id$ $CSID$")

#include <bdea_bitarray.h>
#include <bdec_dayofweekset.h>
#include <bdet_date.h>

#include <bslma_default.h>
#include <bsls_assert.h>

#include <bsl_ostream.h>

namespace BloombergLP {

                            // --------------------
                            // class bdecs_Calendar
                            // --------------------

// PRIVATE MANIPULATORS
void bdecs_Calendar::synchronizeCache()
{
    d_nonBusinessDays.setLength(d_packedCalendar.length());
    d_nonBusinessDays.setAll0();
    for (bdecs_PackedCalendar::HolidayConstIterator it =
                                              d_packedCalendar.beginHolidays();
                                  it != d_packedCalendar.endHolidays(); ++it) {
        d_nonBusinessDays.set1(*it - d_packedCalendar.firstDate());
    }
    addWeekendDays(d_packedCalendar.weekendDays());
    BSLS_ASSERT(d_packedCalendar.numNonBusinessDays() ==
                                                  d_nonBusinessDays.numSet1());
}

// PRIVATE ACCESSORS
bool bdecs_Calendar::isCacheSynchronized() const
{
    for (bdecs_PackedCalendar::HolidayConstIterator it =
                                              d_packedCalendar.beginHolidays();
                                  it != d_packedCalendar.endHolidays(); ++it) {
        if (1 != d_nonBusinessDays[*it - d_packedCalendar.firstDate()]) {
            return false;
        }
    }
    return true;
}

// CREATORS
bdecs_Calendar::bdecs_Calendar(bslma_Allocator *basicAllocator)
: d_packedCalendar(basicAllocator)
, d_nonBusinessDays(basicAllocator)
, d_allocator_p(bslma_Default::allocator(basicAllocator))
{
}

bdecs_Calendar::bdecs_Calendar(const bdet_Date&  firstDate,
                               const bdet_Date&  lastDate,
                               bslma_Allocator  *basicAllocator)
: d_packedCalendar(firstDate, lastDate, basicAllocator)
, d_nonBusinessDays(basicAllocator)
, d_allocator_p(bslma_Default::allocator(basicAllocator))
{
    d_nonBusinessDays.setLength(d_packedCalendar.length(), 0);
}

bdecs_Calendar::bdecs_Calendar(const bdecs_PackedCalendar&  packedCalendar,
                               bslma_Allocator             *basicAllocator)
: d_packedCalendar(packedCalendar, basicAllocator)
, d_nonBusinessDays(basicAllocator)
, d_allocator_p(bslma_Default::allocator(basicAllocator))
{
    synchronizeCache();
}

bdecs_Calendar::bdecs_Calendar(const bdecs_Calendar&  original,
                               bslma_Allocator       *basicAllocator)
: d_packedCalendar(original.d_packedCalendar, basicAllocator)
, d_nonBusinessDays(original.d_nonBusinessDays, basicAllocator)
, d_allocator_p(bslma_Default::allocator(basicAllocator))
{
}

bdecs_Calendar::~bdecs_Calendar()
{
    BSLS_ASSERT(d_packedCalendar.length() == d_nonBusinessDays.length());
    BSLS_ASSERT(d_packedCalendar.numNonBusinessDays() ==
                                                  d_nonBusinessDays.numSet1());

    BSLS_ASSERT_SAFE(isCacheSynchronized());
}

// MANIPULATORS
bdecs_Calendar& bdecs_Calendar::operator=(const bdecs_Calendar& rhs)
{
    if (&rhs != this) {
        d_nonBusinessDays.reserveCapacity(rhs.d_nonBusinessDays.length());
        d_packedCalendar  = rhs.d_packedCalendar;
        d_nonBusinessDays = rhs.d_nonBusinessDays;
    }
    return *this;
}

bdecs_Calendar& bdecs_Calendar::operator=(const bdecs_PackedCalendar& rhs)
{
    // Make sure the calendar will be in a consistent state in case an
    // exception is thrown.

    d_nonBusinessDays.reserveCapacity(rhs.length());

    d_packedCalendar = rhs;
    synchronizeCache();

    return *this;
}

void bdecs_Calendar::addDay(const bdet_Date& date)
{
    bdet_Date firstDate;
    if (date < d_packedCalendar.firstDate()) {
        firstDate = date;
    }
    else {
        firstDate = d_packedCalendar.firstDate();
    }
    bdet_Date lastDate;
    if (date > d_packedCalendar.lastDate()) {
        lastDate = date;
    }
    else {
        lastDate = d_packedCalendar.lastDate();
    }

    if (firstDate != d_packedCalendar.firstDate()
     || lastDate  != d_packedCalendar.lastDate()) {
        setValidRange(firstDate, lastDate);
    }
}

void bdecs_Calendar::addHoliday(const bdet_Date& date)
{
    if (0 == d_packedCalendar.addHolidayIfInRange(date)) {
        d_nonBusinessDays.set1(date - d_packedCalendar.firstDate());
    }
}

void bdecs_Calendar::addHolidayCode(const bdet_Date& date, int holidayCode)
{
    if (0 == d_packedCalendar.addHolidayCodeIfInRange(date, holidayCode)) {
        d_nonBusinessDays.set1(date - d_packedCalendar.firstDate());
    }
}

void bdecs_Calendar::addWeekendDay(bdet_DayOfWeek::Day weekendDay)
{
    d_packedCalendar.addWeekendDay(weekendDay);
    const int length = d_nonBusinessDays.length();
    int weekendDayIndex =
     ((int)weekendDay - (int)d_packedCalendar.firstDate().dayOfWeek() + 7) % 7;
    while (weekendDayIndex < length) {
        d_nonBusinessDays.set1(weekendDayIndex);
        weekendDayIndex += 7;
    }
}

void bdecs_Calendar::addWeekendDays(const bdec_DayOfWeekSet& weekendDays)
{
    for (bdec_DayOfWeekSet::iterator it = weekendDays.begin();
         it != weekendDays.end(); ++it) {
        addWeekendDay(*it);
    }
}

void bdecs_Calendar::setValidRange(const bdet_Date& firstDate,
                                   const bdet_Date& lastDate)
{
    if (lastDate >= firstDate) {
        d_nonBusinessDays.reserveCapacity(lastDate - firstDate + 1);
    }
    d_packedCalendar.setValidRange(firstDate, lastDate);
    synchronizeCache();
}

void bdecs_Calendar::unionBusinessDays(const bdecs_PackedCalendar& other)
{
    int newLength = length();
    if (other.firstDate() < firstDate()) {
        newLength += firstDate() - other.firstDate();
    }
    if (other.lastDate() > lastDate()) {
        newLength += other.lastDate() - lastDate();
    }
    BSLS_ASSERT(newLength >= length());
    d_nonBusinessDays.reserveCapacity(newLength);
    d_packedCalendar.unionBusinessDays(other);
    synchronizeCache();
}

void bdecs_Calendar::unionNonBusinessDays(const bdecs_PackedCalendar& other)
{
    int newLength = length();
    if (other.firstDate() < firstDate()) {
        newLength += firstDate() - other.firstDate();
    }
    if (other.lastDate() > lastDate()) {
        newLength += other.lastDate() - lastDate();
    }
    BSLS_ASSERT(newLength >= length());
    d_nonBusinessDays.reserveCapacity(newLength);
    d_packedCalendar.unionNonBusinessDays(other);
    synchronizeCache();
}

void bdecs_Calendar::removeHoliday(const bdet_Date& date)
{
    d_packedCalendar.removeHoliday(date);
    if (true == isInRange(date) && false == isWeekendDay(date)) {
        d_nonBusinessDays.set0(date - firstDate());
    }
}

void bdecs_Calendar::swap(bdecs_Calendar *other)
{
    bdea_BitArray tmp(d_nonBusinessDays, d_allocator_p);
    d_nonBusinessDays.reserveCapacity(other->d_nonBusinessDays.length());
    other->d_nonBusinessDays.reserveCapacity(d_nonBusinessDays.length());

    d_nonBusinessDays = other->d_nonBusinessDays;
    other->d_nonBusinessDays = tmp;
    d_packedCalendar.swap(&other->d_packedCalendar);
}

// ACCESSORS
bdecs_Calendar::BusinessDayConstIterator
bdecs_Calendar::beginBusinessDays() const
{
    if (firstDate() <= lastDate()) {
        return bdecs_Calendar_BusinessDayConstIter(d_nonBusinessDays,
                                                   firstDate(),
                                                   firstDate(),
                                                   false);
    }
    else {
        return endBusinessDays();
    }
}

bdet_Date
bdecs_Calendar::getNextBusinessDay(const bdet_Date& initialDate) const
{
    BSLS_ASSERT(d_packedCalendar.weekendDays().length() != 7);

    bdet_Date calendarFirstDate = firstDate();
    bdet_Date currentDate = initialDate;
    ++currentDate;

    // For 'currentDate < calendarFirstDate', only weekend days are considered
    // holidays.  Note that the following loop will run 6 times in the worst
    // case because we know at this point that this calendar has at least one
    // day which is not a weekend day.

    while (currentDate < calendarFirstDate) {
        if (0 == isWeekendDay(currentDate)) {
            return currentDate;
        }
        ++currentDate;
    }

    // If 'currentDate' is in the range of the calendar, we just need to lookup
    // in this calendar's cache.

    if (isInRange(currentDate)) {
        int offset = d_nonBusinessDays.find0AtSmallestIndexGE(
                                              currentDate - calendarFirstDate);
        if (offset != -1) {
            // Found the next business day within the valid range of this
            // calendar.

            return calendarFirstDate + offset;
        }
        // No business day could be found in the range of the calendar.
        // Position 'currentDate' one day past the last valid date of this
        // calendar for the next lookup.

        currentDate = lastDate();
        ++currentDate;
    }

    // Starting from 'lastDate + 1', only weekend days are considered holidays.
    // Note that this loop will run 6 times in the worst case because we know
    // at this point that this calendar has at least one day which is not a
    // weekend day.

    while (isWeekendDay(currentDate)) {
        ++currentDate;
    }

    return currentDate;
}

bdet_Date
bdecs_Calendar::getNextBusinessDay(const bdet_Date& initialDate, int nth) const
{
    BSLS_ASSERT(nth > 1);
    BSLS_ASSERT(d_packedCalendar.weekendDays().length() != 7);

    bdet_Date calendarFirstDate = firstDate();
    bdet_Date currentDate = initialDate;

    ++currentDate;

    // Every time a new business day is found, 'nth' will be decremented by one
    // until it reaches 0, in which case we return the business day found.

    // For 'currentDate < calendarFirstDate', only weekend days are considered
    // holidays.  Note that the following loop will run '6 * nth' times in the
    // worst case because we know at this point that this calendar has at least
    // one day which is not a weekend day.

    while (currentDate < calendarFirstDate) {
        if (0 == isWeekendDay(currentDate)) {
            // Found a business day, so decrement 'nth'.

            --nth;
            if (0 == nth) {
                // Found the next 'nth' business day before the first date.

                return currentDate;
            }
        }
        ++currentDate;
    }
    int offset = currentDate - calendarFirstDate;

    // If 'currentDate' is in the range of the calendar, we just need to lookup
    // in this calendar's cache.

    while (isInRange(calendarFirstDate + offset)) {
        offset = d_nonBusinessDays.find0AtSmallestIndexGE(offset);
        if (offset != -1) {
            // Found the next business day within the valid range of this
            // calendar, so decrement 'nth'.

            --nth;
            if (0 == nth) {
                // Found the next 'nth' business day.

                return calendarFirstDate + offset;
            }
            ++offset;
        }
        else {
            // No more business day could be found in the range of the
            // calendar.  Position 'currentDate' one day past the last valid
            // date of this calendar for the next lookup.

            break;
        }
    }

    // Position 'currentDate' one day past the last valid date of this calendar
    // for the next lookup only if 'currentDate' was within the valid range.

    if (isInRange(currentDate)) {
        currentDate = lastDate();
        ++currentDate;
    }

    // Starting from 'lastDate + 1', only weekend days are considered holidays.
    // Note that this loop will run '6 * nth' times in the worst case because
    // we know at this point that this calendar has at least one day which is
    // not a weekend day.

    while (1) {
        if (!isWeekendDay(currentDate)) {
            // Found a business day, so decrement 'nth'.

            --nth;
            if (0 == nth) {
                // Found the next 'nth' business day after the last date.

                break;
            }
        }
        ++currentDate;
    }

    return currentDate;
}

                    // -----------------------------------------
                    // class bdecs_Calendar_BusinessDayConstIter
                    // -----------------------------------------

// PRIVATE CREATORS
bdecs_Calendar_BusinessDayConstIter::bdecs_Calendar_BusinessDayConstIter(
                                      const bdea_BitArray& nonBusinessDays,
                                      const bdet_Date&     firstDateOfCalendar,
                                      const bdet_Date&     startDate,
                                      bool                 endIterFlag)
: d_nonBusinessDays_p(&nonBusinessDays)
, d_firstDate(firstDateOfCalendar)
, d_currentOffset(startDate - firstDateOfCalendar)
{
    if (d_firstDate > startDate) {
        d_currentOffset = -1;
        return;
    }
    BSLS_ASSERT(d_currentOffset >= 0);

    if (endIterFlag) {
        // This constructor is called from the overloaded 'endBusinessDays'
        // method.  If 'startDate' is the last date in the valid range of the
        // calendar, mark this iterator as an 'end' iterator and return.
        // Otherwise, advance the iterator to reference the next date so we can
        // find the next business day.

        if (d_currentOffset == d_nonBusinessDays_p->length() - 1) {
            d_currentOffset = -1;
            return;
        }
        ++d_currentOffset;
    }

    d_currentOffset = d_nonBusinessDays_p->find0AtSmallestIndexGE(
                                                              d_currentOffset);
}

}  // close namespace BloombergLP

// ---------------------------------------------------------------------------
// NOTICE:
//      Copyright (C) Bloomberg L.P., 2005
//      All Rights Reserved.
//      Property of Bloomberg L.P. (BLP)
//      This software is made available solely pursuant to the
//      terms of a BLP license agreement which governs its use.
// ----------------------------- END-OF-FILE ---------------------------------
