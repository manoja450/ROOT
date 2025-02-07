//This code gives the plots of waveforms and creates a combined canvas according to the physical location of the PMTS/SiPMs.
//( EventID is not  specified, so it gives a plot of the first event).It also creates a legend on Combined canvas.

#include <iostream> // Include the standard input/output stream library for console I/O
#include <TFile.h> // Include ROOT's TFile class for file handling
#include <TTree.h> // Include ROOT's TTree class for tree data structure
#include <TGraph.h> // Include ROOT's TGraph class for graph plotting
#include <TCanvas.h> // Include ROOT's TCanvas class for drawing canvases
#include <TAxis.h> // Include ROOT's TAxis class for axis manipulation
#include <TH1F.h> // Include ROOT's TH1F class for 1D histogram manipulation
#include <vector> // Include the standard vector library for dynamic arrays
#include <algorithm> // Include the standard algorithm library for functions like max_element
#include <cmath> // Include the standard math library for mathematical functions
#include "TLatex.h" // Include ROOT's TLatex class for drawing LaTeX-style text

using namespace std; // Use the standard namespace to avoid prefixing std::

// Function to round up a value to the nearest bin size
double roundUpToBin(double value, double binSize) {
    return ceil((value + 0.5) / binSize) * binSize; // Add a small offset to ensure proper rounding
}

