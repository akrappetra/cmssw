#include "FWCore/Framework/interface/stream/EDProducer.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/EventSetup.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/ParameterSet/interface/ConfigurationDescriptions.h"
#include "FWCore/ParameterSet/interface/ParameterSetDescription.h"
#include "FWCore/Framework/interface/MakerMacros.h"
#include "FWCore/MessageLogger/interface/MessageLogger.h"
#include "FWCore/Utilities/interface/EDGetToken.h"

#include "DataFormats/FTLRecHit/interface/FTLMergedCluster.h"
#include "DataFormats/FTLRecHit/interface/FTLClusterCollections.h"
#include "DataFormats/Common/interface/Handle.h"
#include "DataFormats/ForwardDetId/interface/BTLDetId.h"
#include "DataFormats/ForwardDetId/interface/ETLDetId.h"

#include "Geometry/Records/interface/MTDDigiGeometryRecord.h"
#include "Geometry/Records/interface/MTDTopologyRcd.h"
#include "Geometry/MTDGeometryBuilder/interface/MTDGeometry.h"
#include "Geometry/MTDGeometryBuilder/interface/MTDTopology.h"
#include "Geometry/CommonDetUnit/interface/GeomDet.h"
#include "DataFormats/GeometryVector/interface/LocalPoint.h"
#include "DataFormats/GeometryVector/interface/GlobalPoint.h"
#include "DataFormats/GeometryCommonDetAlgo/interface/MeasurementPoint.h"
#include "Geometry/MTDGeometryBuilder/interface/RectangularMTDTopology.h"
#include "Geometry/MTDGeometryBuilder/interface/ProxyMTDTopology.h"

#include <iostream>
#include <vector>
#include <map>
#include <set>
#include <cmath>
#include <limits>

class MTDMergedClusterProducer : public edm::stream::EDProducer<> {
public:
    explicit MTDMergedClusterProducer(const edm::ParameterSet& conf);
    ~MTDMergedClusterProducer() override = default;
    
    void produce(edm::Event& e, const edm::EventSetup& es) override;
    static void fillDescriptions(edm::ConfigurationDescriptions& descriptions); 

private:
    edm::EDGetTokenT<FTLClusterCollection> btlClustersToken_;
    edm::EDGetTokenT<FTLClusterCollection> etlClustersToken_;
    std::string btlMergedClusterInstance_;
    std::string etlMergedClusterInstance_;

    double timeThreshold_;  // 10 sigma timing window?
    double energyThreshold_;   
    
    edm::ESGetToken<MTDGeometry, MTDDigiGeometryRecord> mtdgeoToken_;
    edm::ESGetToken<MTDTopology, MTDTopologyRcd> mtdtopoToken_;    

    bool areTimingCompatible(const FTLCluster* c1, const FTLCluster* c2);
    FTLMergedCluster mergeClusters(const std::vector<const FTLCluster*>& clusters, const DetId& seedId, const MTDGeometry& geom); 
};

MTDMergedClusterProducer::MTDMergedClusterProducer(const edm::ParameterSet& conf) 
    : btlClustersToken_(consumes<FTLClusterCollection>(conf.getParameter<edm::InputTag>("btlBarrel"))),
      etlClustersToken_(consumes<FTLClusterCollection>(conf.getParameter<edm::InputTag>("etlEndcap"))),
      btlMergedClusterInstance_(conf.getParameter<std::string>("btlMergedClusterInstance")),
      etlMergedClusterInstance_(conf.getParameter<std::string>("etlMergedClusterInstance")),
      timeThreshold_(conf.getParameter<double>("timeThreshold")),
      energyThreshold_(conf.getParameter<double>("energyThreshold")),
      mtdgeoToken_(esConsumes<MTDGeometry, MTDDigiGeometryRecord>()),
      mtdtopoToken_(esConsumes<MTDTopology, MTDTopologyRcd>()) {
    
    produces<FTLMergedClusterCollection>(btlMergedClusterInstance_); 
    produces<FTLMergedClusterCollection>(etlMergedClusterInstance_);

    std::cout << "MTDMergedClusterProducer: Time threshold = " << timeThreshold_ << " sigma" << std::endl;
    std::cout << "MTDMergedClusterProducer: Energy threshold = " << energyThreshold_ << " MeV" << std::endl;
    std::cout << "=== MTDMergedClusterProducer: Constructor finished ===" << std::endl;
}

