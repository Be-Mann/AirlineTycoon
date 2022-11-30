#include "BotPlaner.h"

#include "BotHelper.h"
#include "TeakLibW.h"

#include <array>
#include <chrono>
#include <iostream>

// #define PRINT_DETAIL 1
// #define PRINT_OVERALL 1

int kNumToAdd = 99;
int kNumToRemove = 3;
int kTempStart = 1000;
extern const int kScheduleForNextDays;

inline int pathLength(const Graph &g, int start) {
    int n = g.nodeState[start].nextNode;
    int num = 0;
    while (n != -1) {
        num++;
        n = g.nodeState[n].nextNode;
    }
    return num;
}

inline bool SA_accept(int diff, double temperature, int rand) {
    if (diff > 0) {
        return true;
    }
    auto probability = static_cast<int>(std::round(100 * exp(diff / temperature)));
    return rand <= probability;
}

void BotPlaner::savePath(int planeIdx, std::vector<int> &path) const {
    auto &planeState = mPlaneStates[planeIdx];
    auto &g = mGraphs[planeState.planeTypeId];
    int n = g.nodeState[planeIdx].nextNode;
    path.clear();
    while (n != -1) {
        path.push_back(n);
        n = g.nodeState[n].nextNode;
    }
}

void BotPlaner::restorePath(int planeIdx, const std::vector<int> &path) {
    auto &planeState = mPlaneStates[planeIdx];
    auto &g = mGraphs[planeState.planeTypeId];
    for (int n : path) {
        algo2InsertNode(g, planeIdx, n);
    }
}

void BotPlaner::killPath(int planeIdx) {
    auto &planeState = mPlaneStates[planeIdx];
    auto &g = mGraphs[planeState.planeTypeId];

    int n = g.nodeState[planeIdx].nextNode;
    g.nodeState[planeIdx].nextNode = -1;
    while (n != -1) {
        const auto &curInfo = g.nodeInfo[n];
        int nextNode = g.nodeState[n].nextNode;

        mJobList[curInfo.jobIdx].assignedtoPlaneIdx = -1;

        g.nodeState[n].cameFrom = -1;
        g.nodeState[n].nextNode = -1;

        n = nextNode;
    }

    planeState.currentPremium = 0;
    planeState.currentCost = 0;
    planeState.currentNode = planeIdx;
    planeState.currentTime = g.nodeState[planeIdx].startTime;
    planeState.numNodes = 0;
}

void BotPlaner::printNodeInfo(const Graph &g, int nodeIdx) const {
    (void)g;
    (void)nodeIdx;
#ifdef PRINT_DETAIL
    const auto &curInfo = g.nodeInfo[nodeIdx];
    const auto &curState = g.nodeState[nodeIdx];
    if (nodeIdx < g.nPlanes) {
        auto &qPlaneState = mPlaneStates[nodeIdx];
        const auto &qPlane = qPlanes[qPlaneState.planeId];
        hprintf("Node %d is start for plane %s, available %s %d", nodeIdx, (LPCTSTR)qPlane.Name, (LPCTSTR)Helper::getWeekday(qPlaneState.availTime.getDate()),
                qPlaneState.availTime.getHour());
    } else {
        const auto &qJob = mJobList[curInfo.jobIdx];
        if (qJob.assignedtoPlaneIdx != -1) {
            auto &qPlaneState = mPlaneStates[qJob.assignedtoPlaneIdx];
            const auto &qPlane = qPlanes[qPlaneState.planeId];
            hprintf("Node %d is job %s scheduled for plane %s (starting %s %d)", nodeIdx, Helper::getJobName(qJob.auftrag).c_str(), (LPCTSTR)qPlane.Name,
                    (LPCTSTR)Helper::getWeekday(curState.startTime.getDate()), curState.startTime.getHour());
        } else {
            hprintf("Node %d is job %s (unscheduled)", nodeIdx, Helper::getJobName(qJob.auftrag).c_str());
        }
    }
#endif
}

