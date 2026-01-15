#include <iostream>
#include "sim-topology.hh"

using namespace Utils;

TopologyNode::TopologyNode(int newId, bool isHost)
{
  id = newId;
  isEndHost = isHost;
  isUntracedHost = false;
  neighbors = std::vector<int>();
}

void TopologyNode::addNeighbor (int neighborId, bool isEndHost)
{
  neighbors.push_back(neighborId);
  if (!isEndHost)
  {
    routerNeighbors.push_back(neighborId);
  }
}

bool TopologyNode::isNeighbor( int nodeId )
{
  for (int i = 0; i < (int) neighbors.size(); i++)
  {
    if (nodeId == neighbors.at(i))
    {
      return true;
    }
  }
  return false;
}

bool TopologyNode::canAddNewNeighbor(int candidates)
{
  return candidates > (int) neighbors.size();
}

bool TopologyNode::canAddNewRouterNeighbor(int candidates)
{
  return candidates > (int) routerNeighbors.size();
}


Link::Link(Utils::TopologyNode *n1, Utils::TopologyNode *n2, std::string rate, std::string propDelay, std::string qLen, std::string addr)
{
  node1 = n1;
  node2 = n2;
  linkRate = rate;
  delay = propDelay;
  bufferLen = qLen;
  addresses = addr;
  mask = "255.255.255.0";
}

std::string Link::toString(void)
{
  std::stringstream ss;
  ss << "Link from node " << node1->id << " to node " << node2->id << 
  " (rate= " << linkRate << "; delay= " << delay << "; bufferLen= " << bufferLen << "; address= " << addresses << " " << mask << ")";
  return ss.str();
}

std::string Topology::toString(void)
{
  std::stringstream ss;
  for (int i = 0; i < (int) links.size(); i++)
  {
    ss << links.at(i).toString() << std::endl;
  }
  return ss.str();
}

std::string Topology::getNewSubnet()
{
  std::string addr = std::to_string(addressBytes[0]) + "." + std::to_string(addressBytes[1]) + "." + std::to_string(addressBytes[2]) + "." + std::to_string(addressBytes[3]);
  if (addressBytes[2] == 255) {
    addressBytes[2] = 0;
    addressBytes[1] += 1;
  }
  addressBytes[2] += 1;
  return addr;
}

uint16_t Topology::getPort()
{
  currPort += 1;
  return currPort;
}

void Topology::clearState()
{
  for (TopologyNode* node : getAllNodes())
  {
    std::fill_n(node->isTraced, MAX_FANOUT, false);
  }
}

/* ---------- Random Topology ---------- */

RandomTopologyParameter::RandomTopologyParameter(int nHosts, int nRouters)
{
  numEndHosts = nHosts;
  numRouters = nRouters;
  is_deterministic = false;
}

std::string RandomTopologyParameter::toString(void)
{
  std::stringstream ss;
  ss << "RandomTopologyParameter{EndHosts=" << numEndHosts << ";Routers=" << numRouters << "}";
  return ss.str();
}

RandomTopology::RandomTopology (void)
{
  endHostNodes = std::vector<Utils::TopologyNode*>();
  routerNodes = std::vector<Utils::TopologyNode*>();
  allNodes = std::vector<Utils::TopologyNode*>();
  links = std::vector<Link>();
  rateOptions = std::vector<int>({10, 20, 50}); //TODO: Parameterize this
  accessLinkRate = 200; // To avoid access links being bottlenecks
  delayOptions = std::vector<int>({2, 5, 20}); //TODO: Parameterize this
}

int RandomTopology::getBufferLen(int rate)
{
  int qlen = (rate * 50 * pow(10, 3)) / (1500 * 8); // 50ms of buffering TODO: Change this into a constant!
  // Based on MTU-sized packets, should it be min size?
  return qlen;
}

Utils::TopologyNode* RandomTopology::getNodeById(int id) // TODO: this should be more clean
{
  if (id < (int) endHostNodes.size())
  {
    return endHostNodes.at(id);
  }
  else 
  {
    return routerNodes.at(id - endHostNodes.size());
  }
}

