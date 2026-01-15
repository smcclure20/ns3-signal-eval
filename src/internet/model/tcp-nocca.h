#ifndef TCPNOCCA_H
#define TCPNOCCA_H

#include "ns3/tcp-congestion-ops.h"
#include "ns3/tcp-socket-base.h"
#include "ns3/data-rate.h"

namespace ns3 {

/**
 * RemyCC for NS3
 */
class TcpNoCca : public TcpCongestionOps
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  TcpNoCca ();

  /**
   * Copy constructor
   * \param sock Socket to copy
   */
  TcpNoCca (const TcpNoCca& sock);

  virtual std::string GetName () const;
  virtual void PktsAcked (Ptr<TcpSocketState> tcb, uint32_t segmentsAcked, const Time& rtt);
  virtual void IncreaseWindow (Ptr<TcpSocketState> tcb, uint32_t segmentsAcked);
  virtual uint32_t GetSsThresh (Ptr<const TcpSocketState> tcb, uint32_t bytesInFlight);
  virtual void CongestionStateSet (Ptr<TcpSocketState> tcb,
                                   const TcpSocketState::TcpCongState_t newState);

  virtual Ptr<TcpCongestionOps> Fork ();
  virtual bool HasCongControl () const { return true; }
  virtual void CongControl (Ptr<TcpSocketState> tcb,
                            const TcpRateOps::TcpRateConnection &rc,
                            const TcpRateOps::TcpRateSample &rs);
  virtual bool ChangesCwndOnRto () const { return false; }

};

} // namespace ns3

#endif // TCNOCCA_H
