#include "algorithms/KnapsackDP.hpp"
#include "algorithms/AlgorithmTrace.hpp"
#include <chrono>
#include <cmath>
#include <algorithm>
#include <sstream>
#include <iomanip>

namespace dhaka
{

    KnapsackResult KnapsackDP::solve(
        double vehicleRemainingCapKg,
        const std::vector<const CollectionPoint *> &candidateBins,
        double weightUnitKg,
        AlgorithmTrace *traceOut,
        int vehicleId)
    {
        auto startTime = std::chrono::high_resolution_clock::now();

        KnapsackResult result;
        result.vehicleCapacityKg = vehicleRemainingCapKg;

        if (traceOut)
        {
            traceOut->clear();
            traceOut->type = TraceType::LOAD_SELECT_KNAPSACK;
            std::ostringstream ss;
            ss << "0/1 Knapsack DP Load Selection (Truck " << (vehicleId >= 0 ? std::to_string(vehicleId + 1) : "")
               << ", Capacity: " << static_cast<int>(vehicleRemainingCapKg) << " kg)";
            traceOut->title = ss.str();
        }

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
                continue;
            }

            int val = static_cast<int>(std::round(bin->currentWasteKg * (1.0 + 3.0 * bin->urgencyScore)));
            items.push_back({bin->id, bin->currentWasteKg, wt, val, bin->urgencyScore, bin->name});
        }

        int n = static_cast<int>(items.size());
        result.itemsConsidered = n;

        if (n == 0)
            return result;

        if (traceOut)
        {
            AlgorithmStep step;
            step.type = TraceType::LOAD_SELECT_KNAPSACK;
            step.activeVehicleId = vehicleId;
            step.remainingCapacityKg = vehicleRemainingCapKg;
            std::ostringstream ss;
            ss << "Evaluating " << n << " candidate bins for Truck " << (vehicleId >= 0 ? std::to_string(vehicleId + 1) : "")
               << ". Capacity W = " << static_cast<int>(vehicleRemainingCapKg) << " kg (" << W << " discrete units).";
            step.narration = ss.str();
            traceOut->addStep(step);
        }

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
        std::vector<int> chosenBins;
        std::vector<int> rejectedBins;
        double currentPayload = 0.0;

        for (int i = n; i >= 1; --i)
        {
            bool kept = (dp[i][w] != dp[i - 1][w]);
            int binId = items[i - 1].binId;

            if (kept)
            {
                chosenBins.push_back(binId);
                result.selectedBinIds.push_back(binId);
                result.totalWeightKg += items[i - 1].originalWeightKg;
                currentPayload += items[i - 1].originalWeightKg;
                w -= items[i - 1].scaledWeight;
            }
            else
            {
                rejectedBins.push_back(binId);
            }

            if (traceOut && traceOut->steps.size() < 30)
            {
                AlgorithmStep step;
                step.type = TraceType::LOAD_SELECT_KNAPSACK;
                step.activeBinId = binId;
                step.activeVehicleId = vehicleId;
                step.knapsackSelectedBins = chosenBins;
                step.knapsackRejectedBins = rejectedBins;
                step.evaluatingItemIndex = i;
                step.payloadWeightKg = currentPayload;
                step.remainingCapacityKg = std::max(0.0, vehicleRemainingCapKg - currentPayload);
                step.accumulatedValue = dp[i][w];
                step.itemKept = kept;

                std::ostringstream ss;
                ss << "Truck " << (vehicleId >= 0 ? std::to_string(vehicleId + 1) : "fleet")
                   << " evaluating " << items[i - 1].name << " — urgency "
                   << std::fixed << std::setprecision(2) << items[i - 1].urgencyScore
                   << ", adds " << static_cast<int>(items[i - 1].originalWeightKg) << "kg, "
                   << "capacity remaining " << static_cast<int>(step.remainingCapacityKg) << "kg -> "
                   << (kept ? "KEEP (adds value)" : "SKIP (sub-optimal)");
                step.narration = ss.str();
                traceOut->addStep(step);
            }
        }

        std::reverse(result.selectedBinIds.begin(), result.selectedBinIds.end());
        result.itemsSelected = static_cast<int>(result.selectedBinIds.size());

        auto endTime = std::chrono::high_resolution_clock::now();
        result.executionTimeMs = std::chrono::duration<double, std::milli>(endTime - startTime).count();

        if (traceOut)
        {
            AlgorithmStep step;
            step.type = TraceType::LOAD_SELECT_KNAPSACK;
            step.activeVehicleId = vehicleId;
            step.knapsackSelectedBins = result.selectedBinIds;
            step.knapsackRejectedBins = rejectedBins;
            step.payloadWeightKg = result.totalWeightKg;
            step.remainingCapacityKg = std::max(0.0, vehicleRemainingCapKg - result.totalWeightKg);
            step.accumulatedValue = result.totalValue;

            std::ostringstream ss;
            ss << "0/1 Knapsack DP optimal solution settled: selected " << result.itemsSelected
               << " bins (" << static_cast<int>(result.totalWeightKg) << " kg packed, "
               << static_cast<int>(step.remainingCapacityKg) << " kg slack, DP value "
               << result.totalValue << ") in " << std::fixed << std::setprecision(2)
               << result.executionTimeMs << " ms.";
            step.narration = ss.str();
            traceOut->addStep(step);
        }

        return result;
    }

    KnapsackResult KnapsackDP::solve(
        double vehicleRemainingCapKg,
        const std::vector<CollectionPoint> &allBins,
        const std::vector<int> &candidateBinIds,
        double weightUnitKg,
        AlgorithmTrace *traceOut,
        int vehicleId)
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
        return solve(vehicleRemainingCapKg, ptrs, weightUnitKg, traceOut, vehicleId);
    }

} // namespace dhaka