bool BotPlaner::algo2ShiftLeft(Graph &g, int nodeToShift, int shiftT, bool commit) {
    if (shiftT <= 0) {
        return false;
    }
    int currentNode = nodeToShift;
    auto currentTime = g.nodeState[currentNode].startTime - shiftT;

    while (true) {
        auto &curState = g.nodeState[currentNode];
        const auto &curInfo = g.nodeInfo[currentNode];

        /* check if we can shift this node */
        if (currentNode < g.nPlanes) {
            assert(!commit);
            return false;
        }
        if (currentTime.getDate() < curInfo.earliest) {
            assert(!commit);
            return false;
        }

        /* shift node */
        if (commit) {
            curState.startTime = currentTime;
        }

        int prevNode = curState.cameFrom;
        if (prevNode == -1) {
            break;
        }

        /* calculate new start time of previous node */
        currentTime -= g.adjMatrix[prevNode][currentNode].duration;
        currentTime -= g.nodeInfo[prevNode].duration;

        /* check if we have to move previous node as well */
        if (currentTime >= g.nodeState[prevNode].startTime) {
            break;
        }

        currentNode = prevNode;
    }
    return true;
}

bool BotPlaner::algo2ShiftRight(Graph &g, int nodeToShift, int shiftT, bool commit) {
    if (shiftT <= 0) {
        return false;
    }
    int currentNode = nodeToShift;
    auto currentTime = g.nodeState[currentNode].startTime + shiftT;

    while (true) {
        auto &curState = g.nodeState[currentNode];
        const auto &curInfo = g.nodeInfo[currentNode];

        /* check if we can shift this node */
        if (currentTime.getDate() > Sim.Date + kScheduleForNextDays) {
            assert(!commit);
            return false;
        }
        if (currentTime.getDate() > curInfo.latest) {
            assert(!commit);
            return false;
        }

        /* shift node */
        if (commit) {
            curState.startTime = currentTime;
        }

        int nextNode = curState.nextNode;
        if (nextNode == -1) {
            break;
        }
        auto &nextState = g.nodeState[nextNode];

        /* calculate new start time of next node */
        currentTime += curInfo.duration;
        currentTime += g.adjMatrix[currentNode][nextNode].duration;
        assert(g.adjMatrix[currentNode][nextNode].duration >= 0);

        /* check if we have to move next node as well */
        if (currentTime <= nextState.startTime) {
            break;
        }

        currentNode = nextNode;
    }
    return true;
}

bool BotPlaner::algo2MakeRoom(Graph &g, int nodeToMoveLeft, int nodeToMoveRight) {
    assert(nodeToMoveLeft >= g.nPlanes || nodeToMoveRight >= g.nPlanes);

    int shiftLeft = 1;
    int shiftRight = 1;
    if (nodeToMoveLeft >= g.nPlanes) {
        while (shiftLeft < 24 && algo2ShiftLeft(g, nodeToMoveLeft, shiftLeft, false)) {
            shiftLeft++;
        }
    }
    if (nodeToMoveRight >= g.nPlanes) {
        while (shiftRight < 24 && algo2ShiftRight(g, nodeToMoveRight, shiftRight, false)) {
            shiftRight++;
        }
    }
    if (shiftLeft == 1 && shiftRight == 1) {
        return false;
    }

#ifdef PRINT_DETAIL
    if (nodeToMoveLeft >= g.nPlanes && nodeToMoveRight >= g.nPlanes) {
        const auto &prevJob = mJobList[g.nodeInfo[nodeToMoveLeft].jobIdx].auftrag;
        const auto &job = mJobList[g.nodeInfo[nodeToMoveRight].jobIdx].auftrag;
        std::cout << "Make room between " << Helper::getJobName(prevJob).c_str() << " and " << Helper::getJobName(job).c_str() << std::endl;
    } else if (nodeToMoveLeft >= g.nPlanes) {
        const auto &prevJob = mJobList[g.nodeInfo[nodeToMoveLeft].jobIdx].auftrag;
        std::cout << "Make room after " << Helper::getJobName(prevJob).c_str() << std::endl;
    } else {
        const auto &job = mJobList[g.nodeInfo[nodeToMoveRight].jobIdx].auftrag;
        std::cout << "Make room before " << Helper::getJobName(job).c_str() << std::endl;
    }
    std::cout << "Maximum shift: " << shiftLeft - 1 << ", " << shiftRight - 1 << std::endl;
#endif

    algo2ShiftLeft(g, nodeToMoveLeft, shiftLeft - 1, true);
    algo2ShiftRight(g, nodeToMoveRight, shiftRight - 1, true);
    return true;
}

