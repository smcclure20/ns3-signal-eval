#pragma once
#ifndef TRACING_UTILS_H
#define TRACING_UTILS_H

#include <ostream>
#include <map>
#include <functional>
#include <numeric>
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/traffic-control-module.h"
#include "ns3/tcp-option-ts.h"
#include "ns3/tcp-option.h"

#include "sim-topology.hh"

using namespace ns3;

namespace Utils
{

struct FlowId 
{
    int sourceId;
    int destinationId;
};

uint32_t GenerateFlowId(uint32_t sourceId, uint32_t dstId);
FlowId DeserializeFlowId(uint32_t flowId);

class FlowScoreTracker
{
    public:
        double total_delay;
        double worst_delay;
        uint64_t total_bytes;
        double total_share;
        double total_time;
        uint64_t total_packets;
        bool is_on;
        uint64_t initial_seq;
        uint64_t ack_diff;
        int id;

        FlowScoreTracker(int name);
        FlowScoreTracker();

        double score(bool remyShare = false, int delayCoef = 1, int tputCoef = 1);
        double getShareRatio(void);

        std::string toString();
};

class AllScoreTracker
{
    public:
        std::unordered_map<double, FlowScoreTracker> flowTrackers;
        int num_flows;
        double last_flow_change;
        uint64_t bandwidth;
        int total_flows;

        AllScoreTracker(uint64_t btlbw, int flows);

        void updateBytes(std::string context, ns3::SequenceNumber32 oldValue, ns3::SequenceNumber32 newValue);
        void updatePacketsAndDelay(std::string context, const Ptr<const Packet> packet, const TcpHeader& header,
                           const Ptr<const TcpSocketBase> socket);
        void trackTX(std::string context, const Ptr<const Packet> packet, const TcpHeader& header,
                           const Ptr<const TcpSocketBase> socket);
        void updateShare(std::string context, bool oldValue, bool newValue);
        void updateShareFinal(double endTime);
        void setupAppScoreTrace(ApplicationContainer serverApps, int nodeId, int socketId);

        double calculateFairness(std::vector<double> throughputs);

        double score(double endTime, bool remyShare = false, int delayCoef = 1, int tputCoef = 1);

        static void setupScoreTrace(AllScoreTracker* scorer, ApplicationContainer serverApps, int nodeId, int socketId);
};

void BoolTrace (std::string context, bool oldValue, bool newValue);
void UintTrace (std::string context, uint32_t oldValue, uint32_t newValue);
void CongStateTrace (std::string context, TcpSocketState::TcpCongState_t oldValue, TcpSocketState::TcpCongState_t newValue);
void DataRateTrace (std::string context, DataRate oldValue, DataRate newValue);
void TimeTrace (std::string context, ns3::Time oldValue, ns3::Time newValue);
void AckTrace (std::string context, ns3::SequenceNumber32 oldValue, ns3::SequenceNumber32 newValue);
void PacketSizeTrace (std::string context, Ptr<Packet const> pkt);
void PacketDropTrace (std::string context, Ptr<QueueDiscItem const> item);
void TcpTracing (ApplicationContainer serverApps, int nodeId, int socketId); // Note: this is not actually the socket ID
void ApplicationTrace(Ptr<Node> node, int appIndex);
void ApplicationOnOffTrace(ApplicationContainer serverApps, int nodeId, int remoteId);

void setupBwTrace(Ptr<Node> node, NetDeviceContainer linkDevices, int deviceIndex, std::string source = "MacRx");
void setupNodeTrace(Ptr<Node> node, NetDeviceContainer linkDevices, int deviceIndex, Link link, Ptr<QueueDisc> queueDisc);

}

#endif /* TRACING_UTILS_H */