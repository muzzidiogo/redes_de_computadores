import pandas as pd
import matplotlib.pyplot as plt
import os

INPUT_FILE = "part2_results.csv"
OUTPUT_PLOT_FILE = "part2_rtt_fairness_goodput.png"
FLOW_COUNTS = [2, 4, 6, 8]

def plot_part2_results(df):
    """Plots the average goodput vs. nFlows for Dest 1 and Dest 2, by protocol."""
    
    BOTTLENECK_RATE = 1.0 # 1 Mbps
    
    plt.figure(figsize=(12, 7))
    
    colors = {'TcpCubic': 'blue', 'TcpNewReno': 'red'}
    styles = {
        'Dest 1': {'marker': 'o', 'linestyle': '-', 'label_suffix': ' (Short RTT)'},
        'Dest 2': {'marker': 's', 'linestyle': '--', 'label_suffix': ' (Long RTT)'}
    }
    
    for protocol in df['Protocol'].unique():
        proto_df = df[df['Protocol'] == protocol]
        color = colors[protocol]
        
        plt.plot(proto_df['NFlows'], proto_df['Avg Goodput Dest 1 (Short RTT)'], 
                 marker=styles['Dest 1']['marker'], linestyle=styles['Dest 1']['linestyle'], 
                 color=color, linewidth=2, markersize=8, 
                 label=f"{protocol} Dest 1{styles['Dest 1']['label_suffix']}")
                 
        plt.plot(proto_df['NFlows'], proto_df['Avg Goodput Dest 2 (Long RTT)'], 
                 marker=styles['Dest 2']['marker'], linestyle=styles['Dest 2']['linestyle'], 
                 color=color, linewidth=2, markersize=8, 
                 label=f"{protocol} Dest 2{styles['Dest 2']['label_suffix']}")

    plt.title('RTT Fairness: Average Goodput vs. Number of Flows (1 Short RTT, 1 Long RTT Group)', fontsize=14)
    plt.xlabel('Total Number of Flows (N)', fontsize=12)
    plt.ylabel('Average Goodput Per Flow (Mbps)', fontsize=12)
    
    ideal_share_dest1 = BOTTLENECK_RATE / 2.0
    ideal_share_dest2 = BOTTLENECK_RATE / 2.0
    
    plt.axhline(y=BOTTLENECK_RATE / 2, color='gray', linestyle=':', linewidth=1.0, label='Ideal Equal Share (0.5 Mbps)')
    
    plt.grid(True, linestyle=':', alpha=0.6)
    plt.legend(loc='upper right', fontsize=10)
    plt.xticks(FLOW_COUNTS)
    plt.ylim(0, BOTTLENECK_RATE + 0.1)
    
    plt.tight_layout()
    plt.savefig(OUTPUT_PLOT_FILE, dpi=300)
    print(f"\nPlot successfully saved to: {OUTPUT_PLOT_FILE}")

if __name__ == "__main__":
    df = pd.read_csv(INPUT_FILE)
    plot_part2_results(df)