bool MTDMergedClusterProducer::areTimingCompatible(const FTLCluster* c1, const FTLCluster* c2) {
    double timeDiff = std::abs(c1->time() - c2->time());
    double timeError1 = c1->timeError();
    double timeError2 = c2->timeError();
    double combinedError = std::sqrt(timeError1*timeError1 + timeError2*timeError2);
    
    bool compatible = timeDiff < (timeThreshold_ * combinedError);

    /*std::cout << "    Timing check: dt=" << timeDiff << " ns, combined_error=" 
              << combinedError << " ns, limit=" << (timeThreshold_ * combinedError) 
              << " ns -> " << (compatible ? "COMPATIBLE" : "INCOMPATIBLE") << std::endl;*/
    
    return compatible;
}

FTLMergedCluster MTDMergedClusterProducer::mergeClusters(const std::vector<const FTLCluster*>& clusters, const DetId& seedId, const MTDGeometry& geom) { 
    float totalEnergy = 0;
    float weightedTime = 0;
    float weightedTimeError2 = 0;
    //float weightedX = 0;
    //float weightedY = 0;
    // ok first in global, then convert in seed-local
    double weightedGlobalX = 0;
    double weightedGlobalY = 0;
    double weightedGlobalZ = 0;
    std::vector<DetId> clusterIds;

    // get primary cluster -- if only one, pick that detid, else pick the one with earliest time
    const FTLCluster* primary = clusters.front();
    if (clusters.size() > 1) {
        for (const auto* c : clusters) {
            if (c->time() < primary->time()) {
                primary = c;
            } else if (c->time() == primary->time() && c->energy() > primary->energy()) {
                primary = c;
            }
        }
    }

    if (clusters.size() == 1) {
        const FTLCluster* c = clusters.front();
        const GeomDet* det = geom.idToDetUnit(c->id());
        const GeomDet* seedDet = geom.idToDetUnit(seedId);
        GlobalPoint gp_local_to_global(0., 0., 0.), gp_roundtrip(0., 0., 0.);
        if (det) {
            const ProxyMTDTopology& topoproxy = static_cast<const ProxyMTDTopology&>(det->topology());
            const RectangularMTDTopology& topo = static_cast<const RectangularMTDTopology&>(topoproxy.specificTopology());
            
            float localX = c->getClusterPosX();
            float localY = 0.0f;
            
            if (c->getClusterErrorX() < 0.) {
                MeasurementPoint mp(c->x(), c->y());
                const LocalPoint localPos = topo.localPosition(mp);
                localX = localPos.x();
                localY = localPos.y();
            } else {
                localY = topo.localY(c->y());
            }
            
            const LocalPoint lp_in(localX, localY, 0.0f);
            gp_local_to_global = det->surface().toGlobal(lp_in);
        }
        if (seedDet) {
            const LocalPoint lp_rt = seedDet->surface().toLocal(gp_local_to_global);
            gp_roundtrip = GlobalPoint(lp_rt.x(), lp_rt.y(), lp_rt.z());
        }
        std::cout << "[DEBUG avgx] single cluster det = " << c->id().rawId()
                  << " cluster.x/y = (" << c->x() << ", " << c->y() << ")"
                  << " computed avgGlobal = (" << gp_local_to_global.x() << ", " << gp_local_to_global.y() << ")"
                  << " roundtrip local = " << "(" << gp_roundtrip.x() << ", " << gp_roundtrip.y() << ")"
                  << " seedid = " << seedId.rawId() << std::endl; // zajeb!

    }
    // primary first, then the rest
    clusterIds.reserve(clusters.size());
    clusterIds.push_back(primary->id());
    for (const auto* c : clusters) {
        if (c->id() != primary->id()) clusterIds.push_back(c->id());
    }

    for (const auto* cluster : clusters) {
        float energy = cluster->energy();
        
        totalEnergy += energy;
        weightedTime += energy * cluster->time();
        weightedTimeError2 += energy * energy * cluster->timeError() * cluster->timeError();
        
        // convert
        const GeomDet* det = geom.idToDetUnit(cluster->id());
        if (det) {
            const ProxyMTDTopology& topoproxy = static_cast<const ProxyMTDTopology&>(det->topology());
            const RectangularMTDTopology& topo = static_cast<const RectangularMTDTopology&>(topoproxy.specificTopology());
    
            float localX = cluster->getClusterPosX(); // get from the cluster the position along crystal length (in cm)
            float localY = 0.0f;

            if (cluster->getClusterErrorX() < 0.) { // in case its not set, use topology
                MeasurementPoint mp(cluster->x(), cluster->y());
                const LocalPoint localPos = topo.localPosition(mp);
                localX = localPos.x();
                localY = localPos.y();
            } else { // valid x from SiPM readout, but get y from topology (rows/cols)
                localY = topo.localY(cluster->y());
            }

            const LocalPoint localPos(localX, localY, 0.0f);
            const GlobalPoint gp = det->surface().toGlobal(localPos);
            weightedGlobalX += static_cast<double>(energy) * gp.x();
            weightedGlobalY += static_cast<double>(energy) * gp.y();
            weightedGlobalZ += static_cast<double>(energy) * gp.z();
        } else {
            std::cout << "[MTDMergedClusterProducer -- debug] Warning: Unable to convert cluster DetId " 
                      << cluster->id().rawId() << " to GlobalPoint, geometry not available" <<std::endl;
        }
    }

    //float avgTime = weightedTime / totalEnergy;
    //float avgTimeError = std::sqrt(weightedTimeError2) / totalEnergy;
    float avgTime = 0.f;
    float avgTimeError = 0.f;
    if (totalEnergy > 0) {
        avgTime = weightedTime / totalEnergy;
        avgTimeError = std::sqrt(weightedTimeError2) / totalEnergy;
    }
    
    //float avgX = weightedX / totalEnergy;
    //float avgY = weightedY / totalEnergy;
    GlobalPoint avgGlobal(0., 0., 0.);
    if (totalEnergy > 0) {
        avgGlobal = GlobalPoint(weightedGlobalX / totalEnergy,
                                weightedGlobalY / totalEnergy,
                                weightedGlobalZ / totalEnergy);
    }

    // now seed-local
    float avgX = 0;
    float avgY = 0;
    const GeomDet* seedDet = geom.idToDetUnit(seedId);
    if (seedDet && totalEnergy > 0) {
        LocalPoint lp = seedDet->surface().toLocal(avgGlobal);
        avgX = lp.x(); // along crystal (phi)
        avgY = lp.y(); // perpendicular to crystal (eta)
    } else {
        std::cout << "[MTDMergedClusterProducer -- debug] Warning: Unable to convert avgGlobal to seed-local coordinates for seedId " 
                  << seedId.rawId() << " , geometry not available" <<std::endl;
    }

    float avgXError = 0.f;
    float avgYError = 0.f;

    if (totalEnergy > 0) {
        double weightedErrorX2 = 0.;
        double weightedErrorY2 = 0.;

        for (const auto* cluster : clusters) {
            float energy = cluster->energy();
            const GeomDet* det = geom.idToDetUnit(cluster->id());
            if (!det) continue;

            const ProxyMTDTopology& topoproxy = static_cast<const ProxyMTDTopology&>(det->topology());
            const RectangularMTDTopology& topo = static_cast<const RectangularMTDTopology&>(topoproxy.specificTopology());

            float localXError = 0.f;
            float localYError = 0.f;

            if (cluster->getClusterErrorX() < 0.) {
                MeasurementPoint mp(cluster->x(), cluster->y());
                float sigma_flat = 1.0f / std::sqrt(12.0f);
                float sigma2 = cluster->positionError(sigma_flat);
                sigma2 *= sigma2;
                MeasurementError posErr(sigma2, 0, sigma2);
                LocalError localErr = topo.localError(mp, posErr);
                localXError = std::sqrt(localErr.xx());
                localYError = std::sqrt(localErr.yy());
            } else { // use cluster provided error
                localXError = cluster->getClusterErrorX();
                // Y error from topology
                MeasurementPoint mp(cluster->x(), cluster->y());
                float sigma_flat = 1.0f / std::sqrt(12.0f);
                float sigma2 = cluster->positionError(sigma_flat);
                sigma2 *= sigma2;
                MeasurementError posErr(sigma2, 0, sigma2);
                LocalError localErr = topo.localError(mp, posErr);
                localYError = std::sqrt(localErr.yy());
            } 

            std::cout << "[DEBUG error] cluster " << cluster->id().rawId() 
              << " localXError=" << localXError 
              << " energy=" << energy 
              << " contribution=" << (energy*energy*localXError*localXError) << std::endl;
    
            weightedErrorX2 += energy * energy * localXError * localXError; //approx -- lets say they're in the same ref frame
            weightedErrorY2 += energy * energy * localYError * localYError;
        }

        std::cout << "[DEBUG ERROR] Total: weightedErrorX2=" << weightedErrorX2 
          << " totalEnergy=" << totalEnergy 
          << " avgXError=" << (std::sqrt(weightedErrorX2) / totalEnergy) << std::endl;

        avgXError = std::sqrt(weightedErrorX2) / totalEnergy;
        avgYError = std::sqrt(weightedErrorY2) / totalEnergy;
    }

    /*std::cout << "[DEBUG avgX] seedId=" << seedId.rawId() 
          << " weightedGlobalX=" << weightedGlobalX
          << " totalEnergy=" << totalEnergy
          << " avgGlobal.x=" << avgGlobal.x()
          << " avgGlobal.y=" << avgGlobal.y()
          << " avgGlobal.z=" << avgGlobal.z()
          << " avgX=" << avgX
          << " avgY=" << avgY
    << std::endl;*/
    
    //return FTLMergedCluster(seedId, totalEnergy, avgTime, avgTimeError, avgX, avgY, clusterIds);
    DetId mergedId = DetId(seedId.rawId());
    FTLMergedCluster mergedCluster(mergedId, totalEnergy, avgTime, avgTimeError, avgX, avgY, avgXError, avgYError, clusterIds);

    std::cout << "[DEBUG MERGED] Created mergedCluster: id=" << mergedCluster.id().rawId()
          << " xError()=" << mergedCluster.xError()
          << " yError()=" << mergedCluster.yError()
          << " x()=" << mergedCluster.x()
          << " y()=" << mergedCluster.y() << std::endl;

    const double eps = 1e-6 * std::max(1.f, totalEnergy);
    const double diff = static_cast<double>(mergedCluster.energy()) - static_cast<double>(totalEnergy);
    if (std::abs(diff) > eps || std::isnan(mergedCluster.energy()) || std::isnan(mergedCluster.time()) || std::isinf(mergedCluster.energy()) || std::isinf(mergedCluster.time())) {
        /*std::cout << "[MTDMergedClusterProducer -- debug] Energy mismatch in merge: N=" << clusters.size()
                  << " sum(E_i)=" << totalEnergy
                  << " merged.E=" << mergedCluster.energy()
                  << " diff=" << diff << std::endl;*/
        // Optional: list inputs
        for (const auto* c : clusters) {
            /*std::cout << "    input DetId=" << c->id().rawId()
                      << " E=" << c->energy()
                      << " t=" << c->time() << std::endl;*/
        }
        std::cout << "[MTDMergedClusterProducer - debug] merged cluster suspicious (seed=" << seedId.rawId() << "). Nclusters=" << clusters.size()
                  << " sumE=" << totalEnergy << " mergedE=" << mergedCluster.energy()
                  << " mergedT=" << mergedCluster.time() << std::endl;
        for (const auto* c : clusters) {
            std::cout << "    input DetId=" << c->id().rawId()
                      << " E=" << c->energy()
                      << " t=" << c->time() << " tErr=" << c->timeError() << std::endl;
        }
    }
    return mergedCluster;
}

