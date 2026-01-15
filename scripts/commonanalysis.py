import math

def parse_memory(memstring):
    memory = {}
    tokens = memstring.split(", ")
    for token in tokens:
        var = token.split("=")
        name = var[0]
        value = var[1]
        memory[name] = value
    return memory

def get_whisker_file_names(result):
    keys = result["output"].keys()
    whisker_files = []
    for key in keys:
        if key.startswith("whiskers"):
            whisker_files.append(key)
    return whisker_files

# BUG: right now, its only getting the first sender. Should we change? Average them?
def parse_whisker_stats(result):
    whiskers = {}
    whisker_files = get_whisker_file_names(result)
    for wf in whisker_files:
        output = result['output'][wf]
        lines = output.split("\n")
        for line in lines:
            if line.startswith("["):
                line = line.strip("[").strip("]")
                parts = line.split(" => ")
                memory_range = parts[0].strip("{(").strip(")}")
                hi_and_low = memory_range.split(" hi: ")
                low_mem = parse_memory(hi_and_low[0].strip("lo: <").strip(">"))
                hi_mem = parse_memory(hi_and_low[1].strip("<").strip(">"))
                action_and_use = parts[1]
                use = action_and_use.split(") (")[1].split(": ")[1].strip(")")
                # (win: %d + (%f * win) intersend: %.2f ms)
                action = action_and_use.split(") (")[0]
                cwnd_add = action.split(" + ")[0].split(" ")[1]
                cwnd_mult = action.split(" + ")[1].split(" ")[0].strip("(")
                intersend = action.split(": ")[-1].split(" ")[0]

                if memory_range in whiskers.keys():
                    whiskers[memory_range] = (low_mem, hi_mem, cwnd_add, cwnd_mult, intersend, int(use) + whiskers[memory_range][-1])
                else:
                    whiskers[memory_range] = (low_mem, hi_mem, cwnd_add, cwnd_mult, intersend, int(use))
    return list(whiskers.values())
        

def get_avg_subscores(result): #todo: make this an option on the average score function
    total_tput_score = 0
    total_delay_score = 0
    runs = 0
    output = result['output']['stdout']
    lines = output.split("\n")
    for line in lines:
        if line.startswith("Throughput"):
            scores = line.split(";")
            tput = float(scores[0].split(":")[1].strip())
            delay = float(scores[1].split(":")[1].strip())
            total_tput_score += tput
            total_delay_score += delay
            runs += 1
    if runs != 0:
        return (total_tput_score/runs, total_delay_score/runs)
    else:
        return (0,0)

def get_avg_normalized_subscores(result, rate, rtt):
    total_tput_score = 0
    total_delay_score = 0
    runs = 0
    output = result['output']['stdout']
    lines = output.split("\n")
    for line in lines:
        if line.startswith("Tput"):
            scores = line.split(";")
            tput = float(scores[0].split(":")[1].strip())
            delay = float(scores[1].split(":")[1].strip())
            tput_score = 0 if tput == 0 else math.log2(tput / rate) 
            delay_score = 0 if delay == 0 else math.log2(delay / rtt) 
            total_tput_score += tput_score # TODO: Maybe just give the average of these values from the simulation?
            total_delay_score += delay_score
            runs += 1
    if runs != 0:
        return (total_tput_score/runs, total_delay_score/runs)
    else:
        return (0,0)
    
def variance(measurements):
    mean = sum(measurements) / len(measurements)
    squared_differences = [(i - mean) ** 2 for i in measurements]
    variance = sum(squared_differences) / len(measurements)
    return variance

def get_summary_normalized_tput_and_delay(result, rate, rtt):
    runs = 0
    flows = 0
    total_tput = 0
    tput_var = 0
    total_delay = 0
    delay_var = 0
    flow_tputs = []
    flow_delays = []
    output = result['output']['stdout']
    lines = output.split("\n")
    for line in lines:
        if line.startswith("Simulating..."):
            if runs != 0:
                if flows == 0:
                    total_tput += 0
                    tput_var += 0
                    total_delay += 0
                    delay_var += 0
                else:
                    total_tput += sum(flow_tputs) / flows
                    tput_var += variance(flow_tputs)
                    total_delay += sum(flow_delays) / flows
                    delay_var += variance(flow_delays)
            runs += 1
            flows = 0
            flow_tputs = []
            flow_delays = []
        if line.startswith("Tput"):
            flows += 1
            parts = line.split(";")
            tput = float(parts[0].split(":")[-1])
            delay = float(parts[1].split(":")[-1])
            flow_tputs.append(tput / rate)
            flow_delays.append(delay / rtt)
    if flows == 0:
        total_tput += 0
        tput_var += 0
        total_delay += 0
        delay_var += 0
    else:
        total_tput += sum(flow_tputs) / flows
        tput_var += variance(flow_tputs)
        total_delay += sum(flow_delays) / flows
        delay_var += variance(flow_delays)
    return total_tput/runs, total_delay/runs, tput_var/runs, delay_var/runs

