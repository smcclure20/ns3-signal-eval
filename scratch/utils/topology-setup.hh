#ifndef TOPO_SETUP_H
#define TOPO_SETUP_H

#include <random>
#include <vector>

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/traffic-control-module.h"
#include "string.h"

#include "sim-topology.hh"
#include "tracing-utils.hh"

using namespace ns3;

namespace Utils {

class ApplicationPattern
{
  public: 
    double onMean;
    double onBound;
    double offMean;
    double offBound;

  ApplicationPattern(double onMean, double onBound, double offMean, double offBound);

  std::string toString(void);
};

std::pair<std::string, std::string> splitBufferLen(QueueSize bufferLen, double deviceFraction = 0.9);

void topologySetup(Topology* topo, std::string queueType, std::string routeTableFile, std::vector<ApplicationPattern> appPatterns, 
                    std::vector<uint64_t> sendingRates, double errorRate, double simTime = 10, bool intEnabled = false, bool linkIntUil = false, 
                    int byteCounterInterval = 10,
                    bool traceNetDevices = true, bool traceHosts = true, 
                    bool isByteSwitched = false, AllScoreTracker* scoreTracker = NULL, void (*socketFunc) (void) = NULL );

}

#endif /* TOPO_SETUP_H */