void MTDMergedClusterProducer::produce(edm::Event& e, const edm::EventSetup& es) {    
    // Get topology for navigation
    auto const& geom = es.getData(mtdgeoToken_); 
    auto topologyHandle = es.getTransientHandle(mtdtopoToken_);
    const MTDTopology* topology = topologyHandle.product();
    
    edm::Handle<FTLClusterCollection> btlClustersHandle;
    e.getByToken(btlClustersToken_, btlClustersHandle);
    
    std::cout << "MTDMergedClusterProducer: Processing event " << e.id() << std::endl;
    //std::cout << "Time threshold: " << timeThreshold_ << " sigma, Energy threshold: " << energyThreshold_ << " MeV" << std::endl;
    //std::cout << "Input BTL clusters: " << btlClustersHandle->size() << std::endl;

    auto btlOutput = std::make_unique<FTLMergedClusterCollection>();
    auto etlOutput = std::make_unique<FTLMergedClusterCollection>();
    
    std::map<uint32_t, std::vector<FTLMergedCluster>> mergedByDet;
    std::map<uint32_t, std::vector<FTLMergedCluster>> etlByDet;

    if (!btlClustersHandle.isValid() || btlClustersHandle->size() == 0) {
        std::cout << "No valid BTL clusters found in event" << std::endl;
        e.put(std::move(btlOutput), btlMergedClusterInstance_);
        return;
    } // FIXME add else {...} so it continues to process ETL clusters too
     
    // collect clusters and sort by module ID 
    std::vector<const FTLCluster*> allClusters;
    std::map<BTLDetId, std::vector<const FTLCluster*>> clusterMap;
    
    for (const auto& detSet : *btlClustersHandle) {
        for (const auto& cluster : detSet) {
            if (cluster.energy() < energyThreshold_) continue;
            allClusters.push_back(&cluster);
            clusterMap[cluster.id()].push_back(&cluster);
        }
    }

    // sorting...
    std::sort(allClusters.begin(), allClusters.end(), [](const FTLCluster* a, const FTLCluster* b) {
        BTLDetId idA = a->id();
        BTLDetId idB = b->id();

        // side -> rod -> module
        if (idA.zside() != idB.zside()) return idA.zside() < idB.zside();
        if (idA.mtdRR() != idB.mtdRR()) return idA.mtdRR() < idB.mtdRR();
        return idA.module() < idB.module();
    });

    std::cout << "[DEBUG] All clusters for event:" << std::endl;
    for (const auto* cluster : allClusters) {
        std::cout << "  Cluster ptr: " << cluster
                << " DetId: " << cluster->id().rawId()
                << " energy: " << cluster->energy()
                << " time: " << cluster->time()
                << std::endl;
    }

    std::cout << "[DEBUG] clusterMap contents:" << std::endl;
    for (const auto& entry : clusterMap) {
        std::cout << "  DetId: " << entry.first.rawId() << " has " << entry.second.size() << " clusters:" << std::endl;
        for (const auto* cluster : entry.second) {
            std::cout << "    Cluster ptr: " << cluster
                      << " energy: " << cluster->energy()
                      << " time: " << cluster->time()
                      << std::endl;
        }
    }

    //std::cout << "Found " << allClusters.size() << " BTL clusters above energy threshold" << std::endl;

    std::set<const FTLCluster*> processedClusters;
    
    // Process clusters 
    for (const auto* cluster : allClusters) {
        if (processedClusters.count(cluster)) {
            //std::cout << "[DEBUG] Skipping cluster (already processed): " << cluster->id().rawId() << std::endl;
            continue;
        }
        //std::cout << "[DEBUG] Seeding merge with cluster: " << cluster->id().rawId() << std::endl;

        BTLDetId cluId = cluster->id();
        /*std::cout << "Cluster 1: E=" << cluster.energy() 
            << " t=" << cluster.time() 
            << " x=" << cluster.x() 
            << " y=" << cluster.y();*/
            
        std::vector<const FTLCluster*> mergedClusterClusters = {cluster};
        processedClusters.insert(cluster);
        //std::cout << "[DEBUG] Marked seed cluster as processed: " << cluster->id().rawId() << std::endl;
        
        // Check for edge hits in current cluster
        bool edgeHitIn0 = false;
        bool edgeHitIn15 = false;
            
        for (int i = 0; i < cluster->size(); ++i) {
            auto hit = cluster->hit(i);
            int hit_col = hit.y();
            if (hit_col == 0) {
                edgeHitIn0 = true;
            } else if (hit_col == 15) {
                edgeHitIn15 = true;
            }
        }
            
        bool hasEdgeHitCurrent = edgeHitIn0 || edgeHitIn15;
        //std::cout << "  Edge hits: col0=" << edgeHitIn0 << ", col15=" << edgeHitIn15 << std::endl;
            
        // Get topology indices
        std::pair<uint32_t, uint32_t> indices = topology->btlIndex(cluId.rawId());
        uint32_t iphi = indices.first;
        uint32_t ieta = indices.second;
        //std::cout << "  BTL indices: iphi=" << iphi << ", ieta=" << ieta << std::endl;
            
        // ETA DIRECTION MERGING
        if (hasEdgeHitCurrent && iphi != std::numeric_limits<uint32_t>::max() && ieta != std::numeric_limits<uint32_t>::max()) {
                
            std::vector<int> etaOffsets = {1, -1};
            for (int etaOffset : etaOffsets) {
                uint32_t adjDetIdRaw = topology->btlidFromIndex(iphi, ieta + etaOffset);
                if (adjDetIdRaw == 0) continue;
                    
                BTLDetId adjDetId(adjDetIdRaw);
                auto it = clusterMap.find(adjDetId);
                if (it == clusterMap.end()) continue;
                //if (processedClusters.count(it->second)) continue;
                for (const auto* adjClusterPtr : it->second) {
                    if (processedClusters.count(adjClusterPtr)) {
                        //std::cout << "[DEBUG] Skipping neighbor (already processed): " << adjClusterPtr->id().rawId()
                        //          << " (seed: " << cluster->id().rawId() << ")" << std::endl;
                        continue;
                    }
                

                    //std::cout << "[DEBUG] Considering neighbor: " << adjClusterPtr->id().rawId()
                    //        << " (seed: " << cluster->id().rawId() << ")" << std::endl;

                    const FTLCluster* adjCluster = adjClusterPtr;
                    /*std::cout << "  Found eta neighbor at " << adjDetId.rawId() << std::endl;
                    std::cout << "  Neighbor cluster: E=" << adjCluster->energy() 
                                << " t=" << adjCluster->time() 
                                << " x=" << adjCluster->x() 
                                << " y=" << adjCluster->y();*/
                        
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
                    
                    /*if (hasOppositeEdgeHit && areTimingCompatible(cluster, adjCluster)) {
                        std::cout << "  -> MERGING ETA neighbor: " << cluId.rawId() 
                                << " with " << adjDetId.rawId() << std::endl;
                        std::cout << "  -> Cluster 1 module: " << cluId.module() 
                                << ", Cluster 2 module: " << adjDetId.module() << std::endl;
                        mergedClusterClusters.push_back(adjCluster);
                        processedClusters.insert(adjCluster);
                        std::cout << "[DEBUG] Marked neighbor as processed: " << adjCluster->id().rawId() << std::endl;
                    }*/

                    std::cout << "    edge check: seed(col0=" << edgeHitIn0 << ",col15=" << edgeHitIn15
                            << ") neighborOpposite=" << hasOppositeEdgeHit << std::endl;

                    bool timeOk = areTimingCompatible(cluster, adjCluster);
                    std::cout << "    timing: dt=" << std::abs(cluster->time()-adjCluster->time())
                            << " ns -> timingOk=" << timeOk << std::endl;

                    if (hasOppositeEdgeHit && timeOk) {
                        std::cout << "  -> MERGING ETA neighbor: " << cluId.rawId()
                                << " with " << adjDetId.rawId() << std::endl;
                        std::cout << "  -> Cluster 1 module: " << cluId.module()
                                << ", Cluster 2 module: " << adjDetId.module() << std::endl;
                        mergedClusterClusters.push_back(adjCluster);
                        // mark neighbor as processed immediately to avoid reuse
                        processedClusters.insert(adjCluster);
                        std::cout << "[DEBUG] Marked neighbor as processed: " << adjCluster->id().rawId() << std::endl;
                    }
                }
            }
        }
        
        // create mergedcluster from merged + leftover singles
        FTLMergedCluster mergedCluster = mergeClusters(mergedClusterClusters, cluId, geom);
        mergedByDet[mergedCluster.id().rawId()].push_back(std::move(mergedCluster));   
    }
    
    for (auto const& entry : mergedByDet) {
        uint32_t rawDetId = entry.first;
        const auto& vec = entry.second;
        edmNew::DetSetVector<FTLMergedCluster>::FastFiller filler(*btlOutput, rawDetId);
        for (const auto& mc : vec) filler.push_back(mc);
    }
    
    std::cout << "About to put " << btlOutput->size() << " MergedClusters into event..." << std::endl;
    e.put(std::move(btlOutput), btlMergedClusterInstance_);
    std::cout << "=== Successfully put BTL MergedClusters into event ===" << std::endl;

    edm::Handle<FTLClusterCollection> etlClustersHandle;
    e.getByToken(etlClustersToken_, etlClustersHandle);

    if (!etlClustersHandle.isValid() || etlClustersHandle->size() == 0) {
        std::cout << "No valid ETL clusters found in event" << std::endl;
        e.put(std::move(etlOutput), etlMergedClusterInstance_);
    } else {
        for (const auto& detSet : *etlClustersHandle) {
            for (const auto& cluster : detSet) {
                if (cluster.energy() < energyThreshold_) continue;
                std::vector<const FTLCluster*> singleClusterVec = {&cluster};
                FTLMergedCluster mergedCluster = mergeClusters(singleClusterVec, cluster.id(), geom);
                etlByDet[mergedCluster.id().rawId()].push_back(std::move(mergedCluster));
            }
        }

        for (auto const& entry : etlByDet) {
            uint32_t rawDetId = entry.first;
            const auto& vec = entry.second;
            edmNew::DetSetVector<FTLMergedCluster>::FastFiller filler(*etlOutput, rawDetId);
            for (const auto& mc : vec) filler.push_back(mc);
        }
        std::cout << "About to put " << etlOutput->size() << " ETL MergedClusters into event..." << std::endl;
        e.put(std::move(etlOutput), etlMergedClusterInstance_);
    }
}

void MTDMergedClusterProducer::fillDescriptions(edm::ConfigurationDescriptions& descriptions) {
    edm::ParameterSetDescription desc;
    desc.add<edm::InputTag>("btlBarrel", edm::InputTag("mtdClusters", "FTLBarrel"));
    desc.add<edm::InputTag>("etlEndcap", edm::InputTag("mtdClusters", "FTLEndcap"));
    desc.add<std::string>("btlMergedClusterInstance", "FTLBarrel");    
    desc.add<std::string>("etlMergedClusterInstance", "FTLEndcap");
    desc.add<double>("timeThreshold", 2.0);
    desc.add<double>("energyThreshold", 0.0);  // MeV
    descriptions.add("MTDMergedClusterProducer", desc);
}

DEFINE_FWK_MODULE(MTDMergedClusterProducer);