void BotPlaner::algo2ApplySolutionToGraph() {
    for (int p = 0; p < mPlaneStates.size(); p++) {
        auto &qPlaneState = mPlaneStates[p];
        const auto &qPlane = qPlanes[qPlaneState.planeId];
        auto &g = mGraphs[qPlaneState.planeTypeId];
        const auto &qFlightPlan = qPlane.Flugplan.Flug;

        qPlaneState.currentNode = p;

        int autoFlightDuration = -1;
        for (int d = 0; d < qFlightPlan.AnzEntries(); d++) {
            const auto &qFPE = qFlightPlan[d];
            if (qFPE.ObjectType <= 0) {
                break;
            }

            auto planeTime = PlaneTime{qFPE.Startdate, qFPE.Startzeit};
            if (planeTime < qPlaneState.availTime) {
                continue;
            }

            if (qFPE.Okay != 0) {
                redprintf("BotPlaner::algo2ApplySolutionToGraph(): Not scheduled correctly, skipping: %ld", qFPE.ObjectId);
                continue;
            }

            if (qFPE.ObjectType == 2) {
                auto it = mExistingJobsById.find(qFPE.ObjectId);
                if (it == mExistingJobsById.end()) {
                    redprintf("BotPlaner::algo2ApplySolutionToGraph(): Unknown job in flight plan: %ld", qFPE.ObjectId);
                    continue;
                }
                int currentNode = qPlaneState.currentNode;
                int nextNode = it->second + g.nPlanes;

                /* check duration of any previous automatic flight */
                if (autoFlightDuration != -1 && autoFlightDuration != g.adjMatrix[currentNode][nextNode].duration) {
                    redprintf("BotPlaner::algo2ApplySolutionToGraph(): Duration of automatic flight does not match before FPE: %ld", qFPE.ObjectId);
                }

                qPlaneState.currentTime = planeTime;
                if (autoFlightDuration != -1) {
                    qPlaneState.currentTime -= autoFlightDuration;
                }
                algo2InsertNode(g, p, nextNode);

                autoFlightDuration = -1;
            } else if (qFPE.ObjectType == 3) {
                assert(autoFlightDuration == -1);
                autoFlightDuration = 24 * (qFPE.Landedate - qFPE.Startdate) + (qFPE.Landezeit - qFPE.Startzeit);
                autoFlightDuration += kDurationExtra;
            } /* TODO: Fracht */
        }
    }
}

void BotPlaner::algo2GenSolutionsFromGraph(int planeIdx) {
    auto &planeState = mPlaneStates[planeIdx];
    auto &g = mGraphs[planeState.planeTypeId];

    planeState.currentSolution = {planeState.currentPremium - planeState.currentCost};
    planeState.currentSolution.planeId = planeState.planeId;
    planeState.currentSolution.scheduleFromTime = mScheduleFromTime;

    auto &qJobList = planeState.currentSolution.jobs;
    int node = g.nodeState[planeIdx].nextNode;
    while (node != -1) {
        int jobIdx = g.nodeInfo[node].jobIdx;
        assert(planeIdx == mJobList[jobIdx].assignedtoPlaneIdx);

        qJobList.emplace_front(jobIdx, g.nodeState[node].startTime, g.nodeState[node].startTime + g.nodeInfo[node].duration);
        printNodeInfo(g, node);

        node = g.nodeState[node].nextNode;
    }
}

