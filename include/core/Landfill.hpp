#pragma once

#include "Types.hpp"
#include <string>
#include <algorithm>

namespace dhaka
{

    class Landfill
    {
    public:
        int id = -1;
        int nodeId = -1;
        std::string name;
        std::string shortName;
        Corporation corporationServed = Corporation::DNCC;
        Vec2 position;

        double dailyCapacityKg = 30000.0;
        double currentIntakeKg = 0.0;
        int queueCount = 0;
        double unloadingRateKgPerMin = 200.0; // Speed at which a truck can dump waste

        Landfill() = default;

        Landfill(int id_, int nodeId_, const std::string &name_, const std::string &shortName_,
                 Corporation corp_, const Vec2 &pos_, double dailyCapKg, double unloadingRate)
            : id(id_), nodeId(nodeId_), name(name_), shortName(shortName_),
              corporationServed(corp_), position(pos_), dailyCapacityKg(dailyCapKg),
              unloadingRateKgPerMin(unloadingRate) {}

        double getIntakePercentage() const
        {
            return dailyCapacityKg > 0.0 ? (currentIntakeKg / dailyCapacityKg) * 100.0 : 0.0;
        }

        double getRemainingCapacity() const
        {
            return std::max(0.0, dailyCapacityKg - currentIntakeKg);
        }

        bool isFull() const
        {
            return currentIntakeKg >= dailyCapacityKg;
        }

        bool canAccept(double amountKg) const
        {
            return (currentIntakeKg + amountKg) <= dailyCapacityKg;
        }

        bool dumpWaste(double amountKg)
        {
            if (canAccept(amountKg))
            {
                currentIntakeKg += amountKg;
                return true;
            }
            currentIntakeKg = dailyCapacityKg;
            return false;
        }

        void resetDaily()
        {
            currentIntakeKg = 0.0;
            queueCount = 0;
        }
    };

} // namespace dhaka
