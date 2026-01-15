#ifndef SIM_TOPOLOGY_H
#define SIM_TOPOLOGY_H

#include <ostream>
#include <vector>
#include <sstream>
#include <cmath>
#include "string.h"

namespace Utils
{

const int MAX_FANOUT = 32;

class TopologyNode 
{
public:
    int id;
    bool isEndHost;
    bool isUntracedHost;
    std::vector<int> neighbors;
    std::vector<int> routerNeighbors;
    bool isTraced[MAX_FANOUT] = { false };

    TopologyNode(int newId, bool isHost);

    void addNeighbor (int neighborId, bool isEndHost);
    bool isNeighbor( int nodeId );
    bool canAddNewNeighbor(int candidates);
    bool canAddNewRouterNeighbor(int candidates);
};

class Link 
{
public:
    TopologyNode * node1;
    TopologyNode * node2;
    std::string linkRate;
    std::string delay;
    std::string bufferLen;
    std::string addresses;
    std::string mask;

    Link(TopologyNode *n1, TopologyNode *n2, std::string rate, std::string propDelay, std::string qLen, std::string addr);
    std::string toString(void);
};

template <typename T>
class TopologyGenerator
{
public:
    virtual void GenerateTopology(T* param) = 0;
};

class TopologyParameter
{
public:
    bool is_deterministic;

    TopologyParameter() {};
    
    virtual std::string toString(void) = 0;
};

class TopologyConfig
{
public:
    TopologyConfig() {};

    virtual std::string toString(void) = 0;
};

class Topology
{
public:
    std::vector<Link> links;
    uint8_t addressBytes[4] = {10, 0, 100, 0};
    uint16_t currPort = 5000;
    int accessLinkRate;

    Topology() {};
    // ~Topology() {};
    std::string toString(void);
    uint16_t getPort();
    void clearState();
    virtual std::vector<TopologyNode*> getAllNodes() = 0;
    virtual std::vector<TopologyNode*> getHosts() = 0;
    virtual std::vector<std::vector<int>> getTrafficMatrix() = 0; // TODO: this should be a hashmap

protected:
    std::vector<TopologyNode*> allNodes;

    std::string getNewSubnet();
};

class RandomTopologyParameter: public TopologyParameter
{
public:
    int numEndHosts;
    int numRouters;

    RandomTopologyParameter(int nHosts, int nRouters);
    // ~RandomTopologyParameter() {};
    std::string toString(void);
};

class RandomTopology: public Topology, public TopologyGenerator<RandomTopologyParameter>
{
public:
    std::vector<TopologyNode*> endHostNodes;
    std::vector<TopologyNode*> routerNodes;
    std::vector<int> rateOptions;
    std::vector<int> delayOptions;
    std::vector<int> qlenOptions;

    RandomTopology (void);
    // ~RandomTopology () {};
    TopologyNode* getNodeById(int id);
    int getBufferLen(int rate);
    std::vector<TopologyNode*> getAllNodes();
    std::vector<TopologyNode*> getHosts();
    void GenerateTopology (RandomTopologyParameter* params);
    std::vector<std::vector<int>> getTrafficMatrix();

private:
    void addLink(TopologyNode *n1, TopologyNode *n2);
};

enum LinkType{
    HOST,
    TOR,
    AGGREGATOR
};

class DatacenterTopologyParameter: public TopologyParameter
{
public:
    int _k;

    DatacenterTopologyParameter(int k);
    std::string toString(void);
};

class DatacenterTopologyConfig: public TopologyConfig
{
public:
    double serverUplinkRate = 100;
    double torUplinkRate = 100;
    double aggUplinkRate = 100;
    double linkDelay = 0.001;
    int bufferLen = 100;

    DatacenterTopologyConfig(double serverRate=0, double torUpRate=0, double aggUpRate=0, double delay=0, int bufLen=0);

    std::string toString(void);
};

class DatacenterTopology: public Topology, public TopologyGenerator<DatacenterTopologyParameter>
{
public:
    std::vector<TopologyNode*> coreNodes;
    std::vector<TopologyNode*> aggNodes;
    std::vector<TopologyNode*> torNodes;
    std::vector<TopologyNode*> hostNodes;
    DatacenterTopologyConfig* config;

    DatacenterTopology (DatacenterTopologyConfig* cfg);
    TopologyNode* getNodeById(int id);
    std::vector<TopologyNode*> getAllNodes();
    std::vector<TopologyNode*> getHosts();
    void GenerateTopology (DatacenterTopologyParameter* params);
    std::vector<std::vector<int>> getTrafficMatrix();

private:
    int _k;

    void addLink(TopologyNode *n1, TopologyNode *n2, LinkType type);
};

class DumbbellTopologyParameter: public TopologyParameter
{
public:
    int _n; 
    bool hasExtraFlows;

