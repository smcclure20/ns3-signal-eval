import sem
import sys
import math
import os

from commonanalysis import *

#TODO: Be more efficient about only running through the results once

# NOTE: Set these paths for your environment! (then set paths_set to True)
# Strongly recommend you use absolute paths (could not get references to home (~) to work)
paths_set = False

ns_path = "/path/to/ns-3.36"
script = "single-link-v2"
results_dir = "/path/to/output/dir/" 
base_rplusoutput_path = "/path/to/rplus/results/{}/{}/" # First spot is filled with provided rplus dir name (-w) and second is with the int type (eg, link)
netconfig_path = "/path/to/netconfig/dir"

if not paths_set:
    print("You must set the paths at the beginning of the script for your environment")
    exit(1)


cca = "Remy" # This is the internal name for rplus
int_type_list = []
gen_list = None
load = False
suffix = None
config = None
whisker_dir = None
delay_coefficient = 1
tput_coefficient = 1
byteswitched = False
save_whiskers = False
link_int_util = True
link_interval = 10
tput_normalized = True
linetopo = False
sim_time = 11.0
overwrite = False
pktsize = 590 # TODO: add this as a parameter, but note this is only used for a backwards compatibility issue anyways (usually not used)

for i in range(len(sys.argv)):
    if sys.argv[i] == "-c":
        cca = sys.argv[i+1]
    elif sys.argv[i] == "-t":
        int_type_list = sys.argv[i+1].split(",")
    elif sys.argv[i] == "-g":
        gen_list = sys.argv[i+1]
    elif sys.argv[i] == "-l":
        load = True
    elif sys.argv[i] == "-s":
        suffix = sys.argv[i+1]
    elif sys.argv[i] == "-w":
        whisker_dir = sys.argv[i+1]
    elif sys.argv[i] == "-cf":
        config = sys.argv[i+1]
    elif sys.argv[i] == "-dc":
        delay_coefficient = int(sys.argv[i+1])
    elif sys.argv[i] == "-tc":
        tput_coefficient= int(sys.argv[i+1])
    elif sys.argv[i] == "-b":
        byteswitched = True
        sim_time = 10.0
    elif sys.argv[i] == "-ws":
        save_whiskers = True
    elif sys.argv[i] == "-av":
        link_int_util = False
    elif sys.argv[i] == "-li":
        link_interval = int(sys.argv[i+1])
    elif sys.argv[i] == "-tn":
        tput_normalized = False
    elif sys.argv[i] == "-lp":
        print("Warning: not implemented")
        linetopo = True
    elif sys.argv[i] == "-O":
        overwrite = True

if cca == None:
    print("You must set a CCA")
    exit(1)
if int_type_list == None:
    print("You must set a whisker type")
    exit(1)
if gen_list == None:
    print("You must set a whisker generation")
    exit(1)
if whisker_dir == None:
    print("You must set a whisker directory")
    exit(1)
if config == None:
    print("You must set a config")
    exit(1)

gens = gen_list.split(",")


results_dir = results_dir # + "{}-results/".format(cca)
file_suffix = "-{}".format(whisker_dir)
if not(suffix is None):
    file_suffix += "-{}".format(suffix)
campaign_dir = results_dir + "sem/sem{}".format(file_suffix)

whisker_type_dirs = []
int_enabled_values = {"queue": True, "link": True, "int": True, "no-int": False, "ds": False, 
                      "sr": False, "dsrb": False, "dr": False, "srint": True, "srintl": True, "srintq": True,
                      "loss": False, "losslink": True, "lossqueue": True, "lossint": True, "lossdelay": False}

int_type_to_dir_map = {"intq": "queue", "intl": "link", "noint": "no-int"} # Some dirs are named slightly differently than the input here
for int_type in int_type_list:
    whisker_type_dir=int_type_to_dir_map.get(int_type, int_type)

    whisker_type_dirs.append(whisker_type_dir)

if not load:
    campaign = sem.CampaignManager.new(ns_path, script, campaign_dir, overwrite=overwrite, max_parallel_processes=50, check_repo=False, skip_configuration=True)
else:
    campaign = sem.CampaignManager.load(campaign_dir)

print(campaign)

runs = 200
subruns = 10

whisker_file_name = "{}.{}"
final_result_file_format = results_dir + "results-{}{}-g{}.csv"
whisker_stat_file_format = results_dir + "whiskerstats-{}{}-g{}"
whisker_stat_file_suffix = "-r{}.csv"


whiskerfiles = []
final_result_files = []
whiskerstat_files = []
int_type_to_whiskers = {"link": [], "queue": [], "int" : [], "no-int": [], "ds": [], "sr": [], "dsrb": [], "dr": [], "srint": [], "srintl": [], "srintq": [],
                         "loss": [], "losslink": [], "lossqueue": [], "lossint": [], "lossdelay": []}
