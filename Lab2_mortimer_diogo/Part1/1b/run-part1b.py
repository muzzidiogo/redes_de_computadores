import subprocess
import csv
import re
import time
import os
import sys

FLOW_COUNTS = [1, 2, 4]
DELAYS_MS = [50, 100, 150, 200, 250, 300]
PROTOCOLS = ["TcpCubic", "TcpNewReno"]
DATA_RATE = "1Mbps"
ERROR_RATE = 0.00001
SIM_NAME = "lab2-part1"
OUTPUT_FILE = "part1b_results.csv"

try:
    SIM_EXECUTABLE = "./ns3 run " + SIM_NAME + " --"
    subprocess.run(SIM_EXECUTABLE.split()[:-1], check=True, capture_output=True) 
except subprocess.CalledProcessError:
    print("WARNING: Direct './ns3 run' failed. Checking 'build/scratch/ns3-...' path.")
    try:
        version_dir = [d for d in os.listdir("build/scratch") if d.startswith("ns3.")]
        if version_dir:
            exec_name = [f for f in os.listdir(os.path.join("build/scratch", version_dir[0])) if f.startswith("ns3") and SIM_NAME in f][0]
            SIM_EXECUTABLE = f"build/scratch/{version_dir[0]}/{exec_name} --"
        else:
            print("FATAL ERROR: Could not locate ns-3 executable path. Is the script compiled?")
            sys.exit(1)
    except Exception as e:
        print(f"FATAL ERROR: Could not determine executable path: {e}")
        sys.exit(1)


def run_simulation(n_flows, delay_ms, protocol):
    """
    Executes the ns-3 script and captures the Total Aggregate Goodput.
    """
    delay_str = f"{delay_ms}ms"
    
    args = [
        f"--nFlows={n_flows}",
        f"--transport_prot={protocol}",
        f"--dataRate={DATA_RATE}",
        f"--delay={delay_str}",
        f"--errorRate={ERROR_RATE}"
    ]
    
    cmd = SIM_EXECUTABLE.split() + args
    
    print(f"-> Running: {protocol}, {n_flows} flows, Delay: {delay_str}", flush=True)

    try:
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
            
    except subprocess.CalledProcessError as e:
        print(f"   -> ERROR: Simulation failed (SIGABRT). Check compilation. Stderr:\n{e.stderr[:500]}...")
        return 0.0
    except FileNotFoundError:
        print(f"   -> ERROR: ns3 executable not found at '{SIM_EXECUTABLE.split()[0]}'.")
        return 0.0


def main():
    """Main function to loop through all scenarios and collect data."""
    
    print("="*60)
    print(f"Starting Part 1b simulations. Total runs: {len(FLOW_COUNTS) * len(DELAYS_MS) * len(PROTOCOLS)}")
    print("Bottleneck: 1 Mbps, 0.00001 error rate.")
    print("="*60)
    
    results = [["Protocol", "NFlows", "Delay (ms)", "Aggregate Goodput (Mbps)"]]
    
    start_time = time.time()
    
    for protocol in PROTOCOLS:
        for n_flows in FLOW_COUNTS:
            for delay_ms in DELAYS_MS:
                goodput = run_simulation(n_flows, delay_ms, protocol)
                results.append([protocol, n_flows, delay_ms, goodput])
    
    end_time = time.time()
    
    with open(OUTPUT_FILE, 'w', newline='') as f:
        writer = csv.writer(f)
        writer.writerows(results)

    print("\n" + "="*60)
    print(f"SIMULATION COMPLETE. Total time: {end_time - start_time:.2f} seconds.")
    print(f"Results saved to {OUTPUT_FILE}")
    print("Next step: Run 'plot_part1b.py' to generate the graph.")
    print("="*60)

if __name__ == "__main__":
    main()
