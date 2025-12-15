#include "FWCore/Framework/interface/one/EDAnalyzer.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/MakerMacros.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/ServiceRegistry/interface/Service.h"
#include "CommonTools/UtilAlgos/interface/TFileService.h"

#include "DataFormats/TrackerRecHit2D/interface/MTDTrackingRecHit.h"
#include "DataFormats/FTLRecHit/interface/FTLMergedCluster.h"
#include <iostream>
#include "TH1F.h"
#include "TH2F.h"

class MTDTrackingRecHitDump : public edm::one::EDAnalyzer<edm::one::SharedResources> {
public:
  explicit MTDTrackingRecHitDump(const edm::ParameterSet& cfg);
  ~MTDTrackingRecHitDump() override = default;

  void analyze(const edm::Event& e, const edm::EventSetup&) override;

private:
  const edm::InputTag src_;
  const edm::EDGetTokenT<MTDTrackingDetSetVector> token_;
  const bool onlySingleClusters_;

  // Histograms
  TH1F* h_nHits_;
  TH1F* h_time_;
  TH1F* h_timeErr_;
  TH1F* h_energy_;
  TH1F* h_posX_;
  TH1F* h_posY_;
  TH1F* h_errXX_;
  TH2F* h_posXY_;
  TH1F* h_detId_;

  TH1F* h_time_single_;
  TH1F* h_timeErr_single_;
  TH1F* h_energy_single_;
  TH1F* h_posX_single_;
  TH1F* h_posY_single_;
  TH1F* h_errXX_single_;
  TH2F* h_posXY_single_;

  TH1F* h_time_multi_;
  TH1F* h_timeErr_multi_;
  TH1F* h_energy_multi_;
  TH1F* h_posX_multi_;
  TH1F* h_posY_multi_;
  TH1F* h_errXX_multi_;
  TH2F* h_posXY_multi_;
};

MTDTrackingRecHitDump::MTDTrackingRecHitDump(const edm::ParameterSet& cfg)
    : src_(cfg.getParameter<edm::InputTag>("src")), token_(consumes<MTDTrackingDetSetVector>(src_)), onlySingleClusters_(cfg.getParameter<bool>("onlySingleClusters")) {
  usesResource(TFileService::kSharedResource);
  edm::Service<TFileService> fs;

  // Book histograms
  h_nHits_ = fs->make<TH1F>("nHits", "Number of MTD hits per event;N_{hits};Events", 100, 0, 1000);
  h_time_ = fs->make<TH1F>("time", "Hit time;Time [ns];Hits", 100, 0, 25);
  h_timeErr_ = fs->make<TH1F>("timeErr", "Hit time error;Time Error [ns];Hits", 100, 0, 0.1);
  h_energy_ = fs->make<TH1F>("energy", "Hit energy;Energy [MeV];Hits", 100, 0, 35);
  h_posX_ = fs->make<TH1F>("posX", "Hit local position X;x [cm];Hits", 100, -10, 10);
  h_posY_ = fs->make<TH1F>("posY", "Hit local position Y;y [cm];Hits", 100, -10, 10);
  h_errXX_ = fs->make<TH1F>("errXX", "Hit position error XX;#sigma_{xx} [cm^{2}];Hits", 100, 0, 0.1);
  h_posXY_ = fs->make<TH2F>("posXY", "Hit local position XY;x [cm];y [cm];Hits", 100, -10, 10, 100, -10, 10);
  //h_detId_ = fs->make<TH1F>("detId", "Detector ID;DetId;Hits", 100, 0, 1e9);

  h_time_single_ = fs->make<TH1F>("time_single", "Hit time (single-cluster);Time [ns];Hits", 100, 0, 25);
  h_timeErr_single_ = fs->make<TH1F>("timeErr_single", "Hit time error (single-cluster);Time Error [ns];Hits", 100, 0, 0.1);
  h_energy_single_ = fs->make<TH1F>("energy_single", "Hit energy (single-cluster);Energy [MeV];Hits", 100, 0, 35);
  h_posX_single_ = fs->make<TH1F>("posX_single", "Hit local position X (single-cluster);x [cm];Hits", 100, -10, 10);
  h_posY_single_ = fs->make<TH1F>("posY_single", "Hit local position Y (single-cluster);y [cm];Hits", 100, -10, 10);
  h_errXX_single_ = fs->make<TH1F>("errXX_single", "Hit position error XX (single-cluster);#sigma_{xx} [cm^{2}];Hits", 100, 0, 0.1);
  h_posXY_single_ = fs->make<TH2F>("posXY_single", "Hit local position XY (single-cluster);x [cm];y [cm];Hits", 100, -10, 10, 100, -10, 10);

  h_time_multi_ = fs->make<TH1F>("time_multi", "Hit time (multi-cluster);Time [ns];Hits", 100, 0, 25);
  h_timeErr_multi_ = fs->make<TH1F>("timeErr_multi", "Hit time error (multi-cluster);Time Error [ns];Hits", 100, 0, 0.1);
  h_energy_multi_ = fs->make<TH1F>("energy_multi", "Hit energy (multi-cluster);Energy [MeV];Hits", 100, 0, 35);
  h_posX_multi_ = fs->make<TH1F>("posX_multi", "Hit local position X (multi-cluster);x [cm];Hits", 100, -10, 10);
  h_posY_multi_ = fs->make<TH1F>("posY_multi", "Hit local position Y (multi-cluster);y [cm];Hits", 100, -10, 10);     
  h_errXX_multi_ = fs->make<TH1F>("errXX_multi", "Hit position error XX (multi-cluster);#sigma_{xx} [cm^{2}];Hits", 100, 0, 0.1);
  h_posXY_multi_ = fs->make<TH2F>("posXY_multi", "Hit local position XY (multi-cluster);x [cm];y [cm];Hits", 100, -10, 10, 100, -10, 10);
}