std::vector<TopologyNode*> RandomTopology::getAllNodes()
{
  if (allNodes.size() != endHostNodes.size() + routerNodes.size()) // TODO: What if you add and remove the same amount?
  {
    allNodes.clear();
    allNodes.reserve( endHostNodes.size() + routerNodes.size() );
    allNodes.insert( allNodes.end(), endHostNodes.begin(), endHostNodes.end() );
    allNodes.insert( allNodes.end(), routerNodes.begin(), routerNodes.end() );
  }
  
  return allNodes;
}

std::vector<TopologyNode*> RandomTopology::getHosts()
{
  return endHostNodes;
}

void RandomTopology::addLink(Utils::TopologyNode *n1, Utils::TopologyNode *n2)
{
  n1->addNeighbor(n2->id, n2->isEndHost);
  n2->addNeighbor(n1->id, n1->isEndHost);
  int rate;
  if (n1->isEndHost || n2->isEndHost)
  {
    rate = accessLinkRate;
  }
  else
  {
    rate = rateOptions.at(rand() % rateOptions.size());
  }
  std::string r = std::to_string(rate) + "Mbps"; 
  std::string d = std::to_string(delayOptions.at(rand() % delayOptions.size())) + "ms";
  std::string q = std::to_string(getBufferLen(rate)) + "p"; // TODO: check that this is the right unit
  links.push_back(Link(n1, n2, r, d, q, getNewSubnet()));
}

std::vector<std::vector<int>> RandomTopology::getTrafficMatrix(void) 
{
  std::vector<std::vector<int>> matrix;
  for (int i = 0; i < (int) endHostNodes.size(); i++ )
  {
    std::vector<int> receivers;
    for (int j = 0; j < (int) endHostNodes.size(); j++ )
    {
      if (i != j)
      {
        receivers.push_back(j);
      }
    }
    matrix.push_back(receivers);
  }
  return matrix;
}

void RandomTopology::GenerateTopology (RandomTopologyParameter* params)
{
  int numEndHosts = params->numEndHosts;
  int numRouters = params->numRouters;

  // printf("Building topology...\n");

  // Create the nodes
  for (int i = 0; i < numEndHosts; i++)
  {
    TopologyNode* newNode = new TopologyNode(i, true);
    endHostNodes.push_back(newNode);
    // printf("Created end host node with id: %d\n", newNode->id);
  }
  for (int i = 0; i < numRouters; i++)
  {
    TopologyNode* newNode = new TopologyNode(i + numEndHosts, false);
    routerNodes.push_back(newNode);
    // printf("Created router node with id: %d\n", newNode->id);
  }
  // printf("Created nodes.\n");

  // Connect end hosts to exactly one router
  for (int i = 0; i < numEndHosts; i++)
  {
    TopologyNode* neighbor = routerNodes.at(rand() % numRouters);
    addLink(endHostNodes.at(i), neighbor); // TODO: This might cause issues, not quite sure if the pointers work here
    // printf("Added link from %d to %d.\n", endHostNodes.at(i)->id, neighbor->id);
  }
  // printf("Added host links.\n");

  // Connect routers to each other
  // Cannot be separate graphs (need an easy way to tell this), and should not be fully connected in most cases
  for (int i = 0; i < numRouters; i++)
  {
    TopologyNode* currentRouter = routerNodes.at(i);
    if (!currentRouter->canAddNewRouterNeighbor(numRouters - 1))
    {
      // printf("Router %d already connected to all nodes.\n", currentRouter->id);
      continue;
    }
    TopologyNode* neighbor = routerNodes.at(rand() % numRouters);
    // printf("Tried to link from %d to %d.\n", currentRouter->id, neighbor->id);
    while ((neighbor->id == currentRouter->id || currentRouter->isNeighbor(neighbor->id)) && neighbor->canAddNewRouterNeighbor(numRouters - 1))
    {
      neighbor = routerNodes.at(rand() % numRouters);
      // printf("Tried to link from %d to %d.\n", currentRouter->id, neighbor->id);
    }
    addLink(routerNodes.at(i), neighbor);
    // printf("Added link from %d to %d.\n", currentRouter->id, neighbor->id);
  }
  // printf("Added router links.\n");

  // TODO: Add motifs to topology (see notes)
  // TODO: Add some variation in excess links
}

/* ---------- Datacenter Topology ---------- */

DatacenterTopologyParameter::DatacenterTopologyParameter(int k)
{
  _k = k;
  is_deterministic = true;
}

std::string DatacenterTopologyParameter::toString(void)
{
  std::stringstream ss;
  ss << "DatacenterTopology{k=" << _k << "}";
  return ss.str();
}

