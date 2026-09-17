#pragma once

#include <string>

namespace dhaka
{

    enum class DisruptionType
    {
        CONGESTION_SPIKE,  // Heavy gridlock (3x - 5x travel delay)
        ROAD_CLOSURE,      // Road completely impassable (infinite weight)
        BIN_OVERFLOW_SPIKE // Sudden surge in waste volume at a bin
    };

    struct DisruptionEvent
    {
        int id = -1;
        DisruptionType type = DisruptionType::CONGESTION_SPIKE;
        int targetId = -1; // Edge ID or CollectionPoint ID
        std::string name;
        std::string description;
        double severity = 3.0; // Multiplier or waste addition
        double timeTriggeredHours = 0.0;
        double durationMinutes = 30.0;
        bool isActive = true;
    };

} // namespace dhaka
