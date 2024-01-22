extern SLONG TankSize[4];
extern SLONG TankPrice[4];

extern SLONG SabotagePrice[5];
extern SLONG SabotagePrice2[4];
extern SLONG SabotagePrice3[6];

extern SLONG RocketPrices[10];

extern SLONG StationPrices[10];

class GameMechanic {
  public:
    static void bankruptPlayer(PLAYER &qPlayer);

    /* Kerosin */
    static bool buyKerosinTank(PLAYER &qPlayer, SLONG type, SLONG amount);
    static bool toggleKerosinTankOpen(PLAYER &qPlayer);
    static bool setKerosinTankOpen(PLAYER &qPlayer, BOOL open);
    static bool buyKerosin(PLAYER &qPlayer, SLONG type, SLONG amount);
    struct KerosinTransaction {
        __int64 Kosten{};
        __int64 Rabatt{};
        __int64 Menge{};
    };
    static KerosinTransaction calcKerosinPrice(PLAYER &qPlayer, __int64 typ, __int64 menge);

    /* Sabotage */
    static SLONG setSaboteurTarget(PLAYER &qPlayer, SLONG target);
    static bool checkSaboteurBusy(PLAYER &qPlayer) { return (qPlayer.ArabMode != 0) || (qPlayer.ArabMode2 != 0) || (qPlayer.ArabMode3 != 0); }
    enum class CheckSabotageResult { Ok, DeniedInvalidParam, DeniedSecurity, DeniedNotEnoughMoney, DeniedNoLaptop };
    struct CheckSabotage {
        CheckSabotageResult result{CheckSabotageResult::DeniedInvalidParam};
        int dialogID{};
        CString dialogParam{};
    };
    static CheckSabotage checkPrerequisitesForSaboteurJob(PLAYER &qPlayer, SLONG type, SLONG number);
    static bool activateSaboteurJob(PLAYER &qPlayer);
    static void paySaboteurFine(SLONG player, SLONG opfer);

    /* Kredite */
    static bool takeOutCredit(PLAYER &qPlayer, __int64 amount);
    static bool payBackCredit(PLAYER &qPlayer, __int64 amount);

    /* Flugzeuge */
    static void setPlaneTargetZustand(PLAYER &qPlayer, SLONG idx, SLONG zustand);

    /* Security */
    static bool setSecurity(PLAYER &qPlayer, SLONG securityType, bool targetState);
    static bool toggleSecurity(PLAYER &qPlayer, SLONG securityType);
    static bool sabotageSecurityOffice(PLAYER &qPlayer);

    /* Makler */
    static bool checkPlaneTypeAvailable(SLONG planeType);
    static std::vector<SLONG> buyPlane(PLAYER &qPlayer, SLONG planeType, SLONG amount);

    /* Museum */
    static bool buyUsedPlane(PLAYER &qPlayer, SLONG planeID);
    static bool sellPlane(PLAYER &qPlayer, SLONG planeID);

    /* Designer */
    static bool buyXPlane(PLAYER &qPlayer, const CString &filename, SLONG amount);

    /* Bank */
    static bool buyStock(PLAYER &qPlayer, SLONG airlineNum, SLONG amount);
    static bool sellStock(PLAYER &qPlayer, SLONG airlineNum, SLONG amount);

    enum class OvertakeAirlineResult { Ok, DeniedInvalidParam, DeniedYourAirline, DeniedAlreadyGone, DeniedNoStock, DeniedNotEnoughStock, DeniedEnemyStock };
    static OvertakeAirlineResult canOvertakeAirline(PLAYER &qPlayer, SLONG targetAirline);
    static bool overtakeAirline(PLAYER &qPlayer, SLONG targetAirline, bool liquidate);

    enum class EmitStockResult { Ok, DeniedTooMuch, DeniedValueTooLow };
    static EmitStockResult canEmitStock(PLAYER &qPlayer, SLONG *outHowMany = nullptr);
    static bool emitStock(PLAYER &qPlayer, SLONG neueAktien, SLONG mode);

