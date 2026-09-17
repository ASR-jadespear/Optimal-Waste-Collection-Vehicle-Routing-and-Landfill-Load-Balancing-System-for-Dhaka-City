# Optimal Waste Collection Vehicle Routing & Landfill Load-Balancing System for Dhaka City

> **CSE 4403: Algorithms — Assignment 2**  
> Dynamic Algorithmic Simulation & Digital Twin for Solid Waste Management across Dhaka North (DNCC) and Dhaka South (DSCC).

---

## 📌 Executive Summary & Problem Overview

Dhaka City faces systemic solid waste challenges: thousands of community dumpsters and Secondary Transfer Stations (STS) overflow while limited collection fleets navigate unpredictable traffic congestion, narrow roads, and disproportionate dump loads at the two terminal sanitary landfills: **Aminbazar** (North-West) and **Matuail** (South-East).

This project implements a real-time, algorithmic routing and load-balancing digital twin in **C++** using **Raylib** for visual rendering and interactive disruption simulation. The backend strictly decouples the data pipeline into **five core algorithms**:

```
[ Real-Time Waste Sensors / Reports ]
                │
                ▼
   1. Urgency Management (MergeSort + Priority Queue)
                │
                ▼
   2. Truck Service Subsets (0/1 Knapsack Dynamic Programming)
                │
                ▼
   3. Visit Order Sequencing (Greedy Priority Nearest Neighbour)
                │
                ▼
   4. Time-Varying Point-to-Point Routing (A* Algorithm with Euclidean Heuristic)
                │
                ▼
   5. Terminal Load Balancing (Max-Flow via Edmonds-Karp Residual Network)
```

---

## 🧠 Algorithmic Architecture

| Role | Algorithm | Complexity | Purpose |
| :--- | :--- | :--- | :--- |
| **Urgency Sorting** | **MergeSort** (Batch) & **Priority Queue** (Heap) | $O(N \log N)$ / $O(\log N)$ | Keeps community bins prioritized by overflow risk, dwell time, and fill percentage. |
| **Target Selection** | **0/1 Knapsack (Dynamic Programming)** | $O(N \cdot W)$ | Selects the optimal subset of bins maximizing collected waste and urgency without exceeding vehicle payload. |
| **Visit Order** | **Greedy (Priority Nearest Neighbour)** | $O(K^2)$ | Orders the knapsack subset to service critical overflow hazards first while minimizing travel detours. |
| **Point Routing** | **Time-Varying A* / Dijkstra** | $O(E + V \log V)$ | Computes fastest travel-time paths over time-varying road graph $G(V, E, t)$ with dynamic congestion and closures. |
| **Landfill Balancing** | **Max-Flow (Edmonds-Karp)** | $O(V \cdot E^2)$ | Models truck-to-landfill dispatch as a flow network to balance daily intake quotas between Aminbazar and Matuail. |

---

## 🗺️ Dhaka City Digital Twin Model

The road network $G(V, E, t)$ accurately models key geographical landmarks, arterial avenues, corporate jurisdictions, and waterways:

* **DNCC (North Dhaka):**
  * **Depot:** Uttara Fleet Depot (Node 2)
  * **Hubs & Corridors:** Airport Roundabout, Kuril Flyover, Banani, Gulshan 1 & 2, Mohakhali Inter-district Terminal, Mirpur 12, Mirpur 10, Mirpur 1, Kalyanpur, Agargaon.
  * **Terminal:** **Aminbazar Sanitary Landfill** (Node 0, West of Gabtoli across Turag River).
* **DSCC (South Dhaka):**
  * **Depot:** Dholai Khal Fleet Depot (Node 3)
  * **Hubs & Corridors:** Farmgate, Karwan Bazar, Moghbazar, Kakrail, Dhanmondi 27 & 32, Science Lab, Shahbagh, Motijheel Shapla Chattar, Sayedabad, Jatrabari Flyover, Lalbagh Fort, Chawkbazar, Sadarghat River Port.
  * **Terminal:** **Matuail Sanitary Landfill** (Node 1, South-East near Demra).
* **Waterways:** Turag River (Western border), Buriganga River (Southern border), Hatirjheel Lake (Central).

---

## 🚀 Zero-Dependency One-Click Quick Start

The project uses CMake with automatic **Raylib FetchContent fallback**. Anyone cloning the repository can build and run on Linux or Windows **without manually installing Raylib or external dependencies**!

### On Linux (Fedora, Ubuntu, Arch, Debian)

```bash
# Clone the repository
git clone https://github.com/ASR-jadespear/Optimal-Waste-Collection-Vehicle-Routing-and-Landfill-Load-Balancing-System-for-Dhaka-City.git
cd Optimal-Waste-Collection-Vehicle-Routing-and-Landfill-Load-Balancing-System-for-Dhaka-City

# One-click build and launch GUI:
bash run.sh

# Or run the automated console algorithm verification test suite:
bash test.sh
```

### On Windows (MSVC / MinGW)

Simply double-click `run.bat` or open PowerShell / Command Prompt:

```cmd
run.bat
```

*(CMake will automatically download and build Raylib 5.5 and compile the project into `build\Release\dhaka_waste_sim.exe`)*

---

## 🎮 Interactive Controls & Disruption Simulation

The simulation features real-time interactive disruptions to test dynamic algorithmic adaptation:

