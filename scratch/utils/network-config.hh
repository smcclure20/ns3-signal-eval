#ifndef NETWORK_CONFIG_H
#define NETWORK_CONFIG_H

#include <vector>
#include <cmath>
#include <stdlib.h>
#include <time.h>  
#include <string.h>
#include <random>

#include "parameter-range.hh"
#include "configrange.hh"


class NetworkConfig
{
    public: 
        double rtt;
        double link_rate;
        double remy_rate;
        int n_src;
        double on_mean;
        double on_bound;
        double off_mean;
        double off_bound;
        double error_rate;
        double buffer_size;

    NetworkConfig(double rtt, double link_rate, double remy_rate, int n_src, double on_mean, double on_bound, double off_mean, double off_bound, double error_rate, double buffer_size) : 
                 rtt( rtt ), 
                 link_rate( link_rate ),
                 remy_rate( remy_rate ),
                 n_src( n_src ),
                 on_mean( on_mean ),
                 on_bound( on_bound ),
                 off_mean( off_mean ),
                 off_bound( off_bound ),
                 error_rate( error_rate ),
                 buffer_size( buffer_size )
                 {}

    std::string toString();
};

class NetworkRange
{
    public:
        ParameterRange rtt;
        ParameterRange rate;
        ParameterRange nsrc;
        ParameterRange on;
        ParameterRange off;
        ParameterRange sloss;
        ParameterRange buffer;

    NetworkRange(double min_rtt, double max_rtt, double rtt_incr, double min_link_rate, double max_link_rate, double link_incr, int min_nsrc, int max_nsrc, int nsrc_incr,
                 double min_on, double max_on, double on_incr, double min_off, double max_off, double off_incr, double min_sloss, double max_sloss, double sloss_incr, 
                 int min_buf_size, int max_buf_size, int buf_size_incr) : 
                 rtt (ParameterRange(min_rtt, max_rtt, rtt_incr)),
                 rate (ParameterRange(min_link_rate, max_link_rate, link_incr)),
                 nsrc (ParameterRange(min_nsrc, max_nsrc, nsrc_incr)),
                 on (ParameterRange(min_on, max_on, on_incr)),
                 off (ParameterRange(min_off, max_off, off_incr)),
                 sloss (ParameterRange(min_sloss, max_sloss, sloss_incr)),
                 buffer (ParameterRange(min_buf_size, max_buf_size, buf_size_incr))
                 {}

    static double _sample_range(double min, double max, double incr, std::default_random_engine* prng) {  int index = (*prng)() % (int)(floor((max - min) / incr) + 1); return min + (incr * index); };

    static std::vector<NetworkConfig> generateNetworks(ConfigRange configRange, std::default_random_engine* prng, int sample_size=0, bool int_headers=false, int packet_size=590); //  TODO: Non-magic number (590 = 536 (Segment size) + 20 (TCP) + 20 (IP) + 14 (Eth) )
};

#endif /* NETWORK_CONFIG_H */