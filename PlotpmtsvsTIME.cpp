#include <TFile.h>
#include <TTree.h>
#include <TCanvas.h>
#include <TGraph.h>
#include <TString.h>
#include <iostream>
#include <TAxis.h>

using namespace std;

void PlotpmtsvsTIME() {
    // Open the ROOT file
    TFile *file = TFile::Open("run15731_processed_v5.root");
    if (!file || file->IsZombie()) {
        cerr << "Error opening file!" << endl;
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

    // Get the number of entries in the TTree
    Long64_t nEntries = tree->GetEntries();

    // Assuming a sample rate of 1 unit per sample for simplicity
    double timeStep = 1.0;  // Adjust this if you know the actual sample rate (e.g., in microseconds)

    // Loop through the 23 channels
    for (int i = 0; i < 23; i++) {
        TGraph *graph = new TGraph();
        
        // Loop through all entries in the tree
        for (Long64_t j = 0; j < nEntries; j++) {
            tree->GetEntry(j);  // Load the entry
            for (int k = 0; k < 45; k++) {  // Loop through the 45 samples per channel
                double time = k * timeStep;  // Time in arbitrary units (adjust based on sample rate)
                double adcValue = adcVal[i][k];  // ADC value for the i-th channel, k-th sample
                graph->SetPoint(k, time, adcValue);  // Set the point in the graph
            }
        }

        // Set graph title and axis labels
        graph->SetTitle(Form("PMT %d Signal vs Time", i + 1));
        graph->GetXaxis()->SetTitle("Time (arbitrary units)");  // Modify if you know the time scale
        graph->GetYaxis()->SetTitle("ADC Value");

        // Create a canvas to draw the graph
        TCanvas *c1 = new TCanvas(Form("c1_%d", i), "PMT Signals", 800, 600);
        graph->Draw("AL");
        
        // Save the canvas as a PNG file
        c1->SaveAs(Form("PMT_%d_signal.png", i + 1));
    }

    // Close the file
    file->Close();
}

int main() {
    // Call the function to plot the PMT signals
    PlotpmtsvsTIME();
    return 0;
}
