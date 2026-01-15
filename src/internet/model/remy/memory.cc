#include <boost/functional/hash.hpp>
#include <vector>
#include <cassert>

#include "memory.hh"

using namespace std;

static const double alpha = 1.0 / 8.0;

static const double slow_alpha = 1.0 / 256.0;

static const double loss_memory = 20;

void Memory::packets_received( const vector< RemyPacket > & packets )
{
  for ( const auto &x : packets ) {
    const double rtt = x.tick_received - x.tick_sent;
    if ( _last_tick_sent == 0 || _last_tick_received == 0 ) {
      _last_tick_sent = x.tick_sent;
      _last_tick_received = x.tick_received;
      _min_rtt = rtt;

    } 
    else {
      if (x.seq_no > _last_seq_no + 1)
      {
        _recent_loss = (1 - alpha) * _recent_loss + alpha;
      } else {
        _recent_loss = (1 - alpha) * _recent_loss;
      }
      _last_seq_no = max(_last_seq_no, x.seq_no);
      double intersend = (x.tick_sent >= _last_tick_sent) ? (x.tick_sent - _last_tick_sent) : _rec_send_ewma; 
      double interreceive = (x.tick_received > _last_tick_received) ? (x.tick_received - _last_tick_received) : _rec_rec_ewma;

      _rec_send_ewma = (1 - alpha) * _rec_send_ewma + alpha * intersend;
      _rec_rec_ewma = (1 - alpha) * _rec_rec_ewma + alpha * interreceive;
      _slow_rec_rec_ewma = (1 - slow_alpha) * _slow_rec_rec_ewma + slow_alpha * interreceive;

      _last_tick_sent = (x.tick_sent >= _last_tick_sent) ? x.tick_sent : _last_tick_sent; 
      _last_tick_received = (x.tick_received > _last_tick_received) ? x.tick_received : _last_tick_received;

      _min_rtt = min( _min_rtt, (double)rtt );
      _rtt_ratio = double( rtt ) / double( _min_rtt );
      assert( _rtt_ratio >= 1.0 );

      _int_queue = x.queue_stat;
      _int_link = (1 - alpha) * _int_link + alpha * x.link_stat;

      if ( _slow_rec_rec_ewma > 16380 ) _slow_rec_rec_ewma = 16380;
      if ( _rec_send_ewma > 16380 ) _rec_send_ewma = 16380;
      if ( _rec_rec_ewma > 16380 ) _rec_rec_ewma = 16380;
      if ( _rtt_ratio > 16380 ) _rtt_ratio = 16380;

    }
  }
}


void Memory::update(bool is_first, double sendrate, double deliveryrate, uint32_t losses, double minrtt, double lastrtt, uint32_t inflight)
{
  if (!is_first){ // Skip the first update
    _rec_send_ewma = (1 - alpha) * _rec_send_ewma + alpha * (sendrate);
    _rec_rec_ewma = (1 - alpha) * _rec_rec_ewma + alpha * (deliveryrate);
    _slow_rec_rec_ewma = (1 - slow_alpha) * _slow_rec_rec_ewma + slow_alpha * (deliveryrate);
  }
  _recent_loss = losses > loss_memory ? loss_memory : losses;
  _min_rtt = minrtt;
  _rtt_ratio = double( lastrtt ) / double( _min_rtt );
  assert( _rtt_ratio >= 1.0 );
  _rtt_diff = lastrtt - _min_rtt;
  assert( _rtt_diff >= 0 );
  _queueing_delay = min((double)(_rec_rec_ewma * inflight), (double)163839);

  _rec_rec_ewma = min(_rec_rec_ewma, (double)163839);
}

string Memory::str( void ) const
{
  char tmp[ 512 ];
  snprintf( tmp, 512, "sewma=%f, rewma=%f, rttr=%f, slowrewma=%f, rttd=%f, qdelay=%f, loss=%f, intq=%f, intl=%f", _rec_send_ewma, _rec_rec_ewma, _rtt_ratio, _slow_rec_rec_ewma, _rtt_diff, _queueing_delay, _recent_loss, _int_queue, _int_link );
  return tmp;
}


Memory::Memory( const RemyBuffers::Memory & dna, unsigned int datasize )
  : _rec_send_ewma( dna.rec_send_ewma() ),
    _rec_rec_ewma( dna.rec_rec_ewma() ),
    _rtt_ratio( dna.rtt_ratio() ),
    _slow_rec_rec_ewma( dna.slow_rec_rec_ewma() ),
    _rtt_diff( dna.rtt_diff() ),
    _queueing_delay( dna.queueing_delay() ),
    _recent_loss( dna.recent_loss() ),
    _int_queue( dna.int_queue() ),
    _int_link( dna.int_link() ),
    _last_tick_sent( 0 ),
    _last_tick_received( 0 ),
    _min_rtt( 0 ),
    _losses( ),
    _datasize(datasize)
{
}
