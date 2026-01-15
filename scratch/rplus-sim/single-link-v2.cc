#include <stdlib.h>
#include <iostream>
#include <string>
#include <fstream>
#include <cmath>

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"

#include "../utils/sim-topology.hh"
#include "../utils/tracing-utils.hh"
#include "../utils/topology-setup.hh"
#include "../utils/topology-setup.hh"
#include "../utils/parameter-range.hh"
#include "../utils/network-config.hh"
 
using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("SingleLinkSimUtilVer");

void setSocketParams()
{
  Config::Set ("/NodeList/*/$ns3::TcpL4Protocol/SocketList/*/Timestamp", BooleanValue (true));
  Config::Set ("/NodeList/*/$ns3::TcpL4Protocol/SocketList/*/TimestampForRtt", BooleanValue (true));
  Config::Set ("/NodeList/*/$ns3::TcpL4Protocol/SocketList/*/Sack", BooleanValue (true));
  Config::Set ("/NodeList/*/$ns3::TcpL4Protocol/SocketList/*/SndBufSize", UintegerValue (6000 * 512));
  Config::Set ("/NodeList/*/$ns3::TcpL4Protocol/SocketList/*/RcvBufSize", UintegerValue (6000 * 512));
  Config::Set ("/NodeList/*/$ns3::TcpL4Protocol/SocketList/*/DelAckCount", UintegerValue (1));
  Config::Set("/NodeList/*/$ns3::TcpL4Protocol/SocketList/*/ClockGranularity", TimeValue (Time("1ns")));
  return;
}

void setSocketParamsWithInt() // TODO: Do this with arbitrary arguments instead
{
  Config::Set ("/NodeList/*/$ns3::TcpL4Protocol/SocketList/*/Timestamp", BooleanValue (true));
  Config::Set ("/NodeList/*/$ns3::TcpL4Protocol/SocketList/*/TimestampForRtt", BooleanValue (true));
  Config::Set ("/NodeList/*/$ns3::TcpL4Protocol/SocketList/*/EnableInt", BooleanValue (true));
  Config::Set ("/NodeList/*/$ns3::TcpL4Protocol/SocketList/*/Sack", BooleanValue (true));
  Config::Set ("/NodeList/*/$ns3::TcpL4Protocol/SocketList/*/SndBufSize", UintegerValue (6000 * 512));
  Config::Set ("/NodeList/*/$ns3::TcpL4Protocol/SocketList/*/RcvBufSize", UintegerValue (6000 * 512));
  Config::Set ("/NodeList/*/$ns3::TcpL4Protocol/SocketList/*/DelAckCount", UintegerValue (1));
  Config::Set("/NodeList/*/$ns3::TcpL4Protocol/SocketList/*/ClockGranularity", TimeValue (Time("1ns")));
  return;
}

