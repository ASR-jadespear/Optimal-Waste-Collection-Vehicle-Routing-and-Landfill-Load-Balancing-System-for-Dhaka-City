#include "algorithms/GreedyTSP.hpp"
#include <cmath>
#include <limits>
#include <algorithm>

namespace dhaka
{

    std::vector<int> GreedyTSP::sequenceVisits(
        int startNodeId,
        const std::vector<int> &selectedBinIds,
        const std::vector<CollectionPoint> &allBins,
        const Graph &graph,
        double alpha,
        double beta)
    {
        if (selectedBinIds.size() <= 1)
        {
            return selectedBinIds;
        }

        std::vector<int> unvisited = selectedBinIds;
        std::vector<int> orderedTour;
        orderedTour.reserve(selectedBinIds.size());

        int currentNode = startNodeId;
        Vec2 currentPos = (startNodeId >= 0 && startNodeId < graph.getNodeCount()) ? graph.getNode(startNodeId).position : Vec2{0.0f, 0.0f};

        while (!unvisited.empty())
        {
            int bestIdx = -1;
            double bestScore = -std::numeric_limits<double>::infinity();

            for (size_t i = 0; i < unvisited.size(); ++i)
            {
                int binId = unvisited[i];
                const auto &bin = allBins[binId];

                // Distance estimate in meters
                Vec2 binPos = bin.position;
                if (bin.nodeId >= 0 && bin.nodeId < graph.getNodeCount())
                {
                    binPos = graph.getNode(bin.nodeId).position;
                }

                double distMeters = currentPos.distanceTo(binPos) * 20.0;     // Scaled meters
                double travelTimeSec = distMeters / (30.0 * 1000.0 / 3600.0); // Approx at 30 km/h

                // Priority-based Nearest Neighbour metric:
                // Score = (Urgency + epsilon)^alpha / (TravelTime + delta)^beta
                double urgencyTerm = std::pow(std::max(0.01, bin.urgencyScore + 0.1), alpha);
                double distanceTerm = std::pow(std::max(1.0, travelTimeSec + 5.0), beta);

                double score = urgencyTerm / distanceTerm;

                if (score > bestScore)
                {
                    bestScore = score;
                    bestIdx = static_cast<int>(i);
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
            }
            else
            {
                // Fallback: append remaining
                for (int remainingId : unvisited)
                {
                    orderedTour.push_back(remainingId);
                }
                break;
            }
        }

        return orderedTour;
    }

} // namespace dhaka