int BotPlaner::algo2FindNext(const Graph &g, PlaneState &planeState, int choice) const {
    auto currentNode = planeState.currentNode;
    const auto &curInfo = g.nodeInfo[currentNode];

#ifdef PRINT_DETAIL
    std::cout << "algo2FindNext(): planeState.currentNode = ";
    printNodeInfo(g, planeState.currentNode);
    std::cout << "algo2FindNext(): planeState.currentTime = " << Helper::getWeekday(planeState.currentTime.getDate()) << planeState.currentTime.getHour()
              << std::endl;
#endif

    /* determine next node for plane */
    int iter = 0;
    int outIdx = -1;
    while (iter < curInfo.bestNeighbors.size()) {
        outIdx = (iter + choice) % curInfo.bestNeighbors.size();
        int nextNode = curInfo.bestNeighbors[outIdx];
        assert(nextNode >= g.nPlanes && nextNode < g.nNodes);
        const auto &nextInfo = g.nodeInfo[nextNode];

#ifdef PRINT_DETAIL
        std::cout << std::endl;
        std::cout << "??? Now examining node: ";
        printNodeInfo(g, nextNode);
#endif

        int jobIdx = nextInfo.jobIdx;
        if (mJobList[jobIdx].assignedtoPlaneIdx != -1) {
            iter++;
#ifdef PRINT_DETAIL
            std::cout << "??? Already assigned to plane " << mJobList[jobIdx].assignedtoPlaneIdx << std::endl;
#endif
            continue; /* job already assigned */
        }

        auto tEarliest = planeState.currentTime + g.adjMatrix[currentNode][nextNode].duration;
#ifdef PRINT_DETAIL
        std::cout << "??? tEarliest = " << Helper::getWeekday(tEarliest.getDate()) << tEarliest.getHour() << std::endl;
#endif
        if (tEarliest.getDate() > nextInfo.latest) {
            iter++;
#ifdef PRINT_DETAIL
            std::cout << "??? we are too late" << std::endl;
#endif
            continue; /* we are too late */
        }

        if (tEarliest.getDate() < nextInfo.earliest) {
            tEarliest.setDate(nextInfo.earliest);
        }

#ifdef PRINT_DETAIL
        std::cout << "??? tEarliest (job not before) = " << Helper::getWeekday(tEarliest.getDate()) << tEarliest.getHour() << std::endl;
#endif

        /* if the current node already has a successor, then we need to check
         * whether the new node can be inserted inbetween.
         * We must not shift it by even one 1 hour since the makeRoom function
         * already pushed everything as far back as possible. */
        int overnextNode = g.nodeState[currentNode].nextNode;
        if (overnextNode != -1) {
            auto tEarliestOvernext = tEarliest + nextInfo.duration;
            int duration = g.adjMatrix[nextNode][overnextNode].duration;
            tEarliestOvernext += duration;
#ifdef PRINT_DETAIL
            std::cout << "??? overnextNode = ";
            printNodeInfo(g, overnextNode);
            std::cout << "??? tEarliestOvernext = " << Helper::getWeekday(tEarliestOvernext.getDate()) << tEarliestOvernext.getHour() << std::endl;
            std::cout << "??? duration = " << duration << std::endl;
#endif
            if (duration < 0 || tEarliestOvernext > g.nodeState[overnextNode].startTime) {
                iter++;
#ifdef PRINT_DETAIL
                std::cout << "??? no room for auto flight to next node" << std::endl;
#endif
                continue; /* no room */
            }
        }
        break;
    }
    if (iter == curInfo.bestNeighbors.size()) {
        return -1;
    }

#ifdef PRINT_DETAIL
    std::cout << "??? Select node: " << curInfo.bestNeighbors[outIdx] << std::endl << std::endl;
    printNodeInfo(g, curInfo.bestNeighbors[outIdx]);
#endif
    return curInfo.bestNeighbors[outIdx];
}