void MTDTrackingRecHitDump::analyze(const edm::Event& e, const edm::EventSetup&) {
  edm::Handle<MTDTrackingDetSetVector> h;
  e.getByToken(token_, h);
  if (!h.isValid()) {
    std::cout << "MTDTrackingRecHitDump: no collection " << src_ << " in this event\n";
    return;
  }
  
  size_t total = 0;
  size_t single_cluster_count = 0;
  size_t multi_cluster_count = 0;

  for (const auto& detset : *h) {
    const unsigned int detId = detset.detId();
    for (const auto& hit : detset) {

      auto mergedClusterRef = hit.omniCluster().cluster_merged_mtd(); // get mc ref
      
      // Count cluster types
      size_t nClusters = mergedClusterRef.isNonnull() ? mergedClusterRef->nClusters() : 0;
      std::string clusterType = "";
      if (nClusters == 1) {
        single_cluster_count++;
        clusterType = "[SINGLE]";
      } else if (nClusters > 1) {
        multi_cluster_count++;
        clusterType = "[MULTI]";
      }
        
      ++total;
      auto lp = hit.localPosition();
      auto le = hit.localPositionError();
      
      // Print to console
      std::cout << clusterType << "DetId " << detId << "  nClusters=" << nClusters
                << "  pos = (" << lp.x() << "," << lp.y() << ")"
                << "  err_xx=" << le.xx() << "  time=" << hit.time()
                << "  timeErr=" << hit.timeError() << "  energy=" << hit.energy() << "\n";
      
      // Fill histograms
      h_time_->Fill(hit.time());
      h_timeErr_->Fill(hit.timeError());
      h_energy_->Fill(hit.energy());
      h_posX_->Fill(lp.x());
      h_posY_->Fill(lp.y());
      h_errXX_->Fill(le.xx());
      h_posXY_->Fill(lp.x(), lp.y());
      //h_detId_->Fill(detId);

      if (clusterType == "[MULTI]") {
        h_time_multi_->Fill(hit.time());
        h_timeErr_multi_->Fill(hit.timeError());
        h_energy_multi_->Fill(hit.energy());
        h_posX_multi_->Fill(lp.x());
        h_posY_multi_->Fill(lp.y());
        h_errXX_multi_->Fill(le.xx());
        h_posXY_multi_->Fill(lp.x(), lp.y());
      } else if (clusterType == "[SINGLE]") {
        h_time_single_->Fill(hit.time());
        h_timeErr_single_->Fill(hit.timeError());
        h_energy_single_->Fill(hit.energy());
        h_posX_single_->Fill(lp.x());
        h_posY_single_->Fill(lp.y());
        h_errXX_single_->Fill(le.xx());
        h_posXY_single_->Fill(lp.x(), lp.y());
      }
    }
  }

  h_nHits_->Fill(total);
  std::cout << "MTDTrackingRecHitDump: total hits processed = " << total
            << "  (Single-cluster: " << single_cluster_count 
            << ", Multi-cluster: " << multi_cluster_count << ")\n";
}

DEFINE_FWK_MODULE(MTDTrackingRecHitDump);