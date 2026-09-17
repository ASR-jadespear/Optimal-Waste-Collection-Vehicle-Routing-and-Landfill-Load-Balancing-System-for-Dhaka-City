#include "simulation/DhakaMapData.hpp"

namespace dhaka
{

    SimulationData DhakaMapBuilder::buildDhakaNetwork()
    {
        SimulationData data;
        auto &g = data.graph;

        // -------------------------------------------------------------------------
        // 1. Waterways & Geography (Decorative Context)
        // -------------------------------------------------------------------------
        // Turag River (West boundary)
        data.waterways.push_back({{120.0f, 180.0f}, {140.0f, 280.0f}, {150.0f, 380.0f}, {180.0f, 460.0f}, {230.0f, 540.0f}, {300.0f, 620.0f}});

        // Buriganga River (South boundary)
        data.waterways.push_back({{350.0f, 740.0f}, {440.0f, 820.0f}, {550.0f, 880.0f}, {680.0f, 890.0f}, {800.0f, 870.0f}, {920.0f, 890.0f}});

        // Hatirjheel Lake (Central)
        data.waterways.push_back({{590.0f, 480.0f}, {640.0f, 470.0f}, {710.0f, 490.0f}, {680.0f, 520.0f}, {620.0f, 510.0f}, {590.0f, 480.0f}});

        data.districtLabels.push_back({"DNCC (Dhaka North)", {580.0f, 70.0f}});
        data.districtLabels.push_back({"DSCC (Dhaka South)", {580.0f, 920.0f}});

        // -------------------------------------------------------------------------
        // 2. Road Network Nodes (Intersections, Landfills, Depots, Bins)
        // -------------------------------------------------------------------------
        // Landfills
        int nAminbazarLandfill = g.addNode("Aminbazar Landfill Terminal", {160.0f, 410.0f}, NodeType::LANDFILL, 0);
        int nMatuailLandfill = g.addNode("Matuail Landfill Terminal", {900.0f, 840.0f}, NodeType::LANDFILL, 1);

        // Depots
        int nUttaraDepot = g.addNode("DNCC Uttara Fleet Depot", {600.0f, 120.0f}, NodeType::DEPOT, 0);
        int nDholaiDepot = g.addNode("DSCC Dholai Khal Depot", {610.0f, 820.0f}, NodeType::DEPOT, 1);

        // North Hubs (DNCC)
        int nAirport = g.addNode("Airport Roundabout", {600.0f, 190.0f});
        int nKuril = g.addNode("Kuril Flyover Hub", {680.0f, 250.0f});
        int nBanani = g.addNode("Banani Chairmanbari", {610.0f, 310.0f});
        int nGulshan2 = g.addNode("Gulshan 2 Circle", {710.0f, 310.0f});
        int nGulshan1 = g.addNode("Gulshan 1 Circle", {700.0f, 390.0f});
        int nMohakhali = g.addNode("Mohakhali Inter-district", {570.0f, 380.0f});

        int nMirpur12 = g.addNode("Mirpur 12 Bus Stand", {400.0f, 220.0f});
        int nMirpur10 = g.addNode("Mirpur 10 Roundabout", {430.0f, 300.0f});
        int nMirpur1 = g.addNode("Mirpur 1 Sony Square", {350.0f, 360.0f});
        int nGabtoli = g.addNode("Gabtoli Bus Terminal", {270.0f, 420.0f});
        int nKalyanpur = g.addNode("Kalyanpur / Shyamoli", {370.0f, 450.0f});
        int nAgargaon = g.addNode("Agargaon Computer City", {460.0f, 430.0f});

        // Central Corridor
        int nBijoySarani = g.addNode("Bijoy Sarani Intersection", {490.0f, 490.0f});
        int nFarmgate = g.addNode("Farmgate Khamarbari", {510.0f, 540.0f});
        int nKarwanBazar = g.addNode("Karwan Bazar Wholesale", {530.0f, 590.0f});
        int nMoghbazar = g.addNode("Moghbazar Wireless", {620.0f, 590.0f});
        int nKakrail = g.addNode("Kakrail Mosque Mor", {630.0f, 660.0f});

        int nDhanmondi27 = g.addNode("Dhanmondi 27 Mirpur Rd", {410.0f, 550.0f});
        int nDhanmondi32 = g.addNode("Dhanmondi 32 Lake Road", {430.0f, 610.0f});
        int nScienceLab = g.addNode("Science Lab Intersection", {450.0f, 670.0f});
        int nShahbagh = g.addNode("Shahbagh Intersections", {540.0f, 670.0f});

        // South Hubs (DSCC)
        int nHighCourt = g.addNode("High Court / Press Club", {580.0f, 720.0f});
        int nMotijheel = g.addNode("Motijheel Shapla Chattar", {660.0f, 730.0f});
        int nSayedabad = g.addNode("Sayedabad Bus Terminal", {730.0f, 760.0f});
        int nJatrabari = g.addNode("Jatrabari Flyover Base", {780.0f, 810.0f});
        int nDemraJunction = g.addNode("Demra Road Junction", {840.0f, 820.0f});

        int nLalbagh = g.addNode("Lalbagh Fort / Azimpur", {460.0f, 760.0f});
        int nChawkbazar = g.addNode("Chawkbazar Historic Mor", {510.0f, 800.0f});
        int nSadarghat = g.addNode("Sadarghat River Launch", {550.0f, 850.0f});

        // -------------------------------------------------------------------------
        // 3. Road Network Edges (Arterials, Avenues, Local Streets)
        // -------------------------------------------------------------------------
        // Airport & Pragati Sarani Corridor
        g.addEdge(nUttaraDepot, nAirport, 2800.0, 50.0, "Dhaka-Mymensingh Hwy");
        g.addEdge(nAirport, nKuril, 3100.0, 55.0, "Airport Expressway");
        g.addEdge(nKuril, nGulshan2, 2600.0, 40.0, "Pragati Sarani North");
        g.addEdge(nGulshan2, nGulshan1, 1800.0, 35.0, "Gulshan Ave");
        g.addEdge(nKuril, nBanani, 2200.0, 45.0, "Airport Road - Banani");
        g.addEdge(nBanani, nMohakhali, 1900.0, 40.0, "Bir Uttam AK Khandakar Rd");

        // Mirpur Corridor
        g.addEdge(nMirpur12, nMirpur10, 2400.0, 35.0, "Begum Rokeya Sarani North");
        g.addEdge(nMirpur10, nMirpur1, 2100.0, 30.0, "Mirpur Road Branch");
        g.addEdge(nMirpur1, nGabtoli, 2500.0, 35.0, "Mazar Road");
        g.addEdge(nGabtoli, nAminbazarLandfill, 3200.0, 50.0, "Dhaka-Aricha Hwy / Turag Bridge");
        g.addEdge(nMirpur1, nKalyanpur, 2200.0, 30.0, "Darussalam Road");
        g.addEdge(nMirpur10, nAgargaon, 2600.0, 40.0, "Begum Rokeya Sarani Metro Corridor");
        g.addEdge(nAgargaon, nBijoySarani, 1800.0, 40.0, "Rokeya Sarani South");

        // East-West Connectors
        g.addEdge(nMohakhali, nGulshan1, 1600.0, 30.0, "Gulshan Link Road");
        g.addEdge(nMohakhali, nBijoySarani, 2400.0, 35.0, "Jahangir Gate - Old Airport");
        g.addEdge(nBijoySarani, nFarmgate, 1400.0, 25.0, "Khamarbari Road");
        g.addEdge(nKalyanpur, nDhanmondi27, 2800.0, 30.0, "Mirpur Road Middle");
        g.addEdge(nDhanmondi27, nFarmgate, 2000.0, 35.0, "Manik Mia Ave (Parliament)");

        // Central Spine (Kazi Nazrul Islam Ave)
        g.addEdge(nMohakhali, nFarmgate, 2200.0, 30.0, "Mohakhali-Farmgate Arterial");
        g.addEdge(nFarmgate, nKarwanBazar, 1500.0, 25.0, "Kazi Nazrul Islam Ave");
        g.addEdge(nKarwanBazar, nShahbagh, 1900.0, 25.0, "Kazi Nazrul Islam Ave South");
        g.addEdge(nKarwanBazar, nMoghbazar, 1700.0, 25.0, "Tejgaon-Moghbazar Link");
        g.addEdge(nGulshan1, nMoghbazar, 2800.0, 35.0, "Hatirjheel Promenade");
        g.addEdge(nMoghbazar, nKakrail, 1600.0, 25.0, "Outer Circular Road");

        // Dhanmondi & West Spine
        g.addEdge(nDhanmondi27, nDhanmondi32, 1200.0, 30.0, "Mirpur Road - Russell Sq");
        g.addEdge(nDhanmondi32, nScienceLab, 1400.0, 25.0, "Mirpur Road South");
        g.addEdge(nScienceLab, nShahbagh, 1800.0, 25.0, "Elephant Road");
        g.addEdge(nScienceLab, nLalbagh, 2300.0, 25.0, "Azimpur Road");

        // South Core (DSCC & Old Dhaka)
        g.addEdge(nShahbagh, nHighCourt, 1600.0, 30.0, "Kazi Nazrul Islam Ave Extension");
        g.addEdge(nHighCourt, nMotijheel, 1500.0, 30.0, "Topkhana - Bangabandhu Ave");
        g.addEdge(nKakrail, nMotijheel, 1800.0, 25.0, "Bijoynagar - DIT Ave");
        g.addEdge(nMotijheel, nSayedabad, 2400.0, 30.0, "Kamalapur - Atish Dipankar Rd");
        g.addEdge(nSayedabad, nJatrabari, 1500.0, 35.0, "Mayor Hanif Flyover North");
        g.addEdge(nJatrabari, nDemraJunction, 2200.0, 45.0, "Dhaka-Chittagong Expressway");
        g.addEdge(nDemraJunction, nMatuailLandfill, 2500.0, 50.0, "Matuail Access Highway");

        // Old Dhaka Historic Grid
        g.addEdge(nHighCourt, nLalbagh, 2100.0, 20.0, "Shahid Minar - Bakshibazar Rd");
        g.addEdge(nLalbagh, nChawkbazar, 1200.0, 15.0, "Lalbagh Historic Lane");
        g.addEdge(nChawkbazar, nSadarghat, 1600.0, 15.0, "Chawk Circular Road");
        g.addEdge(nSadarghat, nDholaiDepot, 1400.0, 20.0, "Dholai Khal Canal Road");
        g.addEdge(nDholaiDepot, nSayedabad, 2000.0, 25.0, "Dholai Khal Link Road");
        g.addEdge(nChawkbazar, nDholaiDepot, 1800.0, 20.0, "English Road");
        g.addEdge(nDholaiDepot, nJatrabari, 2600.0, 25.0, "Gandaria - Jurain Road");

        // -------------------------------------------------------------------------
        // 4. Landfills (Aminbazar & Matuail)
        // -------------------------------------------------------------------------
        data.landfills.push_back({
            0, nAminbazarLandfill,
            "Aminbazar Sanitary Landfill", "Aminbazar",
            Corporation::DNCC, g.getNode(nAminbazarLandfill).position,
            35000.0, 300.0 // 35 Ton daily limit, 300 kg/min dump rate
        });

        data.landfills.push_back({
            1, nMatuailLandfill,
            "Matuail Sanitary Landfill", "Matuail",
            Corporation::DSCC, g.getNode(nMatuailLandfill).position,
            35000.0, 300.0 // 35 Ton daily limit, 300 kg/min dump rate
        });

        // -------------------------------------------------------------------------
        // 5. Community Bins & Secondary Transfer Stations (STS)
        // -------------------------------------------------------------------------
        // Helper lambda to register bin
        auto registerBin = [&](int binId, int nodeId, const std::string &name,
                               Corporation corp, double initWaste, double cap, double rate)
        {
            data.bins.emplace_back(
                binId, nodeId, name, corp, g.getNode(nodeId).position,
                initWaste, cap, rate);
            // Mark node as collection point
            g.nodes[nodeId].type = NodeType::COLLECTION_POINT;
            g.nodes[nodeId].entityId = binId;
        };

        // DNCC Bins (North)
        registerBin(0, nUttaraDepot, "Uttara Sector 3 STS", Corporation::DNCC, 850.0, 1500.0, 60.0);
        registerBin(1, nAirport, "Airport Roundabout Bin", Corporation::DNCC, 620.0, 1200.0, 45.0);
        registerBin(2, nKuril, "Kuril Flyover Community Bin", Corporation::DNCC, 980.0, 1400.0, 55.0);
        registerBin(3, nMirpur12, "Mirpur 12 STS", Corporation::DNCC, 1250.0, 1800.0, 80.0);
        registerBin(4, nMirpur10, "Mirpur 10 Circle Dumpster", Corporation::DNCC, 1750.0, 2000.0, 110.0); // Overflow hazard
        registerBin(5, nMirpur1, "Mirpur 1 Sony Cinema STS", Corporation::DNCC, 900.0, 1500.0, 65.0);
        registerBin(6, nGabtoli, "Gabtoli Cattle Market STS", Corporation::DNCC, 2100.0, 2500.0, 130.0); // Very high
        registerBin(7, nKalyanpur, "Kalyanpur Bus Stand Bin", Corporation::DNCC, 750.0, 1200.0, 50.0);
        registerBin(8, nAgargaon, "Agargaon Computer City STS", Corporation::DNCC, 680.0, 1400.0, 40.0);
        registerBin(9, nBanani, "Banani Road 11 Community Bin", Corporation::DNCC, 540.0, 1100.0, 35.0);
        registerBin(10, nGulshan2, "Gulshan 2 Diplomatic Zone Bin", Corporation::DNCC, 480.0, 1000.0, 30.0);
        registerBin(11, nGulshan1, "Gulshan 1 DCC Market STS", Corporation::DNCC, 1400.0, 1800.0, 85.0);
        registerBin(12, nMohakhali, "Mohakhali Wireless Bus STS", Corporation::DNCC, 1600.0, 2000.0, 95.0);

        // DSCC Bins (South)
        registerBin(13, nFarmgate, "Farmgate Ananda Cinema STS", Corporation::DSCC, 1550.0, 1800.0, 90.0);
        registerBin(14, nKarwanBazar, "Karwan Bazar Wholesale STS", Corporation::DSCC, 2400.0, 2500.0, 160.0); // Critical overflow!
        registerBin(15, nMoghbazar, "Moghbazar Chourangi Bin", Corporation::DSCC, 880.0, 1300.0, 55.0);
        registerBin(16, nDhanmondi27, "Dhanmondi 27 STS", Corporation::DSCC, 1100.0, 1600.0, 70.0);
        registerBin(17, nDhanmondi32, "Dhanmondi 32 Lake View Bin", Corporation::DSCC, 720.0, 1200.0, 45.0);
        registerBin(18, nScienceLab, "Science Lab City College STS", Corporation::DSCC, 1300.0, 1600.0, 75.0);
        registerBin(19, nShahbagh, "Shahbagh BSMMU Hospital STS", Corporation::DSCC, 1450.0, 1700.0, 85.0);
        registerBin(20, nMotijheel, "Motijheel Shapla Chattar STS", Corporation::DSCC, 1050.0, 1500.0, 60.0);
        registerBin(21, nSayedabad, "Sayedabad Terminal STS", Corporation::DSCC, 1800.0, 2200.0, 105.0);
        registerBin(22, nLalbagh, "Lalbagh Historic Fort STS", Corporation::DSCC, 920.0, 1400.0, 55.0);
        registerBin(23, nChawkbazar, "Chawkbazar Wholesale Dumpster", Corporation::DSCC, 2350.0, 2400.0, 150.0); // Critical overflow!
        registerBin(24, nSadarghat, "Sadarghat River Launch STS", Corporation::DSCC, 2150.0, 2300.0, 140.0);     // Critical overflow!

        // -------------------------------------------------------------------------
        // 6. Fleet of Waste Collection Vehicles
        // -------------------------------------------------------------------------
        // DNCC Fleet (North - based at Uttara Depot or Gabtoli)
        data.vehicles.emplace_back(0, "DNCC Compactor #1", Corporation::DNCC,
                                   nUttaraDepot, g.getNode(nUttaraDepot).position, 6000.0);
        data.vehicles.emplace_back(1, "DNCC Heavy Tipper #2", Corporation::DNCC,
                                   nUttaraDepot, g.getNode(nUttaraDepot).position, 5500.0);
        data.vehicles.emplace_back(2, "DNCC Mini Tipper #3", Corporation::DNCC,
                                   nGabtoli, g.getNode(nGabtoli).position, 4500.0);

        // DSCC Fleet (South - based at Dholai Khal Depot or Sayedabad)
        data.vehicles.emplace_back(3, "DSCC Compactor #4", Corporation::DSCC,
                                   nDholaiDepot, g.getNode(nDholaiDepot).position, 6500.0);
        data.vehicles.emplace_back(4, "DSCC Heavy Tipper #5", Corporation::DSCC,
                                   nDholaiDepot, g.getNode(nDholaiDepot).position, 5500.0);
        data.vehicles.emplace_back(5, "DSCC Compact Tipper #6", Corporation::DSCC,
                                   nSayedabad, g.getNode(nSayedabad).position, 4500.0);

        return data;
    }

} // namespace dhaka