void BotPlaner::algo2InsertNode(Graph &g, int planeIdx, int nextNode) {
    assert(nextNode >= g.nPlanes && nextNode < g.nNodes);

    auto &qPlaneState = mPlaneStates[planeIdx];
    auto currentNode = qPlaneState.currentNode;
    const auto &nextInfo = g.nodeInfo[nextNode];

    /* edge */
    qPlaneState.currentCost += g.adjMatrix[currentNode][nextNode].cost;
    qPlaneState.currentTime += g.adjMatrix[currentNode][nextNode].duration;
    assert(g.adjMatrix[currentNode][nextNode].duration >= 0);

    /* job premium */
    qPlaneState.currentPremium += nextInfo.premium;

    /* job duration */
    if (qPlaneState.currentTime.getDate() < nextInfo.earliest) {
        qPlaneState.currentTime.setDate(nextInfo.earliest);
    }
    g.nodeState[nextNode].startTime = qPlaneState.currentTime;
    qPlaneState.currentTime += nextInfo.duration;

    /* assign job */
    assert(mJobList[nextInfo.jobIdx].assignedtoPlaneIdx == -1);
    mJobList[nextInfo.jobIdx].assignedtoPlaneIdx = planeIdx;

    /* did current already have a successor? This will now become the overnext node */
    int overnextNode = g.nodeState[currentNode].nextNode;
    if (overnextNode != -1) {
        g.nodeState[nextNode].nextNode = overnextNode;
        g.nodeState[overnextNode].cameFrom = nextNode;

        /* re-calc edge cost */
        qPlaneState.currentCost -= g.adjMatrix[currentNode][overnextNode].cost;
        assert(g.adjMatrix[currentNode][overnextNode].duration >= 0);
        qPlaneState.currentCost += g.adjMatrix[nextNode][overnextNode].cost;
        assert(g.adjMatrix[nextNode][overnextNode].duration >= 0);
    }

#ifdef PRINT_DETAIL
    if (overnextNode != -1) {
        std::cout << ">>> Node " << nextNode << " is inserted after " << currentNode << " and before " << overnextNode << std::endl;
    } else {
        std::cout << ">>> Node " << nextNode << " is inserted after " << currentNode << " and is the last node" << std::endl;
    }
    printNodeInfo(g, currentNode);
    printNodeInfo(g, nextNode);
    if (overnextNode != -1) {
        printNodeInfo(g, overnextNode);
    }
#endif

    g.nodeState[currentNode].nextNode = nextNode;
    g.nodeState[nextNode].cameFrom = currentNode;

    qPlaneState.currentNode = nextNode;
    qPlaneState.numNodes++;
    assert(pathLength(g, planeIdx) == qPlaneState.numNodes);

#ifdef PRINT_DETAIL
    std::cout << ">>> currentPremium: " << qPlaneState.currentPremium << std::endl;
    std::cout << ">>> currentCost: " << qPlaneState.currentCost << std::endl;
    std::cout << ">>> currentTime: " << Helper::getWeekday(qPlaneState.currentTime.getDate()) << qPlaneState.currentTime.getHour() << std::endl;
    std::cout << std::endl;
#endif
}

