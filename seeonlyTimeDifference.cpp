//The program now identifies muon candidates and searches for Michel electrons within a 10 μs window.
//The time difference between the muon and Michel electron signals is calculated and displayed.
//You can still limit the number of events processed using the maxEvents parameter.
#include <iostream>
#include <TFile.h>
#include <TTree.h>
#include <TGraph.h>
#include <TCanvas.h>
#include <TAxis.h>
#include <TH1F.h>
#include <vector>
#include <algorithm>
#include <cmath>
#include "TLatex.h"
#include <sys/stat.h> // For mkdir

using namespace std;

// Function to find the time of the peak in a waveform
double findPeakTime(TGraph* graph) {
    double maxADC = -1;
    double peakTime = -1;
    double x, y;

    for (int i = 0; i < graph->GetN(); i++) {
        graph->GetPoint(i, x, y);
        if (y > maxADC) {
            maxADC = y;
            peakTime = x;
        }
    }
    return peakTime;
}

void analyzeMuonDecay(const char *fileName, Long64_t maxEvents = -1) {
    TFile *file = TFile::Open(fileName);
    if (!file || file->IsZombie()) {
        cerr << "Error opening file: " << fileName << endl;
        return;
    }

    TTree *tree = (TTree*)file->Get("tree");
    if (!tree) {
        cerr << "Error accessing TTree 'tree'!" << endl;
        file->Close();
        return;
    }

    Short_t adcVal[23][45]; // ADC values for 23 channels and 45 time bins
    Long64_t nsTime; // Event time in nanoseconds from the start of the run
    tree->SetBranchAddress("adcVal", adcVal);
    tree->SetBranchAddress("nsTime", &nsTime); // Add nsTime branch

    Long64_t nEntries = tree->GetEntries();
    cout << "Total events in the file: " << nEntries << endl;

    // If maxEvents is not specified or is larger than the total number of events, process all events
    if (maxEvents < 0 || maxEvents > nEntries) {
        maxEvents = nEntries;
    }
    cout << "Processing " << maxEvents << " events..." << endl;

    // Declare PMT and SiPM channel maps
    int pmtChannelMap[12] = {0, 10, 7, 2, 6, 3, 8, 9, 11, 4, 5, 1}; // PMTs inside the detector
    int sipmChannelMap[10] = {12, 13, 14, 15, 16, 17, 18, 19, 20, 21}; // SiPMs in the veto system

    // Threshold for muon detection (adjust as needed)
    double muonThreshold = 1000; // Example threshold in ADC counts

    // Loop through the specified number of events
    for (Long64_t EventID = 0; EventID < maxEvents; EventID++) {
        tree->GetEntry(EventID); // Load the current event

        // Variables to store peak times
        double muonPeakTime = -1;
        double michelPeakTime = -1;

        // Analyze SiPMs (veto system) and PMTs for muon signal
        for (int i = 0; i < 10; i++) { // Loop over SiPMs
            TGraph *sipmGraph = new TGraph();
            int adcIndex = sipmChannelMap[i];

            for (int k = 0; k < 45; k++) {
                double time = (k + 1) * 16.0; // Time of the sample in the waveform
                double adcValue = adcVal[adcIndex][k];
                sipmGraph->SetPoint(k, time, adcValue);
            }

            double peakTime = findPeakTime(sipmGraph);
            if (peakTime > muonPeakTime) {
                muonPeakTime = peakTime; // Update muon peak time
            }
            delete sipmGraph;
        }

        for (int i = 0; i < 12; i++) { // Loop over PMTs
            TGraph *pmtGraph = new TGraph();
            int adcIndex = pmtChannelMap[i];

            for (int k = 0; k < 45; k++) {
                double time = (k + 1) * 16.0; // Time of the sample in the waveform
                double adcValue = adcVal[adcIndex][k];
                pmtGraph->SetPoint(k, time, adcValue);
            }

            double peakTime = findPeakTime(pmtGraph);
            if (peakTime > muonPeakTime) {
                muonPeakTime = peakTime; // Update muon peak time
            }
            delete pmtGraph;
        }

        // Check if a muon candidate is found
        if (muonPeakTime != -1) {
            // Calculate absolute time of the muon signal
            double muonAbsoluteTime = nsTime + muonPeakTime;

            // Look for Michel electron signal within 10 μs window
            double michelWindowStart = muonAbsoluteTime;
            double michelWindowEnd = muonAbsoluteTime + 10000; // 10 μs window

            // Loop through subsequent events to find Michel electron
            for (Long64_t nextEventID = EventID + 1; nextEventID < maxEvents; nextEventID++) {
                tree->GetEntry(nextEventID); // Load the next event

                // Analyze PMTs for Michel electron signal
                for (int i = 0; i < 12; i++) { // Loop over PMTs
                    TGraph *pmtGraph = new TGraph();
                    int adcIndex = pmtChannelMap[i];

                    for (int k = 0; k < 45; k++) {
                        double time = (k + 1) * 16.0; // Time of the sample in the waveform
                        double adcValue = adcVal[adcIndex][k];
                        pmtGraph->SetPoint(k, time, adcValue);
                    }

                    double peakTime = findPeakTime(pmtGraph);
                    double michelAbsoluteTime = nsTime + peakTime;

                    // Check if the Michel electron signal is within the 10 μs window
                    if (michelAbsoluteTime >= michelWindowStart && michelAbsoluteTime <= michelWindowEnd) {
                        michelPeakTime = peakTime; // Update Michel electron peak time
                        break;
                    }
                    delete pmtGraph;
                }

                if (michelPeakTime != -1) {
                    break; // Michel electron found, stop searching
                }
            }

            // Calculate time difference
            if (michelPeakTime != -1) {
                double timeDifference = (nsTime + michelPeakTime) - muonAbsoluteTime;
                cout << "Event " << EventID << ": Muon absolute time = " << muonAbsoluteTime << " ns, Michel electron absolute time = " << (nsTime + michelPeakTime) << " ns" << endl;
                cout << "Time difference (Michel - Muon) = " << timeDifference << " ns" << endl;
            } else {
                cout << "Event " << EventID << ": Muon detected, but no Michel electron found within 10 μs window." << endl;
            }
        } else {
            cout << "Event " << EventID << ": No muon candidate found." << endl;
        }
    }

    file->Close();
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        cerr << "Usage: " << argv[0] << " <root_file> [max_events]" << endl;
        return 1;
    }

    const char* fileName = argv[1]; // First argument is the ROOT file name

    // Second argument is the maximum number of events to process (optional)
    Long64_t maxEvents = -1; // Default: process all events
    if (argc >= 3) {
        maxEvents = atoi(argv[2]); // Convert argument to integer
    }

    analyzeMuonDecay(fileName, maxEvents); // Process events in the file

    return 0;
}
