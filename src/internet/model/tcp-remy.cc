#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <iostream>
#include <math.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <iostream> 
#include <fstream>

#include "ns3/log.h"

#include "tcp-remy.h"

// #define NS_LOG_APPEND_CONTEXT 
//   { std::clog << Simulator::Now ().GetSeconds () << " "; }


NS_LOG_COMPONENT_DEFINE ("TcpRemy");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (TcpRemy);

TypeId
TcpRemy::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::TcpRemy")
    .SetParent<TcpSocketBase> ()
    .AddConstructor<TcpRemy> ()
    .SetGroupName ("Internet")
  ;
  return tid;
}

TcpRemy::TcpRemy ()
  : TcpCongestionOps ()
{
  NS_LOG_FUNCTION (this);
  id = rand() % 1000;
  
  /* get whisker filename */
	const char *filename = getenv( "WHISKERS" );
	if ( !filename ) {
		fprintf( stderr, "RemyTCP: Missing WHISKERS environment variable.\n" );
		throw 1;
	}

	/* open file */
	int fd = open( filename, O_RDONLY );
	if ( fd < 0 ) {
		perror( "open" );
		throw 1;
	}

	/* parse whisker definition */
	RemyBuffers::WhiskerTree tree;
	if ( !tree.ParseFromFileDescriptor( fd ) ) {
		fprintf( stderr, "RemyTCP: Could not parse whiskers in \"%s\".\n", filename );
		throw 1;
	}

	/* close file */
	if ( ::close( fd ) < 0 ) {
		perror( "close" );
		throw 1;
	}

	/* store whiskers */
	_whiskers = new WhiskerTree( tree );

	_pkts_acked = false;
	_acks = 0;
	_pkts = 0;
	_reset = false;
	_is_idle = false;
}

TcpRemy::~TcpRemy ()
{
	NS_LOG_FUNCTION (this);
	
	int save_whiskers = atoi(getenv( "SAVEWHISKERS" ));

	if (save_whiskers == 1) {
		/* dump whiskers to file */
		/* open file */
		std::ofstream fd;
		std::ostringstream oss;
  		oss << "./whiskers-" << id << ".txt";
		fd.open( oss.str().c_str() );

		/* parse whisker definition */
		fd << _whiskers->str();

		/* close file */
		fd.close();
	}
}


TcpRemy::TcpRemy (const TcpRemy &sock)
  : TcpCongestionOps (sock),
    _whiskers (sock._whiskers),
    _memory (sock._memory),
    _intersend_time (sock._intersend_time),
	_last_last_send_attempt (sock._last_last_send_attempt),
	_pkts_acked (sock._pkts_acked),
	_cwnd (sock._cwnd),
	id(sock.id),
	_acks(sock._acks),
	_pkts(sock._pkts),
	_reset(sock._reset),
	_is_idle(sock._is_idle)
{
  NS_LOG_FUNCTION (this);
}

std::string
TcpRemy::GetName () const
{
	return "TcpRemy";
}

void
TcpRemy::update_cwnd_and_pacing( Ptr<TcpSocketState> tcb )
{
	NS_LOG_FUNCTION (this << tcb);
	NS_LOG_DEBUG ("Updating CWND and pacing for Remy ID " << id);

	const Whisker & current_whisker( _whiskers->use_whisker( _memory ) );
	NS_LOG_DEBUG ("Current memory: " << _memory.str());
	NS_LOG_DEBUG ("Whisker: " << current_whisker.str());
	
	unsigned int new_cwnd = current_whisker.window( _cwnd );

	if (new_cwnd > 16384)
	{
		new_cwnd = 16384;
	}
	_cwnd = new_cwnd;
	tcb->m_cWnd = (new_cwnd) * tcb->m_segmentSize;

	NS_LOG_DEBUG("Intersend: " << current_whisker.intersend());

	_intersend_time =  current_whisker.intersend() * _memory.field(1); //

	if (_intersend_time == 0) 
	{
		tcb->m_pacing = false;
		NS_LOG_DEBUG ("New state: cwnd=" << tcb->GetCwndInSegments() << " intersend=" << _intersend_time << " (rate=NAN)");
	}
	else 
	{
		tcb->m_pacing = true;
		DataRate rate = DataRate(((tcb->m_segmentSize) * 8 * pow(10, 6)) / _intersend_time); // comes from whiskers in us/packet
    	tcb->m_pacingRate = rate;
		NS_LOG_DEBUG ("New state: cwnd=" << tcb->GetCwndInSegments() << " intersend=" << _intersend_time << " (rate=" << rate.GetBitRate()/pow(10,6) << ")");
	}
}

void
TcpRemy::IncreaseWindow (Ptr<TcpSocketState> tcb, uint32_t segmentsAcked)
{
	NS_LOG_FUNCTION (this << tcb << segmentsAcked);

}

