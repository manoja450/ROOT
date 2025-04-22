#include <iostream>
#include <TFile.h>
#include <TTree.h>
#include <TH1F.h>
#include <TCanvas.h>
#include <TF1.h>
#include <TPaveStats.h>
#include <TLatex.h>
#include <TSystem.h>
#include <TStyle.h>
#include <TROOT.h>
#include <vector>
#include <algorithm>
#include <cmath>

using namespace std;

// Define the fitting function
Double_t SPEfit(Double_t *x, Double_t *par) {
    Double_t A0 = par[0], mu0 = par[1], sigma0 = par[2];
    Double_t A1 = par[3], mu1 = par[4], sigma1 = par[5];
    Double_t A2 = par[6], A3 = par[7];

    Double_t term1 = A0 * exp(-0.5 * pow((x[0] - mu0) / sigma0, 2));
    Double_t term2 = A1 * exp(-0.5 * pow((x[0] - mu1) / sigma1, 2));
    Double_t term3 = A2 * exp(-0.5 * pow((x[0] - sqrt(2) * mu1)
                         / sqrt(2*sigma1*sigma1 - sigma0*sigma0), 2));
    Double_t term4 = A3 * exp(-0.5 * pow((x[0] - sqrt(3) * mu1)
                         / sqrt(3*sigma1*sigma1 - 2*sigma0*sigma0), 2));
    return term1 + term2 + term3 + term4;
}

