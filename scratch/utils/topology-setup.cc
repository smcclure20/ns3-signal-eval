#include "topology-setup.hh"


Utils::ApplicationPattern::ApplicationPattern(double onMean, double onBound, double offMean, double offBound)
  : onMean(onMean),
    onBound(onBound),
    offMean(offMean),
    offBound(offBound) {};

std::string Utils::ApplicationPattern::toString(void)
{
    std::stringstream ss;
    ss << "AP{" << onMean << ", " << onBound << ", " << offMean << ", " << offBound << "}";
    return ss.str();
};

std::pair<std::string, std::string> Utils::splitBufferLen(QueueSize bufferLen, double deviceFraction)
{
    int totalLen = bufferLen.GetValue();
    int deviceLen = floor(deviceFraction * totalLen);
    int qdiscLen = ceil((1 - deviceFraction) * totalLen);
    std::string unit = (bufferLen.GetUnit() == PACKETS) ? "p" : "B";
    std::string deviceValue = std::to_string(deviceLen) + unit;
    std::string qdiscValue = std::to_string(qdiscLen) + unit;
    return std::make_pair(deviceValue, qdiscValue);
}

void Utils::topologySetup(Topology* topo, std::string queueType, std::string routeTableFile, std::vector<ApplicationPattern> appPatterns, 
                            std::vector<uint64_t> sendingRates, double errorRate, double simTime, bool intEnabled, bool linkIntUtil, int byteCounterInterval,
                            bool traceNetDevices, bool traceHosts, bool isByteSwitched, AllScoreTracker* scoreTracker, void (*socketFunc) (void))
{
    // Create nodes
    std::vector<TopologyNode*> allNodes = topo->getAllNodes();
    std::vector<TopologyNode*> hosts = topo->getHosts();
    int endHostCount = (int) hosts.size();
    NodeContainer nodes;
    nodes.Create ((int)allNodes.size());
    
    NodeContainer endHosts;
    for (int i = 0; i < (int) allNodes.size(); i++)
    {
        TopologyNode* node = allNodes.at(i);
        if (node->isEndHost)
        {
            endHosts.Add(nodes.Get(i)); // endHosts will appear in the order they exist in allNodes
        }
    }

    InternetStackHelper stack;
    stack.Install (nodes);

    if (routeTableFile != "")
    {
        std::ofstream routeFile;
        routeFile.open(routeTableFile);
        routeFile << "Endhosts=" << endHostCount << std::endl;
        routeFile.close();
    }


    // Create all the point-to-point links
    PointToPointHelper pointToPoint;
    NetDeviceContainer linkDevices;
    Ipv4Address endHostAddrs[(int)allNodes.size()]; // indexed by node ids (not indices in other lists)
    for (int i = 0; i < (int) topo->links.size(); i++)
    {
        Link link = topo->links.at(i);
        pointToPoint.SetDeviceAttribute ("DataRate", StringValue (link.linkRate));
        pointToPoint.SetChannelAttribute ("Delay", StringValue (link.delay));
        pointToPoint.SetQueue (queueType, "MaxSize", StringValue (link.bufferLen));
        pointToPoint.SetDeviceAttribute("IntEnabled", BooleanValue(intEnabled));
        pointToPoint.SetDeviceAttribute("LinkMetricUtil", BooleanValue(linkIntUtil));
        pointToPoint.SetDeviceAttribute("ByteCounterInterval", TimeValue(MicroSeconds(byteCounterInterval)));

        linkDevices = pointToPoint.Install (nodes.Get(link.node1->id), nodes.Get(link.node2->id));

        TrafficControlHelper tch;
        tch.SetRootQueueDisc ("ns3::FifoQueueDisc", "MaxSize", StringValue ("1p")); 
        QueueDiscContainer queueDiscs = tch.Install (linkDevices);

        Ptr<RateErrorModel> errorModel = CreateObject<RateErrorModel>();
        errorModel->SetUnit (ns3::RateErrorModel::ERROR_UNIT_PACKET);
        errorModel->SetRate (errorRate);
        linkDevices.Get(1)->SetAttribute ("ReceiveErrorModel", PointerValue(errorModel));
        
        int node1Interface = linkDevices.Get(0)->GetIfIndex();
        int node2Interface = linkDevices.Get(1)->GetIfIndex();
        if (traceNetDevices && !link.node1->isTraced[node1Interface])
        {
            link.node1->isTraced[node1Interface] = true;
            setupNodeTrace(nodes.Get(link.node1->id), linkDevices, 0, link, queueDiscs.Get(0));
        }
        if (traceNetDevices && !link.node2->isTraced[node2Interface])
        {
            link.node2->isTraced[node2Interface] = true;
            setupNodeTrace(nodes.Get(link.node2->id), linkDevices, 1, link, queueDiscs.Get(1));
        }
        
        Ipv4AddressHelper address;
        Ipv4Address addr = link.addresses.c_str();
        Ipv4Mask mask = link.mask.c_str();
        address.SetBase (addr, mask);
        
        Ipv4InterfaceContainer interfaces = address.Assign (linkDevices);
        if (link.node1->isEndHost) // TODO: Parameterize tracing
        {
        endHostAddrs[link.node1->id] = interfaces.GetAddress(0); // TODO: this implicitly assumes hosts have one uplink
        }
        if (link.node2->isEndHost)
        {
        endHostAddrs[link.node2->id] = interfaces.GetAddress(1);
        }

        // Save links as context for route file
        if (routeTableFile != "")
        {
            std::ofstream routeFile;
            routeFile.open(routeTableFile, std::ios::app);
            routeFile << "Link: Node/" << link.node1->id << "/Iface/" << node1Interface << "/Address/" << interfaces.GetAddress(0) << 
            " to Node/" << link.node2->id << "/Iface/" << node2Interface << "/Address/" << interfaces.GetAddress(1) << std::endl;
            routeFile.close();
        }
    }

    // Create application flows
    double startTime = 1.0;
    std::vector<std::vector<int>> matrix = topo->getTrafficMatrix();
    for (int i = 0; i < (int) matrix.size(); i++)
    {
        Ptr<Node> node = endHosts.Get(i); // Assumes that the TM is in the same order as the nodes in endHosts (which came from allNodes)
        for (int j = 0; j < (int) matrix.at(i).size(); j++) // All to all communication
        {
        Ptr<Node> remoteNode = endHosts.Get(matrix.at(i).at(j)); // endHosts container is indexed by order (not counting routers)
        Ipv4Address remoteAddr = endHostAddrs[hosts.at(matrix.at(i).at(j))->id]; // addresses are indexed by node id (counts routers) (though hosts in indexed by host order-not counting routers)
        uint16_t port = topo->getPort();

        // int numSenders = (int) (endHostCount);
        // std::string protocol = "ns3::TcpSocketFactory";
        // if (i >= numSenders) {
        //     std::stringstream nodeId;
        //     nodeId << node->GetId();
        //     std::string specificNodeSocketTypePath = "/NodeList/" + nodeId.str() + "/$ns3::TcpL4Protocol/SocketType";

        //     // Set the congestion control algorithm for the specific node (e.g., TcpVegas)
        //     Config::Set(specificNodeSocketTypePath, TypeIdValue(TypeId::LookupByName("ns3::TcpRemy2")));
        // }

        OnOffHelper applicationHelper (protocol, InetSocketAddress(remoteAddr, port));
        applicationHelper.SetAttribute("MaxBytes", UintegerValue(0));
        // applicationHelper.SetAttribute("PacketSize", UintegerValue(1000));

        uint64_t rate = sendingRates.at(rand() % sendingRates.size());
        DataRate dataRate = DataRate(rate);
        applicationHelper.SetAttribute("DataRate", DataRateValue(dataRate));

        Ptr<ExponentialRandomVariable> onDist = CreateObject<ExponentialRandomVariable>();
        Utils::ApplicationPattern appPattern = appPatterns.at(rand() % appPatterns.size());
        if (!isByteSwitched) {
            onDist->SetAttribute("Mean", DoubleValue( appPattern.onMean ));
            onDist->SetAttribute("Bound", DoubleValue(appPattern.onBound));
        }
        else {
            onDist->SetAttribute("Mean", DoubleValue(100000) ); //((appPattern.onMean * 512 * 8) / rate) * 1000 )
            onDist->SetAttribute("Bound", DoubleValue(0));
        }
        Ptr<ExponentialRandomVariable> offDist = CreateObject<ExponentialRandomVariable>();
        offDist->SetAttribute("Mean", DoubleValue( appPattern.offMean));
        offDist->SetAttribute("Bound", DoubleValue(appPattern.offBound));
        applicationHelper.SetAttribute("OnTime", PointerValue(onDist));
        applicationHelper.SetAttribute("OffTime", PointerValue(offDist));

        if (isByteSwitched) {
            applicationHelper.SetAttribute("MaxBytes", UintegerValue(appPattern.onMean * 512));
            Ptr<ExponentialRandomVariable> bytesDist = CreateObject<ExponentialRandomVariable>();
            bytesDist->SetAttribute("Mean", DoubleValue( appPattern.onMean * 512));
            applicationHelper.SetAttribute("MaxBytesDistribution", PointerValue(bytesDist));
        }
        
        applicationHelper.SetAttribute("EnableFlowTrace", BooleanValue(true));
        applicationHelper.SetAttribute("FlowId", UintegerValue(Utils::GenerateFlowId(node->GetId(), remoteNode->GetId())));

        ApplicationContainer serverApps = applicationHelper.Install (node);
        serverApps.Start (MilliSeconds (startTime));
        serverApps.Stop (MilliSeconds (simTime));

        PacketSinkHelper applicationSink (protocol, InetSocketAddress(Ipv4Address::GetAny (), port)); // do they need different ports?
        
        ApplicationContainer clientApps = applicationSink.Install (remoteNode);
        clientApps.Start (MilliSeconds (startTime));
        clientApps.Stop (MilliSeconds (simTime));

        if (traceHosts)
        {
            Utils::ApplicationOnOffTrace(serverApps, node->GetId(), remoteNode->GetId());
            Simulator::Schedule(NanoSeconds(startTime * 1000000  + 10), &TcpTracing, serverApps, node->GetId(), remoteNode->GetId());
        }
        if (scoreTracker != NULL  && !(hosts.at(i)->isUntracedHost))
        {
            scoreTracker->setupAppScoreTrace(serverApps, node->GetId(), remoteNode->GetId());
            Simulator::Schedule(NanoSeconds(startTime * 1000000 + 10), &Utils::AllScoreTracker::setupScoreTrace, scoreTracker, serverApps, node->GetId(), remoteNode->GetId());
        }
        }
    }

    if (socketFunc != NULL)
    {
        Simulator::Schedule(NanoSeconds(startTime * 1000000 + 20), socketFunc);
    }

    // Install routing tables
    Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
    if (routeTableFile != "")
    {
        Ptr<OutputStreamWrapper> routingStream = Create<OutputStreamWrapper>(routeTableFile, std::ios::app);
        Ipv4RoutingHelper::PrintRoutingTableAllAt(MilliSeconds(1.0), routingStream);
    }
}