    static bool setDividend(PLAYER &qPlayer, SLONG dividend);

    /* Boss */
    enum class ExpandAirportResult { Ok, DeniedFreeGates, DeniedLimitReached, DeniedTooEarly, DeniedAlreadyExpanded, DeniedExpandingRightNow };
    static ExpandAirportResult canExpandAirport(PLAYER &qPlayer);
    static bool expandAirport(PLAYER &qPlayer);
    static bool bidOnGate(PLAYER &qPlayer, SLONG idx);
    static bool bidOnCity(PLAYER &qPlayer, SLONG idx);

    /* Mechanic */
    static SLONG setMechMode(PLAYER &qPlayer, SLONG mode);

    /* HR */
    static void increaseAllSalaries(PLAYER &qPlayer);
    static void decreaseAllSalaries(PLAYER &qPlayer);
    enum class EndStrikeMode { Salary, Threat, Drunk };
    static void endStrike(PLAYER &qPlayer, EndStrikeMode mode);

    /* Ads */
    static bool buyAdvertisement(PLAYER &qPlayer, SLONG adCampaignType, SLONG adCampaignSize, SLONG routeA = -1);

    /* Duty free */
    enum class BuyItemResult { Ok, DeniedInvalidParam, DeniedVirus1, DeniedVirus2, DeniedVirus3, DeniedLaptopNotYetAvailable, DeniedLaptopAlreadySold };
    static BuyItemResult buyDutyFreeItem(PLAYER &qPlayer, UBYTE item);

    /* Flights */
    static bool takeFlightJob(PLAYER &qPlayer, SLONG jobId, SLONG &outObjectId);
    static bool takeLastMinuteJob(PLAYER &qPlayer, SLONG jobId, SLONG &outObjectId);
    static bool takeFreightJob(PLAYER &qPlayer, SLONG jobId, SLONG &outObjectId);
    static bool canCallInternational(PLAYER &qPlayer, SLONG cityId);
    static bool takeInternationalFlightJob(PLAYER &qPlayer, SLONG cityId, SLONG jobId, SLONG &outObjectId);
    static bool takeInternationalFreightJob(PLAYER &qPlayer, SLONG cityId, SLONG jobId, SLONG &outObjectId);
    static bool killFlightJob(PLAYER &qPlayer, SLONG par1, bool payFine);
    static bool killFreightJob(PLAYER &qPlayer, SLONG par1, bool payFine);
    static bool killFlightPlan(PLAYER &qPlayer, SLONG par1);
    static bool killFlightPlanFrom(PLAYER &qPlayer, SLONG planeId, SLONG hours);
    static bool refillFlightJobs(SLONG cityNum);
    static bool planFlightJob(PLAYER &qPlayer, SLONG planeID, SLONG objectID, SLONG date, SLONG time);
    static bool planFreightJob(PLAYER &qPlayer, SLONG planeID, SLONG objectID, SLONG date, SLONG time);
    static bool planRouteJob(PLAYER &qPlayer, SLONG planeID, SLONG objectID, SLONG date, SLONG time);

    /* Crew */
    static bool hireWorker(PLAYER &qPlayer, SLONG workerId);
    static bool fireWorker(PLAYER &qPlayer, SLONG workerId);

    /* Routes */
    static bool killCity(PLAYER &qPlayer, SLONG cityID);
    static BUFFER_V<BOOL> getBuyableRoutes(PLAYER &qPlayer);
    static bool killRoute(PLAYER &qPlayer, SLONG routeA);
    static bool rentRoute(PLAYER &qPlayer, SLONG routeA);
    static SLONG findRouteInReverse(PLAYER &qPlayer, SLONG routeA);

    /* Execution routines */
    static void executeAirlineOvertake();
    static void executeSabotageMode1();
    static void executeSabotageMode2(bool &outBAnyBombs);
    static void executeSabotageMode3();

  private:
    static bool _planFlightJob(PLAYER &qPlayer, SLONG planeID, SLONG objectID, SLONG objectType, SLONG date, SLONG time);
};