#include "algorithms/GreedyTSP.hpp"
#include "algorithms/AlgorithmTrace.hpp"
#include <cmath>
#include <limits>
#include <algorithm>
#include <sstream>
#include <iomanip>

namespace dhaka
{

    std::vector<int> GreedyTSP::sequenceVisits(
        int startNodeId,
        const std::vector<int> &selectedBinIds,
        const std::vector<CollectionPoint> &allBins,
        const Graph &graph,
        double alpha,
        double beta,
        AlgorithmTrace *traceOut,
        int vehicleId)
    {
        if (traceOut)
        {
            traceOut->clear();
            traceOut->type = TraceType::SEQUENCING_GREEDY;
            std::ostringstream ss;
            ss << "Greedy Nearest-Neighbour Visit Sequencing (Truck "
               << (vehicleId >= 0 ? std::to_string(vehicleId + 1) : "") << ")";
            traceOut->title = ss.str();
        }

        if (selectedBinIds.size() <= 1)
        {
            if (traceOut && !selectedBinIds.empty())
            {
                AlgorithmStep step;
                step.type = TraceType::SEQUENCING_GREEDY;
                step.activeVehicleId = vehicleId;
                step.activeBinId = selectedBinIds[0];
                step.tourBinIdsSoFar = selectedBinIds;
                step.narration = "Only 1 bin selected by knapsack. Immediate single stop scheduled.";
                traceOut->addStep(step);
            }
            return selectedBinIds;
        }

        std::vector<int> unvisited = selectedBinIds;
        std::vector<int> orderedTour;
        orderedTour.reserve(selectedBinIds.size());

        int currentNode = startNodeId;
        Vec2 currentPos = (startNodeId >= 0 && startNodeId < graph.getNodeCount()) ? graph.getNode(startNodeId).position : Vec2{0.0f, 0.0f};

        if (traceOut)
        {
            AlgorithmStep step;
            step.type = TraceType::ROUTING_ASTAR;
            step.activeVehicleId = vehicleId;
            step.activeNodeId = startNodeId;
            std::ostringstream ss;
            ss << "Starting Greedy TSP sequencing from Node #" << startNodeId << ". Sequencing "
               << selectedBinIds.size() << " target stops using alpha = " << alpha
               << " (urgency priority) and beta = " << beta << " (travel penalty).";
            step.narration = ss.str();
            traceOut->addStep(step);
        }

        int stopNum = 1;
        while (!unvisited.empty())
        {
            int bestIdx = -1;
            double bestScore = -std::numeric_limits<double>::infinity();
            double bestDist = 0.0;
            double bestUrgency = 0.0;

            for (size_t i = 0; i < unvisited.size(); ++i)
            {
                int binId = unvisited[i];
                const auto &bin = allBins[binId];

                Vec2 binPos = bin.position;
                if (bin.nodeId >= 0 && bin.nodeId < graph.getNodeCount())
                {
                    binPos = graph.getNode(bin.nodeId).position;
                }

                double distMeters = currentPos.distanceTo(binPos) * 20.0;
                double travelTimeSec = distMeters / (30.0 * 1000.0 / 3600.0);

                double urgencyTerm = std::pow(std::max(0.01, bin.urgencyScore + 0.1), alpha);
                double distanceTerm = std::pow(std::max(1.0, travelTimeSec + 5.0), beta);
                double score = urgencyTerm / distanceTerm;

                if (score > bestScore)
                {
                    bestScore = score;
                    bestIdx = static_cast<int>(i);
                    bestDist = distMeters;
                    bestUrgency = bin.urgencyScore;
                }
            }

            if (bestIdx >= 0)
            {
                int chosenBinId = unvisited[bestIdx];
                orderedTour.push_back(chosenBinId);

                const auto &chosenBin = allBins[chosenBinId];
                currentNode = chosenBin.nodeId;
                if (currentNode >= 0 && currentNode < graph.getNodeCount())
                {
                    currentPos = graph.getNode(currentNode).position;
                }

                unvisited.erase(unvisited.begin() + bestIdx);

                if (traceOut)
                {
                    AlgorithmStep step;
                    step.type = TraceType::SEQUENCING_GREEDY;
                    step.activeVehicleId = vehicleId;
                    step.activeBinId = chosenBinId;
                    step.tourBinIdsSoFar = orderedTour;
                    step.candidateBinId = chosenBinId;
                    step.candidateScore = bestScore;

                    std::ostringstream ss;
                    ss << "Stop #" << stopNum++ << " assigned -> " << chosenBin.name
                       << " (dist: " << static_cast<int>(bestDist) << "m, urgency: "
                       << std::fixed << std::setprecision(2) << bestUrgency
                       << ", composite score: " << std::setprecision(3) << bestScore << ").";
                    step.narration = ss.str();
                    traceOut->addStep(step);
                }
            }
            else
            {
                for (int remainingId : unvisited)
                {
                    orderedTour.push_back(remainingId);
                }
                break;
            }
        }

        if (traceOut)
        {
            AlgorithmStep step;
            step.type = TraceType::SEQUENCING_GREEDY;
            step.activeVehicleId = vehicleId;
            step.tourBinIdsSoFar = orderedTour;
            std::ostringstream ss;
            ss << "Greedy TSP sequence complete! Optimal visit order established for "
               << orderedTour.size() << " collection stops.";
            step.narration = ss.str();
            traceOut->addStep(step);
        }

        return orderedTour;
    }

} // namespace dhaka