DatacenterTopologyConfig::DatacenterTopologyConfig(double serverRate, double torUpRate, double aggUpRate, double delay, int bufLen)
{
  serverUplinkRate = serverRate != 0 ? serverRate : serverUplinkRate;
  torUplinkRate = torUpRate != 0 ? torUpRate : torUplinkRate;
  aggUplinkRate = aggUpRate != 0 ? aggUpRate : aggUplinkRate;
  linkDelay = delay != 0? delay : linkDelay;
  bufferLen = bufLen != 0 ? bufLen : bufferLen;
}

std::string DatacenterTopologyConfig::toString(void)
{
  std::stringstream ss;
  ss << "DCTopologyConfig{serverUpRate=" << serverUplinkRate << ";torUpRate=" << torUplinkRate << ";aggUpRate=" 
  << aggUplinkRate << ";linkDelay=" << linkDelay << ";bufLen" << bufferLen << "}";
  return ss.str();
}

DatacenterTopology::DatacenterTopology (DatacenterTopologyConfig* cfg)
{
  hostNodes = std::vector<Utils::TopologyNode*>();
  torNodes = std::vector<Utils::TopologyNode*>();
  aggNodes = std::vector<Utils::TopologyNode*>();
  coreNodes = std::vector<Utils::TopologyNode*>();
  allNodes = std::vector<Utils::TopologyNode*>();
  links = std::vector<Link>();
  config = cfg;
}

std::vector<TopologyNode*> DatacenterTopology::getAllNodes()
{
  if (allNodes.size() != hostNodes.size() + torNodes.size() + aggNodes.size() + coreNodes.size()) // TODO: What if you add and remove the same amount?
  {
    allNodes.clear();
    allNodes.reserve( hostNodes.size() + torNodes.size() + aggNodes.size() + coreNodes.size() );
    allNodes.insert( allNodes.end(), hostNodes.begin(), hostNodes.end() );
    allNodes.insert( allNodes.end(), torNodes.begin(), torNodes.end() );
    allNodes.insert( allNodes.end(), aggNodes.begin(), aggNodes.end() );
    allNodes.insert( allNodes.end(), coreNodes.begin(), coreNodes.end() );
  }
  
  return allNodes;
}

std::vector<TopologyNode*> DatacenterTopology::getHosts()
{
  return hostNodes;
}

void DatacenterTopology::addLink(Utils::TopologyNode *n1, Utils::TopologyNode *n2, LinkType type)
{
  n1->addNeighbor(n2->id, n2->isEndHost);
  n2->addNeighbor(n1->id, n1->isEndHost);
  
  int rate;
  switch(type) {
    case HOST:
      rate = config->serverUplinkRate;
      break;
    case TOR:
      rate = config->torUplinkRate;
      break;
    case AGGREGATOR:
      rate = config->aggUplinkRate;
      break;
  }
  
  std::string r = std::to_string(rate) + "Mbps"; 
  std::string d = std::to_string(config->linkDelay) + "ms";
  std::string q = std::to_string(config->bufferLen) + "p"; // TODO: check that this is the right unit
  
  links.push_back(Link(n1, n2, r, d, q, getNewSubnet()));
}

std::vector<std::vector<int>> DatacenterTopology::getTrafficMatrix(void) 
{
  std::vector<std::vector<int>> matrix;
  for (int i = 0; i < (int) hostNodes.size(); i++ )
  {
    std::vector<int> receivers;
    for (int j = 0; j < (int) hostNodes.size(); j++ )
    {
      if (i != j)
      {
        receivers.push_back(j);
      }
    }
    matrix.push_back(receivers);
  }
  return matrix;
}


