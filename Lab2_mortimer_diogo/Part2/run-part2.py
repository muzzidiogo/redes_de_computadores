import subprocess
import csv
import re
import time
import os
import sys

FLOW_COUNTS = [2, 4, 6, 8]
PROTOCOLS = ["TcpCubic", "TcpNewReno"]
NUM_RUNS = 10 
DATA_RATE = "1Mbps"
BOTTLENECK_DELAY = "20ms"
ERROR_RATE = 0.00001
SIM_NAME = "lab2-part2" 
OUTPUT_FILE = "part2_results.csv"

SIM_EXECUTABLE = "./ns3 run " + SIM_NAME + " --"
subprocess.run(SIM_EXECUTABLE.split()[:-1], check=True, capture_output=True) 


def run_single_simulation(n_flows, protocol, run_index):
    """
    Executes the ns-3 script with specific parameters and run index.
    Returns (avg_dest1_goodput, avg_dest2_goodput) in Mbps.
    """
    
    args = [
        f"--nFlows={n_flows}",
        f"--transport_prot={protocol}",
        f"--dataRate={DATA_RATE}",
        f"--delay={BOTTLENECK_DELAY}",
        f"--errorRate={ERROR_RATE}",
        f"--run={run_index}" 
    ]
    
    cmd = SIM_EXECUTABLE.split() + args

    result = subprocess.run(
        cmd,
        capture_output=True,
        text=True,
        check=True,
    )
    output = result.stdout
    
    match1 = re.search(r"Average Goodput \(Dest 1 - Short RTT\):\s*([\d.]+)\s*Mbps", output)
    match2 = re.search(r"Average Goodput \(Dest 2 - Long RTT\):\s*([\d.]+)\s*Mbps", output)
    
    if match1 and match2:
        return float(match1.group(1)), float(match2.group(1))
    else:
        print(f"   -> WARNING: Could not parse goodput output for {protocol} run {run_index}.")
        return 0.0, 0.0


def main():
    """Main function to loop through all scenarios and collect data."""
    
    total_scenarios = len(FLOW_COUNTS) * len(PROTOCOLS)
    print("="*60)
    print(f"Starting Part 2 RTT Fairness Study. Total runs per scenario: {NUM_RUNS}")
    print(f"Total simulations: {total_scenarios * NUM_RUNS}")
    print("="*60)
    
    results = [["Protocol", "NFlows", "Avg Goodput Dest 1 (Short RTT)", "Avg Goodput Dest 2 (Long RTT)"]]
    
    start_time = time.time()
    
    for protocol in PROTOCOLS:
        for n_flows in FLOW_COUNTS:
            print(f"\n--- Scenario: {protocol} with {n_flows} flows ---")
            
            dest1_goodputs = []
            dest2_goodputs = []
            
            for run_index in range(1, NUM_RUNS + 1):
                print(f"  > Running trial {run_index}/{NUM_RUNS}", end='\r', flush=True)
                g1, g2 = run_single_simulation(n_flows, protocol, run_index)
                dest1_goodputs.append(g1)
                dest2_goodputs.append(g2)

            avg_g1 = sum(dest1_goodputs) / NUM_RUNS if NUM_RUNS > 0 else 0.0
            avg_g2 = sum(dest2_goodputs) / NUM_RUNS if NUM_RUNS > 0 else 0.0
            
            print(f"\r  AVG RESULTS: Dest 1: {avg_g1:.4f} Mbps, Dest 2: {avg_g2:.4f} Mbps")
            
            results.append([protocol, n_flows, avg_g1, avg_g2])
    
    end_time = time.time()
    
    with open(OUTPUT_FILE, 'w', newline='') as f:
        writer = csv.writer(f)
        writer.writerows(results)

    print(f"SIMULATION COMPLETE. Total time: {end_time - start_time:.2f} seconds.")
    print(f"Results saved to {OUTPUT_FILE}")

if __name__ == "__main__":
    main()
