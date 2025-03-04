//This code do Gaussian fit for low light trigger events. Which is a single-pe calibration. It plots the histogram of area and fits the Gaussian.
#include <iostream>
#include <TFile.h>
#include <TTree.h>
#include <TH1F.h>
#include <TCanvas.h>
#include <TF1.h>
#include <TPaveStats.h> // Include TPaveStats header
#include <TLatex.h>     // Include TLatex header
#include <vector>
#include <algorithm>
#include <cmath>

using namespace std;

// Define the fitting function
Double_t SPEfit(Double_t *x, Double_t *par) {
    Double_t A0 = par[0];    // Amplitude of the pedestal peak
    Double_t mu0 = par[1];   // Mean of the pedestal peak
    Double_t sigma0 = par[2]; // Standard deviation of the pedestal peak
    Double_t A1 = par[3];    // Amplitude of the single photoelectron peak
    Double_t mu1 = par[4];   // Mean of the single photoelectron peak
    Double_t sigma1 = par[5]; // Standard deviation of the single photoelectron peak
    Double_t A2 = par[6];    // Amplitude of the second peak
    Double_t A3 = par[7];    // Amplitude of the third peak

    // Define the Gaussian terms for the fit
    Double_t term1 = A0 * exp(-0.5 * pow((x[0] - mu0) / sigma0, 2)); // Pedestal peak
    Double_t term2 = A1 * exp(-0.5 * pow((x[0] - mu1) / sigma1, 2)); // Single photoelectron peak
    Double_t term3 = A2 * exp(-0.5 * pow((x[0] - sqrt(2) * mu1) / sqrt(2 * sigma1 * sigma1 - sigma0 * sigma0), 2)); // Second peak
    Double_t term4 = A3 * exp(-0.5 * pow((x[0] - sqrt(3) * mu1) / sqrt(3 * sigma1 * sigma1 - 2 * sigma0 * sigma0), 2)); // Third peak

    return term1 + term2 + term3 + term4; // Return the sum of all terms
}

