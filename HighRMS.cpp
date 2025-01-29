#include <TFile.h>
#include <TTree.h>
#include <TCanvas.h>
#include <TH1.h>
#include <TString.h>
#include <iostream>
#include <vector>
#include <TAxis.h>  // Include TAxis header for proper definition

void ExtractAndPlotHighRMS(const char* filename, double highRMSThreshold = 2.0) {
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

    // Define branches (correct data types)
    Double_t baselineRMS[23];  // Assuming 23 channels (for PMTs, SiPMs, and Event61)
    Short_t adcVal[23][45];    // Assuming 23 channels with 45 samples each
    Int_t eventID;             // Single event ID
    Int_t nSamples[23];        // Number of samples for each channel

    // Set branch addresses
    if (tree->SetBranchAddress("baselineRMS", baselineRMS) < 0) {
        std::cerr << "Error: Cannot set branch address for baselineRMS" << std::endl;
        return;
    }
    if (tree->SetBranchAddress("adcVal", adcVal) < 0) {
        std::cerr << "Error: Cannot set branch address for adcVal" << std::endl;
        return;
    }
    if (tree->SetBranchAddress("eventID", &eventID) < 0) {
        std::cerr << "Error: Cannot set branch address for eventID" << std::endl;
        return;
    }
    if (tree->SetBranchAddress("nSamples", nSamples) < 0) {
        std::cerr << "Error: Cannot set branch address for nSamples" << std::endl;
        return;
    }

    // Loop over all events and apply the RMS threshold
    std::vector<int> selectedEventIDs;
    std::vector<std::vector<Short_t>> selectedADCValues[23]; // Store selected ADC values for each channel

    int nEntries = tree->GetEntries();
    for (int entry = 0; entry < nEntries; ++entry) {
        tree->GetEntry(entry);

        // Loop through all channels
        for (int i = 0; i < 23; ++i) {
            double rms = baselineRMS[i];
            if (rms > highRMSThreshold) {
                // Event passes the RMS cut, so save this event's data
                selectedEventIDs.push_back(eventID);  // Save the event ID
                std::vector<Short_t> adcValuesForEvent;
                
                // Save the ADC values for this event (only the ones for the current channel)
                for (int sample = 0; sample < nSamples[i]; ++sample) {
                    adcValuesForEvent.push_back(adcVal[i][sample]);
                }
                selectedADCValues[i].push_back(adcValuesForEvent);
            }
        }
    }

    // Check if we have any selected events
    if (selectedEventIDs.empty()) {
        std::cerr << "No events passed the RMS threshold!" << std::endl;
        return;
    }

    // Create canvas for visualizing traces of selected events
    TCanvas *traceCanvas = new TCanvas("TraceCanvas", "High RMS Event Traces", 1200, 800);
    traceCanvas->Divide(3, 8);  // Adjust the number of pads (3x8 for 23 channels)

    // Plot the ADC values for each channel for the selected events
    for (int ch = 0; ch < 23; ++ch) {
        traceCanvas->cd(ch + 1); // Go to the next pad

        TString histName = TString::Format("trace_ch%d", ch);
        TH1D *traceHist = new TH1D(histName, TString::Format("ADC Trace for Channel %d", ch), 45, 0, 45);

        // Collect ADC values for the selected events
        for (size_t i = 0; i < selectedEventIDs.size(); ++i) {
            // Plot all ADC values for this event for the current channel
            for (size_t j = 0; j < selectedADCValues[ch].size(); ++j) {
                for (int sample = 0; sample < nSamples[ch]; ++sample) {
                    traceHist->Fill(sample, selectedADCValues[ch][j][sample]);
                }
            }
        }

        // Set axis titles
        traceHist->GetXaxis()->SetTitle("Sample Index");
        traceHist->GetYaxis()->SetTitle("ADC Value");

        // Draw the histogram
        traceHist->Draw();
    }

    // Save the traces as a PNG image
    traceCanvas->SaveAs("highRMS_event_traces.png");

    // Clean up: Close the file
    file->Close();
}

// Main function to accept filename from terminal and call ExtractAndPlotHighRMS
int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <root_file>" << std::endl;
        return 1;
    }

    const char* filename = argv[1]; // Get the filename from the command line
    ExtractAndPlotHighRMS(filename); // Call the function to extract and plot high RMS events
    return 0;
}
