#include "tracing-utils.hh"


uint32_t Utils::GenerateFlowId(uint32_t sourceId, uint32_t dstId)
{
  return (sourceId << 16) | dstId;
}

Utils::FlowId Utils::DeserializeFlowId(uint32_t flowId)
{
  FlowId flow;
  flow.sourceId = flowId >> 16;
  flow.destinationId = flowId & 65535;
  return flow;
}

void Utils::BoolTrace (std::string context, bool oldValue, bool newValue)
{
  std::cout << Simulator::Now ().GetSeconds() << "," << context << "," << newValue << std::endl;
}

void Utils::UintTrace (std::string context, uint32_t oldValue, uint32_t newValue)
{
  std::cout << Simulator::Now ().GetSeconds() << "," << context << "," << newValue << std::endl;
}

void Utils::CongStateTrace (std::string context, TcpSocketState::TcpCongState_t oldValue, TcpSocketState::TcpCongState_t newValue)
{
  std::cout << Simulator::Now ().GetSeconds() << "," << context << ",CS," << newValue << std::endl;
}

void Utils::DataRateTrace (std::string context, DataRate oldValue, DataRate newValue)
{
  std::cout << Simulator::Now ().GetSeconds() << "," << context << ",RATE," << newValue << std::endl;
}

void Utils::TimeTrace (std::string context, ns3::Time oldValue, ns3::Time newValue)
{
  std::cout << Simulator::Now ().GetSeconds() << "," << context << ",RTT," << newValue << std::endl;
}

void Utils::AckTrace (std::string context, ns3::SequenceNumber32 oldValue, ns3::SequenceNumber32 newValue)
{
  std::cout << Simulator::Now ().GetSeconds() << "," << context << ",ACK," << newValue << std::endl;
}

void Utils::PacketSizeTrace (std::string context, Ptr<Packet const> pkt)
{
  uint32_t size = pkt->GetSize();
  std::cout << Simulator::Now ().GetSeconds() << "," << context << "," << size << std::endl;
}

void Utils::PacketDropTrace (std::string context, Ptr<QueueDiscItem const> item)
{
  Ptr<Packet> pkt = item->GetPacket();
  FlowIdTag flowId;
  pkt->PeekPacketTag(flowId);
  FlowId flow = Utils::DeserializeFlowId(flowId.GetFlowId());
  std::cout << Simulator::Now ().GetSeconds() << "," << context << ",DROP," << flow.sourceId << "." << flow.destinationId << std::endl;
}

void Utils::TcpTracing (ApplicationContainer serverApps, int nodeId, int socketId) // Note: this is not actually the socket ID
{
  std::ostringstream oss;
  Ptr<Socket> socket = StaticCast<OnOffApplication>(serverApps.Get(0))->GetSocket();
  oss << "N/" << nodeId << "/S/" << socketId; //socket->GetBoundNetDevice()->GetIfIndex();
  StaticCast<TcpSocket>(socket)->TraceConnect("CongState", oss.str(), MakeCallback(&Utils::CongStateTrace));
  StaticCast<TcpSocket>(socket)->TraceConnect("RTT", oss.str(), MakeCallback(&Utils::TimeTrace));
  StaticCast<TcpSocket>(socket)->TraceConnect("HighestRxAck", oss.str(), MakeCallback(&Utils::AckTrace));
  StaticCast<TcpSocket>(socket)->TraceConnect("PacingRate", oss.str(), MakeCallback(&Utils::DataRateTrace));
  oss << ",CWND";
  StaticCast<TcpSocket>(socket)->TraceConnect("CongestionWindow", oss.str(), MakeCallback(&Utils::UintTrace));
}

void Utils::ApplicationTrace(Ptr<Node> node, int appIndex)
{
  std::ostringstream oss1;
  oss1 << "N/" << node->GetId () << "/A/" << appIndex << "/" << "$OnOff,TX";
  Ptr<Application> app = node->GetApplication(appIndex);
  app->TraceConnect("Tx", oss1.str(), MakeCallback(&Utils::PacketSizeTrace));
}


