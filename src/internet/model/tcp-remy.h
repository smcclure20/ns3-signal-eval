#ifndef TCPREMY_H
#define TCPREMY_H

#include <algorithm>
#include <cmath>
#include <limits>

#include "ns3/tcp-congestion-ops.h"
#include "ns3/tcp-socket-base.h"
#include "ns3/data-rate.h"
#include "tcp-option-ts.h"

#include "ns3/memory.hh"
#include "ns3/whiskertree.hh"
#include "ns3/packet.hh"

namespace ns3 {

/**
 * RemyCC for NS3
 */
class TcpRemy : public TcpCongestionOps
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  TcpRemy ();

  /**
   * Copy constructor
   * \param sock Socket to copy
   */
  TcpRemy (const TcpRemy& sock);
  ~TcpRemy ();

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
  virtual void Init (Ptr<TcpSocketState> tcb);
  virtual bool ChangesCwndOnRto () const { return false; }
  virtual void Reset (Ptr<TcpSocketState> tcb, Ptr<RttEstimator> rtt);
  virtual void NotifyIdle (bool is_idle);

private:

    const WhiskerTree * _whiskers;
    Memory _memory;
    double _intersend_time;
    double _last_last_send_attempt;
    bool _pkts_acked;
    uint64_t _cwnd; 
    int id;
    int _acks;
    int _pkts;
    bool _reset;
    bool _is_idle;

    void update_cwnd_and_pacing( Ptr<TcpSocketState> tcb );
    void update_memory( const RemyPacket packet );
};

} // namespace ns3

#endif // TCPREMY_H
