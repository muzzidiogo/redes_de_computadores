import re
import matplotlib.pyplot as plt

def extract_rtt_from_file(filepath):
    send_pattern = re.compile(r"At time \+([\d\.]+)s client sent")
    receive_pattern = re.compile(r"At time \+([\d\.]+)s client received")
    
    send_times = []
    receive_times = []
    rtts = []
    
    with open(filepath, 'r') as f:
        for line in f:
            send_match = send_pattern.search(line)
            if send_match:
                send_times.append(float(send_match.group(1)))
                continue
            
            receive_match = receive_pattern.search(line)
            if receive_match:
                receive_times.append(float(receive_match.group(1)))

    num_pairs = min(len(send_times), len(receive_times))
    for i in range(num_pairs):
        rtt_in_seconds = receive_times[i] - send_times[i]
        rtts.append(rtt_in_seconds * 1000)
        
    return rtts

def main():
    files_to_plot = [
        'second-output.txt',
        'lab1-part2-output.txt'
    ]

    plt.figure(figsize=(10, 6))

    for file_path in files_to_plot:
        rtt_data = extract_rtt_from_file(file_path)
        if rtt_data:
            packet_numbers = range(1, len(rtt_data) + 1)
            plt.plot(packet_numbers, rtt_data, marker='o', linestyle='-', label=file_path)

    plt.title('End-to-End Delay vs. Packet Number')
    plt.xlabel('Packet Number')
    plt.ylabel('Delay (RTT) in ms')
    plt.legend()
    plt.grid(True)

    plt.savefig('delay_plot.png')
    plt.show()

if __name__ == "__main__":
    main()