void DatacenterTopology::GenerateTopology (DatacenterTopologyParameter* params)
{
  int k = params->_k;
  // printf("Building topology...\n");
  // printf("k=%d\n", k);

  int num_servers = k * pow((k/2), 2);
  int num_tors = k * (k / 2);
  int num_agg = k * (k / 2);
  int num_core = pow((k/2), 2);

  // Create the servers
  // printf("Creating servers...\n");
  for (int i = 0; i < num_servers; i++)
  {
    TopologyNode* newNode = new TopologyNode(i, true);
    hostNodes.push_back(newNode);
  }

  // Create the ToRs
  // printf("Creating tors...\n");
  for (int i = 0; i < num_tors; i++)
  {
    TopologyNode* newNode = new TopologyNode(i + num_servers, false);
    torNodes.push_back(newNode);
  }

  // Create the aggregation switches
  // printf("Creating aggs...\n");
  for (int i = 0; i < num_agg; i++)
  {
    TopologyNode* newNode = new TopologyNode(i + num_servers + num_tors, false);
    aggNodes.push_back(newNode);
  }

  // Create the core switches
  // printf("Creating cores...\n");
  for (int i = 0; i < num_core; i++)
  {
    TopologyNode* newNode = new TopologyNode(i + num_servers + num_tors + num_core, false);
    coreNodes.push_back(newNode);
  }
  
  // Connect servers to ToRs
  // printf("Connecting servers...\n");
  for (int i = 0; i < num_servers; i++)
  {
    TopologyNode* server = hostNodes.at(i);
    int tor_num = (int) i / ( k / 2 );
    TopologyNode* tor = torNodes.at(tor_num);
    addLink(server, tor, HOST);
    // printf("Connecting server %d to tor %d\n", i, tor_num);
  }

  // Connect ToRs to Aggregators
  // printf("Connecting tors...\n");
  for (int i = 0; i < num_tors; i++)
  {
    TopologyNode* tor = torNodes.at(i);
    int aggregator_num = ((int) i / ( k / 2 )) * k / 2;
    for (int j = 0; j < (k/2); j++)
    {
      addLink(tor, aggNodes.at(aggregator_num + j), TOR); 
      // printf("Connecting tor %d to agg %d\n", i, aggregator_num + j);
    }
  }

  // Connect Aggregators to Cores
  // printf("Connecting agg...\n");
  for (int i = 0; i < num_agg; i++)
  {
    TopologyNode* agg = aggNodes.at(i);
    int core_num = ((int) i % ( k / 2 )) * k / 2;
    for (int j = 0; j < (k/2); j++)
    {
      addLink(agg, coreNodes.at(core_num + j), AGGREGATOR); 
      // printf("Connecting agg %d to core %d\n", i, core_num + j);
    }
  }
}

/* ---------- Dumbbell Topology ---------- */

DumbbellTopologyParameter::DumbbellTopologyParameter(int n)
{
  _n = n;
  is_deterministic = true;
  hasExtraFlows = false;
}

std::string DumbbellTopologyParameter::toString(void)
{
  std::stringstream ss;
  ss << "DumbbellTopology{n=" << _n << "}";
  return ss.str();
}

DumbbellTopologyConfig::DumbbellTopologyConfig(double btlRate, double btlDelay, int btlBuf, int hostBuf, double hostRate)
{
  bottleneckRate = btlRate != 0 ? btlRate : bottleneckRate;
  bottleneckDelay = btlDelay != 0 ? btlDelay : bottleneckDelay;
  bottleneckBufferLen = btlBuf != 0 ? btlBuf : bottleneckBufferLen;
  hostBufferLen = hostBuf != 0 ? hostBuf : hostBufferLen;
  hostLinkRate = hostRate != 0 ? hostRate : hostLinkRate;
}

std::string DumbbellTopologyConfig::toString(void)
{
  std::stringstream ss;
  ss << "DumbbellTopologyConfig{btlRate=" << bottleneckRate << ";btlDelay=" << bottleneckDelay << ";btlBuf=" 
  << bottleneckBufferLen << ";hostBuf=" << hostBufferLen << ";hostRate" << hostLinkRate << "}";
  return ss.str();
}

DumbbellTopology::DumbbellTopology (DumbbellTopologyConfig* cfg)
{
  leftNodes = std::vector<Utils::TopologyNode*>();
  rightNodes = std::vector<Utils::TopologyNode*>();
  extraLeftNodes = std::vector<Utils::TopologyNode*>();
  extraRightNodes = std::vector<Utils::TopologyNode*>();
  linkNodes = std::vector<Utils::TopologyNode*>();
  allNodes = std::vector<Utils::TopologyNode*>();
  links = std::vector<Link>();
  config = cfg;
}

