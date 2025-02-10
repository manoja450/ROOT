#include <TFile.h>
#include <TTree.h>
#include <TCanvas.h>
#include <TH1.h>
#include <TString.h>
#include <iostream>
#include "TStyle.h"
#include <TLatex.h>

void HistBaselineRMS(const char* filename) {
    // Open the ROOT file
    TFile *file = TFile::Open(filename);
    if (!file || file->IsZombie()) {
        std::cerr << "Error: Cannot open the file!" << std::endl;
        return;
    }

    // Get the TTree from the file
    TTree *tree = (TTree*)file->Get("tree");
    if (!tree) {
        std::cerr << "Error: Cannot find the TTree 'tree'!" << std::endl;
        file->Close();
        return;
    }

    // Define channel mappings
    int pmtChannelMap[12] = {0, 10, 7, 2, 6, 3, 8, 9, 11, 4, 5, 1};
    int sipmChannelMap[10] = {12, 13, 14, 15, 16, 17, 18, 19, 20, 21};

    // Create a master canvas with sufficient pads (6 rows, 5 columns)
    TCanvas *masterCanvas = new TCanvas("MasterCanvas", "Combined PMT and SiPM Histogram", 3600, 3000); 
    masterCanvas->Divide(5, 6); // 5 columns and 6 rows to accommodate 23 plots

    // Define the custom layout for the combined chart based on channel mapping
    int layout[6][5] = {
        {-1,  -1,  20,  21, -1},   // Row 1 (SiPM20, SiPM21)
        {16,  9,   3,   7,  12},   // Row 2 (SiPM16, PMT9, PMT3, PMT7, SiPM12)
        {15,  5,   4,   8,   -1},  // Row 3 (SiPM15, PMT5, PMT4, PMT8)
        {19,  0,   6,   1,  17},   // Row 4 (SiPM19, PMT0, PMT6, PMT1, SiPM17)
        {-1,  10,  11,  2,  13},   // Row 5 (PMT10, PMT11, PMT2, SiPM13)
        {-1,  14,  18,  -1, -1}    // Row 6 (SiPM14, SiPM18)
    };
    
    / Create a large font textbox on the master canvas
    masterCanvas->cd(0); // Select the canvas itself (outside the pads)
    TLatex *textbox = new TLatex(); // Create a TLatex object for drawing text
    textbox->SetTextSize(0.02); // Set text size
    textbox->SetTextAlign(13);  // Align bottom-left
    textbox->SetNDC(true);      // Use normalized device coordinates
    textbox->DrawLatex(0.01, 0.10, "X axis: BaselineRMS"); // Draw the first line of text
    textbox->DrawLatex(0.01, 0.08, "Y axis: Counts"); // Draw the second line of text
    // Loop through the layout and plot histograms for each channel
    for (int row = 0; row < 6; ++row) {
        for (int col = 0; col < 5; ++col) {
            int ch = layout[row][col];
            if (ch == -1) continue; // Skip empty spots in the layout

            int isPMT = (ch < 12); // Check if it's a PMT channel (0-11 for PMTs)
            int isSiPM = (ch >= 12 && ch < 22); // Check if it's a SiPM channel (12-21 for SiPMs)

            masterCanvas->cd(row * 5 + col + 1); // Switch to the correct pad based on the layout

            // Define a unique histogram name for each channel
            TString histName = TString::Format("hist_baselineRMS_ch%d", ch);

            // Construct the draw command: baselineRMS[ch] is plotted with 100 bins in the range [0, 10]
            TString drawCmd;
            if (isPMT && ch < 12) {
                drawCmd = TString::Format("baselineRMS[%d] >> %s(100, 0, 10)", pmtChannelMap[ch], histName.Data());
            } else if (isSiPM && ch - 12 < 10) {
                drawCmd = TString::Format("baselineRMS[%d] >> %s(100, 0, 10)", sipmChannelMap[ch - 12], histName.Data());
            }

            // Draw the first histogram for the current channel (all data)
            tree->Draw(drawCmd, "", "hist");

            // Retrieve the histogram from the pad
            TH1 *hist = (TH1*)gPad->GetPrimitive(histName);
            if (hist) {
                // Calculate the mean of the histogram
                double mean = hist->GetMean();

                // Create a new histogram for data after applying the cut (values above the mean)
                TString histAfterCutName = TString::Format("hist_baselineRMS_cut_ch%d", ch);
                TString drawCmdAfterCut;
                if (isPMT && ch < 12) {
                    drawCmdAfterCut = TString::Format("baselineRMS[%d] >> %s(100, 0, 10)", pmtChannelMap[ch], histAfterCutName.Data());
                } else if (isSiPM && ch - 12 < 10) {
                    drawCmdAfterCut = TString::Format("baselineRMS[%d] >> %s(100, 0, 10)", sipmChannelMap[ch - 12], histAfterCutName.Data());
                }

                // Draw the second histogram with the cut (only values greater than mean)
                tree->Draw(drawCmdAfterCut, TString::Format("baselineRMS[%d] > %f", (isPMT ? pmtChannelMap[ch] : sipmChannelMap[ch - 12]), mean), "hist same");

                // Retrieve the new histogram
                TH1 *histAfterCut = (TH1*)gPad->GetPrimitive(histAfterCutName);
                if (histAfterCut) {
                    histAfterCut->SetLineColor(kRed); // Set the color for the histogram after cut (Red)
                }
                     // Set large font size for title
                    gStyle->SetTitleFontSize(0.11);  // You can adjust this size to fit your needs
                // Set titles and labels for the histograms
                if (isPMT && ch < 12) {
                    int pmtIndex = -1;
                    for (int i = 0; i < 12; ++i) {
                        if (pmtChannelMap[i] == ch) {
                            pmtIndex = i + 1; // PMT number starts from 1
                            break;
                        }
                    }
                    hist->SetTitle(TString::Format("PMT %d ", pmtIndex));
                } else if (isSiPM && ch - 12 < 10) {
                    hist->SetTitle(TString::Format("SiPM %d ", sipmChannelMap[ch - 12] - 11)); // SiPM number starts from 1
                }
                hist->GetXaxis()->SetTitle("Baseline RMS"); // Label the X-axis
                hist->GetYaxis()->SetTitle("Counts");       // Label the Y-axis

                // Save the individual histogram as a PNG image
                TCanvas *individualCanvas = new TCanvas(TString::Format("Canvas_ch%d", ch), TString::Format("Channel %d Histogram", ch), 800, 600);
                hist->Draw();
                histAfterCut->Draw("same"); // Overlay the second histogram
                individualCanvas->SaveAs(TString::Format("channel_%d_histogram.png", ch));
                delete individualCanvas; // Clean up the individual canvas
            }
        }
    }

    // Save the entire canvas as a PNG image for later review
    masterCanvas->SaveAs("combined_baselineRMS_histograms.png");

    // Clean up: Close the file (optional but good practice)
    file->Close();
}

// Main function to accept filename from terminal and call HistBaselineRMS
int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <root_file>" << std::endl;
        return 1;
    }

    const char* filename = argv[1]; // Get the filename from the command line
    HistBaselineRMS(filename); // Call the function to plot histograms
    return 0;
}

