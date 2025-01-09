#include <TFile.h>
#include <TTree.h>
#include <TCanvas.h>
#include <TGraph.h>
#include <TString.h>
#include <iostream>
#include <TAxis.h>
#include <TLatex.h>

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

    // Define the PMT channel mapping (1 to 12) to adcVal indices (0 to 11)
    int pmtChannelMap[12] = {0, 10, 7, 2, 6, 3, 8, 9, 11, 4, 5, 1}; // Mapping for PMT channels

    // Define the SiPM channel mapping (1 to 10) to adcVal indices (12 to 21)
    int sipmChannelMap[10] = {12, 13, 14, 15, 16, 17, 18, 19, 20, 21}; // Mapping for SiPM channels

    // Create a combined canvas to arrange all plots in one image
    TCanvas *combinedCanvas = new TCanvas("combinedCanvas", "All PMTs, SiPMs, and Event 61 Plots", 1600, 1200);
    combinedCanvas->Divide(3, 5);  // Arrange the plots in a 3x5 grid (15 slots)

    int plotIndex = 1;  // To keep track of the position on the combined canvas

    // Loop through the 12 PMT channels (1 to 12)
    for (int i = 0; i < 12; i++) {
        TGraph *graph = new TGraph();

        // Determine the title for each PMT channel
        TString title = Form("PMT %d Waveform", i + 1);

        // Get the corresponding adcVal index from the PMT mapping
        int adcIndex = pmtChannelMap[i];

        // Loop through all entries in the tree
        for (Long64_t j = 0; j < nEntries; j++) {
            tree->GetEntry(j);  // Load the entry
            for (int k = 0; k < 45; k++) {  // Loop through the 45 samples per channel
                double time = k * timeStep;  // Time in arbitrary units
                double adcValue = adcVal[adcIndex][k];  // ADC value for the current PMT channel, k-th sample
                graph->SetPoint(k, time, adcValue);  // Set the point in the graph
            }
        }

        // Set graph title and axis labels
        graph->SetTitle(title);
        graph->GetXaxis()->SetTitle("Time (16ns)");  // Modify if you know the time scale
        graph->GetYaxis()->SetTitle("ADC Value");

        // Set the y-axis range for PMTs to [0, 1500]
        graph->GetYaxis()->SetRangeUser(0, 1500);

        // Draw the graph on the combined canvas
        combinedCanvas->cd(plotIndex++);
        graph->Draw("AL");

        // Add custom labels using TLatex
        TLatex latex;
        latex.SetNDC(); // Use normalized device coordinates (0 to 1)
        latex.SetTextSize(0.03); // Set text size
        latex.DrawLatex(0.15, 0.85, title);  // Title of the plot
        latex.DrawLatex(0.75, 0.85, Form("Entries: %lld", nEntries));  // Number of entries (aligned to the right)

        // Save individual plots as PNG
        TCanvas *individualCanvas = new TCanvas(Form("c1_PMTS_%d", i), "Waveform", 800, 600);
        graph->Draw("AL");
        individualCanvas->SaveAs(Form("PMT_%d_signal.png", i + 1));
        
        // Clean up
        delete individualCanvas;
        delete graph;
    }

    // Loop through the 10 SiPM channels (1 to 10)
    for (int i = 0; i < 10; i++) {
        TGraph *graph = new TGraph();

        // Determine the title for each SiPM channel
        TString title = Form("SiPM %d Waveform", i + 1);

        // Get the corresponding adcVal index from the SiPM mapping
        int adcIndex = sipmChannelMap[i];

        // Loop through all entries in the tree
        for (Long64_t j = 0; j < nEntries; j++) {
            tree->GetEntry(j);  // Load the entry
            for (int k = 0; k < 45; k++) {  // Loop through the 45 samples per channel
                double time = k * timeStep;  // Time in arbitrary units
                double adcValue = adcVal[adcIndex][k];  // ADC value for the current SiPM channel, k-th sample
                graph->SetPoint(k, time, adcValue);  // Set the point in the graph
            }
        }

        // Set graph title and axis labels
        graph->SetTitle(title);
        graph->GetXaxis()->SetTitle("Time (16ns)");  // Modify if you know the time scale
        graph->GetYaxis()->SetTitle("ADC Value");

        // Set the y-axis range for SiPMs to [100, 250]
        graph->GetYaxis()->SetRangeUser(180, 250);

        // Draw the graph on the combined canvas
        combinedCanvas->cd(plotIndex++);
        graph->Draw("AL");

        // Add custom labels for SiPM channels
        TLatex latex;
        latex.SetNDC(); // Use normalized device coordinates (0 to 1)
        latex.SetTextSize(0.03); // Set text size
        latex.DrawLatex(0.75, 0.85, Form("Entries: %lld", nEntries));  // Number of entries (aligned to the right)

        // Save individual plots as PNG
        TCanvas *individualCanvas = new TCanvas(Form("c1_SiPM_%d", i), "Waveform", 800, 600);
        graph->Draw("AL");
        individualCanvas->SaveAs(Form("SiPM_%d_signal.png", i + 1));
        
        // Clean up
        delete individualCanvas;
        delete graph;
    }

    // Handle Event 61 (channel 22)
    TGraph *event61Graph = new TGraph();
    TString eventTitle = "Event 61 Waveform";

    // Loop through all entries for Event 61 (channel 22)
    for (Long64_t j = 0; j < nEntries; j++) {
        tree->GetEntry(j);  // Load the entry
        for (int k = 0; k < 45; k++) {  // Loop through the 45 samples for Event 61
            double time = k * timeStep;  // Time in arbitrary units
            double adcValue = adcVal[22][k];  // ADC value for Event 61 (channel 22)
            event61Graph->SetPoint(k, time, adcValue);  // Set the point in the graph
        }
    }

    // Set graph title and axis labels for Event 61
    event61Graph->SetTitle(eventTitle);
    event61Graph->GetXaxis()->SetTitle("Time (16ns)");  // Modify if you know the time scale
    event61Graph->GetYaxis()->SetTitle("ADC Value");

    // Set the y-axis range for Event 61 to [0, 1500] (same as PMTs)
    event61Graph->GetYaxis()->SetRangeUser(0, 1500);

    // Draw the Event 61 graph on the combined canvas
    combinedCanvas->cd(plotIndex++);
    event61Graph->Draw("AL");

    // Add custom labels for Event 61
    TLatex latex;
    latex.SetNDC(); // Use normalized device coordinates (0 to 1)
    latex.SetTextSize(0.03); // Set text size
    latex.DrawLatex(0.15, 0.85, eventTitle);  // Title of the plot
    latex.DrawLatex(0.75, 0.85, Form("Entries: %lld", nEntries));  // Number of entries (aligned to the right)

    // Save the Event 61 plot as PNG
    TCanvas *individualCanvas = new TCanvas("c1_Event61", "Event 61 Waveform", 800, 600);
    event61Graph->Draw("AL");
    individualCanvas->SaveAs("Event_61_signal.png");

    // Clean up
    delete individualCanvas;
    delete event61Graph;

    // Save the combined canvas as a single PNG file
    combinedCanvas->SaveAs("combined_plots.png");

    // Close the file
    file->Close();
}

int main() {
    // Call the function to plot the signals
    PlotpmtsvsTIME();
    return 0;
}
