import pandas as pd
import matplotlib.pyplot as plt
import os

CUBIC_FILE = "cubic-cwnd-trace-flow.csv"
RENO_FILE = "newreno-cwnd-trace-flow.csv"
OUTPUT_PLOT_FILE = "cwnd_comparison_plot.png"

def load_data(filename, label):
    """Loads the ns-3 trace file (time, value) and assigns column names."""
    if not os.path.exists(filename):
        print(f"Error: File not found: {filename}")
        return None
        
    df = pd.read_csv(filename, sep=' ', header=None, names=['Time (s)', 'Congestion Window (segments)'])
    print(f"Successfully loaded data from {filename}. Rows: {len(df)}")
    return df

def plot_comparison(df1:pd.DataFrame, df2:pd.DataFrame)-> None:
    """Generates and saves the comparison plot."""
    
    plt.figure(figsize=(12, 6))
    
    plt.plot(df1['Time (s)'], df1['Congestion Window (segments)'], 
             label='TCP CUBIC', color='blue', linewidth=1.5)
             
    plt.plot(df2['Time (s)'], df2['Congestion Window (segments)'], 
             label='TCP NewReno', color='red', linestyle='--', linewidth=1)
    
    plt.title('TCP Congestion Window Comparison (10 Mbps, 100 ms Delay, 1 Flow)', fontsize=14)
    plt.xlabel('Time (s)', fontsize=12)
    plt.ylabel('Congestion Window (segments)', fontsize=12)
    plt.grid(True, linestyle=':', alpha=0.7)
    plt.legend(loc='upper right', fontsize=10)
    plt.xlim(0, 20)
    plt.tight_layout()
    plt.savefig(OUTPUT_PLOT_FILE, dpi=300)
    print(f"\nPlot successfully saved to: {OUTPUT_PLOT_FILE}")

def main():
    df_cubic = load_data(CUBIC_FILE, 'TCP CUBIC')
    df_reno = load_data(RENO_FILE, 'TCP NewReno')
    plot_comparison(df_cubic, df_reno)

if __name__ == "__main__":
    main()
