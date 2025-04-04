import os
import pandas as pd
import matplotlib.pyplot as plt

def backup_existing_pdf(filename):
    if os.path.exists(filename):
        base, ext = os.path.splitext(filename)
        i = 1
        while os.path.exists(f"{base}_{i}{ext}"):
            i += 1
        os.rename(filename, f"{base}_{i}{ext}")
        print(f"Renamed existing file to: {base}_{i}{ext}")

def plotExperiment(experiment):
    data_file = f"{experiment}.dat"
    output_file = f'{experiment}.pdf'

    # Load the tab-separated file (skip comment lines)
    df = pd.read_csv(f'build/{data_file}', sep='\t', comment='#', header=None, names=['Event', 'Duration'])

    # Clean up event names
    df['Event'] = df['Event'].str.strip('"').str.strip()

    # # Optional: sort by duration descending
    # df = df.sort_values(by='Duration', ascending=False)

    # Plot vertical bar chart
    plt.figure(figsize=(14, 6))
    plt.bar(df['Event'], df['Duration'], color='steelblue')
    plt.ylabel('Duration (ms)')
    plt.xlabel('Event')
    # plt.title('Event Durations')

    # Rotate x-axis labels for better readability
    plt.xticks(rotation=45, ha='right')

    backup_existing_pdf(output_file)

    plt.tight_layout()
    plt.savefig(output_file, dpi=300)
    plt.show()

if (__name__ == "__main__"):
    experiments = ["ex61profile_1m", "ex62profile_1m"]
    for exp in experiments:
        plotExperiment(exp)