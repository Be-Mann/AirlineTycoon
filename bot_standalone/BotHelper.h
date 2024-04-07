#ifndef BOT_HELPER_H_
#define BOT_HELPER_H_

#include <array>
#include <climits>
#include <iostream>
#include <optional>

#include "compat.h"
#include "compat_misc.h"

class PlaneTime {
  public:
    PlaneTime() = default;
    PlaneTime(int date, int time) : mDate(date), mTime(time) { normalize(); }

    int getDate() const { return mDate; }
    int getHour() const { return mTime; }
    int convertToHours() const { return 24 * mDate + mTime; }

    PlaneTime &operator+=(int delta) {
        mTime += delta;
        normalize();
        return *this;
    }

    PlaneTime &operator-=(int delta) {
        mTime -= delta;
        normalize();
        return *this;
    }
    PlaneTime operator+(int delta) const {
        PlaneTime t = *this;
        t += delta;
        return t;
    }
    PlaneTime operator-(int delta) const {
        PlaneTime t = *this;
        t -= delta;
        return t;
    }
    int operator-(const PlaneTime &time) const {
        PlaneTime t = *this - time.convertToHours();
        return t.convertToHours();
    }
    bool operator==(const PlaneTime &other) const { return (mDate == other.mDate && mTime == other.mTime); }
    bool operator!=(const PlaneTime &other) const { return (mDate != other.mDate || mTime != other.mTime); }
    bool operator>(const PlaneTime &other) const {
        if (mDate == other.mDate) {
            return (mTime > other.mTime);
        }
        return (mDate > other.mDate);
    }
    bool operator>=(const PlaneTime &other) const {
        if (mDate == other.mDate) {
            return (mTime >= other.mTime);
        }
        return (mDate > other.mDate);
    }
    bool operator<(const PlaneTime &other) const { return other > *this; }
    bool operator<=(const PlaneTime &other) const { return other >= *this; }
    void setDate(int date) {
        mDate = date;
        mTime = 0;
    }

  private:
    void normalize() {
        while (mTime >= 24) {
            mTime -= 24;
            mDate++;
        }
        while (mTime < 0) {
            mTime += 24;
            mDate--;
        }
    }
    int mDate{0};
    int mTime{0};
};

namespace Helper {

CString getWeekday(UWORD date);
CString getWeekday(const PlaneTime &time);

struct FreightInfo {
    std::vector<CString> planeNames;
    std::vector<SLONG> tonsPerPlane;
    SLONG tonsOpen{0};
    SLONG smallestDecrement{INT_MAX};
};

struct ScheduleInfo {
    SLONG jobs{0};
    SLONG freightJobs{0};
    SLONG gain{0};
    SLONG passengers{0};
    SLONG tons{0};
    SLONG miles{0};
    SLONG uhrigFlights{0};
    SLONG hoursFlights{0};
    SLONG hoursAutoFlights{0};
    SLONG keroseneFlights{0};
    SLONG keroseneAutoFlights{0};
    SLONG numPlanes{0};
    PlaneTime scheduleStart{INT_MAX, 0};
    PlaneTime scheduleEnd{0, 0};
    /* for job statistics */
    std::array<SLONG, 5> jobTypes{};
    std::array<SLONG, 6> jobSizeTypes{};

    DOUBLE getRatioFlights() const { return 100.0 * hoursFlights / ((scheduleEnd - scheduleStart) * numPlanes); }
    DOUBLE getRatioAutoFlights() const { return 100.0 * hoursAutoFlights / ((scheduleEnd - scheduleStart) * numPlanes); }
    DOUBLE getKeroseneRatio() const { return 100.0 * keroseneFlights / (keroseneFlights + keroseneAutoFlights); }

    ScheduleInfo &operator+=(ScheduleInfo delta) {
        jobs += delta.jobs;
        freightJobs += delta.freightJobs;
        gain += delta.gain;
        passengers += delta.passengers;
        tons += delta.tons;
        miles += delta.miles;
        uhrigFlights += delta.uhrigFlights;
        hoursFlights += delta.hoursFlights;
        hoursAutoFlights += delta.hoursAutoFlights;
        keroseneFlights += delta.keroseneFlights;
        keroseneAutoFlights += delta.keroseneAutoFlights;
        numPlanes += delta.numPlanes;
        scheduleStart = std::min(scheduleStart, delta.scheduleStart);
        scheduleEnd = std::max(scheduleEnd, delta.scheduleEnd);
        /* for job statistics */
        for (SLONG i = 0; i < jobTypes.size(); i++) {
            jobTypes[i] += delta.jobTypes[i];
        }
        for (SLONG i = 0; i < jobSizeTypes.size(); i++) {
            jobSizeTypes[i] += delta.jobSizeTypes[i];
        }
        return *this;
    }