void Utils::ApplicationOnOffTrace(ApplicationContainer serverApps, int nodeId, int remoteId)
{
  std::ostringstream oss1;
  oss1 << "N/" << nodeId << "/S/" << remoteId << "/A" << ",On";
  Ptr<Application> app = serverApps.Get(0);
  StaticCast<OnOffApplication>(app)->TraceConnect("OnOff", oss1.str(), MakeCallback(&Utils::BoolTrace));
}

void Utils::setupBwTrace(Ptr<Node> node, NetDeviceContainer linkDevices, int deviceIndex, std::string source)
{
  // Utilization tracing
  std::ostringstream oss1;
  oss1 << "N/" << node->GetId () << "/D/" << linkDevices.Get(deviceIndex)->GetIfIndex() << "/" << "ND" << "/" << source;
  Ptr<PointToPointNetDevice> netDevice = StaticCast<PointToPointNetDevice> (linkDevices.Get (deviceIndex));
  Ptr<DropTailQueue<Packet>> queue = StaticCast<DropTailQueue<Packet>> (netDevice->GetQueue());
  netDevice->TraceConnect(source, oss1.str(), MakeCallback(&Utils::PacketSizeTrace));
  std::ostringstream oss2;
  oss2 << "N/" << node->GetId () << "/D/" << linkDevices.Get(deviceIndex)->GetIfIndex() << "/" << "ND" << "/Q/" << "ENQ";
  queue->TraceConnect("Enqueue", oss2.str(), MakeCallback(&Utils::PacketSizeTrace));
}

void Utils::setupNodeTrace(Ptr<Node> node, NetDeviceContainer linkDevices, int deviceIndex, Link link, Ptr<QueueDisc> queueDisc)
{
  // Queue length tracing
  std::ostringstream oss;
  oss << "N/" << node->GetId () << "/D/" << linkDevices.Get(deviceIndex)->GetIfIndex() << ",TXQ";
  Ptr<Queue<Packet> > queue = StaticCast<PointToPointNetDevice> (linkDevices.Get (deviceIndex))->GetQueue ();
  queue->TraceConnect ("PacketsInQueue", oss.str(), MakeCallback(&Utils::UintTrace));

  // Utilization tracing
  std::ostringstream oss2;
  oss2 << "N/" << node->GetId () << "/D/" << linkDevices.Get(deviceIndex)->GetIfIndex() << ",MRX";
  Ptr<PointToPointNetDevice> netDevice = StaticCast<PointToPointNetDevice> (linkDevices.Get (deviceIndex));
  netDevice->TraceConnect("MacRx", oss2.str(), MakeCallback(&Utils::PacketSizeTrace));
  oss2 << ": rate:" << link.linkRate << "; qlen:" << link.bufferLen << std::endl;
  printf(oss2.str().c_str());


  // Queue drop tracing
  std::ostringstream oss1;
  oss1 << "N/" << node->GetId () << "/D/" << linkDevices.Get(deviceIndex)->GetIfIndex();
  queueDisc->TraceConnect ("Drop", oss1.str(), MakeCallback(&Utils::PacketDropTrace));
}


Utils::FlowScoreTracker::FlowScoreTracker(int name)
{
  total_delay = 0;
  total_bytes = 0;
  total_share = 0;
  total_time = 0;
  worst_delay = 0;
  total_packets = 0;
  is_on = false;
  id = name;
}

Utils::FlowScoreTracker::FlowScoreTracker()
{
  total_delay = 0;
  total_bytes = 0;
  total_share = 0;
  total_time = 0;
  total_packets = 0;
  is_on = false;
  initial_seq = 0;
  ack_diff = 0;
  id = 0;
}

