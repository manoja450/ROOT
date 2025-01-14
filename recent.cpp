#include <iostream>
#include <TFile.h>
#include <TTree.h>
#include <TGraph.h>
#include <TCanvas.h>
#include <TAxis.h>
#include <vector>
#include <algorithm>
#include <cmath>
#include <map>

using namespace std;

// Function to find the nearest whole number above the maximum value
double roundUpToBin(double value, double binSize) {
    return ceil((value + 0.5) / binSize) * binSize; // Add a small offset to ensure proper rounding
}

// Function to calculate the mode of pulse heights
double getMode(const vector<double>& pulseHeights) {
    // Find the most frequent pulse height
    map<double, int> frequencyMap;
    for (double value : pulseHeights) {
        frequencyMap[value]++;
    }

    // Find the mode (the value with the highest frequency)
    double mode = pulseHeights[0];
    int maxCount = 0;
    for (const auto& entry : frequencyMap) {
        if (entry.second > maxCount) {
            mode = entry.first;
            maxCount = entry.second;
        }
    }

    return mode;
}

void PlotpmtsAndSipms(const char *fileName) {
    // Open the ROOT file
    TFile *file = TFile::Open(fileName);
    if (!file || file->IsZombie()) {
        cerr << "Error opening file: " << fileName << endl;
        return;
    }

    // Access the TTree in the file
    TTree *tree = (TTree*)file->Get("tree");
    if (!tree) {
        cerr << "Error accessing TTree 'tree'!" << endl;
        file->Close();
        return;
    }

    // Declare the adcVal array (23 channels, 45 samples per channel)
    Short_t adcVal[23][45];
    tree->SetBranchAddress("adcVal", adcVal);

    // Declare the pulseH array (23 channels)
    double_t pulseH[23];
    tree->SetBranchAddress("pulseH", pulseH);

    // Get the number of entries in the TTree
    Long64_t nEntries = tree->GetEntries();

    // Vectors to store pulse heights for PMTs and SiPMs
    vector<double> pmtPulseHeights;
    vector<double> sipmPulseHeights;

    // Loop through the entries to find the max pulse heights and store pulse heights
    for (Long64_t j = 0; j < nEntries; j++) {
        tree->GetEntry(j);  // Load the entry
        for (int i = 0; i < 12; i++) {  // PMTs (indices 0–11)
            pmtPulseHeights.push_back(pulseH[i]);
        }
        for (int i = 12; i < 22; i++) {  // SiPMs (indices 12–21)
            sipmPulseHeights.push_back(pulseH[i]);
        }
    }

    // Find the maximum pulse height for PMTs and SiPMs
    double maxPMT = *max_element(pmtPulseHeights.begin(), pmtPulseHeights.end());
    double maxSiPM = *max_element(sipmPulseHeights.begin(), sipmPulseHeights.end());

    // Display the maximum pulse heights in the terminal
    cout << "Maximum PMT Pulse Height: " << maxPMT << endl;
    cout << "Maximum SiPM Pulse Height: " << maxSiPM << endl;

    // Define the PMT channel mapping (1 to 12) to adcVal indices (0 to 11)
    int pmtChannelMap[12] = {0, 10, 7, 2, 6, 3, 8, 9, 11, 4, 5, 1};

    // Define the SiPM channel mapping (13 to 22) to adcVal indices (12 to 21)
    int sipmChannelMap[10] = {12, 13, 14, 15, 16, 17, 18, 19, 20, 21};

    // Plot each PMT channel individually
    for (int i = 0; i < 12; i++) {
        TCanvas *canvas = new TCanvas(Form("PMT%d_canvas", i + 1), Form("PMT %d", i + 1), 800, 600);
        TGraph *graph = new TGraph();

        TString title = Form("PMT %d Waveform", i + 1);
        int adcIndex = pmtChannelMap[i];

        for (int k = 0; k < 45; k++) {
            double time = (k + 1) * 16.0;  // Time in nanoseconds (1st sample at 16 ns)
            double adcValue = adcVal[adcIndex][k];
            graph->SetPoint(k, time, adcValue);
        }

        graph->SetTitle(title);
        graph->GetXaxis()->SetTitle("Time (ns)");
        graph->GetYaxis()->SetTitle("ADC Value");
        graph->SetMinimum(0); // Set minimum Y-axis value to zero
        graph->SetMaximum(maxPMT * 1.05); // Set maximum Y-axis value to 5% above the maximum PMT pulse height
        graph->GetXaxis()->SetRangeUser(0, 720); // Ensure x-axis ends exactly at 720 ns

        graph->Draw("AL");
        canvas->SaveAs(Form("/root/gears/waveform720ns/PMT%d_%s.png", i + 1, fileName));  // Save plot to specified directory
        delete graph;
        delete canvas;
    }

    // Plot each SiPM channel individually
    for (int i = 0; i < 10; i++) {
        TCanvas *canvas = new TCanvas(Form("SiPM%d_canvas", i + 1), Form("SiPM %d", i + 1), 800, 600);
        TGraph *graph = new TGraph();

        TString title = Form("SiPM %d Waveform", i + 1);
        int adcIndex = sipmChannelMap[i];

        for (int k = 0; k < 45; k++) {
            double time = (k + 1) * 16.0;  // Time in nanoseconds (1st sample at 16 ns)
            double adcValue = adcVal[adcIndex][k];
            graph->SetPoint(k, time, adcValue);
        }

        graph->SetTitle(title);
        graph->GetXaxis()->SetTitle("Time (ns)");
        graph->GetYaxis()->SetTitle("ADC Value");
        graph->SetMinimum(0); // Set minimum Y-axis value to zero
        graph->SetMaximum(maxSiPM * 1.05); // Set maximum Y-axis value to 5% above the maximum SiPM pulse height
        graph->GetXaxis()->SetRangeUser(0, 720); // Ensure x-axis ends exactly at 720 ns

        graph->Draw("AL");
        canvas->SaveAs(Form("/root/gears/waveform720ns/SiPM%d_%s.png", i + 1, fileName));  // Save plot to specified directory
        delete graph;
        delete canvas;
    }

    // Close the file
    file->Close();
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        cerr << "Usage: " << argv[0] << " <root_file>" << endl;
        return 1;
    }

    const char* fileName = argv[1];  // Get the file name from the command-line argument
    PlotpmtsAndSipms(fileName);

    return 0;
}
