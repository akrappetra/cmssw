#ifndef Validation_MtdValidation_SuperClusterValidationExample_h
#define Validation_MtdValidation_SuperClusterValidationExample_h

#include "FWCore/Framework/interface/Frameworkfwd.h"
#include "FWCore/Framework/interface/one/EDAnalyzer.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/MakerMacros.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/ServiceRegistry/interface/Service.h"
#include "CommonTools/UtilAlgos/interface/TFileService.h"

#include "DataFormats/FTLRecHit/interface/FTLSuperCluster.h"
#include "DataFormats/FTLRecHit/interface/FTLClusterCollections.h"
#include "DataFormats/ForwardDetId/interface/BTLDetId.h"

#include "TH1F.h"
#include "TH2F.h"
#include "TTree.h"

class SuperClusterValidationExample : public edm::one::EDAnalyzer<edm::one::SharedResources> {
public:
    explicit SuperClusterValidationExample(const edm::ParameterSet&);
    ~SuperClusterValidationExample() = default;

private:
    void analyze(const edm::Event&, const edm::EventSetup&) override;
    void beginJob() override;
    void endJob() override;

    edm::EDGetTokenT<FTLSuperClusterCollection> superClustersToken_;
    edm::EDGetTokenT<FTLClusterCollection> clustersToken_;

    TH1F* h_sc_energy_;
    TH1F* h_sc_time_;
    TH1F* h_sc_timeError_;
    TH1F* h_sc_x_;
    TH1F* h_sc_y_;
    TH1F* h_sc_nClusters_;
    TH1F* h_cluster_energy_;
    TH1F* h_cluster_time_;
    
    TH2F* h_sc_energy_vs_time_;
    TH2F* h_sc_xy_;
    TH2F* h_sc_energy_vs_nClusters_;
    TH2F* h_merging_efficiency_;

    TTree* tree_;
    
    int evt_run_, evt_event_;
    int sc_n_;
    std::vector<float> sc_energy_, sc_time_, sc_timeError_, sc_x_, sc_y_;
    std::vector<int> sc_nClusters_;
    std::vector<uint32_t> sc_seedId_;
    std::vector<std::vector<uint32_t>> sc_clusterIds_;

    int cluster_n_;
    std::vector<float> cluster_energy_, cluster_time_, cluster_x_, cluster_y_;
    std::vector<uint32_t> cluster_detId_;
    std::vector<bool> cluster_inSuperCluster_;
};

#endif