#ifndef REMYPACKET_HH
#define REMYPACKET_HH

class RemyPacket
{
public:
  double tick_sent, tick_received;
  double seq_no;
  double queue_stat;
  double link_stat;

  RemyPacket( const double & s_tick_sent, const double & s_tick_received, const double & seqno )
    : tick_sent( s_tick_sent ), tick_received( s_tick_received ), seq_no(seqno), queue_stat( 0 ), link_stat( 0 )
  {}

  RemyPacket( const double & s_tick_sent, const double & s_tick_received, const double & seqno, const double & s_queue, const double & s_link )
    : tick_sent( s_tick_sent ), tick_received( s_tick_received ), seq_no(seqno), queue_stat( s_queue ), link_stat( s_link )
  {}

  RemyPacket( const RemyPacket & other )
    : tick_sent( other.tick_sent ),
      tick_received( other.tick_received ),
      seq_no ( other.seq_no ),
      queue_stat( other.queue_stat ),
      link_stat( other.link_stat )
  {}
};

#endif