for int_type in whisker_type_dirs:
    whisker_file_names = os.listdir(base_rplusoutput_path.format(whisker_dir, int_type))
    whisker_file_names = [base_rplusoutput_path.format(whisker_dir, int_type) + x for x in whisker_file_names]
    gen_tokens = [x.split(".")[-1] for x in whisker_file_names]
    for g in gens:
        if g in gen_tokens:
            index = gen_tokens.index(g)
            whiskerfiles.append(whisker_file_names[index])
            int_type_to_whiskers[int_type].append(whisker_file_names[index])
            final_result_files.append(final_result_file_format.format(int_type.replace("-", ""), file_suffix, g))
            whiskerstat_files.append(whisker_stat_file_format.format(int_type.replace("-", ""), file_suffix, g) + whisker_stat_file_suffix)
        else:
            print("WARNING: Generation {} not available for whisker directory {}".format(g,base_rplusoutput_path.format(whisker_dir, int_type)) )

for whisker_int_type in whisker_type_dirs:
    params_list = []
    for genfile in int_type_to_whiskers[whisker_int_type]:
        for seed in range(runs):
            params = {
                'cca': "ns3::Tcp{}".format(cca.capitalize()), 
                'netfile': netconfig_path + "/{}".format(config),
                'samplesize': 1,
                'whiskerfile': genfile,
                'seed' : seed,
                'configruns': subruns,
                'intenabled': int_enabled_values[whisker_int_type],
                'linkintutil': link_int_util,
                'linkinterval': link_interval,
                'delaycoef': delay_coefficient,
                'tputcoef': tput_coefficient,
                'byteswitched': byteswitched,
                'simtime': sim_time,
                'savewhiskerstats': save_whiskers,
                'RngRun': seed
            }
            params_list.append(params)

    if load:
        campaign.load(campaign_dir) # TODO: move this way up 
    else:
        campaign.run_missing_simulations(params_list)

i = 0
for wf in whiskerfiles:
    search_params = {"whiskerfile": wf}
    results = campaign.db.get_complete_results(search_params)
    output_file = open(final_result_files[i], "w")
    
    config_param_names = sorted(get_config(results[0]).keys())
    output_file.write("Run,Seed,Score,ScoreVar,TputScore,DelayScore,TputNorm,DelayNorm,Tput,Delay,TputRunVar,DelayRunVar,TputFlowVar,DelayFlowVar,Fairness,AvgTailDelay,AvgMaxFCT,{}\n".format(",".join(config_param_names)))

    run_num = 0

    for result in results:
        config = get_config(result)
        sorted_values = [config[x] for x in sorted(config.keys())]
        config_string = ",".join(sorted_values)
        if not tput_normalized:
            score = get_non_share_normalized_score(result, float(config["remyrate"]))
        else:
            score = get_avg_score(result)
        score_var = get_reproducibility_score(result)
        tput_score, delay_score = get_avg_subscores(result)

        pktsize = pktsize + 8 if params["intenabled"] else pktsize
        remy_rate =  float(config["remyrate"]) if "remyrate" in config.keys() else float(config["rate"]) / (pktsize * 8 * 10**3) # This is just for backwards compatibility
        n_tput_score, n_delay_score = get_avg_normalized_subscores(result, remy_rate, float(config["rtt"]))
        raw_n_tput, raw_n_delay, raw_n_tput_var, raw_n_delay_var = get_summary_normalized_tput_and_delay(result, remy_rate, float(config["rtt"]))
        tput_run_var, delay_run_var = get_reproducibility_tput_delay(result, remy_rate, float(config["rtt"]))
        tail_delay, tail_fct = get_tail_metrics(result)
        fairness_score = get_avg_fairness(result)
        output_file.write("{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{}\n".format(result["meta"]["id"], result["params"]["seed"], score, score_var, tput_score, delay_score,
                                                                                      n_tput_score, n_delay_score, raw_n_tput, raw_n_delay, tput_run_var, delay_run_var, 
                                                                                      raw_n_tput_var, raw_n_delay_var, fairness_score, tail_delay, tail_fct, config_string))

        # Whisker stats
        if save_whiskers:
            whisker_csv = open(whiskerstat_files[i].format(run_num), "w")
            whisker_stats = parse_whisker_stats(result)
            ordered_keys = whisker_stats[0][0].keys()

            low_names = ["lo_{}".format(x) for x in ordered_keys]
            hi_names = ["hi_{}".format(x) for x in ordered_keys]
            header = ",".join(low_names) + "," +  ",".join(hi_names) + ",cadd,cmult,inter,use"
            whisker_csv.write(header+"\n")

            for whisker in whisker_stats:
                low_stats = [whisker[0][x] for x in ordered_keys]
                hi_stats = [whisker[1][x] for x in ordered_keys]
                line = ",".join(low_stats) + "," +  ",".join(hi_stats) + "," + str(whisker[-4]) + "," + str(whisker[-3]) + "," + str(whisker[-2]) + "," + str(whisker[-1])
                whisker_csv.write(line+"\n")
            whisker_csv.close()

        run_num += 1
    i += 1

    output_file.close()