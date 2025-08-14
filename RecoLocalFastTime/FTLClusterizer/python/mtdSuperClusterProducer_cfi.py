import FWCore.ParameterSet.Config as cms

mtdSuperClusters = cms.EDProducer("MTDSuperClusterProducer",
    btlClusters = cms.InputTag("mtdClusters", "FTLBarrel"),
    timeThreshold = cms.double(10.0),
    energyThreshold = cms.double(1.0),
    btlSuperClusterInstance = cms.string("FTLBarrel")
)