void
TcpRemy::Reset (Ptr<TcpSocketState> tcb, Ptr<RttEstimator> rtt)
{
	NS_LOG_FUNCTION (this << tcb);
	rtt->Reset();
	_memory.reset();
	_cwnd = 0;
	_pkts_acked = false;
	update_cwnd_and_pacing(tcb);
	tcb->m_initialCWnd = tcb->m_cWnd / tcb->m_segmentSize;
}

void 
TcpRemy::NotifyIdle (bool is_idle)
{
	NS_LOG_FUNCTION (this << is_idle);
	_is_idle = is_idle;
}

void
TcpRemy::PktsAcked (Ptr<TcpSocketState> tcb, uint32_t segmentsAcked,
                     const Time &rtt)
{
	NS_LOG_FUNCTION (this << tcb << segmentsAcked << rtt);
	_acks = segmentsAcked;
	_pkts = segmentsAcked > 0 ? _pkts + 1 : _pkts;
	_pkts_acked = (!_pkts_acked && segmentsAcked == 0) ? false : true; // do not want to do anything for just the syn packet
}

uint32_t
TcpRemy::GetSsThresh (Ptr<const TcpSocketState> tcb, uint32_t bytesInFlight)
{
	NS_LOG_FUNCTION (this << tcb << bytesInFlight);
	return tcb->m_ssThresh;
}

void
TcpRemy::CongestionStateSet (Ptr<TcpSocketState> tcb, const TcpSocketState::TcpCongState_t newState)
{
  	NS_LOG_FUNCTION (this << tcb << newState);
}

void
TcpRemy::Init (Ptr<TcpSocketState> tcb)
{
	NS_LOG_FUNCTION (this << tcb);
	tcb->m_pacing = true;
	tcb->m_paceInitialWindow = true;
	_cwnd = 0;
	update_cwnd_and_pacing(tcb);
	tcb->m_initialCWnd = tcb->m_cWnd / tcb->m_segmentSize;
}

Ptr<TcpCongestionOps>
TcpRemy::Fork (void)
{
	NS_LOG_FUNCTION (this);
	return CopyObject<TcpRemy> (this);
}

void 
TcpRemy::update_memory( const RemyPacket packet )
{
	NS_LOG_FUNCTION (this);
	std::vector< RemyPacket > packets( 1, packet );
	_memory.packets_received( packets );
}

void TcpRemy::CongControl (Ptr<TcpSocketState> tcb, const TcpRateOps::TcpRateConnection &rc, const TcpRateOps::TcpRateSample &rs)
{
	NS_LOG_FUNCTION (this << tcb);
	NS_LOG_DEBUG("ACKS: " << _acks);
	// Don't rate limit a socket that is not sending data, only ACKs
	// Current conflates receiver side with a sender that just hasn't gotten an ACK yet
	// But that's fine because CongControl only called after first ACK
	if (!_pkts_acked)
	{
		tcb->m_pacing = false;
		tcb->m_cWnd = 0; 
		NS_LOG_DEBUG("Not pacing; CWND is " << tcb->m_cWnd);
	}
	else if (_is_idle)
	{
		tcb->m_pacing = false;
		tcb->m_cWnd = 0;
	}
	else 
	{ 
		tcb->m_pacing = true;

		double rtt; 
		double timestamp = TcpOptionTS::ElapsedTimeFromTsValue(tcb->m_rcvTimestampEchoReply).GetNanoSeconds();

		int remy_seq_no = (int) (tcb->m_lastAckedSeq.GetValue() / tcb->m_segmentSize);
		NS_LOG_DEBUG ("Remy sequence number: " << remy_seq_no);
		
		rtt = timestamp /1000.0;
		update_memory( RemyPacket( rc.m_deliveredTime.GetNanoSeconds() / 1000.0 - rtt, rc.m_deliveredTime.GetNanoSeconds()/1000.0, (double) remy_seq_no, (double) tcb->m_lastIntQueue,  (double) tcb->m_lastIntLink ) );


		NS_LOG_DEBUG ("Inflight: " << tcb->m_bytesInFlight / tcb->m_segmentSize);

		update_cwnd_and_pacing(tcb);

		if (tcb->m_bytesInFlight / tcb->m_segmentSize >= _cwnd) {
			NS_LOG_DEBUG("WINDOW LIMITED!!");
		}

		NS_LOG_INFO(id << "," << Simulator::Now().GetMicroSeconds()/1000.0 << "," << _memory.min_rtt() << "," << rtt << "," << _memory.field(0) << "," << _memory.field(1) << "," << _memory.field(2) << "," << _memory.field(3) << "," << _cwnd << "," << _intersend_time);
		if (((int)tcb->m_cWnd.Get() == 0) && ((int)tcb->m_bytesInFlight.Get() == 0))
		{
			NS_LOG_DEBUG ("Flow shutdown!!!");
		}
	}
}
}