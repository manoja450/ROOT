#include <TFile.h>
#include <TTree.h>
#include <TH1F.h>
#include <TCanvas.h>
#include <iostream>

void plotBaselineRMS() {
    // Open the ROOT file and get the tree
    TFile *file = TFile::Open("run15731_processed_v5.root");
    if (!file || file->IsOpen() == false) {
        std::cerr << "Error: Unable to open file!" << std::endl;
        return;
    }
    TTree *tree = (TTree*)file->Get("tree");
    if (!tree) {
        std::cerr << "Error: Unable to get tree!" << std::endl;
        return;
    }

    // Define variables to hold data from the tree
    Double_t baselineRMS[23];
    tree->SetBranchAddress("baselineRMS", baselineRMS);

    // Create a histogram to store RMS values
    TH1F *rmsHist = new TH1F("rmsHist", "Baseline RMS of Traces", 100, 0, 100); // Adjust the range as needed

    // Loop through the events and fill the histogram with RMS values
    int nEntries = tree->GetEntries();
    std::cout << "Processing " << nEntries << " entries..." << std::endl;
    
    for (int i = 0; i < nEntries; i++) {
        tree->GetEntry(i);

        // Fill the histogram with baseline RMS values for each channel
        for (int j = 0; j < 23; j++) {
            rmsHist->Fill(baselineRMS[j]);
        }
    }

    // Draw the histogram
    TCanvas *canvas = new TCanvas("canvas", "RMS Histogram", 800, 600);
    rmsHist->Draw();

    // Optionally, save the histogram as a file
    canvas->SaveAs("baseline_rms_histogram.png");

    // Clean up
    delete rmsHist;
    delete canvas;
    file->Close();
}

int main() {
    // Call the function to plot the baseline RMS
    plotBaselineRMS();
    return 0;
}