    DumbbellTopologyParameter(int n);

    std::string toString(void);
};

class DumbbellTopologyConfig: public TopologyConfig
{
public:
    double bottleneckRate = 1;
    double bottleneckDelay = 10;
    int bottleneckBufferLen = 100;
    int hostBufferLen = 10000;
    double hostLinkRate = 1000000;

    DumbbellTopologyConfig(double btlRate=0, double btlDelay=0, int btlBuf=0, int hostBuf=0, double hostLinkRate=0);

    std::string toString(void);
};

class DumbbellTopology: public Topology, public TopologyGenerator<DumbbellTopologyParameter>
{
public:
    std::vector<TopologyNode*> leftNodes;
    std::vector<TopologyNode*> rightNodes;
    std::vector<TopologyNode*> linkNodes;
    std::vector<TopologyNode*> extraLeftNodes;
    std::vector<TopologyNode*> extraRightNodes;
    DumbbellTopologyConfig* config;
    bool hasExtraFlows;
    int numExtraSenders;
    int numSenders;

    DumbbellTopology (DumbbellTopologyConfig* cfg);
    TopologyNode* getNodeById(int id);
    std::vector<TopologyNode*> getAllNodes();
    std::vector<TopologyNode*> getHosts();
    void GenerateTopology (DumbbellTopologyParameter* params);
    std::vector<std::vector<int>> getTrafficMatrix();

private:
    int _n;

    void addLink(TopologyNode *n1, TopologyNode *n2);
};

class LineTopologyParameter: public TopologyParameter
{
public:
    int numRouters; 
    int numHostsPerRouter;
    int numSenders;
    bool hasExtraFlows;

    LineTopologyParameter(int routers, int hostsPerRouter, int nSenders);

    std::string toString(void);
};

class LineTopologyConfig: public TopologyConfig
{
public:
    double maxRtt = 1;
    double bottleneckRate = 10;
    int bufferLen = 100;
    int hostBufferLen = 10000;
    double hostLinkRate = 1000000;

    LineTopologyConfig(double maxDelay=0, double btlRate=0, int buf=0, int hostBuf=0, double hostRate=0);

    std::string toString(void);
};

class LineTopology: public Topology, public TopologyGenerator<LineTopologyParameter>
{
public:
    std::vector<TopologyNode*> hosts;
    std::vector<TopologyNode*> routers;
    LineTopologyConfig* config;
    int senders;
    int sender_hosts;
    bool hasExtraFlows;

    LineTopology (LineTopologyConfig* cfg);
    TopologyNode* getNodeById(int id);
    std::vector<TopologyNode*> getAllNodes();
    std::vector<TopologyNode*> getHosts();
    void GenerateTopology (LineTopologyParameter* params);
    std::vector<std::vector<int>> getTrafficMatrix();

private:
    int _n;

    void addLink(TopologyNode *n1, TopologyNode *n2);
};

enum TopologyType
{
    Random,
    Datacenter,
    Dumbbell,
    Line
};

class TopologyParameterFactory // TODO: Have the constructors use the constructor args (expand topology parameters to include these)
{
public:
    static TopologyParameter* CreateTopologyParameter(TopologyType topo_type, int arg1, int arg2=0, int arg3=0)
    {
        switch (topo_type)
        {
            case Random: 
                return new RandomTopologyParameter(arg1, arg2);
            case Datacenter:
                return new DatacenterTopologyParameter(arg1);
            case Dumbbell:
                return new DumbbellTopologyParameter(arg1);
            case Line:
                return new LineTopologyParameter(arg1, arg2, arg3);
            default:
                return NULL;
        }
    }
};

class TopologyFactory
{
public:
    static Topology* CreateTopology(TopologyType topo_type, TopologyConfig* config)
    {
        switch (topo_type)
        {
            case Random: 
                return new RandomTopology();
            case Datacenter:
                return new DatacenterTopology((DatacenterTopologyConfig*)config);
            case Dumbbell:
                return new DumbbellTopology((DumbbellTopologyConfig*)config);
            case Line:
                return new LineTopology((LineTopologyConfig*)config);
            default:
                return NULL;
        }
    }
};

class TopologyConfigFactory
{
public:
    static TopologyConfig* CreateTopologyConfig(TopologyType topo_type, double delay=0, int buffer=0) // TODO: add more
    {
        switch (topo_type)
        {
            case Random: 
                return NULL;
            case Datacenter:
                return new DatacenterTopologyConfig();
            case Dumbbell:
                return new DumbbellTopologyConfig(0, delay, buffer);
            case Line:
                return new LineTopologyConfig(delay, 0, buffer);
            default:
                return NULL;
        }
    }
};

}

#endif /* SIM_TOPOLOGY_H */

