import pandas as pd
import matplotlib.pyplot as plt
import os

INPUT_FILE = "part1c_results.csv"
OUTPUT_PLOT_FILE = "part1c_plot.png"
ERROR_RATES = [0.00001, 0.00005, 0.0001, 0.0005, 0.001]

def plot_part1c_results(df):
    """Plots the aggregate goodput vs. error rate for 6 scenarios."""
    
    DATA_RATE = "1 Mbps"
    DELAY = "1 ms"
    bottleneck_capacity = 1.0 
    
    plt.figure(figsize=(12, 7))
    
    styles = {1: ('o', '-'), 2: ('s', '--'), 4: ('^', '-.')}
    colors = {'TcpCubic': 'blue', 'TcpNewReno': 'red'}
    
    df['Error Rate'] = pd.to_numeric(df['Error Rate'])
    
    for protocol in df['Protocol'].unique():
        for n_flows in sorted(df['NFlows'].unique()):
            subset = df[(df['Protocol'] == protocol) & (df['NFlows'] == n_flows)]
            
            label = f"{protocol} ({n_flows} flow{'s' if n_flows > 1 else ''})"
            marker, linestyle = styles[n_flows]
            color = colors[protocol]
            
            plt.plot(subset['Error Rate'], subset['Aggregate Goodput (Mbps)'], marker=marker, linestyle=linestyle, color=color, label=label, linewidth=2, markersize=7)

    plt.title(f'Aggregate Goodput vs. Bottleneck Error Rate (Data Rate: {DATA_RATE}, Delay: {DELAY})', fontsize=14)
    plt.xlabel('Bottleneck Error Rate (Bytes Lost / Byte Transferred)', fontsize=12)
    plt.ylabel('Aggregate Goodput (Mbps)', fontsize=12)
    
    plt.xscale('log')
    plt.xticks(ERROR_RATES, [f'{e:.5f}' for e in ERROR_RATES], rotation=45)
    
    plt.axhline(y=bottleneck_capacity, color='gray', linestyle=':', linewidth=1.5, label=f'Link Capacity ({bottleneck_capacity} Mbps)')
                
    plt.grid(True, linestyle=':', alpha=0.6)
    plt.legend(loc='upper right', fontsize=10)
    plt.ylim(0, bottleneck_capacity * 1.1)
    
    plt.tight_layout()
    plt.savefig(OUTPUT_PLOT_FILE, dpi=300)
    print(f"\nPlot saved to: {OUTPUT_PLOT_FILE}")

if __name__ == "__main__":
    df = pd.read_csv(INPUT_FILE)
    plot_part1c_results(df)