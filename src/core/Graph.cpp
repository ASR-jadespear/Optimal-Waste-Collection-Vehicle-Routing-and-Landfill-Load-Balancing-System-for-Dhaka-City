#include "core/Graph.hpp"
#include <cmath>
#include <limits>

namespace dhaka
{

    int Graph::addNode(const std::string &name, Vec2 pos, NodeType type, int entityId)
    {
        int id = static_cast<int>(nodes.size());
        nodes.push_back({id, name, pos, type, entityId});
        adjacency.emplace_back();
        return id;
    }

    int Graph::addEdge(int from, int to, double lengthMeters, double speedKmh,
                       const std::string &roadName, bool bidirectional)
    {
        int forwardId = static_cast<int>(edges.size());
        edges.push_back({forwardId, from, to, lengthMeters, speedKmh, 1.0, false, roadName});
        adjacency[from].push_back(forwardId);

        if (bidirectional)
        {
            int backwardId = static_cast<int>(edges.size());
            edges.push_back({backwardId, to, from, lengthMeters, speedKmh, 1.0, false, roadName});
            adjacency[to].push_back(backwardId);
        }

        return forwardId;
    }

    int Graph::findNearestNode(const Vec2 &pos) const
    {
        int bestNode = -1;
        float bestDistSq = std::numeric_limits<float>::max();

        for (const auto &node : nodes)
        {
            float d2 = (node.position - pos).lengthSquared();
            if (d2 < bestDistSq)
            {
                bestDistSq = d2;
                bestNode = node.id;
            }
        }
        return bestNode;
    }

    // Distance from point P to line segment AB
    static float pointToSegmentDistance(const Vec2 &p, const Vec2 &a, const Vec2 &b)
    {
        Vec2 ab = b - a;
        float l2 = ab.lengthSquared();
        if (l2 < 1e-6f)
            return p.distanceTo(a);
        float t = std::clamp(((p.x - a.x) * ab.x + (p.y - a.y) * ab.y) / l2, 0.0f, 1.0f);
        Vec2 projection = {a.x + t * ab.x, a.y + t * ab.y};
        return p.distanceTo(projection);
    }

    int Graph::findNearestEdge(const Vec2 &pos, float maxDist) const
    {
        int bestEdge = -1;
        float bestDist = maxDist;

        for (const auto &edge : edges)
        {
            const auto &nA = nodes[edge.fromNode];
            const auto &nB = nodes[edge.toNode];
            float d = pointToSegmentDistance(pos, nA.position, nB.position);
            if (d < bestDist)
            {
                bestDist = d;
                bestEdge = edge.id;
            }
        }
        return bestEdge;
    }

    int Graph::getEdgeBetween(int u, int v) const
    {
        if (u < 0 || u >= static_cast<int>(adjacency.size()))
            return -1;
        for (int edgeId : adjacency[u])
        {
            if (edges[edgeId].toNode == v)
            {
                return edgeId;
            }
        }
        return -1;
    }

    void Graph::setEdgeCongestion(int edgeId, double factor)
    {
        if (edgeId >= 0 && edgeId < static_cast<int>(edges.size()))
        {
            edges[edgeId].congestionFactor = std::max(1.0, factor);
            // Also update twin edge in reverse direction if present
            int u = edges[edgeId].fromNode;
            int v = edges[edgeId].toNode;
            int twinId = getEdgeBetween(v, u);
            if (twinId >= 0 && twinId != edgeId)
            {
                edges[twinId].congestionFactor = std::max(1.0, factor);
            }
        }
    }

    void Graph::setEdgeClosed(int edgeId, bool closed)
    {
        if (edgeId >= 0 && edgeId < static_cast<int>(edges.size()))
        {
            edges[edgeId].isClosed = closed;
            int u = edges[edgeId].fromNode;
            int v = edges[edgeId].toNode;
            int twinId = getEdgeBetween(v, u);
            if (twinId >= 0 && twinId != edgeId)
            {
                edges[twinId].isClosed = closed;
            }
        }
    }

    void Graph::resetAllCongestion()
    {
        for (auto &edge : edges)
        {
            edge.congestionFactor = 1.0;
            edge.isClosed = false;
        }
    }

} // namespace dhaka
