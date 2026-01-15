#define NS_LOG_APPEND_CONTEXT \
  { std::clog << Simulator::Now ().GetSeconds () << " "; }

#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <iostream>
#include <math.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "ns3/log.h"

#include "tcp-nocca.h"

NS_LOG_COMPONENT_DEFINE ("TcpNoCca");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (TcpNoCca);

TypeId
TcpNoCca::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::TcpNoCca")
    .SetParent<TcpSocketBase> ()
    .AddConstructor<TcpNoCca> ()
    .SetGroupName ("Internet")
  ;
  return tid;
}

TcpNoCca::TcpNoCca ()
  : TcpCongestionOps ()
{
  NS_LOG_FUNCTION (this);
}

TcpNoCca::TcpNoCca (const TcpNoCca &sock)
  : TcpCongestionOps (sock)
{
  NS_LOG_FUNCTION (this);
}

std::string
TcpNoCca::GetName () const
{
  return "TcpNoCca";
}

void
TcpNoCca::IncreaseWindow (Ptr<TcpSocketState> tcb, uint32_t segmentsAcked)
{
  NS_LOG_FUNCTION (this << tcb << segmentsAcked);
}

void
TcpNoCca::PktsAcked (Ptr<TcpSocketState> tcb, uint32_t segmentsAcked,
                     const Time &rtt)
{
  NS_LOG_FUNCTION (this << tcb << segmentsAcked << rtt);
}

uint32_t
TcpNoCca::GetSsThresh (Ptr<const TcpSocketState> tcb, uint32_t bytesInFlight)
{
  NS_LOG_FUNCTION (this << tcb << bytesInFlight);

  return tcb->m_ssThresh;
}

void
TcpNoCca::CongestionStateSet (Ptr<TcpSocketState> tcb, const TcpSocketState::TcpCongState_t newState)
{
  NS_LOG_FUNCTION (this << tcb << newState);
}

Ptr<TcpCongestionOps>
TcpNoCca::Fork (void)
{
  NS_LOG_FUNCTION (this);
  return CopyObject<TcpNoCca> (this);
}

void TcpNoCca::CongControl (Ptr<TcpSocketState> tcb, const TcpRateOps::TcpRateConnection &rc, const TcpRateOps::TcpRateSample &rs)
{
    tcb->m_cWnd = std::numeric_limits<uint32_t>::max();
}

}