// Main function to plot combined and individual charts
void PlotCombinedChartAndIndividual(const char *fileName) {
    TFile *file = TFile::Open(fileName); // Open the ROOT file
    if (!file || file->IsZombie()) { // Check if the file opened successfully
        cerr << "Error opening file: " << fileName << endl; // Print error message
        return; // Exit the function
    }

    TTree *tree = (TTree*)file->Get("tree"); // Access the TTree named "tree" from the file
    if (!tree) { // Check if the tree was accessed successfully
        cerr << "Error accessing TTree 'tree'!" << endl; // Print error message
        file->Close(); // Close the file
        return; // Exit the function
    }

    Short_t adcVal[23][45]; // Declare a 2D array to store ADC values
    double_t pulseH[23]; // Declare an array to store pulse heights
    tree->SetBranchAddress("adcVal", adcVal); // Set the branch address for ADC values
    tree->SetBranchAddress("pulseH", pulseH); // Set the branch address for pulse heights

    Long64_t nEntries = tree->GetEntries(); // Get the number of entries in the tree

    vector<double> pmtPulseHeights, sipmPulseHeights; // Declare vectors to store PMT and SiPM pulse heights
    for (Long64_t j = 0; j < nEntries; j++) { // Loop over all entries in the tree
        tree->GetEntry(j); // Load the current entry
        for (int i = 0; i < 12; i++) pmtPulseHeights.push_back(pulseH[i]); // Store PMT pulse heights
        for (int i = 12; i < 22; i++) sipmPulseHeights.push_back(pulseH[i]); // Store SiPM pulse heights
    }

    double maxPMT = *max_element(pmtPulseHeights.begin(), pmtPulseHeights.end()); // Find the maximum PMT pulse height
    double maxSiPM = *max_element(sipmPulseHeights.begin(), sipmPulseHeights.end()); // Find the maximum SiPM pulse height

    int pmtChannelMap[12] = {0, 10, 7, 2, 6, 3, 8, 9, 11, 4, 5, 1}; // Define a mapping for PMT channels
    int sipmChannelMap[10] = {12, 13, 14, 15, 16, 17, 18, 19, 20, 21}; // Define a mapping for SiPM channels

    // Create a master canvas with sufficient pads (6 rows, 5 columns)
    TCanvas *masterCanvas = new TCanvas("MasterCanvas", "Combined PMT and SiPM Waveforms", 3600, 3000);
    masterCanvas->Divide(5, 6); // Divide the canvas into 5 columns and 6 rows

    // Define a layout for the combined chart based on channel mapping
    int layout[6][5] = {
        {-1,  -1,  20,  21, -1},   // Row 1 (SiPM20, SiPM21)
        {16,  9,   3,   7,  12},    // Row 2 (SiPM16, PMT9, PMT3, PMT7, SiPM1)
        {15,  5,   4,   8,   -1},   // Row 3 (SiPM15, PMT5, PMT4, PMT8)
        {19, 0,   6,  1,  17},   // Row 4 (SiPM19, PMT0, PMT6, PMT1, SiPM17)
        {-1,  10,  11,   2,  13},  // Row 5 (   PMT10,PMT11, PMT2, SiPM13)
        {-1, 14,   18,  -1, -1}     // Row 6 (SiPM14,SiPM18)
    };

    // Create a large font textbox on the master canvas
    masterCanvas->cd(0); // Select the canvas itself (outside the pads)
    TLatex *textbox = new TLatex(); // Create a TLatex object for drawing text
    textbox->SetTextSize(0.02); // Set text size
    textbox->SetTextAlign(13);  // Align bottom-left
    textbox->SetNDC(true);      // Use normalized device coordinates
    textbox->DrawLatex(0.01, 0.10, "X axis: Time (0-720) ns"); // Draw the first line of text
    textbox->DrawLatex(0.01, 0.08, "Y axis: ADC values"); // Draw the second line of text

    // Plot PMTs and SiPMs at specific positions according to the layout
    for (int row = 0; row < 6; row++) { // Loop over rows
        for (int col = 0; col < 5; col++) {  // Loop over columns
            int padPosition = layout[row][col]; // Get the pad position from the layout
            if (padPosition >= 0) { // Check if the pad position is valid
                masterCanvas->cd(row * 5 + col + 1); // Select the correct pad
                TGraph *graph = new TGraph(); // Create a new TGraph object

                int adcIndex = -1; // Initialize the ADC index
                if (padPosition < 12) {
                    adcIndex = pmtChannelMap[padPosition];  // Correct PMT mapping
                }
                else {
                    adcIndex = sipmChannelMap[padPosition - 12];  // Correct SiPM mapping
                }

                for (int k = 0; k < 45; k++) { // Loop over ADC values
                    double time = (k + 1) * 16.0; // Calculate time
                    if (time > 720) break; // Break if time exceeds 720 ns
                    double adcValue = adcVal[adcIndex][k]; // Get ADC value
                    graph->SetPoint(k, time, adcValue); // Set point in the graph
                }

                TString title; // Declare a title string
                if (padPosition < 12) {
                    title = Form("PMT %d", padPosition + 1); // Set title for PMT
                    graph->SetMaximum(maxPMT * 1.1); // Set maximum value for PMT graph
                } else {
                    title = Form("SiPM %d", padPosition - 11);  // Set title for SiPM
                    graph->SetMaximum(maxSiPM * 1.1); // Set maximum value for SiPM graph
                }
                graph->SetTitle(""); // Clear the default title
                graph->GetXaxis()->SetTitle("Time (ns)"); // Set X-axis title
                graph->GetYaxis()->SetTitle("ADC Value"); // Set Y-axis title
                graph->SetMinimum(0); // Set minimum value for the graph
                graph->GetXaxis()->SetRangeUser(0, 720); // Set X-axis range

                graph->Draw("AL"); // Draw the graph

                // Add a custom title with larger font size using TLatex
                TLatex *latexTitle = new TLatex(); // Create a TLatex object for the title
                latexTitle->SetTextSize(0.10); // Set larger font size
                latexTitle->SetTextAlign(22);  // Center alignment
                latexTitle->SetNDC(true);      // Use normalized coordinates
                latexTitle->DrawLatex(0.5, 0.94, title); // Draw title at the top center
            }
        }
    }

    // Save the combined chart
    TString combinedChartFileName = Form("/root/gears/new/CombinedChart_SpecificLayout_%s.png", fileName);
    masterCanvas->SaveAs(combinedChartFileName); // Save the combined chart as a PNG file
    cout << "Combined chart saved as " << combinedChartFileName << endl; // Print confirmation message

    // Save individual PMT and SiPM plots
    for (int i = 0; i < 12; i++) { // Loop over PMT channels
        TString individualPMTFileName = Form("/root/gears/new/PMT%d_%s.png", i + 1, fileName);
        TCanvas *individualCanvas = new TCanvas(Form("/root/gears/new/PMT%d_Canvas", i + 1), Form("PMT %d", i + 1), 800, 600);
        TGraph *graph = new TGraph(); // Create a new TGraph object

        int adcIndex = pmtChannelMap[i]; // Get the ADC index for the current PMT
        for (int k = 0; k < 45; k++) { // Loop over ADC values
            double time = (k + 1) * 16.0; // Calculate time
            double adcValue = adcVal[adcIndex][k]; // Get ADC value
            graph->SetPoint(k, time, adcValue); // Set point in the graph
        }

        graph->SetTitle(Form("PMT %d", i + 1)); // Set graph title
        graph->GetXaxis()->SetTitle("Time (ns)"); // Set X-axis title
        graph->GetYaxis()->SetTitle("ADC Value"); // Set Y-axis title
        graph->SetMinimum(0); // Set minimum value for the graph
        graph->SetMaximum(maxPMT * 1.1); // Set maximum value for the graph
        graph->GetXaxis()->SetRangeUser(0, 720); // Set X-axis range

        graph->Draw("AL"); // Draw the graph
        individualCanvas->SaveAs(individualPMTFileName); // Save the individual PMT plot
        delete individualCanvas; // Delete the canvas to free memory
    }

    for (int i = 0; i < 10; i++) { // Loop over SiPM channels
        TString individualSiPMFileName = Form("/root/gears/new/SiPM%d_%s.png", i + 1, fileName);
        TCanvas *individualCanvas = new TCanvas(Form("/root/gears/new/SiPM%d_Canvas", i + 1), Form("SiPM %d", i + 1), 800, 600);
        TGraph *graph = new TGraph(); // Create a new TGraph object

        int adcIndex = sipmChannelMap[i]; // Get the ADC index for the current SiPM
        for (int k = 0; k < 45; k++) { // Loop over ADC values
            double time = (k + 1) * 16.0; // Calculate time
            double adcValue = adcVal[adcIndex][k]; // Get ADC value
            graph->SetPoint(k, time, adcValue); // Set point in the graph
        }

        graph->SetTitle(Form("SiPM %d", i + 1)); // Set graph title
        graph->GetXaxis()->SetTitle("Time (ns)"); // Set X-axis title
        graph->GetYaxis()->SetTitle("ADC Value"); // Set Y-axis title
        graph->SetMinimum(0); // Set minimum value for the graph
        graph->SetMaximum(maxSiPM * 1.1); // Set maximum value for the graph
        graph->GetXaxis()->SetRangeUser(0, 720); // Set X-axis range

        graph->Draw("AL"); // Draw the graph
        individualCanvas->SaveAs(individualSiPMFileName); // Save the individual SiPM plot
        delete individualCanvas; // Delete the canvas to free memory
    }

    file->Close(); // Close the ROOT file
}

int main(int argc, char* argv[]) { // Main function
    if (argc != 2) { // Check if the correct number of arguments is provided
        cerr << "Usage: " << argv[0] << " <root_file>" << endl; // Print usage message
        return 1; // Exit with error code
    }

    const char* fileName = argv[1];  // Get the file name from the command-line argument
    PlotCombinedChartAndIndividual(fileName); // Call the plotting function

    return 0; // Exit successfully
}
