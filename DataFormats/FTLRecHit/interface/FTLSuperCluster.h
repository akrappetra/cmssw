#ifndef DataFormats_FTLRecHit_FTLSuperCluster_h
#define DataFormats_FTLRecHit_FTLSuperCluster_h

#include "DataFormats/DetId/interface/DetId.h"
#include "DataFormats/FTLRecHit/interface/FTLClusterCollections.h"
#include <vector>

class FTLSuperCluster {
public:
    // Default constructor
    constexpr FTLSuperCluster() : id_(0), energy_(0.0), time_(0.0), timeError_(0.0), x_(0.0), y_(0.0), clusterIds_() {}
    
    // Constructor
    FTLSuperCluster(DetId id, float energy, float time, float timeError, float x, float y, const std::vector<DetId>& clusterIds)
        : id_(id), energy_(energy), time_(time), timeError_(timeError), x_(x), y_(y), clusterIds_(clusterIds) {}

    // getteri
    DetId id() const { return id_; }
    float energy() const { return energy_; }
    float time() const { return time_; }
    float timeError() const { return timeError_; }
    float x() const { return x_; }
    float y() const { return y_; }
    const std::vector<DetId>& clusterIds() const { return clusterIds_; }
    size_t nClusters() const { return clusterIds_.size(); }    

private:
    DetId id_;
    float energy_;
    float time_;
    float timeError_;
    float x_;
    float y_;

    std::vector<DetId> clusterIds_;
};
typedef std::vector<FTLSuperCluster> FTLSuperClusterCollection;

#endif