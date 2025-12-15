import FWCore.ParameterSet.Config as cms
from Configuration.Eras.Era_Phase2C17I13M9_cff import Phase2C17I13M9
from Configuration.AlCa.GlobalTag import GlobalTag

process = cms.Process("MTDTrackDump", Phase2C17I13M9)

process.maxEvents = cms.untracked.PSet(input = cms.untracked.int32(-1))
process.source = cms.Source("PoolSource",
    fileNames = cms.untracked.vstring('root://eosuser.cern.ch//eos/user/p/pakrap/MTD/CMSSW_MC_CHECKS/src/RecoLocalFastTime/FTLClusterizer/output.root')
)

process.load("FWCore.MessageLogger.MessageLogger_cfi")
process.MessageLogger.cerr.FwkReport.reportEvery = 1

process.load("Configuration.Geometry.GeometryExtendedRun4D110Reco_cff")
process.load("Configuration.StandardSequences.FrontierConditions_GlobalTag_cff")
process.GlobalTag = GlobalTag(process.GlobalTag, 'auto:phase2_realistic_T33', '')

process.load('RecoLocalFastTime.FTLClusterizer.MTDCPEESProducer_cfi')
process.load('RecoLocalFastTime.FTLRecProducers.mtdTrackingRecHits_cfi')

process.mtdTrackingRecHits.barrelClusters = cms.InputTag("mtdMergedClusters", "FTLBarrel") 
process.mtdTrackingRecHits.endcapClusters = cms.InputTag("mtdMergedClusters", "FTLEndcap")

process.TFileService = cms.Service("TFileService",
    fileName = cms.string('mtd_mergedcluster_tracking_rechit_histograms.root')
)

process.mtdTrackingRecHitDump = cms.EDAnalyzer("MTDTrackingRecHitDump",
    src = cms.InputTag("mtdTrackingRecHits")  
)

process.mtdTrackingRecHitDump = cms.EDAnalyzer("MTDTrackingRecHitDump",
    src = cms.InputTag("mtdTrackingRecHits"),
    onlySingleClusters = cms.bool(True)  # Set to True to filter for single clusters only
)

# Run the producer first, then dump
process.reco_step = cms.Path(process.mtdTrackingRecHits)
process.dump_step = cms.EndPath(process.mtdTrackingRecHitDump)

process.schedule = cms.Schedule(process.reco_step, process.dump_step)