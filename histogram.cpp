#include <iostream>
#include <TFile.h>
#include <TTree.h>
#include <TH1F.h>
#include <TCanvas.h>
#include <TString.h>

using namespace std;

void PlotCombinedHistograms(const char *fileName) {
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

    Short_t adcVal[23][45];
    tree->SetBranchAddress("adcVal", adcVal);

    Long64_t nEntries = tree->GetEntries();

    // PMT and SiPM channel mappings
    int pmtChannelMap[12] = {0, 10, 7, 2, 6, 3, 8, 9, 11, 4, 5, 1};
    int sipmChannelMap[10] = {12, 13, 14, 15, 16, 17, 18, 19, 20, 21};

    // Create a master canvas with sufficient pads (6 rows, 5 columns)
    TCanvas *masterCanvas = new TCanvas("MasterCanvas", "Combined PMT and SiPM Waveforms", 3600, 3000);
    masterCanvas->Divide(5, 6); // 5 columns and 6 rows to accommodate 23 plots

    // Layout based on channel mapping
    int layout[6][5] = {
        {-1,  -1,  20,  21, -1},   // Row 1 (SiPM20, SiPM21)
        {16,  9,   3,   7,  12},    // Row 2 (SiPM16, PMT9, PMT3, PMT7, SiPM1)
        {15,  5,   4,   8,   -1},   // Row 3 (SiPM15, PMT5, PMT4, PMT8)
        {19, 0,   6,  1,  17},   // Row 4 (SiPM19, PMT0, PMT6, PMT1, SiPM17)
        {-1,  10,  11,   2,  13},  // Row 5 (PMT10, PMT11, PMT2, SiPM13)
        {-1, 14,   18,  -1, -1}     // Row 6 (SiPM14, SiPM18)
    };

    // Loop through the layout to create histograms for each channel
    for (int row = 0; row < 6; row++) {
        for (int col = 0; col < 5; col++) {
            int channel = layout[row][col];
            if (channel == -1) continue;  // Skip empty spots in the layout

            TH1F *hist = nullptr;
            if (channel < 12) {
                // PMT channel
                int pmtChannel = pmtChannelMap[channel];
                TString histName = Form("adcVal_PMT_%d", pmtChannel);
                hist = new TH1F(histName, Form("ADC Value Distribution for PMT Channel %d", pmtChannel), 100, 0, 4096);
                for (Long64_t i = 0; i < nEntries; i++) {
                    tree->GetEntry(i);
                    hist->Fill(adcVal[pmtChannel][0]);  // Assuming all samples for this PMT channel are in the first sample (0th index)
                }
            } else {
                // SiPM channel
                int sipmChannel = sipmChannelMap[channel - 12];
                TString histName = Form("adcVal_SiPM_%d", sipmChannel);
                hist = new TH1F(histName, Form("ADC Value Distribution for SiPM Channel %d", sipmChannel), 100, 0, 4096);
                for (Long64_t i = 0; i < nEntries; i++) {
                    tree->GetEntry(i);
                    hist->Fill(adcVal[sipmChannel][0]);  // Assuming all samples for this SiPM channel are in the first sample (0th index)
                }
            }

            // Draw the histogram on the corresponding pad
            masterCanvas->cd(row * 5 + col + 1);  // Select the pad based on row and column
            hist->SetTitle(Form("ADC Value Distribution for Channel %d", channel));
            hist->GetXaxis()->SetTitle("ADC Value");
            hist->GetYaxis()->SetTitle("Entries");
            hist->Draw();

            // Cleanup the histogram
            delete hist;
        }
    }

    // Save the master canvas as a PNG file in the specified directory
    TString outputFileName = "/root/gears/Histogram/Combined_ADCValue_Waveforms.png";
    masterCanvas->SaveAs(outputFileName);
    cout << "Master canvas with combined histograms saved as " << outputFileName << endl;

    // Cleanup the canvas
    delete masterCanvas;

    file->Close();
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        cerr << "Usage: " << argv[0] << " <root_file>" << endl;
        return 1;
    }

    const char* fileName = argv[1];  // Get the file name from the command-line argument
    PlotCombinedHistograms(fileName);

    return 0;
}
