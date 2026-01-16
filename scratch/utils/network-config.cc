#include "network-config.hh"

std::string NetworkConfig::toString()
{
    std::ostringstream oss;
    oss << "{rtt=" << rtt << ";rate=" << link_rate << ";remyrate=" << remy_rate << ";senders=" << n_src << ";onmean=" << on_mean << ";onbound=" << on_bound << ";offmean="
     << off_mean << ";offbound=" << off_bound << ";sloss=" << error_rate << ";buf=" << buffer_size << "}" << std::endl;
    return oss.str();
}

std::vector<NetworkConfig> NetworkRange::generateNetworks(ConfigRange configRange, std::default_random_engine* prng, int sample_size, bool int_headers, int packet_size)
{
    std::vector<NetworkConfig> configs;
    for (int i = 0; i < sample_size; i++ ) {
        double link_ppt =  _sample_range(configRange.link_ppt.low, configRange.link_ppt.high, configRange.link_ppt.incr, prng);
        int packet_size_adj = int_headers ? packet_size + 8 : packet_size; 
        double link_rate = packet_size_adj * 8 * pow(10, 6) * link_ppt; 
        double rtt_val =  _sample_range(configRange.rtt.low, configRange.rtt.high, configRange.rtt.incr, prng);
        int senders =  (int)_sample_range(configRange.num_senders.low, configRange.num_senders.high, configRange.num_senders.incr, prng);
        double on_mean =  _sample_range(configRange.mean_on_duration.low, configRange.mean_on_duration.high, configRange.mean_on_duration.incr, prng);
        double off_mean =  _sample_range(configRange.mean_off_duration.low, configRange.mean_off_duration.high, configRange.mean_off_duration.incr, prng);
        double loss =  _sample_range(configRange.stochastic_loss_rate.low, configRange.stochastic_loss_rate.high, configRange.stochastic_loss_rate.incr, prng);
        double buffer_len =  _sample_range(configRange.buffer_size.low, configRange.buffer_size.high, configRange.buffer_size.incr, prng);
        configs.push_back( NetworkConfig(rtt_val, link_rate, link_ppt, senders, on_mean, on_mean * 10, off_mean, off_mean * 10, loss, buffer_len) );
    }

    return configs;
}