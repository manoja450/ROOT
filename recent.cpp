#include <TFile.h>
#include <TTree.h>
#include <TCanvas.h>
#include <TGraph.h>
#include <TString.h>
#include <iostream>
#include <TAxis.h>
#include <TLatex.h>
#include <limits.h> // For SHRT_MAX and SHRT_MIN

using namespace std;

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

    // Determine the maximum pulseH values for PMTs and SiPMs separately
    double maxPMT = SHRT_MIN; // Variable to track maximum PMT pulse height
    double maxSiPM = SHRT_MIN; // Variable to track maximum SiPM pulse height

    for (Long64_t j = 0; j < nEntries; j++) {
        tree->GetEntry(j);  // Load the entry
        for (int i = 0; i < 12; i++) {  // PMTs (indices 0–11)
            if (pulseH[i] > maxPMT) maxPMT = pulseH[i];
        }
        for (int i = 12; i < 22; i++) {  // SiPMs (indices 12–21)
            if (pulseH[i] > maxSiPM) maxSiPM = pulseH[i];
        }
    }

    cout << "Max PMT pulse height: " << maxPMT << endl;
    cout << "Max SiPM pulse height: " << maxSiPM << endl;

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
        graph->SetMaximum(maxPMT); // Set maximum Y-axis value to the highest PMT pulse height
        graph->GetXaxis()->SetRangeUser(0, 720); // Set x-axis range to 0–720 ns

        graph->Draw("AL");
        canvas->SaveAs(Form("PMT%d_%s.png", i + 1, fileName));
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
        graph->SetMaximum(maxSiPM); // Set maximum Y-axis value to the highest SiPM pulse height
        graph->GetXaxis()->SetRangeUser(0, 720); // Set x-axis range to 0–720 ns

        graph->Draw("AL");
        canvas->SaveAs(Form("SiPM%d_%s.png", i + 1, fileName));
        delete graph;
        delete canvas;
    }

    // Close the file
    file->Close();
}

int main(int argc, char **argv) {
    if (argc < 2) {
        cerr << "Usage: " << argv[0] << " <file1> <file2> ..." << endl;
        return 1;
    }

    for (int i = 1; i < argc; i++) {
        cout << "Processing file: " << argv[i] << endl;
        PlotpmtsAndSipms(argv[i]);
    }

    return 0;
}