double Utils::FlowScoreTracker::score(bool remyShare, int delayCoef, int tputCoef)
{
  long double delay_penalty = 0;
  long double throughput_utility = 0;
  double share_ratio = 0;
  double delay_ratio = 0;
  std::cout << "Flow ID: " << id << std::endl;
  std::cout << "Total delay: " << total_delay << std::endl;
  std::cout << "Total packets: " << total_packets << std::endl;
  std::cout << "Total share: " << total_share << std::endl;
  std::cout << "Total time: " << total_time << std::endl;
  std::cout << "Tail delay: " << worst_delay << std::endl;
  std::cout << "Normed FCT: " << (total_time / total_share) << std::endl;
  if (total_packets != 0 && total_delay != 0)
  {
    delay_ratio = ((double)total_delay / total_packets);
    delay_penalty = log2( delay_ratio / 100.0 );
  }
  if (total_share != 0 && total_bytes != 0)
  {
    share_ratio = remyShare ? total_packets / total_share : ((double)((total_bytes + (total_packets * 58)) * 8))  / total_share; // Adjust for headers 
    throughput_utility = log2( share_ratio ); 
  }
  if (total_packets == 0 and total_share != 0)
  {
    std::cout << "Tput: " << 0 << "; Delay: " << 0 << std::endl;
    std::cout << "Throughput utility: " << 0 << "; Delay penalty: " << 0 << std::endl;
    return -1000;
  }

  std::cout << "Tput: " << share_ratio << "; Delay: " << delay_ratio << std::endl;
  std::cout << "Throughput utility: " << throughput_utility << "; Delay penalty: " << delay_penalty << std::endl;
  return (tputCoef * throughput_utility) - (delayCoef * delay_penalty);
}

double Utils::FlowScoreTracker::getShareRatio()
{
  return total_packets / total_share;
}

std::string Utils::FlowScoreTracker::toString()
{
  std::ostringstream oss;
  oss << "Flowtracker data{id=" << id << ";delay=" << total_delay << ";bytes=" << total_bytes << ";packets=" << total_packets << ";share=" << total_share << "}";
  return oss.str();
}

Utils::AllScoreTracker::AllScoreTracker(uint64_t btlbw, int flows)
{
  num_flows = 0;
  last_flow_change = 0;
  bandwidth = btlbw;
  flowTrackers.reserve(flows);
  total_flows = flows;
}

double Utils::AllScoreTracker::calculateFairness(std::vector<double> throughputs)
{
  double sum = std::accumulate(throughputs.begin(), throughputs.end(), 0.0);
  double mean = sum / throughputs.size();

  double sq_sum = std::inner_product(throughputs.begin(), throughputs.end(), throughputs.begin(), 0.0);
  double stdev = std::sqrt(sq_sum / throughputs.size() - mean * mean);
  return stdev;
}

double Utils::AllScoreTracker::score(double endTime, bool remyShare, int delayCoef, int tputCoef)
{
  updateShareFinal(endTime);
  double total_score = 0;
  std::vector<double> throughputs = std::vector<double>();
  for (auto it = flowTrackers.begin(); it != flowTrackers.end(); ++it)
  {
    if (remyShare)
    {
      it->second.total_share = (it->second.total_share / bandwidth) * 1000000.0; //remove bandwidth normalization and convert to us 
    }
    std::cout << "Flow: " << it->second.id << std::endl;
    total_score += it->second.score(remyShare, delayCoef, tputCoef);
    throughputs.push_back(it->second.getShareRatio());
  }
  std::cout << "Fairness: " << calculateFairness(throughputs) << std::endl;
  return total_score / total_flows;
}

void Utils::AllScoreTracker::updateBytes(std::string context, ns3::SequenceNumber32 oldValue, ns3::SequenceNumber32 newValue) // TODO: Deprecate this since change packet size anyway
{
  double flowId = std::stod(context);
  if (flowTrackers.count(flowId) > 0)
  {
    auto it = flowTrackers.find(flowId);  
    uint64_t bytes = it->second.total_bytes;
    if (it->second.initial_seq == 0)
    {
      it->second.initial_seq = newValue.GetValue();
    }
    uint64_t new_bytes = newValue - oldValue;
    it->second.total_bytes = bytes + new_bytes; 
    it->second.ack_diff = newValue.GetValue() - it->second.initial_seq;
  }
} 