| Action | Control | Algorithmic Reaction |
| :--- | :--- | :--- |
| **Click on Road** | Left-Click on edge | Cycles road state: **Free (1.0x)** $\to$ **Congestion Spike (4.5x)** $\to$ **Emergency Closure** $\to$ **Free**. Trucks en-route detect the blockage and dynamically reroute via **A***. |
| **Click on Bin** | Left-Click on bin circle | Triggers sudden waste surge (+750 kg). Bin turns red, pulses hazard ring, updates **Priority Queue**, and triggers emergency fleet dispatch. |
| **Random Disruption** | `[R]` Key or Button | Injects random real-world Dhaka incidents (e.g. waterlogging on Begum Rokeya Sarani, VIP protocol on Airport Rd, market dump). |
| **Pause / Resume** | `[SPACE]` Key | Pauses/resumes the simulation clock. |
| **Simulation Speed** | `[1]`, `[2]`, `[3]`, `[4]` | Sets simulation speed to 1x, 2x, 5x, or 10x real time. |
| **Pan Map** | Right-Click Drag or `WASD` | Smoothly navigate around Dhaka City. |
| **Zoom Map** | Mouse Scroll Wheel | Zoom in to inspect local bins or zoom out for macro view. |
| **Center Map** | `[F]` Key | Re-centers camera to full Dhaka overview. |
| **Toggle HUD** | `[TAB]` Key | Shows or hides the right analytical dashboard sidebar. |
| **Toggle Paths** | `[P]` Key | Shows or hides active truck A* route polylines. |
| **Toggle Labels** | `[L]` Key | Shows or hides community bin and road name labels. |

---

## 📊 Analytical HUD & Live Dashboard

The interactive interface provides real-time visibility into the algorithmic pipeline:

1. **Top Metric Cards:** Live tracking of total pending waste (kg), total collected waste (kg), active overflow risks, and fleet dispatch count.
2. **Landfill Load-Balancing Gauge:** Dual intake progress bars for Aminbazar and Matuail with the Edmonds-Karp balance percentage.
3. **Algorithm Inspector:**
   * **MergeSort & Priority Queue:** Displays top 3 urgent community bins.
   * **0/1 Knapsack DP Monitor:** Displays DP matrix value achieved, candidate bins evaluated, items selected, and payload packed.
   * **Edmonds-Karp Max-Flow Monitor:** Displays augmenting paths count and balance status (Optimal 50/50 vs Compensating).
4. **Hover Cards:** Hover over any bin, road, vehicle, or landfill terminal to inspect detailed real-time statistics.

---

## 🧪 Console Verification Suite (`algo_tests`)

To verify the backend independently of graphics, run:

```bash
bash test.sh
```

Verifies:

* [x] **Test 1:** MergeSort and Priority Queue urgency ranking correctness ($U_0 \ge U_1 \ge \dots$).
* [x] **Test 2:** 0/1 Knapsack DP capacity constraints and composite value maximization.
* [x] **Test 3:** Greedy priority-based nearest neighbour tour sequencing without duplicates.
* [x] **Test 4:** Time-Varying A* vs Dijkstra shortest travel times and dynamic rerouting around closed edges.
* [x] **Test 5:** Edmonds-Karp Max-Flow balanced distribution (50/50) and saturation redirection.
* [x] **Test 6:** Full simulation clock ticks, vehicle movement, waste collection, and metrics tracking.

---

## 📁 Repository Structure

```
.
├── CMakeLists.txt              # Cross-platform CMake configuration with FetchContent Raylib
├── README.md                   # Comprehensive system documentation
├── run.sh                      # Linux one-click build and launch script
├── run.bat                     # Windows one-click build and launch script
├── test.sh                     # Automated console test launcher
├── include
│   ├── algorithms
│   │   ├── GreedyTSP.hpp       # Priority-based nearest neighbour
│   │   ├── KnapsackDP.hpp      # 0/1 Knapsack DP solver & table tracker
│   │   ├── MaxFlow.hpp         # Edmonds-Karp max-flow landfill balancer
│   │   ├── MergeSort.hpp       # Custom divide-and-conquer urgency sorter
│   │   ├── Pathfinding.hpp     # Time-varying A* & Dijkstra algorithms
│   │   └── PriorityQueue.hpp   # Event-driven urgency heap
│   ├── core
│   │   ├── CollectionPoint.hpp # Community waste bins & STS
│   │   ├── Disruption.hpp      # Real-time traffic & overflow disruptions
│   │   ├── Graph.hpp           # Road network G(V, E, t)
│   │   ├── Landfill.hpp        # Aminbazar & Matuail terminals
│   │   ├── Types.hpp           # Vector2, enumerations, constants
│   │   └── Vehicle.hpp         # Collection truck physics & state machine
│   ├── simulation
│   │   ├── DhakaMapData.hpp    # Authentic Dhaka GIS landmarks and routes
│   │   └── SimulationEngine.hpp# Dispatcher, clock, and event orchestrator
│   └── ui
│       ├── CameraController.hpp# 2D pan and zoom controls
│       ├── Renderer.hpp        # Raylib world renderer & interaction loop
│       └── UIComponents.hpp    # Cyber/Dark GIS dashboard widgets & gauges
├── src
│   ├── algorithms/             # Algorithm implementations
│   ├── core/                   # Graph, Vehicle, Types logic
│   ├── simulation/             # Dhaka map data & Simulation engine
│   ├── ui/                     # Camera, HUD, and Raylib renderer
│   └── main.cpp                # Application entry point (GUI + headless)
└── tests
    └── test_algorithms.cpp     # Standalone console test suite
```

---

## 👥 Authors & Academic Context

* **Institution:** Department of Computer Science and Engineering
* **Course:** CSE 4403 (Algorithms) — Assignment 2
* **Project:** Optimal Waste Collection Vehicle Routing & Landfill Load-Balancing System for Dhaka City
