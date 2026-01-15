#ifndef MEMORY_HH
#define MEMORY_HH

#include <vector>
#include <string>
#include <queue>

#include "dna.pb.h"
#include "packet.hh"

class Memory {
public:
  typedef double DataType;

private:
  DataType _rec_send_ewma;
  DataType _rec_rec_ewma;
  DataType _rtt_ratio;
  DataType _slow_rec_rec_ewma;
  DataType _rtt_diff;
  DataType _queueing_delay;
  DataType _recent_loss;
  DataType _int_queue;
  DataType _int_link;

  double _last_tick_sent;
  double _last_tick_received;
  double _min_rtt;
  double _last_seq_no;
  std::queue< double > _losses;
  const unsigned int _datasize;

public:
  Memory( const std::vector< DataType > & s_data )
    : _rec_send_ewma( s_data.at( 0 ) ),
      _rec_rec_ewma( s_data.at( 1 ) ),
      _rtt_ratio( s_data.at( 2 ) ),
      _slow_rec_rec_ewma( s_data.at( 3 ) ),
      _rtt_diff( s_data.at(4) ),
      _queueing_delay( s_data.at(5) ),
      _recent_loss( s_data.at(6) ),
      _int_queue( s_data.at(7) ),
      _int_link( s_data.at(8) ),
      _last_tick_sent( 0 ),
      _last_tick_received( 0 ),
      _min_rtt( 0 ),
      _last_seq_no ( 0 ),
      _losses( ),
      _datasize(atoi(getenv( "NUMDIMS" )))
  {}

  Memory()
    : _rec_send_ewma( 0 ),
      _rec_rec_ewma( 0 ),
      _rtt_ratio( 0.0 ),
      _slow_rec_rec_ewma( 0 ),
      _rtt_diff( 0 ),
      _queueing_delay( 0 ),
      _recent_loss( 0 ),
      _int_queue( 0 ),
      _int_link( 0 ),
      _last_tick_sent( 0 ),
      _last_tick_received( 0 ),
      _min_rtt( 0 ),
      _last_seq_no ( 0 ),
      _losses( ),
      _datasize(atoi(getenv( "NUMDIMS" )))
  {}

  void reset( void ) { _rec_send_ewma = _rec_rec_ewma = _rtt_ratio = _slow_rec_rec_ewma = _rtt_diff = _queueing_delay = _recent_loss  = _int_queue = _int_link = _last_tick_sent = _last_tick_received = _min_rtt = _last_seq_no = 0; _losses = std::queue< double > (); }

  const DataType & field( unsigned int num ) const { return num == 0 ? _rec_send_ewma : num == 1 ? _rec_rec_ewma : num == 2 ? _rtt_ratio : num == 3 ? _slow_rec_rec_ewma : num == 4 ? _rtt_diff : num == 5 ? _queueing_delay : num == 6 ? _recent_loss : num == 7 ? _int_queue : _int_link ; }
  DataType & mutable_field( unsigned int num )     { return num == 0 ? _rec_send_ewma : num == 1 ? _rec_rec_ewma : num == 2 ? _rtt_ratio : num == 3 ? _slow_rec_rec_ewma : num == 4 ? _rtt_diff : num == 5 ? _queueing_delay : num == 6 ? _recent_loss : num == 7 ? _int_queue : _int_link ; }

  double min_rtt(void) {return _min_rtt;}

  void update( bool is_first, double sendrate, double deliveryrate, uint32_t losses, double minrtt, double lastrtt, uint32_t inflight );
  void packets_received( const std::vector< RemyPacket > & packets );

  std::string str( void ) const;

  bool operator>=( const Memory & other ) const { 
    for (unsigned int i = 0; i < _datasize; i ++) { if ( field(i) < other.field(i) ) return false; }
    return true;
  }
  bool operator<( const Memory & other ) const { 
    for (unsigned int i = 0; i < _datasize; i ++) { if ( field(i) >= other.field(i) ) return false; }
    return true;
  }
  bool operator==( const Memory & other ) const { 
    for (unsigned int i = 0; i < _datasize; i ++) { if ( field(i) != other.field(i) ) return false; }
    return true;
  }

  Memory( const RemyBuffers::Memory & dna, unsigned int datasize );
};

#endif
