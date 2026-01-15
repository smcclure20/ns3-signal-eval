#include <stdio.h>

#include "whisker.hh"

using namespace std;

Whisker::Whisker( const RemyBuffers::Whisker & dna )
  : _window_increment( dna.window_increment() ),
    _window_multiple( dna.window_multiple() ),
    _intersend( dna.intersend() ),
    _domain( dna.domain() )
{
}

string Whisker::str( void ) const
{
  char tmp[ 500 ];
  snprintf( tmp, 500, "{%s} => (win: %d + (%f * win) intersend: %.2f ms) (used: %u)",
            _domain.str().c_str(), _window_increment, _window_multiple, _intersend, count());
  return tmp;
}