// Function to process the ROOT file and generate energy distributions
void processLowLightEvents(const char *fileName) {
    // Open the ROOT file
    TFile *file = TFile::Open(fileName);
    if (!file || file->IsZombie()) {
        cerr << "Error opening file: " << fileName << endl;
        return;
    }

    // Access the TTree named "tree" from the ROOT file
    TTree *tree = (TTree*)file->Get("tree");
    if (!tree) {
        cerr << "Error accessing TTree 'tree'!" << endl;
        file->Close();
        return;
    }

    // Declare variables to store data from the TTree
    Short_t adcVal[23][45]; // ADC values for 23 channels and 45 time bins
    Double_t area[23];      // Area for each channel
    Int_t triggerBits;      // Trigger bits to identify low light LED events

    // Set branch addresses to read data from the TTree
    tree->SetBranchAddress("adcVal", adcVal);
    tree->SetBranchAddress("area", area);
    tree->SetBranchAddress("triggerBits", &triggerBits);

    // Get the total number of entries in the TTree
    Long64_t nEntries = tree->GetEntries();

    // Create histograms to store the area (energy) distributions for each PMT
    TH1F *histArea[12];
    for (int i = 0; i < 12; i++) {
        histArea[i] = new TH1F(Form("PMT%d_Area", i + 1), Form("; Area; Events per 3 ADCs", i + 1), 150, -50, 400);
        histArea[i]->SetLineColor(kRed); // Set histogram line color to red
    }

    // Mapping of PMT channels
    int pmtChannelMap[12] = {0, 10, 7, 2, 6, 3, 8, 9, 11, 4, 5, 1};

    // Loop over all events in the TTree
    for (Long64_t entry = 0; entry < nEntries; entry++) {
        tree->GetEntry(entry);

        // Check if the event is a low light LED event (triggerBits = 16)
        if (triggerBits == 16) {
            // Loop through the 12 PMTs and fill their area distributions
            for (int pmt = 0; pmt < 12; pmt++) {
                int adcIndex = pmtChannelMap[pmt]; // Map PMT channels
                histArea[pmt]->Fill(area[adcIndex]); // Fill the histogram with the area
            }
        }
    }

    // Create a canvas to draw the histograms
    TCanvas *canvas = new TCanvas("canvas", "PMT Energy Distributions", 800, 600);

    // Adjust margins and text size for better visibility
    canvas->SetLeftMargin(0.15);   // Increase left margin
    canvas->SetRightMargin(0.05);  // Adjust right margin
    canvas->SetBottomMargin(0.15); // Increase bottom margin
    canvas->SetTopMargin(0.05);    // Adjust top margin

    // Save each histogram as a PNG file
    for (int i = 0; i < 12; i++) {
        canvas->Clear(); // Clear the canvas for the next histogram

        // Adjust histogram text size
        histArea[i]->GetXaxis()->SetTitleSize(0.05); // Increase x-axis title size
        histArea[i]->GetYaxis()->SetTitleSize(0.05); // Increase y-axis title size
        histArea[i]->GetXaxis()->SetLabelSize(0.04); // Increase x-axis label size
        histArea[i]->GetYaxis()->SetLabelSize(0.04); // Increase y-axis label size

        // Fit the histogram with the SPEfit function
        TF1 *fitFunc = new TF1("fitFunc", SPEfit, -50, 400, 8);
        fitFunc->SetParameters(1000, 0, 10, 1000, 50, 10, 500, 500); // Initial parameters
        fitFunc->SetLineColor(kBlue); // Set fit line color to blue

        // Set custom parameter names
        fitFunc->SetParName(0, "A0");
        fitFunc->SetParName(1, "#mu_{0}");
        fitFunc->SetParName(2, "#sigma_{0}");
        fitFunc->SetParName(3, "A1");
        fitFunc->SetParName(4, "#mu_{1}");
        fitFunc->SetParName(5, "#sigma_{1}");
        fitFunc->SetParName(6, "A2");
        fitFunc->SetParName(7, "A3");

        histArea[i]->Fit("fitFunc", "R"); // Perform the fit

        // Draw the histogram and the fit
        histArea[i]->Draw();
        fitFunc->Draw("same");

        // Add a title to the individual plot
        TLatex *title = new TLatex();
        title->SetTextSize(0.06); // Set text size
        title->SetTextAlign(22);  // Center alignment
        title->SetNDC(true);      // Use normalized coordinates
        title->DrawLatex(0.5, 0.92, Form("PMT %d", i + 1)); // Draw title at the top center

        // Enable statistics box and customize it
        gPad->Update(); // Update the canvas to ensure the stats box is drawn
        TPaveStats *stats = (TPaveStats*)histArea[i]->FindObject("stats");
        if (stats) {
            stats->SetX1NDC(0.4); // Set X position of the stats box (move it further left)
            stats->SetY1NDC(0.4); // Set Y position of the stats box (move it higher)
            stats->SetX2NDC(0.90); // Set width of the stats box (widen the box more)
            stats->SetY2NDC(0.85); // Set height of the stats box (tall the box more)
            stats->SetTextColor(kBlack); // Set text color
            stats->SetTextSize(0.05); // Set larger text size
            stats->SetOptStat(10); // Show only entries
            stats->SetOptFit(111); // Show fit parameters
            stats->SetName(""); // Remove the title from the stats box
        }

        canvas->SaveAs(Form("PMT%d_Energy_Distribution.png", i + 1)); // Save as PNG

        // Clean up
        delete fitFunc;
        delete title;
    }

    // Create a master canvas for the combined plot
    TCanvas *masterCanvas = new TCanvas("MasterCanvas", "Combined PMT Energy Distributions", 3600, 3000);
    masterCanvas->Divide(3, 4, 0, 0); // Adjust spacing between subplots

    // Define the layout of PMT channels on the canvas
    int layout[4][3] = {
        {9, 3, 7},  // Row 1: PMT 10, PMT 4, PMT 8
        {5, 4, 8},  // Row 2: PMT 6, PMT 5, PMT 9
        {0, 6, 1},  // Row 3: PMT 1, PMT 7, PMT 2
        {10, 11, 2} // Row 4: PMT 11, PMT 12, PMT 3
    };

    // Loop through the layout to plot histograms on the master canvas
    for (int row = 0; row < 4; row++) {
        for (int col = 0; col < 3; col++) {
            int padPosition = row * 3 + col + 1; // Calculate pad position (1-12)
            masterCanvas->cd(padPosition); // Switch to the specific pad

            int pmtIndex = layout[row][col]; // Get PMT index from layout

            // Adjust histogram text size
            histArea[pmtIndex]->GetXaxis()->SetTitleSize(0.07); // Increase x-axis title size
            histArea[pmtIndex]->GetYaxis()->SetTitleSize(0.09); // Increase y-axis title size
            histArea[pmtIndex]->GetXaxis()->SetLabelSize(0.04); // Increase x-axis label size
            histArea[pmtIndex]->GetYaxis()->SetLabelSize(0.04); // Increase y-axis label size

            // Ensure Y-axis label is visible
            histArea[pmtIndex]->GetYaxis()->SetTitle("Events per 3 ADCs");
            histArea[pmtIndex]->GetYaxis()->SetTitleOffset(0.8); // Move Y-axis title closer to the axis line

            // Ensure X-axis label is visible
            histArea[pmtIndex]->GetXaxis()->SetTitle("Area");

            // Adjust margins for each subplot
            gPad->SetLeftMargin(0.15);   // Increase left margin
            gPad->SetRightMargin(0.00); // Adjust right margin
            gPad->SetBottomMargin(0.15); // Increase bottom margin
            gPad->SetTopMargin(0.01);    // Adjust top margin

            // Fit the histogram with the SPEfit function
            TF1 *fitFunc = new TF1("fitFunc", SPEfit, -50, 400, 8);
            fitFunc->SetParameters(100, 0, 10, 100, 50, 10, 50, 50); // Initial parameters
            fitFunc->SetLineColor(kBlue); // Set fit line color to blue

            // Set custom parameter names
            fitFunc->SetParName(0, "A0");
            fitFunc->SetParName(1, "#mu_{0}");
            fitFunc->SetParName(2, "#sigma_{0}");
            fitFunc->SetParName(3, "A1");
            fitFunc->SetParName(4, "#mu_{1}");
            fitFunc->SetParName(5, "#sigma_{1}");
            fitFunc->SetParName(6, "A2");
            fitFunc->SetParName(7, "A3");

            histArea[pmtIndex]->Fit("fitFunc", "R"); // Perform the fit

            // Draw the histogram and the fit
            histArea[pmtIndex]->Draw();
            fitFunc->Draw("same");

            // Add a title to the subplot
            TLatex *title = new TLatex();
            title->SetTextSize(0.14); // Set larger font size
            title->SetTextAlign(22);  // Center alignment
            title->SetNDC(true);      // Use normalized coordinates
            title->DrawLatex(0.5, 0.92, Form("PMT %d", pmtIndex + 1)); // Draw title at the top center

            // Enable statistics box and customize it
            gPad->Update(); // Update the canvas to ensure the stats box is drawn
            TPaveStats *stats = (TPaveStats*)histArea[pmtIndex]->FindObject("stats");
            if (stats) {
                stats->SetX1NDC(0.4); // Set X position of the stats box (move it further left)
                stats->SetY1NDC(0.4); // Set Y position of the stats box (move it higher)
                stats->SetX2NDC(0.90); // Set width of the stats box (widen the box more)
                stats->SetY2NDC(0.85); // Set height of the stats box (tall the box more)
                stats->SetTextColor(kBlack); // Set text color
                stats->SetTextSize(0.05); // Set larger text size
                stats->SetOptStat(10); // Show only entries
                stats->SetOptFit(111); // Show fit parameters
                stats->SetName(""); // Remove the title from the stats box
            }

            // Clean up
            delete fitFunc;
            delete title;
        }
    }

    // Save the combined canvas as a PNG file
    masterCanvas->SaveAs("Combined_PMT_Energy_Distributions.png");

    // Clean up
    for (int i = 0; i < 12; i++) {
        delete histArea[i];
    }
    delete canvas;
    delete masterCanvas;
    file->Close();

    cout << "Energy distributions for low light LED events saved as PNG files." << endl;
    cout << "Combined image saved as Combined_PMT_Energy_Distributions.png" << endl;
}

// Main function to handle command-line arguments
int main(int argc, char* argv[]) {
    if (argc != 2) {
        cerr << "Usage: " << argv[0] << " <root_file>" << endl;
        return 1;
    }

    const char* fileName = argv[1];
    processLowLightEvents(fileName);

    return 0;
}
