import pandas as pd
import matplotlib.pyplot as plt
import os

INPUT_FILE = "part1b_results.csv"
OUTPUT_PLOT_FILE = "part1b_aggregate_goodput.png"

def plot_part1b_results(df):
    """Plots the aggregate goodput vs. delay for 6 scenarios."""
    
    DATA_RATE = "1 Mbps"
    ERROR_RATE = "0.00001"
    bottleneck_capacity = 1.0 # 1 Mbps
    
    plt.figure(figsize=(12, 7))
    
    styles = {1: ('o', '-'), 2: ('s', '--'), 4: ('^', '-.')}
    colors = {'TcpCubic': 'blue', 'TcpNewReno': 'red'}
    
    for protocol in df['Protocol'].unique():
        for n_flows in sorted(df['NFlows'].unique()):
            subset = df[(df['Protocol'] == protocol) & (df['NFlows'] == n_flows)]
            
            label = f"{protocol} ({n_flows} flow{'s' if n_flows > 1 else ''})"
            
            marker, linestyle = styles[n_flows]
            color = colors[protocol]
            
            plt.plot(subset['Delay (ms)'], subset['Aggregate Goodput (Mbps)'], 
                     marker=marker, linestyle=linestyle, color=color, 
                     label=label, linewidth=2, markersize=7)

    plt.title(f'Aggregate Goodput vs. Bottleneck Delay (Data Rate: {DATA_RATE}, Error Rate: {ERROR_RATE})', fontsize=14)
    plt.xlabel('Bottleneck Delay (msec)', fontsize=12)
    plt.ylabel('Aggregate Goodput (Mbps)', fontsize=12)
    
    plt.axhline(y=bottleneck_capacity, color='gray', linestyle=':', linewidth=1.5, label=f'Link Capacity ({bottleneck_capacity} Mbps)')
    plt.grid(True, linestyle=':', alpha=0.6)
    plt.legend(loc='lower left', fontsize=10)
    plt.ylim(0, bottleneck_capacity * 1.1)
    plt.tight_layout()
    plt.savefig(OUTPUT_PLOT_FILE, dpi=300)
    print(f"\nPlot successfully saved to: {OUTPUT_PLOT_FILE}")

if __name__ == "__main__":
    if os.path.exists(INPUT_FILE):
        df = pd.read_csv(INPUT_FILE)
        plot_part1b_results(df)
    else:
        print(f"Error: Input file '{INPUT_FILE}' not found. Please run run_part1b.py first.")
