#include "algorithms/KnapsackDP.hpp"
#include <chrono>
#include <cmath>
#include <algorithm>

namespace dhaka
{

    KnapsackResult KnapsackDP::solve(
        double vehicleRemainingCapKg,
        const std::vector<const CollectionPoint *> &candidateBins,
        double weightUnitKg)
    {
        auto startTime = std::chrono::high_resolution_clock::now();

        KnapsackResult result;
        result.vehicleCapacityKg = vehicleRemainingCapKg;

        if (vehicleRemainingCapKg <= 10.0 || candidateBins.empty())
        {
            return result;
        }

        // Discretized capacity
        int W = static_cast<int>(std::floor(vehicleRemainingCapKg / weightUnitKg));
        if (W <= 0)
            return result;
        result.capacityUnits = W;

        // Filter and prepare items
        std::vector<KnapsackItem> items;
        for (const auto *bin : candidateBins)
        {
            if (!bin || bin->isAssigned || bin->currentWasteKg < 10.0)
                continue;

            int wt = std::max(1, static_cast<int>(std::round(bin->currentWasteKg / weightUnitKg)));
            if (wt > W)
            {
                // Even alone, this bin exceeds entire remaining truck capacity
                continue;
            }

            // Composite value: Waste volume prioritized by urgency score
            // High urgency score heavily boosts priority
            int val = static_cast<int>(std::round(bin->currentWasteKg * (1.0 + 3.0 * bin->urgencyScore)));

            items.push_back({bin->id, bin->currentWasteKg, wt, val, bin->urgencyScore, bin->name});
        }

        int n = static_cast<int>(items.size());
        result.itemsConsidered = n;

        if (n == 0)
            return result;

        // Allocate 2D Dynamic Programming matrix: (n + 1) x (W + 1)
        std::vector<std::vector<int>> dp(n + 1, std::vector<int>(W + 1, 0));

        for (int i = 1; i <= n; ++i)
        {
            int wt = items[i - 1].scaledWeight;
            int val = items[i - 1].value;

            for (int w = 0; w <= W; ++w)
            {
                if (wt <= w)
                {
                    dp[i][w] = std::max(dp[i - 1][w], dp[i - 1][w - wt] + val);
                }
                else
                {
                    dp[i][w] = dp[i - 1][w];
                }
            }
        }

        result.totalValue = dp[n][W];

        // Backtrack to reconstruct selected items
        int w = W;
        for (int i = n; i >= 1; --i)
        {
            if (dp[i][w] != dp[i - 1][w])
            {
                result.selectedBinIds.push_back(items[i - 1].binId);
                result.totalWeightKg += items[i - 1].originalWeightKg;
                w -= items[i - 1].scaledWeight;
            }
        }

        // Reverse to maintain natural order
        std::reverse(result.selectedBinIds.begin(), result.selectedBinIds.end());
        result.itemsSelected = static_cast<int>(result.selectedBinIds.size());

        auto endTime = std::chrono::high_resolution_clock::now();
        result.executionTimeMs = std::chrono::duration<double, std::milli>(endTime - startTime).count();

        return result;
    }

    KnapsackResult KnapsackDP::solve(
        double vehicleRemainingCapKg,
        const std::vector<CollectionPoint> &allBins,
        const std::vector<int> &candidateBinIds,
        double weightUnitKg)
    {
        std::vector<const CollectionPoint *> ptrs;
        ptrs.reserve(candidateBinIds.size());
        for (int id : candidateBinIds)
        {
            if (id >= 0 && id < static_cast<int>(allBins.size()))
            {
                ptrs.push_back(&allBins[id]);
            }
        }
        return solve(vehicleRemainingCapKg, ptrs, weightUnitKg);
    }

} // namespace dhaka
