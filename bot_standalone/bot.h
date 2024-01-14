#include <cassert>
#include <climits>
#include <deque>
#include <string>
#include <vector>

// Game interface

class CPlane {
  public:
    std::string Name;
    int ptPassagiere{300};
    int ptReichweite{10000};
    int TypeId{0};
    int mSpeed{1000};
    int mFuelCostPerKM{1};
};

class CAuftrag {
  public:
    int Personen{300};
    int VonCity{0};
    int NachCity{0};

    int Praemie{50000};
    int Strafe{100000};

    int Date{0};
    int BisDate{6};
};

class PlaneTime {
  public:
    PlaneTime() = default;
    PlaneTime(int date, int time) : mDate(date), mTime(time) {}

    int getDate() const { return mDate; }
    int getHour() const { return mTime; }

    PlaneTime &operator+=(int delta) {
        mTime += delta;
        while (mTime >= 24) {
            mTime -= 24;
            mDate++;
        }
        return *this;
    }

    PlaneTime &operator-=(int delta) {
        mTime -= delta;
        while (mTime < 0) {
            mTime += 24;
            mDate--;
        }
        return *this;
    }
    PlaneTime operator+(int delta) {
        PlaneTime t = *this;
        t += delta;
        return t;
    }
    PlaneTime operator-(int delta) {
        PlaneTime t = *this;
        t -= delta;
        return t;
    }
    void setDate(int date) {
        mDate = date;
        mTime = 0;
    }

  private:
    int mDate{0};
    int mTime{0};
};

using CPlanes = std::vector<CPlane>;
using CAuftraege = std::vector<CAuftrag>;

class Graph {
  public:
    struct Node {
        int premium{0};
        int duration{0};
        int earliest{0};
        int latest{0};
        std::vector<int> closestNeighbors;
    };

    struct Edge {
        int cost{0};
        int duration{0};
    };

    struct NodeState {
        bool visitedThisTour{false};
        int numVisited{0};
        PlaneTime availTime{};
        PlaneTime startTime{};
        int outIdx{0};
        int cameFrom{-1};
    };

    Graph(int numPlanes, int numJobs) : nPlanes(numPlanes), nNodes(numPlanes + numJobs), nodeInfo(nNodes), adjMatrix(nNodes), nodeState(nNodes) {
        for (int i = 0; i < nNodes; i++) {
            adjMatrix[i].resize(nNodes);
        }
    }

    void resetNodes() {
        for (int i = 0; i < nNodes; i++) {
            nodeState[i] = {};
        }
    }

    inline constexpr int jobToNode(int i) { return i + nPlanes; }
    inline constexpr int nodeToJob(int i) { return i - nPlanes; }

    int nPlanes;
    int nNodes;
    std::vector<Node> nodeInfo;
    std::vector<std::vector<Edge>> adjMatrix;
    std::vector<NodeState> nodeState;
};

class BotPlaner {
  public:
    enum class JobOwner { Player, PlayerFreight, TravelAgency, LastMinute, Freight, International, InternationalFreight };
    struct Solution {
        std::deque<std::pair<int, PlaneTime>> jobs;
        int totalPremium{0};
    };

    BotPlaner(const CPlanes &planes, JobOwner jobOwner, int intJobSource = -1);

    bool planFlights(const std::vector<int> &planeIds);

  private:
    struct PlaneState {
        int planeId{-1};
        int planeTypeId{-1};
        PlaneTime availTime{};
        int availCity{};
        std::vector<int> assignedJobIds;
        Solution currentSolution{};
    };

    struct FlightJob {
        FlightJob(int i, CAuftrag a, JobOwner o) : id(i), auftrag(a), owner(o) {}
        int id{};
        CAuftrag auftrag;
        JobOwner owner;
        std::vector<int> eligiblePlanes;
        int assignedtoPlaneId{-1};
    };

    void printJob(const CAuftrag &qAuftrag);
    void printJobShort(const CAuftrag &qAuftrag);
    void printSolution(const Solution &solution, const std::vector<FlightJob> &list);
    void printGraph(const std::vector<PlaneState> &planeStates, const std::vector<FlightJob> &list, const Graph &g);

    void findPlaneTypes(std::vector<PlaneState> &planeStates, std::vector<const CPlane *> &planeTypeToPlane);
    Solution findFlightPlan(Graph &g, int planeId, PlaneTime availTime, const std::vector<int> &eligibleJobIds);
    void gatherAndPlanJobs(std::vector<FlightJob> &jobList, std::vector<PlaneState> &planeStates);

    JobOwner mJobOwner;
    int mIntJobSource;
    const CPlanes &qPlanes;
};
