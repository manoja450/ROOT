#include <stdio.h>
#include <string.h>
#include <iostream>
#include <fstream>

/** @brief Properties recorded for each detected pulse in a waveform */
struct pulse {
  float start;  /* Start time of pulse (10% peak) in waveform (ns) */
  float end;    /* End time of pulse (reach baseline) in waveform (ns) */
  float peak;   /* Max amplitude of pulse (photo-electrons) */
  float energy; /* Energy (integral) of pulse (photo-electrons) */
};

/** @brief Constants for pulse and pulse-edge detection */
const int PULSE_THRESHOLD = 150; /* Pulse detected if read above this value */
const int BS_UNCERTAINTY = 10;   /* Baseline uncertainty */

/** @brief Maximum number of waveforms to process from input root file */
const int MAX_NUM_ENTRIES = 10000;


void beccaTest(
    const char *inputFilePath    = "/home/manoja450/run15731_processed_v5.root",  
    const char *inputFileName    = "run15731_processed_v5.root",
    const char *outputFilePath   = "",  
    const char *outputFileName   = "PMTWaveformAnalysis",
    const char *outputStatsName  = "PMTAnalysisStats",
    double integralToPE          = 163.43,
    double amplitudeToPE         = 60.33)
{
  // Read input root file of raw PMT waveform data
  TString inputFile;
  if (strcmp(inputFilePath, "") == 0) {
    inputFile.Form("%s.root", inputFileName);
  } else {
    inputFile.Form("%s/%s.root", inputFilePath, inputFileName);
  }
  
  TFile *fileIn  = new TFile(inputFile, "READ");

  // Grab TTree containing histograms of all raw waveforms
  TTree *T = (TTree *)fileIn->Get("T");

  // Set up variables to contain info from tree branches
  TH1F *h = new TH1F(); /* Histogram of current raw waveform */
  float baseline = 0; /* Baseline LSB of current raw waveform */

  // Set variable addresses to correspond to the Tree branches
  T->SetBranchAddress("trace0", &h);
  T->SetBranchAddress("bl", &baseline);

  // Create output root file of processed waveform data
  TString outputFile;
  if (strcmp(outputFilePath, "") == 0) {
    outputFile.Form("%s.root", outputFileName);
  } else {
    outputFile.Form("%s/%s.root", outputFilePath, outputFileName);
  }
  TFile *fileOut = new TFile(outputFile, "RECREATE");
  TTree *michelTree = new TTree("michelTree", "michelTree");
  michelTree->SetDirectory(fileOut);

  // Data to keep track of for each michelTree entry
  int *br_entry;  /* Entry # from inputFile TTree T */
  double *br_e1;  /* Energy of first pulse (photo-electrons) */
  double *br_p1;  /* Peak (max amplitude) of first pulse (photo-electrons) */
  double *br_t1;  /* Start time (10% peak) of first pulse (nanoseconds) */
  double *br_d1;  /* Duration of first pulse (nanoseconds) */
  double *br_e2;  /* Energy of second pulse (photo-electrons) */
  double *br_p2;  /* Peak (max amplitude) of second pulse (photo-electrons) */
  double *br_t2;  /* Start time (10% peak) of second pulse (nanoseconds) */
  double *br_d2;  /* Duration of second pulse (nanoseconds) */
  double *br_dt;  /* Time separation between pulse onsets (nanoseconds) */
  bool *br_issue; /* Flag to keep track of unusual michelTree entries */

  michelTree->Branch("entry", &br_entry, "entry/I");
  michelTree->Branch("e1", &br_e1, "e1/d");
  michelTree->Branch("p1", &br_p1, "p1/d");
  michelTree->Branch("t1", &br_t1, "t1/d");
  michelTree->Branch("d1", &br_d1, "d1/d");
  michelTree->Branch("e2", &br_e2, "e2/d");
  michelTree->Branch("p2", &br_p2, "p2/d");
  michelTree->Branch("t2", &br_t2, "t2/d");
  michelTree->Branch("d2", &br_d2, "d2/d");
  michelTree->Branch("dt", &br_dt, "dt/d");
  michelTree->Branch("issue", &br_issue, "issue/O");

  // Setup a vector to contain the waveform info
  std::map<int, int> numPulses;

  // Get statistics for up to 1 million entries from fileIn TTree T
  int numEntries = std::min((int)T->GetEntries(), MAX_NUM_ENTRIES);

  // Start looping over entries
  for (int iEnt = 0; iEnt < numEntries; iEnt++){
    std::cout << "Processing event " << iEnt+1 << " of " << numEntries << "\n";
    // Grab an entry from the tree
    // Sets variables from above to values from this entry
    T->GetEntry(iEnt);

    // Create vector to hold info of all pulses detected
    std::vector<struct pulse> pulses;

    // Digitizer cannot read above this so flag entries w/ p1,p2 > maxPeak
    double maxPeak = 15776 / amplitudeToPE;

    // Create variables to hold info of each current pulse
    bool onPulse = false, onLastPulseTail = false;
    int thresholdBin = 0, peakBin = 0;
    float tailWindow = 200;
    float peak = 0.;
    float pulseEnergy = 0.;

    std::cout << h -> GetNbinsX() << "\n";
    for (int i = 0; i < h->GetNbinsX(); i++){
      // Subtract the baseline of the waveform
      h->SetBinContent(i, h->GetBinContent(i) - baseline);
      h->SetBinError(i, 1.);
    }
    
    std::cout << "Waveform content:\t";
    for (int i = 0; i < h->GetNbinsX(); i++){
      float iBinContent = h->GetBinContent(i);
      std::cout << iBinContent << "\t";

      if (pulses.size() > 0) {
        onLastPulseTail = i*2 - pulses.back().start < tailWindow;
      }

      // Find pulse
      if (!onPulse && !onLastPulseTail && iBinContent > PULSE_THRESHOLD) {
        onPulse = true;
        thresholdBin = i;
        peakBin = i;
        peak = iBinContent;
        pulseEnergy += iBinContent;
      } 
      // Pulse found. Find pulse duration & energy
      else if (onPulse) { 
        // Accumlate energy of pulse after threshold bin
        pulseEnergy += iBinContent;

        // Update pulse's peak
        if (peak < iBinContent) {
          peak = iBinContent;
          peakBin = i;
        }
        // Search for end of pulse (falls below noiselevel)
        // Assumes no pulse pileup
        if (iBinContent < BS_UNCERTAINTY) {
          // Create pulse info
          struct pulse p;
          p.start = thresholdBin*2.;
          p.peak = peak / amplitudeToPE;
          // Record end of pulse
          p.end = i*2.;
          // Accumlate energy of pulse before threshold bin
          for (int j = peakBin-1; BS_UNCERTAINTY < h->GetBinContent(j); j--) 
          {
            if (j < thresholdBin) {
              pulseEnergy += h->GetBinContent(j);
            }
            // Record start of pulse (10% of peak)
            if (h -> GetBinContent(j) > peak*0.1) {
              p.start = j*2.;
            }
          }
          // Record energy of pulse
          p.energy = pulseEnergy / integralToPE;
          // Add pulse info to vector of pulses
          pulses.push_back(p);
          // Clear current pulse variables to look for new pulse
          peak = 0.;
          peakBin = 0;
          pulseEnergy = 0.;
          thresholdBin = 0;
          onPulse = false;
        }
      }
    }

    if (pulses.size() > 0){
      if (numPulses[pulses.size()] > 0){
	      numPulses[pulses.size()] += 1;
      } else{
	      numPulses[pulses.size()] = 1;
      }
    }

    // We know the first pulse is interesting; 
    //   don't care about dt between secondary peaks
    if (pulses.size() > 1){
      int entry = iEnt;
      bool issue = false;
      for (int j = 1; j < pulses.size(); j++){
        double e1 = pulses.front().energy;
        double p1 = pulses.front().peak;
        double t1 = pulses.front().start;
        double d1 = pulses.front().end - t1;
        double e2 = pulses[j].energy;
        double p2 = pulses[j].peak;
        double t2 = pulses[j].start;
        double d2 = pulses[j].end - t2;
        double dt = t2 - t1;
        // Potential issues, any xi < 0 or e2 > e1
        if (dt < 0 || e1 < 0 || e2 < 0 || e1 < e2 || 
            p1 > maxPeak || p2 > maxPeak) {
          issue = true;
        }

        michelTree->SetBranchAddress("entry", &entry);
        michelTree->SetBranchAddress("e1", &e1);
        michelTree->SetBranchAddress("p1", &p1);
        michelTree->SetBranchAddress("t1", &t1);
        michelTree->SetBranchAddress("d1", &d1);
        michelTree->SetBranchAddress("e2", &e2);
        michelTree->SetBranchAddress("p2", &p2);
        michelTree->SetBranchAddress("t2", &t2);
        michelTree->SetBranchAddress("d2", &d2);
        michelTree->SetBranchAddress("dt", &dt);
        michelTree->SetBranchAddress("issue", &issue);

        michelTree-> Fill();
      }
    }

    std::cout << iEnt << "\t" << pulses.size() << "\n";
    pulses.clear();
  }
  michelTree->Write();

  std::cout << "\n";

  // Create text file & write how many entries from input root file
  //   had a certain amount of pulses
  TString outputStatsFile;
  if (strcmp(outputFilePath, "") == 0) {
    outputStatsFile.Form("%s.txt", outputStatsName);
  } else {
    outputStatsFile.Form("%s/%s.txt", outputFilePath, outputStatsName);
  }
  ofstream outStream (outputStatsFile.Data());

  std::map<int, int>::iterator it;
  for (it = numPulses.begin(); it != numPulses.end(); it++){
    outStream << "Entries with " << it->first 
              << " pulses: " << it->second << "\n";
  }
  outStream << "\n";

  numPulses.clear();

  // Look for Recorded MichelTrees with issues & write to text file
  int entry;
  double dt, e1, e2, p1, p2;
  bool issue;
  michelTree->SetBranchAddress("entry", &entry);
  michelTree->SetBranchAddress("e1", &e1);
  michelTree->SetBranchAddress("e2", &e2);
  michelTree->SetBranchAddress("p1", &p1);
  michelTree->SetBranchAddress("p2", &p2);
  michelTree->SetBranchAddress("dt", &dt);
  michelTree->SetBranchAddress("issue", &issue);
  outStream << "Recorded MichelTrees with Irregular Values\n";
  outStream << "entry\t\tdt\t\te1\t\te2\t\tp1\t\tp2\n";
  for (int i = 0; i < michelTree -> GetEntries(); i++) {
    michelTree -> GetEntry(i);
    if (issue) {
      outStream << entry << "\t\t" << dt << "\t\t" << e1 << "\t\t";
      outStream << e2 <<"\t\t" << p1 << "\t\t" << p2 << "\n";
    }
  }
  outStream.close();
  
  // Delete all pointers called with "new" to avoid memory leaks
  delete michelTree;
  fileOut->Close();
  
  delete h;
  delete T;
  fileIn->Close();
}
