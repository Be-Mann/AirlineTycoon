#ifndef BOT_H_
#define BOT_H_

#include <deque>
#include <unordered_map>
#include <vector>

class CPlaneType;
class CRoute;
class CRentRoute;
class PLAYER;

typedef long long __int64;

extern const SLONG kMoneyEmergencyFund;
extern const SLONG kSmallestAdCampaign;
extern const SLONG kMaximumRouteUtilization;

class Bot {
  public:
    Bot(PLAYER &player);

    void RobotInit();
    void RobotPlan();
    void RobotExecuteAction();

    // private:
    enum class Prio { None, Low, Medium, High, Top };
    struct RouteInfo {
        RouteInfo() = default;
        RouteInfo(SLONG id, SLONG id2, SLONG typeId) : routeId(id), routeReverseId(id2), planeTypeId(typeId) {}
        SLONG routeId{-1};
        SLONG routeReverseId{-1};
        SLONG planeTypeId{-1};
        SLONG utilization{};
        SLONG image{};
        std::vector<SLONG> planeIds;
    };

    /* in BotConditions.cpp */
    SLONG getMoneyAvailable() const;
    bool haveDiscount() const;
    bool hoursPassed(SLONG room, SLONG hours);
    Prio condAll(SLONG actionId);
    Prio condStartDay();
    Prio condBuero();
    Prio condCallInternational();
    Prio condCheckLastMinute();
    Prio condCheckTravelAgency();
    Prio condCheckFreight();
    Prio condUpgradePlanes(__int64 &moneyAvailable);
    Prio condBuyNewPlane(__int64 &moneyAvailable);
    Prio condVisitHR();
    Prio condBuyKerosine(__int64 &moneyAvailable);
    Prio condBuyKerosineTank(__int64 &moneyAvailable);
    Prio condSabotage(__int64 &moneyAvailable);
    Prio condIncreaseDividend(__int64 &moneyAvailable);
    Prio condRaiseMoney(__int64 &moneyAvailable);
    Prio condDropMoney(__int64 &moneyAvailable);
    Prio condEmitShares();
    Prio condSellShares(__int64 &moneyAvailable);
    Prio condBuyShares(__int64 &moneyAvailable, SLONG dislike);
    Prio condBuyOwnShares(__int64 &moneyAvailable);
    Prio condBuyNemesisShares(__int64 &moneyAvailable, SLONG dislike);
    Prio condVisitMech(__int64 &moneyAvailable);
    Prio condVisitNasa(__int64 &moneyAvailable);
    Prio condVisitMisc();
    Prio condBuyUsedPlane(__int64 &moneyAvailable);
    Prio condVisitDutyFree(__int64 &moneyAvailable);
    Prio condVisitBoss(__int64 &moneyAvailable);
    Prio condVisitRouteBoxPlanning();
    Prio condVisitRouteBoxRenting(__int64 &moneyAvailable);
    Prio condVisitSecurity(__int64 &moneyAvailable);
    Prio condSabotageSecurity();
    Prio condVisitDesigner(__int64 &moneyAvailable);
    Prio condBuyAdsForRoutes(__int64 &moneyAvailable);
    Prio condBuyAds(__int64 &moneyAvailable);

    /* in BotActions.cpp */
    void actionStartDay(__int64 moneyAvailable);
    void actionBuero();
    void actionUpgradePlanes(__int64 moneyAvailable);
    void actionVisitHR();
    std::pair<SLONG, SLONG> kerosineQualiOptimization(__int64 moneyAvailable, DOUBLE targetFillRatio) const;
    void actionBuyKerosine(__int64 moneyAvailable);
    void actionBuyKerosineTank(__int64 moneyAvailable);
    SLONG calcNumberOfShares(__int64 moneyAvailable, DOUBLE kurs) const;
    SLONG calcNumOfFreeShares(SLONG playerId) const;
    void actionEmitShares();
    void actionBuyShares(__int64 moneyAvailable);
    void actionVisitBoss(__int64 moneyAvailable);
    void checkLostRoutes();
    void updateRouteInfo();
    std::pair<SLONG, SLONG> actionFindBestRoute(TEAKRAND &rnd) const;
    void actionRentRoute(SLONG routeA, SLONG planeTypeId);
    void actionPlanRoutes();
    void actionWerbungRoutes(__int64 moneyAvailable);
    void actionWerbung(__int64 moneyAvailable);

    SLONG numPlanes() const { return mPlanesForJobs.size() + mPlanesForRoutes.size() + mPlanesForRoutesUnassigned.size(); }
    std::vector<SLONG> findBestAvailablePlaneType() const;
    SLONG calcCurrentGainFromJobs() const;
    void printGainFromJobs(SLONG oldGain) const;
    const CRentRoute &getRentRoute(const RouteInfo &routeInfo) const;
    const CRoute &getRoute(const RouteInfo &routeInfo) const;

    TEAKRAND LocalRandom;
    PLAYER &qPlayer;

    /* to not keep doing the same thing */
    std::unordered_map<SLONG, SLONG> mLastTimeInRoom;

    /* planes used for what? */
    std::vector<SLONG> mPlanesForJobs;
    std::vector<SLONG> mPlanesForRoutes;
    std::deque<SLONG> mPlanesForRoutesUnassigned;

    /* strategy state */
    SLONG mDislike{-1};
    SLONG mBestPlaneTypeId{-1};
    SLONG mBuyPlaneForRouteId{-1};
    SLONG mWantToRentRouteId{-1};
    bool mFirstRun{true};
    bool mDoRoutes{false};
    bool mOutOfGates{false};
    bool mNeedToDoPlanning{false};

    /* visit boss office */
    SLONG mBossNumCitiesAvailable{-1};
    bool mBossGateAvailable{false};

    /* detect tanks being too small */
    DOUBLE mTanksFilledYesterday{0.0};
    DOUBLE mTanksFilledToday{0.0};
    bool mTankWasEmpty{false};

    /* routes */
    std::vector<RouteInfo> mRoutes;
    std::vector<SLONG> mRoutesSortedByUtilization;
    std::vector<SLONG> mRoutesSortedByImage;
};

TEAKFILE &operator<<(TEAKFILE &File, const Bot &bot);
TEAKFILE &operator>>(TEAKFILE &File, Bot &bot);

#endif // BOT_H_