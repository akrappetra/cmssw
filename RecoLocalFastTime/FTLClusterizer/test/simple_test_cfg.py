import FWCore.ParameterSet.Config as cms

from Configuration.Eras.Era_Phase2C17I13M9_cff import Phase2C17I13M9
process = cms.Process("TEST", Phase2C17I13M9)

# essential things
process.load('SimGeneral.HepPDTESSource.pythiapdt_cfi')
process.load("FWCore.MessageService.MessageLogger_cfi")
process.load("Configuration.Geometry.GeometryExtendedRun4D110Reco_cff")
process.load('Configuration.StandardSequences.MagneticField_cff')
process.load('Configuration.StandardSequences.FrontierConditions_GlobalTag_cff')

from Configuration.AlCa.GlobalTag import GlobalTag
process.GlobalTag = GlobalTag(process.GlobalTag, 'auto:phase2_realistic_T33', '')

process.MessageLogger.cerr.threshold = 'INFO'
process.maxEvents = cms.untracked.PSet(input = cms.untracked.int32(5))

process.source = cms.Source("PoolSource",
    fileNames = cms.untracked.vstring(
        "file:/eos/user/p/pakrap/MTD/CMSSW_15_0_0_pre2/src/Validation/MtdValidation/29706.0_SinglePiFlatPt0p7To10+Run4D110/1000evt/step3.root"
    )
)

process.mtdSuperClusters = cms.EDProducer("MTDSuperClusterProducer",
    srcBarrel = cms.InputTag("mtdClusters", "FTLBarrel"),
    BarrelSuperClusterName = cms.string("FTLBarrel"),
    timeThreshold = cms.double(10.0),
    energyThreshold = cms.double(1.0)
)

process.mtd_reco = cms.Path(
    process.mtdSuperClusters
    )
    
print("Testing BTL MTDSuperClusterProducer with adjacent cluster algorithm...")
