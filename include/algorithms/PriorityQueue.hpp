#pragma once

#include "core/CollectionPoint.hpp"
#include <queue>
#include <vector>

namespace dhaka
{

    struct BinPriorityItem
    {
        int binId;
        double urgencyScore;
        double wasteKg;

        bool operator<(const BinPriorityItem &o) const
        {
            // Max-heap: highest urgency on top
            if (std::abs(urgencyScore - o.urgencyScore) > 1e-4)
            {
                return urgencyScore < o.urgencyScore;
            }
            return wasteKg < o.wasteKg; // Secondary tie-breaker: larger waste volume
        }
    };

    class UrgencyPriorityQueue
    {
    private:
        std::priority_queue<BinPriorityItem> pq;

    public:
        UrgencyPriorityQueue() = default;

        void push(int binId, double urgency, double wasteKg)
        {
            pq.push({binId, urgency, wasteKg});
        }

        void buildFromPoints(const std::vector<CollectionPoint> &points)
        {
            clear();
            for (const auto &pt : points)
            {
                if (!pt.isAssigned && pt.currentWasteKg > 10.0)
                {
                    pq.push({pt.id, pt.urgencyScore, pt.currentWasteKg});
                }
            }
        }

        bool empty() const
        {
            return pq.empty();
        }

        size_t size() const
        {
            return pq.size();
        }

        BinPriorityItem top() const
        {
            return pq.top();
        }

        BinPriorityItem pop()
        {
            BinPriorityItem topItem = pq.top();
            pq.pop();
            return topItem;
        }

        void clear()
        {
            while (!pq.empty())
                pq.pop();
        }
    };

} // namespace dhaka