def get_reproducibility_score(result):
    runs = 0
    scores = []
    output = result['output']['stdout']
    lines = output.split("\n")
    for line in lines:
        if "Score" in line and not("inf" in line):
            score = float(line.split(":")[-1])
            scores.append(score)
            runs += 1
    return variance(scores)

def get_reproducibility_tput_delay(result, rate, rtt):
    runs = 0
    avg_tputs = []
    avg_delays = []
    run_total_tput = 0
    run_total_delay = 0
    flows = 0
    output = result['output']['stdout']
    lines = output.split("\n")
    for line in lines:
        if "Tput" in line:
            scores = line.split(";")
            flows += 1
            tput = float(scores[0].split(":")[1].strip())
            delay = float(scores[1].split(":")[1].strip())
            run_total_tput += tput / rate
            run_total_delay += delay / rtt
        if line.startswith("Simulating..."):
            if runs != 0:
                if flows == 0:
                    avg_tputs.append(0)
                    avg_delays.append(0)
                else:
                    avg_tputs.append(run_total_tput / flows)
                    avg_delays.append(run_total_delay / flows)
                run_total_tput = 0
                run_total_delay = 0
                flows = 0
            runs += 1
    return variance(avg_tputs), variance(avg_delays)


def get_non_share_normalized_score(result, rate):
    total_flow_score = 0
    total_score = 0
    runs = 0
    flow_num = 0
    output = result['output']['stdout']
    lines = output.split("\n")
    for line in lines:
        if line.startswith("Simulating..."):
            runs += 1
            total_score += total_flow_score
        if line.startswith("Total packets"):
            pkts = float(line.split(":")[-1])
        if line.startswith("Total time"):
            time = float(line.split(":")[-1])
        if line.startswith("Throughput utility"):
            delay_score = float(line.split(":")[-1].strip())
            # Done with this flow
            tput_score = 0 if pkts == 0 or time == 0 else math.log2(pkts / (rate * time))
            total_flow_score += tput_score - delay_score
        if line.startswith("Flow"):
            flow_num += 1
    return total_score/runs

def get_avg_score(result):
    total_score = 0
    runs = 0
    output = result['output']['stdout']
    lines = output.split("\n")
    for line in lines:
        if "Score" in line and not("inf" in line):
            score = float(line.split(":")[-1])
            total_score += score
            runs += 1
    return total_score/runs

def get_avg_fairness(result):
    total_fairness = 0
    runs = 0
    output = result['output']['stdout']
    lines = output.split("\n")
    for line in lines:
        if "Fairness" in line and not("inf" in line):
            score = float(line.split(":")[-1])
            total_fairness += score
            runs += 1
    return total_fairness/runs

def get_tail_metrics(result):
    total_run_tail_delay = 0
    run_max_fct = 0
    total_tail_delay = 0
    max_fct = 0
    flow_num = 0
    runs = 0
    output = result['output']['stdout']
    lines = output.split("\n")
    for line in lines:
        if "Tail delay" in line:
            delay = float(line.split(":")[-1].strip())
            total_run_tail_delay += delay
        if "FCT" in line:
            fct = float(line.split(":")[-1].strip())
            run_max_fct = max(run_max_fct, fct)
        if line.startswith("Flow"):
            flow_num += 1
        if line.startswith("Simulating..."):
            if runs != 0:
                total_tail_delay += total_run_tail_delay / flow_num
                max_fct += run_max_fct
                total_run_tail_delay = 0
                run_max_fct = 0
                flow_num = 0
            runs += 1
    return total_tail_delay/runs, max_fct/runs

def net_config_parser(config_string):
    config = {}
    tokens = config_string.strip("{").strip("}").split(";")
    for token in tokens:
        var_name = token.split("=")[0]
        value = token.split("=")[1]
        config[var_name] = value

    return config

def get_config(result): # Assumes only one config per run
    output = result['output']['stdout']
    lines = output.split("\n")
    config = {}
    for line in lines:
        if line.startswith("{"):
            config = net_config_parser(line)
    
    return config