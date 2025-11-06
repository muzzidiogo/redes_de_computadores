import subprocess
import csv
import re
import time
import os
import sys

FLOW_COUNTS = [1, 2, 4]
ERROR_RATES = [0.00001, 0.00005, 0.0001, 0.0005, 0.001]
PROTOCOLS = ["TcpCubic", "TcpNewReno"]
DATA_RATE = "1Mbps"
DELAY_MS = 1
SIM_NAME = "lab2-part1" 
OUTPUT_FILE = "part1c_results.csv"

SIM_EXECUTABLE = "./ns3 run " + SIM_NAME + " --"
subprocess.run(SIM_EXECUTABLE.split()[:-1], check=True, capture_output=True) 


def run_simulation(n_flows, error_rate, protocol):
    """
    Executes the ns-3 script and captures the Total Aggregate Goodput.
    """
    delay_str = f"{DELAY_MS}ms"
    
    args = [
        f"--nFlows={n_flows}",
        f"--transport_prot={protocol}",
        f"--dataRate={DATA_RATE}",
        f"--delay={delay_str}",
        f"--errorRate={error_rate}"
    ]
    
    cmd = SIM_EXECUTABLE.split() + args
    
    print(f"-> Running: {protocol}, {n_flows} flows, Error Rate: {error_rate}", flush=True)

    result = subprocess.run(
        cmd,
        capture_output=True,
        text=True,
        check=True,
    )
    output = result.stdout
    
    match = re.search(r"Total Aggregate Goodput:\s*([\d.]+)\s*Mbps", output)
    
    if match:
        aggregate_goodput = float(match.group(1))
        print(f"   -> Goodput: {aggregate_goodput:.6f} Mbps")
        return aggregate_goodput
    else:
        print("   -> WARNING: Could not find goodput value in output. Check ns-3 logs.")
        return 0.0


def main():
    """Main function to loop through all scenarios and collect data."""
    
    total_runs = len(FLOW_COUNTS) * len(ERROR_RATES) * len(PROTOCOLS)
    print(f"Starting Part 1c simulations. Total runs: {total_runs}")
    print(f"Bottleneck: {DATA_RATE}, {DELAY_MS}ms delay.")
    
    results = [["Protocol", "NFlows", "Error Rate", "Aggregate Goodput (Mbps)"]]
    
    start_time = time.time()
    
    for protocol in PROTOCOLS:
        for n_flows in FLOW_COUNTS:
            for error_rate in ERROR_RATES:
                goodput = run_simulation(n_flows, error_rate, protocol)
                results.append([protocol, n_flows, error_rate, goodput])
    
    end_time = time.time()
    
    with open(OUTPUT_FILE, 'w', newline='') as f:
        writer = csv.writer(f)
        writer.writerows(results)

    print(f"SIMULATION COMPLETE. Total time: {end_time - start_time:.2f} seconds.")
    print(f"Results saved to {OUTPUT_FILE}")

if __name__ == "__main__":
    main()
