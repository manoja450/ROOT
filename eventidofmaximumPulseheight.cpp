#include <TFile.h>
#include <TTree.h>
#include <iostream>
#include <float.h>

void findPulseHeightExtremes(const char* fileName) {
    // Variables to store the pulse height values
    Double_t pulseH[23];  // Array to hold pulse height values for each channel
    Int_t eventID;        // Variable to hold the event ID

    // Load the ROOT file
    TFile *file = TFile::Open(fileName);  // Use the file name passed as an argument
    if (!file || file->IsZombie()) {
        std::cerr << "Error opening the file: " << fileName << std::endl;
        return;
    }

    // Load the tree
    TTree *tree = (TTree*)file->Get("tree");  // Replace with your tree name if different
    if (!tree) {
        std::cerr << "Error: Tree not found in file: " << fileName << std::endl;
        file->Close();
        return;
    }

    // Set the branch addresses
    tree->SetBranchAddress("pulseH", pulseH);
    tree->SetBranchAddress("eventID", &eventID);  // Assuming eventID is stored in the tree

    // Variables to track the maximum pulse height and corresponding eventID
    Double_t maxPulseH = -DBL_MAX;
    Int_t maxPulseEventID = -1;

    // Loop over all entries in the tree
    Long64_t nEntries = tree->GetEntries();
    for (Long64_t i = 0; i < nEntries; i++) {
        tree->GetEntry(i);  // Load the data for the i-th entry

        // Loop over the 23 channels and check the pulse height values
        for (int j = 0; j < 23; j++) {
            if (pulseH[j] > maxPulseH) {
                maxPulseH = pulseH[j];
                maxPulseEventID = eventID;  // Update the eventID for the maximum pulse height
            }
        }
    }

    // Print the results
    std::cout << "Maximum pulse height: " << maxPulseH << std::endl;
    std::cout << "Event ID with maximum pulse height: " << maxPulseEventID << std::endl;

    // Close the file
    file->Close();
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <ROOT file>" << std::endl;
        return 1;
    }

    // Call the function with the file name provided as a command-line argument
    findPulseHeightExtremes(argv[1]);
    return 0;
}
