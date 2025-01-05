import uproot
import numpy as np

# Open the ROOT file
file_path = "/home/manoja450/run15731_processed_v5.root"
root_file = uproot.open(file_path)

# Access the 'tree' inside the ROOT file
tree = root_file["tree"]

# Read the branches you're interested in
eventID = tree["eventID"].array(library="np")[:5]  # First 5 events
adcVal = tree["adcVal"].array(library="np")[:5]  # First 5 events, 23 samples each
baselineMean = tree["baselineMean"].array(library="np")[:5]
baselineRMS = tree["baselineRMS"].array(library="np")[:5]
pulseH = tree["pulseH"].array(library="np")[:5]
peakPosition = tree["peakPosition"].array(library="np")[:5]
nsTime = tree["nsTime"].array(library="np")[:5]
triggerBits = tree["triggerBits"].array(library="np")[:5]

# Print the values of the first few events
for i in range(5):  # Print for the first 5 events
    print(f"Event {i+1}:")
    print(f"  eventID: {eventID[i]}")
    print(f"  adcVal: {adcVal[i]}")
    print(f"  baselineMean: {baselineMean[i]}")
    print(f"  baselineRMS: {baselineRMS[i]}")
    print(f"  pulseH: {pulseH[i]}")
    print(f"  peakPosition: {peakPosition[i]}")
    print(f"  nsTime: {nsTime[i]}")
    print(f"  triggerBits: {triggerBits[i]}")
    print("-" * 40)