double simulate (int num_senders, double onMean, double onBound, double offMean, double offBound, double linkRate, 
                  double linkDelay, double errorRate, int bufferLen, bool intEnabled, bool linkIntUtil, int byteCollectionInterval, int delayCoef, int tputCoef, 
                  bool byteSwitched, double simTime)
{
  // Setup score tracing
  Utils::AllScoreTracker scorer = Utils::AllScoreTracker(linkRate, num_senders);

  // // // Just need a dumbbell topology with these parameters
  Utils::DumbbellTopologyParameter* topo_desc = (Utils::DumbbellTopologyParameter*) Utils::TopologyParameterFactory::CreateTopologyParameter(Utils::TopologyType::Dumbbell, num_senders);
  // Utils::DumbbellTopologyConfig* config = (Utils::DumbbellTopologyConfig*) Utils::TopologyConfigFactory::CreateTopologyConfig(Utils::TopologyType::Dumbbell, linkDelay, bufferLen);
  Utils::DumbbellTopologyConfig config = Utils::DumbbellTopologyConfig(linkRate / pow(10, 9), linkDelay, bufferLen);
  Utils::DumbbellTopology* topo = (Utils::DumbbellTopology*) Utils::TopologyFactory::CreateTopology(Utils::TopologyType::Dumbbell, &config);
  topo->GenerateTopology(topo_desc);

  // TODO: Parameterize topology type
  // Line topology 
  // int hosts_per_router = 1;
  // // max_senders_on_link = (hosts_per_router * floor(num_routers/2)) * (hosts_per_router * ceil(num_routers/2)) = 
  // // int num_routers = std::ceil(std::sqrt((num_senders * 4)/pow(hosts_per_router,2)));
  // int num_routers = std::ceil((double)num_senders / (double)hosts_per_router) * 2;
  // Utils::LineTopologyParameter* topo_desc = (Utils::LineTopologyParameter*) Utils::TopologyParameterFactory::CreateTopologyParameter(Utils::TopologyType::Line, num_routers, hosts_per_router, num_senders);
  // Utils::LineTopologyConfig config = Utils::LineTopologyConfig(linkDelay, linkRate / pow(10, 9), bufferLen);
  // Utils::LineTopology* topo = (Utils::LineTopology*) Utils::TopologyFactory::CreateTopology(Utils::TopologyType::Line, &config);
  // topo->GenerateTopology(topo_desc);
  
  std::vector<Utils::ApplicationPattern> appPatterns = {Utils::ApplicationPattern(onMean, onBound, offMean, offBound)};
  std::vector<uint64_t> rates = {(uint64_t) linkRate};
  if (intEnabled)
  {
    topologySetup(topo, "ns3::DropTailQueue", "", appPatterns, rates, errorRate, simTime, intEnabled, linkIntUtil, byteCollectionInterval, false, false, byteSwitched, &scorer, &setSocketParamsWithInt);
  }
  else
  {
    topologySetup(topo, "ns3::DropTailQueue", "", appPatterns, rates, errorRate, simTime, intEnabled, linkIntUtil, byteCollectionInterval, false, false, byteSwitched, &scorer, &setSocketParams);
  }

  Simulator::Stop (MilliSeconds ((int)simTime)); // Give time for applications to actually stop for accounting purposes
  Simulator::Run ();
  Simulator::Destroy ();
  
  return scorer.score(simTime/1000.0, true, delayCoef, tputCoef);
}