    void printGain() {
        hprintf("Schedule (%s %d - %s %d) with %ld planes gains %s $.", (LPCTSTR)getWeekday(scheduleStart), scheduleStart.getHour(),
                (LPCTSTR)getWeekday(scheduleEnd), scheduleEnd.getHour(), numPlanes, Insert1000erDots(gain).c_str());
    }
    void printDetails() {
        hprintf("====================");
        printGain();

        if (uhrigFlights > 0) {
            hprintf("Flying %ld jobs (%ld from Uhrig, %ld passengers), %ld freight jobs (%ld tons) and %ld miles.", jobs, uhrigFlights, passengers, freightJobs,
                    tons, miles);
        } else {
            hprintf("Flying %ld jobs (%ld passengers), %ld freight jobs (%ld tons) and %ld miles.", jobs, passengers, freightJobs, tons, miles);
        }
        hprintf("%.1f %% of plane schedule are regular flights, %.1f %% are automatic flights (%.1f %% useful kerosene).", getRatioFlights(),
                getRatioAutoFlights(), getKeroseneRatio());

        std::array<CString, 5> typeStr{"normal", "later", "highriskreward", "scam", "no fine"};
        std::array<CString, 6> sizeStr{"VIP", "S", "M", "L", "XL", "XXL"};
        printf("Job types: ");
        for (SLONG i = 0; i < jobTypes.size(); i++) {
            printf("%.0f %% %s", 100.0 * jobTypes[i] / (jobs + freightJobs), (LPCTSTR)typeStr[i]);
            if (i < jobTypes.size() - 1) {
                printf(", ");
            }
        }
        printf("\nJob sizes: ");
        for (SLONG i = 0; i < jobSizeTypes.size(); i++) {
            printf("%.0f %% %s", 100.0 * jobSizeTypes[i] / (jobs + freightJobs), (LPCTSTR)sizeStr[i]);
            if (i < jobSizeTypes.size() - 1) {
                printf(", ");
            }
        }
        printf("\n");
        hprintf("====================");
    }
};

void printJob(const CAuftrag &qAuftrag);
void printRoute(const CRoute &qRoute);
void printFreight(const CFracht &qAuftrag);

std::string getRouteName(const CRoute &qRoute);
std::string getJobName(const CAuftrag &qAuftrag);
std::string getFreightName(const CFracht &qAuftrag);

void printFPE(const CFlugplanEintrag &qFPE);

const CFlugplanEintrag *getLastFlight(const CPlane &qPlane);
const CFlugplanEintrag *getLastFlightNotAfter(const CPlane &qPlane, PlaneTime ignoreFrom);
std::pair<PlaneTime, int> getPlaneAvailableTimeLoc(const CPlane &qPlane, std::optional<PlaneTime> ignoreFrom, std::optional<PlaneTime> earliest);

SLONG checkPlaneSchedule(const PLAYER &qPlayer, SLONG planeId, bool alwaysPrint);
SLONG checkPlaneSchedule(const PLAYER &qPlayer, const CPlane &qPlane, bool alwaysPrint);
SLONG _checkPlaneSchedule(const PLAYER &qPlayer, const CPlane &qPlane, std::unordered_map<SLONG, CString> &assignedJobs,
                          std::unordered_map<SLONG, FreightInfo> &freightTons, bool alwaysPrint);
SLONG checkFlightJobs(const PLAYER &qPlayer, bool alwaysPrint, bool verboseInfo);
void printFlightJobs(const PLAYER &qPlayer, SLONG planeId);
void printFlightJobs(const PLAYER &qPlayer, const CPlane &qPlane);

ScheduleInfo calculateScheduleInfo(const PLAYER &qPlayer, SLONG planeId);

void printAllSchedules(bool infoOnly);

bool checkRoomOpen(SLONG roomId);
SLONG getRoomFromAction(SLONG PlayerNum, SLONG actionId);
SLONG getWalkDistance(int playerNum, SLONG roomId);

const char *getItemName(SLONG item);

} // namespace Helper

#endif // BOT_HELPER_H_