std::vector<TopologyNode*> DumbbellTopology::getAllNodes()
{
  if (allNodes.size() != leftNodes.size() + rightNodes.size() + linkNodes.size() + extraLeftNodes.size() + extraRightNodes.size()) // TODO: What if you add and remove the same amount?
  {
    allNodes.clear();
    allNodes.reserve( leftNodes.size() + rightNodes.size() + linkNodes.size() );
    allNodes.insert( allNodes.end(), leftNodes.begin(), leftNodes.end() );
    allNodes.insert( allNodes.end(), rightNodes.begin(), rightNodes.end() );
    allNodes.insert( allNodes.end(), linkNodes.begin(), linkNodes.end() );
    allNodes.insert( allNodes.end(), extraRightNodes.begin(), extraRightNodes.end() ); // senders always go first
    allNodes.insert( allNodes.end(), extraLeftNodes.begin(), extraLeftNodes.end() );
  }
  
  return allNodes;
}

std::vector<TopologyNode*> DumbbellTopology::getHosts()
{
  std::vector<TopologyNode*> hosts;
  hosts.reserve( leftNodes.size() + rightNodes.size() + extraLeftNodes.size() + extraRightNodes.size() ); // preallocate memory
  hosts.insert( hosts.end(), leftNodes.begin(), leftNodes.end() );
  hosts.insert( hosts.end(), rightNodes.begin(), rightNodes.end() );
  hosts.insert( hosts.end(), extraRightNodes.begin(), extraRightNodes.end() );
  hosts.insert( hosts.end(), extraLeftNodes.begin(), extraLeftNodes.end() );
  return hosts;
}

void DumbbellTopology::addLink(Utils::TopologyNode *n1, Utils::TopologyNode *n2)
{
  n1->addNeighbor(n2->id, n2->isEndHost);
  n2->addNeighbor(n1->id, n1->isEndHost);
  
  double rate;
  double linkDelay;
  int bufferLen;
  if (n1->isEndHost || n2->isEndHost)
  {
    rate = config->hostLinkRate;
    linkDelay = 0.0;
    bufferLen = config->hostBufferLen;
  }
  else
  {
    rate = config->bottleneckRate;
    linkDelay = config->bottleneckDelay;
    bufferLen = config->bottleneckBufferLen;
  }
  
  std::string r = std::to_string(rate) + "Gbps"; 
  std::string d = std::to_string(linkDelay) + "us";
  std::string q = std::to_string(bufferLen) + "p"; 
  
  links.push_back(Link(n1, n2, r, d, q, getNewSubnet()));
}

std::vector<std::vector<int>> DumbbellTopology::getTrafficMatrix(void) 
{
  std::vector<std::vector<int>> matrix;
  for (int i = 0; i < numSenders; i++ )
  {
    std::vector<int> receivers;
    receivers.push_back(numSenders + i);
    matrix.push_back(receivers);
  }

  if (hasExtraFlows) {
    // skip over first right nodes from the original senders (they appear first in list of hosts)
    for (int i = 0; i < numSenders; i++ ){
      matrix.push_back({});
    }
    for (int i = 2*numSenders; i < 2*numSenders+numExtraSenders; i++ )
    {
      std::vector<int> receivers;
      receivers.push_back(numExtraSenders + i);
      matrix.push_back(receivers);
    }

  }
  return matrix;
}


