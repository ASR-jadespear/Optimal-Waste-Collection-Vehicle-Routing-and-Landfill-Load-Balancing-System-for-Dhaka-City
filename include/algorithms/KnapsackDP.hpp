#pragma once

#include "core/CollectionPoint.hpp"
#include <vector>
#include <string>

namespace dhaka
{

    struct KnapsackItem
    {
        int binId = -1;
        double originalWeightKg = 0.0;
        int scaledWeight = 0; // Integer weight for DP table
        int value = 0;        // Composite value based on urgency & waste
        double urgencyScore = 0.0;
        std::string name;
    };

    struct KnapsackResult
    {
        std::vector<int> selectedBinIds;
        double totalWeightKg = 0.0;
        int totalValue = 0;
        int itemsConsidered = 0;
        int itemsSelected = 0;
        int capacityUnits = 0;
        double vehicleCapacityKg = 0.0;
        double executionTimeMs = 0.0;
    };

    class KnapsackDP
    {
    public:
        // Solves the 0/1 Knapsack problem for a truck with given remaining capacity
        // Discretizes continuous weights by weightUnitKg (default 25.0 kg per unit)
        static KnapsackResult solve(
            double vehicleRemainingCapKg,
            const std::vector<const CollectionPoint *> &candidateBins,
            double weightUnitKg = 25.0);

        // Overload accepting CollectionPoint value objects
        static KnapsackResult solve(
            double vehicleRemainingCapKg,
            const std::vector<CollectionPoint> &allBins,
            const std::vector<int> &candidateBinIds,
            double weightUnitKg = 25.0);
    };

} // namespace dhaka