void Utils::AllScoreTracker::updatePacketsAndDelay(std::string context, const Ptr<const Packet> packet, const TcpHeader& header,
                                            const Ptr<const TcpSocketBase> socket)
{
  double flowId = std::stod(context);
  if (flowTrackers.count(flowId) > 0)
  {
    auto it = flowTrackers.find(flowId);

    // Update packet count
    uint64_t packets = it->second.total_packets;
    it->second.total_packets = packets + 1;

    // Update delay total (need an estimate for every packet, not just when it changes)
    double delay = it->second.total_delay;
    double new_delay = socket->GetSocketState()->m_lastTimestampRtt.GetMicroSeconds();
    
    it->second.total_delay = delay + new_delay;
    it->second.worst_delay = std::max(it->second.worst_delay, new_delay);
  }
}

void Utils::AllScoreTracker::trackTX(std::string context, const Ptr<const Packet> packet, const TcpHeader& header,
                                            const Ptr<const TcpSocketBase> socket)
{
  Ptr<const TcpOption> option = header.GetOption (TcpOption::TS);
  Ptr<const TcpOptionTS> ts = DynamicCast<const TcpOptionTS> (option);
  std::cout << context << ": " << Simulator::Now().GetSeconds() << ": Packet Sent with UID: " << header.GetSequenceNumber() << " and TS value: " << ts->GetEcho() << std::endl;
}

void Utils::AllScoreTracker::updateShare(std::string context, bool oldValue, bool newValue)
{
  double flowId = std::stod(context);
  
  if (num_flows != 0 )
  {
    double share = ((double)bandwidth / num_flows) * (Simulator::Now ().GetSeconds() - last_flow_change);
    for (auto it = flowTrackers.begin(); it != flowTrackers.end(); ++it)
    {
      if (it->second.is_on)
      {
        uint64_t current_share = it->second.total_share;
        it->second.total_share = current_share + (uint64_t) share;
        double current_time = it->second.total_time;
        it->second.total_time = current_time + (double) (Simulator::Now ().GetSeconds() - last_flow_change);
      }
    }
  }

  last_flow_change = Simulator::Now ().GetSeconds();

  auto flow = flowTrackers.find(flowId);
  flow->second.is_on = newValue;

  if (newValue)
  {
    num_flows += 1;
  }
  else
  {
    num_flows -= 1;
  }
}

void Utils::AllScoreTracker::updateShareFinal(double endTime)
{
  if (num_flows != 0 )
  {
    double share = ((double)bandwidth / num_flows) * (endTime - last_flow_change);
    for (auto it = flowTrackers.begin(); it != flowTrackers.end(); ++it)
    {
      if (it->second.is_on)
      {
        uint64_t current_share = it->second.total_share;
        it->second.total_share = current_share + (uint64_t) share;
      }
    }
  }
}

void Utils::AllScoreTracker::setupAppScoreTrace(ApplicationContainer serverApps, int nodeId, int socketId)
{
  std::ostringstream oss;
  Ptr<Socket> socket = StaticCast<OnOffApplication>(serverApps.Get(0))->GetSocket();
  oss << nodeId << "." << socketId; 
  Ptr<Application> app = serverApps.Get(0);
  StaticCast<OnOffApplication>(app)->TraceConnect("OnOff", oss.str(), MakeCallback(&Utils::AllScoreTracker::updateShare, this));

  double flowId = std::stod(oss.str());
  flowTrackers[flowId] = FlowScoreTracker(nodeId);
}

void Utils::AllScoreTracker::setupScoreTrace(AllScoreTracker* scorer, ApplicationContainer serverApps, int nodeId, int socketId)
{
  std::ostringstream oss;
  Ptr<Socket> socket = StaticCast<OnOffApplication>(serverApps.Get(0))->GetSocket();
  oss << nodeId << "." << socketId;  

  StaticCast<TcpSocket>(socket)->TraceConnect("HighestRxAck", oss.str(), MakeCallback(&Utils::AllScoreTracker::updateBytes, scorer));
  StaticCast<TcpSocket>(socket)->TraceConnect("Rx", oss.str(), MakeCallback(&Utils::AllScoreTracker::updatePacketsAndDelay, scorer));
}