void BotPlaner::algo2RemoveNode(Graph &g, int planeIdx, int currentNode) {
    assert(currentNode >= g.nPlanes && currentNode < g.nNodes);

    auto &qPlaneState = mPlaneStates[planeIdx];
    const auto &curInfo = g.nodeInfo[currentNode];

    /* go one node back */
    int prevNode = g.nodeState[currentNode].cameFrom;
    assert(prevNode >= 0);

    /* job premium */
    qPlaneState.currentPremium -= curInfo.premium;

    /* edge cost */
    qPlaneState.currentCost -= g.adjMatrix[prevNode][currentNode].cost;
    assert(g.adjMatrix[prevNode][currentNode].duration >= 0);

    /* when was plane available? */
    qPlaneState.currentTime = g.nodeState[prevNode].startTime;
    qPlaneState.currentTime += g.nodeInfo[prevNode].duration;

    /* unassign job */
    assert(mJobList[curInfo.jobIdx].assignedtoPlaneIdx != -1);
    mJobList[curInfo.jobIdx].assignedtoPlaneIdx = -1;

    qPlaneState.currentNode = prevNode;
    qPlaneState.numNodes--;
    assert(qPlaneState.numNodes >= 0);

    /* link next node to previous node */
    int nextNode = g.nodeState[currentNode].nextNode;
    if (nextNode != -1) {
        g.nodeState[prevNode].nextNode = nextNode;
        g.nodeState[nextNode].cameFrom = prevNode;

        /* re-calc edge cost */
        qPlaneState.currentCost -= g.adjMatrix[currentNode][nextNode].cost;
        assert(g.adjMatrix[currentNode][nextNode].duration >= 0);
        qPlaneState.currentCost += g.adjMatrix[prevNode][nextNode].cost;
        assert(g.adjMatrix[prevNode][nextNode].duration >= 0);
    } else {
        g.nodeState[prevNode].nextNode = -1;
    }

    /* unlink current node */
    g.nodeState[currentNode].cameFrom = -1;
    g.nodeState[currentNode].nextNode = -1;

    assert(pathLength(g, planeIdx) == qPlaneState.numNodes);

#ifdef PRINT_DETAIL
    if (nextNode != -1) {
        std::cout << "<<< Node " << currentNode << " is removed. Was after " << prevNode << " and before " << nextNode << std::endl;
    } else {
        std::cout << "<<< Node " << currentNode << " is removed. Was after " << prevNode << " and is the last node" << std::endl;
    }
    printNodeInfo(g, prevNode);
    printNodeInfo(g, currentNode);
    if (nextNode != -1) {
        printNodeInfo(g, nextNode);
    }
#endif
}

int BotPlaner::algo2RunForPlaneRemove(int planeIdx, int numToRemove) {
    auto &planeState = mPlaneStates[planeIdx];
    auto &g = mGraphs[planeState.planeTypeId];

#ifdef PRINT_OVERALL
    std::cout << "****************************************" << std::endl;
    std::cout << "Plane:  " << qPlanes[planeState.planeId].Name.c_str() << std::endl;
    std::cout << "qPlaneState.numNodes " << planeState.numNodes << std::endl;
    algo2GenSolutionsFromGraph(planeIdx);
    GameMechanic::killFlightPlanFrom(qPlayer, planeState.planeId, mScheduleFromTime.getDate(), mScheduleFromTime.getHour());
    applySolutionForPlane(qPlayer, planeState.planeId, planeState.currentSolution);
    Helper::checkPlaneSchedule(qPlayer, qPlanes[planeState.planeId], false);
#endif

    /* remove some nodes */
    int nRemoved = 0;
    for (int pass = 0; pass < numToRemove; pass++) {
        int worstNode = -1;
        int worstGain = INT_MAX;

        int prevNode = planeIdx;
        int currentNode = g.nodeState[prevNode].nextNode;
        while (currentNode != -1) {
            assert(g.adjMatrix[prevNode][currentNode].duration >= 0);

            int jobIdx = g.nodeInfo[currentNode].jobIdx;
            if (!mJobList[jobIdx].wasTaken()) {
                int gain = g.nodeInfo[currentNode].premium - g.adjMatrix[prevNode][currentNode].cost;
                if (gain < worstGain) {
                    worstGain = gain;
                    worstNode = currentNode;
                }
            }

            prevNode = currentNode;
            currentNode = g.nodeState[currentNode].nextNode;
        }

        if (worstNode != -1) {
            algo2RemoveNode(g, planeIdx, worstNode);
            nRemoved++;
        }
    }

    return nRemoved;
}