void DumbbellTopology::GenerateTopology (DumbbellTopologyParameter* params)
{
  int n = params->_n;
  hasExtraFlows = params->hasExtraFlows;
  numExtraSenders = n*2;
  numSenders = n;

  // Create the left hosts
  for (int i = 0; i < n; i++)
  {
    TopologyNode* newNode = new TopologyNode(i, true);
    leftNodes.push_back(newNode);
  }

  // Create the right hosts
  for (int i = 0; i < n; i++)
  {
    TopologyNode* newNode = new TopologyNode(i + n, true);
    rightNodes.push_back(newNode);
  }

  // Create the routers
  for (int i = 0; i < 2; i++)
  {
    TopologyNode* newNode = new TopologyNode(i + n + n, false);
    linkNodes.push_back(newNode);
  }
  
  // Connect the left hosts
  for (int i = 0; i < n; i++)
  {
    TopologyNode* server = leftNodes.at(i);
    TopologyNode* router = linkNodes.at(0);
    addLink(server, router);
  }

  // Connect the right hosts
  for (int i = 0; i < n; i++)
  {
    TopologyNode* server = rightNodes.at(i);
    TopologyNode* router = linkNodes.at(1);
    addLink(server, router);
  }

  // Connect the routers
  TopologyNode* leftRouter = linkNodes.at(0);
  TopologyNode* rightRouter = linkNodes.at(1);
  addLink(leftRouter, rightRouter);

  // Add extra hosts and connect to routers
  int num_std_nodes = 2*n + 2;
  if (hasExtraFlows) {
    // Right nodes have lower indices now
    // 2n is number of normal hosts and there are two routers, so we offset by that
    for (int i = num_std_nodes; i < num_std_nodes + numExtraSenders; i++)
    {
      TopologyNode* newNode = new TopologyNode(i, true);
      newNode->isUntracedHost = true;
      extraRightNodes.push_back(newNode);
    }

    for (int i = num_std_nodes; i < num_std_nodes + numExtraSenders; i++)
    {
      TopologyNode* newNode = new TopologyNode(i + numExtraSenders, true);
      newNode->isUntracedHost = true;
      extraLeftNodes.push_back(newNode);
    }

    // Connect the left hosts
    for (int i = 0; i < numExtraSenders; i++)
    {
      TopologyNode* server = extraLeftNodes.at(i);
      TopologyNode* router = linkNodes.at(0);
      addLink(server, router);
    }

    // Connect the right hosts
    for (int i = 0; i < numExtraSenders; i++)
    {
      TopologyNode* server = extraRightNodes.at(i);
      TopologyNode* router = linkNodes.at(1);
      addLink(server, router);
    }

  }
}

/* ---------- Line Topology ---------- */

LineTopologyParameter::LineTopologyParameter(int routers, int hostsPerRouter, int nSenders)
{
  numRouters = routers;
  numHostsPerRouter = hostsPerRouter;
  numSenders = nSenders;
  is_deterministic = true;
  hasExtraFlows = true; //setting this just for the extra sims - should be disabled generally
}

std::string LineTopologyParameter::toString(void)
{
  std::stringstream ss;
  ss << "LineTopology{routers=" << numRouters << ";numHostsPerRouter=" << numHostsPerRouter << ";extraFlows=" << hasExtraFlows << "}";
  return ss.str();
}

LineTopologyConfig::LineTopologyConfig(double maxDelay, double btlRate, int buf, int hostBuf, double hostRate)
{
  maxRtt = maxDelay != 0 ? maxDelay : maxRtt;
  bottleneckRate = btlRate != 0 ? btlRate : bottleneckRate;
  bufferLen = buf != 0 ? buf : bufferLen;
  hostBufferLen = hostBuf != 0 ? hostBuf : hostBufferLen;
  hostLinkRate = hostRate != 0 ? hostRate : hostLinkRate;
}

std::string LineTopologyConfig::toString(void)
{
  std::stringstream ss;
  ss << "LineTopologyConfig{maxRtt=" << maxRtt << ";btlRate=" << bottleneckRate << ";buf=" 
  << bufferLen << ";hostBuf=" << hostBufferLen << ";hostRate" << hostLinkRate << "}";
  return ss.str();
}

LineTopology::LineTopology (LineTopologyConfig* cfg)
{
  hosts = std::vector<Utils::TopologyNode*>();
  routers = std::vector<Utils::TopologyNode*>();
  allNodes = std::vector<Utils::TopologyNode*>();
  links = std::vector<Link>();
  senders = 0;
  sender_hosts = 0;
  hasExtraFlows = false;
  config = cfg;
}

std::vector<TopologyNode*> LineTopology::getAllNodes()
{
  if (allNodes.size() != hosts.size() + routers.size()) 
  {
    allNodes.clear();
    allNodes.reserve( hosts.size() + routers.size() );
    allNodes.insert( allNodes.end(), hosts.begin(), hosts.end() );
    allNodes.insert( allNodes.end(), routers.begin(), routers.end() );
  }
  
  return allNodes;
}

std::vector<TopologyNode*> LineTopology::getHosts()
{
  return hosts;
}