void processLowLightEvents(const char *fileName) {
    // Create output directory
    gSystem->mkdir("plots", kTRUE);

    // Open the ROOT file
    TFile *file = TFile::Open(fileName);
    if (!file || file->IsZombie()) {
        cerr << "Error opening file: " << fileName << endl;
        return;
    }

    // Access the TTree
    TTree *tree = (TTree*)file->Get("tree");
    if (!tree) {
        cerr << "Error accessing TTree 'tree'!" << endl;
        file->Close();
        return;
    }

    // Branch variables
    Short_t adcVal[23][45];
    Double_t area[23];
    Int_t triggerBits;
    tree->SetBranchAddress("adcVal", adcVal);
    tree->SetBranchAddress("area", area);
    tree->SetBranchAddress("triggerBits", &triggerBits);

    Long64_t nEntries = tree->GetEntries();

    // Prepare histograms
    TH1F *histArea[12];
    for (int i = 0; i < 12; i++) {
        histArea[i] = new TH1F(Form("PMT%d_Area", i+1),
                               Form("; Area; Events per 3 ADCs", i+1),
                               150, -50, 400);
        histArea[i]->SetLineColor(kRed);
        histArea[i]->GetXaxis()->SetLabelFont(42);
        histArea[i]->GetYaxis()->SetLabelFont(42);
        histArea[i]->GetXaxis()->SetTitleFont(42);
        histArea[i]->GetYaxis()->SetTitleFont(42);
    }

    int pmtChannelMap[12] = {0,10,7,2,6,3,8,9,11,4,5,1};

    // Fill histograms
    for (Long64_t ev = 0; ev < nEntries; ++ev) {
        tree->GetEntry(ev);
        if (triggerBits == 16) {
            for (int p = 0; p < 12; ++p) {
                histArea[p]->Fill(area[pmtChannelMap[p]]);
            }
        }
    }

    // (1) Draw individual histograms as before…
    TCanvas *canvas = new TCanvas("canvas","PMT Energy Distributions",1200,800);
    canvas->SetLeftMargin(0.15);
    canvas->SetRightMargin(0.05);
    canvas->SetBottomMargin(0.15);
    canvas->SetTopMargin(0.05);
    for (int i = 0; i < 12; ++i) {
        canvas->Clear();
        histArea[i]->GetXaxis()->SetTitleSize(0.05);
        histArea[i]->GetYaxis()->SetTitleSize(0.05);
        histArea[i]->GetXaxis()->SetLabelSize(0.04);
        histArea[i]->GetYaxis()->SetLabelSize(0.04);

        TF1 *f = new TF1("f", SPEfit, -50, 400, 8);
        f->SetParameters(1000,0,10,1000,50,10,500,500);
        f->SetLineColor(kBlue);
        f->SetParNames("A0","#mu_{0}","#sigma_{0}","A1","#mu_{1}","#sigma_{1}","A2","A3");

        histArea[i]->Fit(f,"R");
        histArea[i]->Draw();
        f->Draw("same");

        TLatex tex;
        tex.SetTextFont(42);
        tex.SetTextSize(0.06);
        tex.SetTextAlign(22);
        tex.SetNDC();
        tex.DrawLatex(0.5, 0.92, Form("PMT %d", i+1));

        gPad->Update();
        if (auto stats = (TPaveStats*)histArea[i]->FindObject("stats")) {
            stats->SetX1NDC(0.65); stats->SetY1NDC(0.65);
            stats->SetX2NDC(0.95); stats->SetY2NDC(0.95);
            stats->SetTextFont(42);
            stats->SetTextSize(0.03);
            stats->SetOptStat(10);
            stats->SetOptFit(111);
            stats->SetName("");
        }

        canvas->SaveAs(Form("plots/PMT%d_Energy_Distribution.png", i+1));
        delete f;
    }
    delete canvas;

    // (2) Create the master canvas with double resolution and vector exports
    //    → Sharp PDF text + high‐res PNG
    gStyle->SetTextFont(42);
    gStyle->SetLabelFont(42, "XYZ");
    gStyle->SetTitleFont(42, "XYZ");

    // Increase export resolution by factor 2
    gStyle->SetImageScaling(2.0);

    TCanvas *master = new TCanvas("MasterCanvas",
                                  "Combined PMT Energy Distributions",
                                  2400, 1600);
    master->Divide(3,4,0,0);
    int layout[4][3] = {
        {0,10,7},
        {2,6,3},
        {8,9,11},
        {4,5,1}
    };

    for (int r = 0; r < 4; ++r) {
      for (int c = 0; c < 3; ++c) {
        int pad = r*3 + c + 1;
        master->cd(pad);

        int idx = layout[r][c];
        histArea[idx]->GetXaxis()->SetTitleSize(0.07);
        histArea[idx]->GetYaxis()->SetTitleSize(0.09);
        histArea[idx]->GetXaxis()->SetLabelSize(0.04);
        histArea[idx]->GetYaxis()->SetLabelSize(0.04);
        histArea[idx]->GetYaxis()->SetTitle("Events per 3 ADCs");
        histArea[idx]->GetYaxis()->SetTitleOffset(0.8);
        histArea[idx]->GetXaxis()->SetTitle("Area");
    

        gPad->SetLeftMargin(0.15);
        gPad->SetRightMargin(0.12);
        gPad->SetBottomMargin(0.15);
        gPad->SetTopMargin(0.10);

        TF1 *f2 = new TF1("f2", SPEfit, -50, 400, 8);
        f2->SetParameters(100,0,10,100,50,10,50,50);
        f2->SetLineColor(kBlue);
        f2->SetParNames("A0","#mu_{0}","#sigma_{0}",
                        "A1","#mu_{1}","#sigma_{1}","A2","A3");

        histArea[idx]->Fit(f2,"R");
        histArea[idx]->Draw();
        f2->Draw("same");

        TLatex tex2;
        tex2.SetTextFont(42);
        tex2.SetTextSize(0.14);
        tex2.SetTextAlign(22);
        tex2.SetNDC();
        tex2.DrawLatex(0.5, 0.92, Form("PMT %d", idx+1));

        gPad->Update();
        if (auto s2 = (TPaveStats*)histArea[idx]->FindObject("stats")) {
          s2->SetX1NDC(0.65); s2->SetY1NDC(0.65);
          s2->SetX2NDC(0.95); s2->SetY2NDC(0.95);
          s2->SetTextFont(42);
          s2->SetTextSize(0.03);
          s2->SetOptStat(10);
          s2->SetOptFit(111);
          s2->SetName("");
        }

        delete f2;
      }
    }

    // Export sharp text
    master->SaveAs("plots/Combined_PMT_Energy_Distributions.pdf");
    master->SaveAs("plots/Combined_PMT_Energy_Distributions.png");

    // Restore default scaling
    gStyle->SetImageScaling(1.0);

    // Cleanup
    for (int i = 0; i < 12; ++i) delete histArea[i];
    delete master;
    file->Close();

    cout << "Plots saved under 'plots/'. "
         << "Vector text in PDF + high‐res PNG available." << endl;
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        cerr << "Usage: " << argv[0] << " <root_file>" << endl;
        return 1;
    }
    processLowLightEvents(argv[1]);
    return 0;
}