int BotPlaner::algo2RunForPlaneAdd(int planeIdx, int numToAdd, int choice) {
    auto &planeState = mPlaneStates[planeIdx];
    auto &g = mGraphs[planeState.planeTypeId];

    /* try to insert new nodes */
#ifdef PRINT_OVERALL
    std::cout << "Trying to insert" << std::endl;
#endif
    int nScheduled = 0;
    for (int pass = 0; pass < numToAdd; pass++) {
        /* make room after random node */
        if (planeState.numNodes > 0) {
            int whereToInsert = getRandInt(0, planeState.numNodes);
            int nodeToMoveLeft = planeIdx;
            while (whereToInsert > 0) {
                whereToInsert--;
                nodeToMoveLeft = g.nodeState[nodeToMoveLeft].nextNode;
            }

            bool success = algo2MakeRoom(g, nodeToMoveLeft, g.nodeState[nodeToMoveLeft].nextNode);

#ifdef PRINT_OVERALL
            if (!success) {
                std::cout << "MakeRoom failed" << std::endl;
            } else {
                std::cout << "MakeRoom success!" << std::endl;
                algo2GenSolutionsFromGraph(planeIdx);
                GameMechanic::killFlightPlanFrom(qPlayer, planeState.planeId, mScheduleFromTime.getDate(), mScheduleFromTime.getHour());
                applySolutionForPlane(qPlayer, planeState.planeId, planeState.currentSolution);
                Helper::checkPlaneSchedule(qPlayer, qPlanes[planeState.planeId], false);
            }
#endif

            if (!success) {
                continue;
            }

            planeState.currentNode = nodeToMoveLeft;
        }

        /* when is the plane available */
        int currentNode = planeState.currentNode;
        planeState.currentTime = g.nodeState[currentNode].startTime;
        planeState.currentTime += g.nodeInfo[currentNode].duration;
        if (planeState.currentTime < planeState.availTime) {
            planeState.currentTime = planeState.availTime;
        }
#ifdef PRINT_OVERALL
        std::cout << "planeState.currentNode " << planeState.currentNode << std::endl;
        std::cout << "g.nodeState[currentNode].startTime " << Helper::getWeekday(g.nodeState[currentNode].startTime.getDate())
                  << g.nodeState[currentNode].startTime.getHour() << std::endl;
        std::cout << "g.nodeInfo[currentNode].duration " << g.nodeInfo[currentNode].duration << std::endl;
        std::cout << "planeState.currentTime " << Helper::getWeekday(planeState.currentTime.getDate()) << planeState.currentTime.getHour() << std::endl;
#endif

        if (planeState.currentTime.getDate() > Sim.Date + kScheduleForNextDays) {
            continue;
        }

        int nextNode = algo2FindNext(g, planeState, choice);
        if (nextNode != -1) {
            algo2InsertNode(g, planeIdx, nextNode);
            nScheduled++;
#ifdef PRINT_OVERALL
            algo2GenSolutionsFromGraph(planeIdx);
            GameMechanic::killFlightPlanFrom(qPlayer, planeState.planeId, mScheduleFromTime.getDate(), mScheduleFromTime.getHour());
            applySolutionForPlane(qPlayer, planeState.planeId, planeState.currentSolution);
            Helper::checkPlaneSchedule(qPlayer, qPlanes[planeState.planeId], false);
#endif
        }
    }

    return nScheduled;
}