void LineTopology::addLink(Utils::TopologyNode *n1, Utils::TopologyNode *n2)
{
  n1->addNeighbor(n2->id, n2->isEndHost);
  n2->addNeighbor(n1->id, n1->isEndHost);
  
  double rate;
  double linkDelay;
  int bufferLen;
  if (n1->isEndHost || n2->isEndHost)
  { 
    if (n1->isUntracedHost || n2->isUntracedHost) // Determines if it is an extra - can do untraced instead if(n1->isUntracedHost || n2->isUntracedHost)
    {
      rate = config->hostLinkRate;
      linkDelay = (config->maxRtt - ((double)config->maxRtt / ((double)routers.size() / 2.0))) / 2.0; // divide by thetwo host connections
      bufferLen = config->hostBufferLen;
    }
    else
    {
      rate = config->hostLinkRate;
      linkDelay = 0.0;
      bufferLen = config->hostBufferLen;
    }
  }
  else
  {
    rate = config->bottleneckRate;
    linkDelay = config->maxRtt / ((int)routers.size() / 2);
    bufferLen = config->bufferLen;
  }
  
  std::string r = std::to_string(rate) + "Gbps"; 
  std::string d = std::to_string(linkDelay) + "us";
  std::string q = std::to_string(bufferLen) + "p";
  
  links.push_back(Link(n1, n2, r, d, q, getNewSubnet()));
}

std::vector<std::vector<int>> LineTopology::getTrafficMatrix(void) 
{
  std::vector<std::vector<int>> matrix;
  for (int i = 0; i < senders; i++ )
  {
    std::vector<int> receivers = {i + sender_hosts};
    matrix.push_back(receivers);
  }
  if (hasExtraFlows) {
    for (int i = senders; i < sender_hosts * 2; i++ ) // these are the receivers ( or non-sending sender hosts)
    {
      std::vector<int> receivers = {};
      matrix.push_back(receivers);
    }
    for (int i = sender_hosts * 2; i < (int) hosts.size(); i++ ) 
    {
      if ((i - (sender_hosts * 2)) % 2 == 0)
      {
        std::vector<int> receivers = {i + 1};
        matrix.push_back(receivers);
      }
      else
      {
        std::vector<int> receivers = {};
        matrix.push_back(receivers);
      }
    }
  }
  return matrix;
}


void LineTopology::GenerateTopology (LineTopologyParameter* params)
{
// Create the hosts
  // std::cout << params->toString() << std::endl;
  senders = params->numSenders;
  int n_hosts = params->numRouters * params->numHostsPerRouter;
  sender_hosts = n_hosts / 2;
  hasExtraFlows = params->hasExtraFlows;
  // std::cout << "Creating " << n_hosts << " hosts..." << std::endl;
  for (int i = 0; i < n_hosts; i++)
  {
    TopologyNode* newNode = new TopologyNode(i, true);
    hosts.push_back(newNode);
  }

  // Create the routers
  // std::cout << "Creating " << params->numRouters << " routers..." << std::endl;
  for (int i = 0; i < params->numRouters; i++)
  {
    TopologyNode* newNode = new TopologyNode(i + n_hosts, false);
    routers.push_back(newNode);
  }
  
  // Connect the hosts
  for (int i = 0; i < n_hosts; i++)
  {
    // std::cout << "Connecting host " << i << " to router " << (int)std::floor(i/params->numHostsPerRouter) << std::endl;
    TopologyNode* server = hosts.at(i);
    TopologyNode* router = routers.at((int)std::floor(i/params->numHostsPerRouter));
    addLink(server, router);
  }

  // Connect the routers
  for (int i = 0; i < (params->numRouters - 1); i++) {
    // std::cout << "Connecting router " << i << " to router " << i+1 << std::endl;
    TopologyNode* router1 = routers.at(i);
    TopologyNode* router2 = routers.at(i+1);
    addLink(router1, router2);
  }

 if (hasExtraFlows) {
  // Create the extra hosts
  int numExtraHosts = (params->numRouters - 1) * 2;
  for (int i = 0; i < numExtraHosts; i++)
  {
    // std::cout << "Creating extra host " << i + n_hosts << std::endl;
    TopologyNode* newNode = new TopologyNode(i + n_hosts + params->numRouters, true);
    newNode->isUntracedHost = true;
    hosts.push_back(newNode);
  }

  // Connect the extra hosts
  for (int i = 0; i < numExtraHosts; i++)
  {
    int routerNum = std::ceil(i/2.0);
    if (i == (numExtraHosts - 1)) {
      routerNum = params->numRouters - 1;
    }
    // std::cout << "Connecting extra host " << i + n_hosts << " to router " << routerNum << std::endl;
    TopologyNode* server = hosts.at(i + n_hosts);
    TopologyNode* router = routers.at(routerNum);
    addLink(server, router);
  }
 }
}