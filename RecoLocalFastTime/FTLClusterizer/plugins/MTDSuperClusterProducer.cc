#include "FWCore/Framework/interface/stream/EDProducer.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/EventSetup.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/ParameterSet/interface/ConfigurationDescriptions.h"
#include "FWCore/ParameterSet/interface/ParameterSetDescription.h"
#include "FWCore/Framework/interface/MakerMacros.h"
#include "FWCore/MessageLogger/interface/MessageLogger.h"
#include "FWCore/Utilities/interface/EDGetToken.h"

#include "DataFormats/FTLRecHit/interface/FTLSuperCluster.h"
#include "DataFormats/FTLRecHit/interface/FTLClusterCollections.h"
#include "DataFormats/Common/interface/Handle.h"
#include "DataFormats/ForwardDetId/interface/BTLDetId.h"

#include "Geometry/Records/interface/MTDDigiGeometryRecord.h"
#include "Geometry/Records/interface/MTDTopologyRcd.h"
#include "Geometry/MTDGeometryBuilder/interface/MTDGeometry.h"
#include "Geometry/MTDGeometryBuilder/interface/MTDTopology.h"

#include <iostream>
#include <vector>
#include <map>
#include <set>
#include <cmath>
#include <limits>

class MTDSuperClusterProducer : public edm::stream::EDProducer<> {
public:
    explicit MTDSuperClusterProducer(const edm::ParameterSet& conf);
    ~MTDSuperClusterProducer() override = default;
    
    void produce(edm::Event& e, const edm::EventSetup& es) override;
    static void fillDescriptions(edm::ConfigurationDescriptions& descriptions); // obaveznooooo tribaaaa

private:
    edm::EDGetTokenT<FTLClusterCollection> btlClustersToken_;
    std::string btlSuperClusterInstance_;

    double timeThreshold_;  // 10 sigma timing window
    double energyThreshold_;   
    
    edm::ESGetToken<MTDGeometry, MTDDigiGeometryRecord> mtdgeoToken_;
    edm::ESGetToken<MTDTopology, MTDTopologyRcd> mtdtopoToken_;    

    bool areTimingCompatible(const FTLCluster* c1, const FTLCluster* c2);
    FTLSuperCluster mergeClusters(const std::vector<const FTLCluster*>& clusters, const BTLDetId& seedId);
};

MTDSuperClusterProducer::MTDSuperClusterProducer(const edm::ParameterSet& conf) 
    : btlClustersToken_(consumes<FTLClusterCollection>(conf.getParameter<edm::InputTag>("srcBarrel"))),
      btlSuperClusterInstance_(conf.getParameter<std::string>("BarrelSuperClusterName")),
      timeThreshold_(conf.getParameter<double>("timeThreshold")),
      energyThreshold_(conf.getParameter<double>("energyThreshold")),
      mtdgeoToken_(esConsumes<MTDGeometry, MTDDigiGeometryRecord>()),
      mtdtopoToken_(esConsumes<MTDTopology, MTDTopologyRcd>()) {
    
    std::cout << "=== MTDSuperClusterProducer: Constructor called ===" << std::endl;
    
    btlClustersToken_ = consumes<FTLClusterCollection>(conf.getParameter<edm::InputTag>("srcBarrel"));
    btlSuperClusterInstance_ = conf.getParameter<std::string>("BarrelSuperClusterName");
    
    produces<FTLSuperClusterCollection>(btlSuperClusterInstance_);

    std::cout << "MTDSuperClusterProducer: Time threshold = " << timeThreshold_ << " sigma" << std::endl;
    std::cout << "MTDSuperClusterProducer: Energy threshold = " << energyThreshold_ << " MeV" << std::endl;
    std::cout << "=== MTDSuperClusterProducer: Constructor finished ===" << std::endl;
}

bool MTDSuperClusterProducer::areTimingCompatible(const FTLCluster* c1, const FTLCluster* c2) {
    double timeDiff = std::abs(c1->time() - c2->time());
    double timeError1 = c1->timeError();
    double timeError2 = c2->timeError();
    double combinedError = std::sqrt(timeError1*timeError1 + timeError2*timeError2);
    
    bool compatible = timeDiff < (timeThreshold_ * combinedError);

    std::cout << "    Timing check: dt=" << timeDiff << " ns, combined_error=" 
              << combinedError << " ns, limit=" << (timeThreshold_ * combinedError) 
              << " ns -> " << (compatible ? "COMPATIBLE" : "INCOMPATIBLE") << std::endl;
    
    return compatible;
}