bool BotPlaner::algo2(int64_t timeBudget) {
    timeBudget = 1000 * std::max((int64_t)1, timeBudget);
    auto t_begin = std::chrono::steady_clock::now();

    for (auto &g : mGraphs) {
        for (int n = 0; n < g.nPlanes; n++) {
            g.nodeState[n].startTime = mPlaneStates[n].availTime;
            g.nodeState[n].cameFrom = -1;
            g.nodeState[n].nextNode = -1;
        }
    }

    /* apply existing solution to graph and plane state */
    int oldGain = allPlaneGain();
    int overallBestGain = oldGain;
    std::vector<std::vector<int>> overallBestPath(mPlaneStates.size());
    algo2ApplySolutionToGraph();
    for (int planeIdx = 0; planeIdx < mPlaneStates.size(); planeIdx++) {
        savePath(planeIdx, overallBestPath[planeIdx]);
    }

    /* current best */
    int currentBestGain = allPlaneGain();
    std::vector<std::vector<int>> currentBestPath(mPlaneStates.size());
    for (int planeIdx = 0; planeIdx < mPlaneStates.size(); planeIdx++) {
        savePath(planeIdx, currentBestPath[planeIdx]);
    }

    /* main algo */
    int temperature = kTempStart;
    while (temperature > 0) {
        int iterScheduled = 0;
        for (int planeIdx = 0; planeIdx < mPlaneStates.size(); planeIdx++) {
            iterScheduled += algo2RunForPlaneRemove(planeIdx, kNumToRemove);
        }

        for (int i = 0; i < kNumToAdd; i++) {
            int added = 0;
            for (int planeIdx = 0; planeIdx < mPlaneStates.size(); planeIdx++) {
                added += algo2RunForPlaneAdd(planeIdx, 1, getRandInt(0, 3));
            }
            if (added == 0) {
                break;
            }
            iterScheduled += added;
        }

        int iterGain = allPlaneGain();
        if (!SA_accept(iterGain, temperature, getRandInt(1, 100))) {
            /* roll back */
            for (int planeIdx = 0; planeIdx < mPlaneStates.size(); planeIdx++) {
                killPath(planeIdx);
            }
            for (int planeIdx = 0; planeIdx < mPlaneStates.size(); planeIdx++) {
                restorePath(planeIdx, currentBestPath[planeIdx]);
            }
        } else {
            /* save current best solution */
            currentBestGain = iterGain;
            for (int planeIdx = 0; planeIdx < mPlaneStates.size(); planeIdx++) {
                savePath(planeIdx, currentBestPath[planeIdx]);
            }

            if (iterGain > overallBestGain) {
                int test = 0;
                overallBestGain = iterGain;
                for (int planeIdx = 0; planeIdx < mPlaneStates.size(); planeIdx++) {
                    savePath(planeIdx, overallBestPath[planeIdx]);
                    test += mPlaneStates[planeIdx].currentPremium - mPlaneStates[planeIdx].currentCost;
                }
                assert(test == overallBestGain);
            }
        }

        /* adjust temperature based on amount of time left */
        if (temperature == 1) {
            break;
        }
        auto t_end = std::chrono::steady_clock::now();
        auto delta = std::chrono::duration_cast<std::chrono::microseconds>(t_end - t_begin).count();
        int newTemperature = kTempStart - (kTempStart * delta / timeBudget);
        temperature = std::min(temperature - 1, newTemperature);
        if (temperature < 1) {
            temperature = 1; /* ensure final greedy run */
        }

#ifdef PRINT_OVERALL
        hprintf("%f ms left, temp now %d. Current gain = %d (overall = %d)", (timeBudget - delta) / 1000.0, temperature, currentBestGain, overallBestGain);
#endif
    }

    /* restore best path */
    for (int planeIdx = 0; planeIdx < mPlaneStates.size(); planeIdx++) {
        killPath(planeIdx);
    }

    /* generate solution */
    for (int planeIdx = 0; planeIdx < mPlaneStates.size(); planeIdx++) {
        restorePath(planeIdx, overallBestPath[planeIdx]);
        algo2GenSolutionsFromGraph(planeIdx);
    }

    return (overallBestGain > oldGain);
}