int main (int argc, char *argv[]){
    CommandLine cmd (__FILE__);
    std::string cca;
    std::string netfile = "";
    std::string whiskerfile = "";
    int samplesize;
    int seed = time(NULL);
    int configruns = 1;
    bool intenabled = false;
    bool linkIntUtil = true;
    int linkInterval = 10;
    int delayCoef = 1;
    int tputCoef = 1;
    bool byteSwitched = false;
    double simTime = 11.0;
    bool saveWhiskerStats = false;
    cmd.AddValue ("cca", "cca", cca);
    cmd.AddValue ("netfile", "Network configuration file", netfile);
    cmd.AddValue ("samplesize", "number of network configurations", samplesize);
    cmd.AddValue ("whiskerfile", "Whiskers for RemyCCA.", whiskerfile);
    cmd.AddValue ("seed", "Seed for configuration randomization. If none, uses time.", seed);
    cmd.AddValue ("configruns", "Number of runs for each configuration.", configruns);
    cmd.AddValue ("intenabled", "Whether INT is enabled or not", intenabled);
    cmd.AddValue ("linkintutil", "Whether link INT is utilization or not", linkIntUtil);
    cmd.AddValue ("linkinterval", "Number of miliseconds over which to collect link data", linkInterval);
    cmd.AddValue ("delaycoef", "Coefficient on delay in the score", delayCoef);
    cmd.AddValue ("tputcoef", "Coefficient on tput in the score", tputCoef);
    cmd.AddValue ("byteswitched", "Whether the senders are byteswitched or not", byteSwitched);
    cmd.AddValue ("simtime", "Simulation time", simTime);
    cmd.AddValue ("savewhiskerstats", "Set to true to save whisker usage stats", saveWhiskerStats);
    // cmd.AddValue ("reversewhisker", "Reversepath whisker file", revwhiskerfile);
    cmd.Parse (argc, argv);

    // Set CCA Parameters
    setenv("WHISKERS", whiskerfile.c_str(), 1);
    int dims = intenabled ? 9 : 7;
    setenv("NUMDIMS", std::to_string(dims).c_str(), 1);
    setenv("SAVEWHISKERS", std::to_string((int)saveWhiskerStats).c_str(), 1);
    Config::SetDefault ("ns3::TcpL4Protocol::SocketType", StringValue (cca));

    Config::SetDefault("ns3::FifoQueueDisc::MarkEcnThreshold", DoubleValue(7));

    std::cout << "Parsed input" << std::endl;

    Time::SetResolution (Time::NS);

    // LogComponentEnable ("OnOffApplication", LogLevel(LOG_DEBUG | LOG_PREFIX_NODE));
    // LogComponentEnable ("TcpSocketBase", LogLevel(LOG_FUNCTION | LOG_DEBUG | LOG_PREFIX_NODE ));
    // LogComponentEnable ("TcpRateOps", LogLevel(LOG_FUNCTION | LOG_INFO | LOG_PREFIX_NODE ));
    // LogComponentEnable ("TcpSocketBase", LogLevel(LOG_FUNCTION | LOG_DEBUG | LOG_PREFIX_NODE | LOG_PREFIX_FUNC)); 
    // LogComponentEnable ("TcpSocketBase", LogLevel(LOG_DEBUG | LOG_PREFIX_NODE | LOG_PREFIX_TIME)); 
    // LogComponentEnable ("PointToPointNetDevice", LogLevel( LOG_DEBUG | LOG_PREFIX_NODE));
    // LogComponentEnable ("IntPacketTag", LogLevel(LOG_DEBUG | LOG_PREFIX_NODE));
    // LogComponentEnable ("TcpSocketBase", LogLevel(LOG_DEBUG | LOG_PREFIX_NODE));
    // LogComponentEnable ("TcpRemy", LogLevel(LOG_DEBUG | LOG_PREFIX_NODE ));
    // LogComponentEnable ("TcpL4Protocol", LogLevel(LOG_FUNCTION | LOG_DEBUG | LOG_PREFIX_NODE ));
    // LogComponentEnable ("Ipv4L3Protocol", LogLevel(LOG_FUNCTION | LOG_DEBUG | LOG_PREFIX_NODE ));
    // LogComponentEnable ("Ipv4Interface", LogLevel(LOG_FUNCTION | LOG_LOGIC | LOG_DEBUG | LOG_PREFIX_NODE ));
    // LogComponentEnable ("TrafficControlLayer", LogLevel(LOG_FUNCTION | LOG_LOGIC | LOG_DEBUG | LOG_PREFIX_NODE ));
    // LogComponentEnable ("FifoQueueDisc", LogLevel(LOG_FUNCTION | LOG_LOGIC | LOG_DEBUG | LOG_PREFIX_NODE ));
    // LogComponentEnable ("PointToPointNetDevice", LogLevel(LOG_FUNCTION | LOG_LOGIC | LOG_DEBUG | LOG_PREFIX_NODE ));

    std::default_random_engine prng(seed);

    ns3::RngSeedManager::SetSeed(prng());

    std::ifstream file;
    file.open( netfile );
    if ( !file.is_open() ) {
      std::cerr << "Could not open file " << netfile << std::endl;
      return -1;
    }

    RemyBuffers::ConfigRange configrange;
    if ( !configrange.ParseFromIstream( &file ) ) {
      std::cerr << "Could not parse file " << netfile << std::endl;
      return -1;
    }

    file.close();

    std::vector<NetworkConfig> netConfigs =  NetworkRange::generateNetworks(configrange, &prng, samplesize, intenabled); // TODO: Move necessary stuff to utils 

    double total_score = 0;
    for (NetworkConfig cfg : netConfigs)
    {
      for (int i = 0; i < configruns; i++)
      {
        std::cout << "Simulating... " << std::endl;
        double score = simulate(cfg.n_src, cfg.on_mean, 0.0, cfg.off_mean, 0.0, cfg.link_rate, 
                                cfg.rtt/2, cfg.error_rate, cfg.buffer_size, intenabled, linkIntUtil, linkInterval, delayCoef, 
                                tputCoef, byteSwitched, simTime);
        total_score += score;
        std::cout << cfg.toString();
        std::cout << "Score: " << score << std::endl;
      }
    }
}