FTLSuperCluster MTDSuperClusterProducer::mergeClusters(const std::vector<const FTLCluster*>& clusters, const BTLDetId& seedId) {
    float totalEnergy = 0;
    float weightedTime = 0;
    float weightedTimeError2 = 0;
    float weightedX = 0;
    float weightedY = 0;
    std::vector<DetId> clusterIds;

    for (const auto* cluster : clusters) {
        float energy = cluster->energy();
        
        totalEnergy += energy;
        weightedTime += energy * cluster->time();
        weightedTimeError2 += energy * energy * cluster->timeError() * cluster->timeError();
        weightedX += energy * cluster->x();
        weightedY += energy * cluster->y();
        clusterIds.push_back(cluster->id());
    }

    float avgTime = weightedTime / totalEnergy;
    float avgTimeError = std::sqrt(weightedTimeError2) / totalEnergy;
    float avgX = weightedX / totalEnergy;
    float avgY = weightedY / totalEnergy;

    return FTLSuperCluster(seedId, totalEnergy, avgTime, avgTimeError, avgX, avgY, clusterIds);
}

void MTDSuperClusterProducer::produce(edm::Event& e, const edm::EventSetup& es) {
    std::cout << "=== MTDSuperClusterProducer: Event " << e.id().event() << " ===" << std::endl;
    
    // Get topology for navigation
    auto topologyHandle = es.getTransientHandle(mtdtopoToken_);
    const MTDTopology* topology = topologyHandle.product();
    
    edm::Handle<FTLClusterCollection> btlClustersHandle;
    e.getByToken(btlClustersToken_, btlClustersHandle);
    
    auto btlOutput = std::make_unique<FTLSuperClusterCollection>();
    
    if (!btlClustersHandle.isValid() || btlClustersHandle->size() == 0) {
        std::cout << "No valid BTL clusters found in event" << std::endl;
        e.put(std::move(btlOutput), btlSuperClusterInstance_);
        return;
    }
    
    // Create cluster map for fast lookup
    std::map<BTLDetId, const FTLCluster*> clusterMap;
    for (const auto& detSet : *btlClustersHandle) {
        for (const auto& cluster : detSet) {
            if (cluster.energy() >= energyThreshold_) { 
                clusterMap[cluster.id()] = &cluster;
            }
        }
    }
    
    std::cout << "Found " << clusterMap.size() << " BTL clusters above energy threshold" << std::endl;
    
    // Track processed clusters
    std::set<const FTLCluster*> processedClusters;
    
    // Process clusters with full merging logic
    for (const auto& detSet : *btlClustersHandle) {
        for (const auto& cluster : detSet) {
            if (cluster.energy() < energyThreshold_ || processedClusters.count(&cluster)) continue;
            
            BTLDetId cluId = cluster.id();
            std::cout << "Processing cluster DetId " << cluId.rawId() 
                      << " with energy " << cluster.energy() << " MeV" << std::endl;
            
            // Start with current cluster
            std::vector<const FTLCluster*> superClusterClusters = {&cluster};
            processedClusters.insert(&cluster);
            
            // Check for edge hits in current cluster
            bool edgeHitIn0 = false;
            bool edgeHitIn15 = false;
            
            for (int i = 0; i < cluster.size(); ++i) {
                auto hit = cluster.hit(i);
                int hit_col = hit.y();
                if (hit_col == 0) {
                    edgeHitIn0 = true;
                } else if (hit_col == 15) {
                    edgeHitIn15 = true;
                }
            }
            
            bool hasEdgeHitCurrent = edgeHitIn0 || edgeHitIn15;
            std::cout << "  Edge hits: col0=" << edgeHitIn0 << ", col15=" << edgeHitIn15 << std::endl;
            
            // Get topology indices
            std::pair<uint32_t, uint32_t> indices = topology->btlIndex(cluId.rawId());
            uint32_t iphi = indices.first;
            uint32_t ieta = indices.second;
            std::cout << "  BTL indices: iphi=" << iphi << ", ieta=" << ieta << std::endl;
            
            // ETA DIRECTION MERGING
            if (hasEdgeHitCurrent && iphi != std::numeric_limits<uint32_t>::max() && ieta != std::numeric_limits<uint32_t>::max()) {
                
                std::vector<int> etaOffsets = {1, -1};
                for (int etaOffset : etaOffsets) {
                    uint32_t adjDetIdRaw = topology->btlidFromIndex(iphi, ieta + etaOffset);
                    if (adjDetIdRaw == 0) continue;
                    
                    BTLDetId adjDetId(adjDetIdRaw);
                    auto it = clusterMap.find(adjDetId);
                    if (it == clusterMap.end()) continue;
                    if (processedClusters.count(it->second)) continue;
                    
                    const FTLCluster* adjCluster = it->second;
                    std::cout << "  Found eta neighbor at " << adjDetId.rawId() << std::endl;
                    
                    // Check for opposite edge hit
                    bool hasOppositeEdgeHit = false;
                    for (int j = 0; j < adjCluster->size(); ++j) {
                        auto hit = adjCluster->hit(j);
                        int hit_col = hit.y();
                        if ((edgeHitIn0 && hit_col == 15) || (edgeHitIn15 && hit_col == 0)) {
                            hasOppositeEdgeHit = true;
                            break;
                        }
                    }
                    
                    if (hasOppositeEdgeHit && areTimingCompatible(&cluster, adjCluster)) {
                        std::cout << "  -> MERGING ETA neighbor: " << cluId.rawId() 
                                  << " with " << adjDetId.rawId() << std::endl;
                        superClusterClusters.push_back(adjCluster);
                        processedClusters.insert(adjCluster);
                    }
                }
            }
            
            // PHI DIRECTION MERGING
            if (iphi != std::numeric_limits<uint32_t>::max() && ieta != std::numeric_limits<uint32_t>::max()) {
                
                std::vector<int> phiOffsets = {1, -1};
                for (int phiOffset : phiOffsets) {
                    uint32_t adjDetIdRaw = topology->btlidFromIndex(iphi + phiOffset, ieta);
                    if (adjDetIdRaw == 0) continue;
                    
                    BTLDetId adjDetId(adjDetIdRaw);
                    auto it = clusterMap.find(adjDetId);
                    if (it == clusterMap.end()) continue;
                    if (processedClusters.count(it->second)) continue;
                    
                    const FTLCluster* adjCluster = it->second;
                    std::cout << "  Found phi neighbor at " << adjDetId.rawId() << std::endl;
                    
                    if (areTimingCompatible(&cluster, adjCluster)) {
                        std::cout << "  -> MERGING PHI neighbor: " << cluId.rawId() 
                                  << " with " << adjDetId.rawId() << std::endl;
                        superClusterClusters.push_back(adjCluster);
                        processedClusters.insert(adjCluster);
                    }
                }
            }
            
            // Create supercluster from merged clusters
            FTLSuperCluster superCluster = mergeClusters(superClusterClusters, cluId);
            btlOutput->push_back(superCluster);
            
            std::cout << "Created SuperCluster from " << superClusterClusters.size() 
                      << " clusters: E=" << superCluster.energy() 
                      << " MeV, t=" << superCluster.time() << " ns" << std::endl;
        }
    }
    
    std::cout << "About to put " << btlOutput->size() << " SuperClusters into event..." << std::endl;
    e.put(std::move(btlOutput), btlSuperClusterInstance_);
    std::cout << "=== Successfully put SuperClusters into event ===" << std::endl;
}
void MTDSuperClusterProducer::fillDescriptions(edm::ConfigurationDescriptions& descriptions) {
    edm::ParameterSetDescription desc;
    desc.add<edm::InputTag>("srcBarrel", edm::InputTag("mtdClusters", "FTLBarrel"));
    desc.add<std::string>("BarrelSuperClusterName", "FTLBarrel");    // Add parameter descriptions here if needed obaveznoooooooo
    desc.add<double>("timeThreshold", 10.0);
    desc.add<double>("energyThreshold", 1.0);  // MeV
    descriptions.add("MTDSuperClusterProducer", desc);
}

DEFINE_FWK_MODULE(MTDSuperClusterProducer);

