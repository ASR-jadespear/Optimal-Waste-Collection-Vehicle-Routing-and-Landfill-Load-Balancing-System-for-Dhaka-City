#pragma once

#include "core/Graph.hpp"
#include "core/CollectionPoint.hpp"
#include "core/Vehicle.hpp"
#include "core/Landfill.hpp"
#include <vector>

namespace dhaka
{

    struct SimulationData
    {
        Graph graph;
        std::vector<CollectionPoint> bins;
        std::vector<Vehicle> vehicles;
        std::vector<Landfill> landfills;

        // Landmark decorative lines (e.g. Turag River, Buriganga River, Hatirjheel)
        std::vector<std::vector<Vec2>> waterways;
        std::vector<std::pair<std::string, Vec2>> districtLabels;
    };

    class DhakaMapBuilder
    {
    public:
        static SimulationData buildDhakaNetwork();
    };

} // namespace dhaka
