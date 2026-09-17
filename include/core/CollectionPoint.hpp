#pragma once

#include "Types.hpp"
#include <string>
#include <algorithm>

namespace dhaka
{

    class CollectionPoint
    {
    public:
        int id = -1;
        int nodeId = -1;
        std::string name;
        Corporation corporation = Corporation::DNCC;
        Vec2 position;

        double currentWasteKg = 0.0;
        double capacityKg = 1000.0;
        double accumulationRateKgPerHour = 50.0;
        double urgencyScore = 0.0; // Normalized 0.0 to 1.0+ (>1.0 indicates critical overflow)
        double lastServicedTimeHours = 0.0;
        int overflowCount = 0;
        bool isAssigned = false; // Whether a vehicle is currently en route to collect this bin

        CollectionPoint() = default;

        CollectionPoint(int id_, int nodeId_, const std::string &name_, Corporation corp_,
                        const Vec2 &pos_, double initialWasteKg, double capKg, double rateKgPerHour)
            : id(id_), nodeId(nodeId_), name(name_), corporation(corp_), position(pos_),
              currentWasteKg(initialWasteKg), capacityKg(capKg), accumulationRateKgPerHour(rateKgPerHour)
        {
            recalculateUrgency(0.0);
        }

        double getFillRatio() const
        {
            return capacityKg > 0.0 ? (currentWasteKg / capacityKg) : 0.0;
        }

        bool isOverflowing() const
        {
            return currentWasteKg >= capacityKg;
        }

        void recalculateUrgency(double currentSimTimeHours)
        {
            double fillRatio = getFillRatio();
            double dwellHours = std::max(0.0, currentSimTimeHours - lastServicedTimeHours);

            // Urgency function:
            // 1. Fill ratio weight: 0.60
            // 2. Projected overflow weight: 0.25 (how soon it overflows based on accumulation rate)
            // 3. Dwell time weight: 0.15 (service fairness)
            // 4. Critical overflow surcharge if currentWaste >= capacity
            double projectedOverflowFactor = 0.0;
            if (capacityKg > currentWasteKg && accumulationRateKgPerHour > 0.0)
            {
                double hoursToOverflow = (capacityKg - currentWasteKg) / accumulationRateKgPerHour;
                projectedOverflowFactor = 1.0 / (1.0 + hoursToOverflow);
            }
            else if (isOverflowing())
            {
                projectedOverflowFactor = 1.0;
            }

            double dwellFactor = std::clamp(dwellHours / 12.0, 0.0, 1.0);

            urgencyScore = (0.60 * fillRatio) + (0.25 * projectedOverflowFactor) + (0.15 * dwellFactor);

            if (isOverflowing())
            {
                // Critical priority surcharge
                urgencyScore += 1.0 + ((currentWasteKg - capacityKg) / capacityKg);
            }
        }

        void update(double dtHours, double currentSimTimeHours)
        {
            currentWasteKg += accumulationRateKgPerHour * dtHours;
            if (isOverflowing())
            {
                overflowCount++;
            }
            recalculateUrgency(currentSimTimeHours);
        }

        void addWaste(double amountKg, double currentSimTimeHours)
        {
            currentWasteKg += amountKg;
            recalculateUrgency(currentSimTimeHours);
        }

        double emptyWaste(double currentSimTimeHours)
        {
            double collected = currentWasteKg;
            currentWasteKg = 0.0;
            lastServicedTimeHours = currentSimTimeHours;
            isAssigned = false;
            recalculateUrgency(currentSimTimeHours);
            return collected;
        }
    };

} // namespace dhaka
