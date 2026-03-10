import FWCore.ParameterSet.Config as cms


from Configuration.Eras.Era_Phase2C17I13M9_cff import Phase2C17I13M9
process = cms.Process('mtdValidation',Phase2C17I13M9)

process.load('SimGeneral.HepPDTESSource.pythiapdt_cfi')
process.load("FWCore.MessageService.MessageLogger_cfi")
process.load('Configuration.EventContent.EventContent_cff')
process.load('Configuration.StandardSequences.EndOfProcess_cff')
process.load('SimGeneral.MixingModule.mixNoPU_cfi')

process.load("Configuration.Geometry.GeometryExtendedRun4D110Reco_cff")
process.load('Configuration.StandardSequences.MagneticField_cff')
process.load('Configuration.StandardSequences.FrontierConditions_GlobalTag_cff')
from Configuration.AlCa.GlobalTag import GlobalTag
process.GlobalTag = GlobalTag(process.GlobalTag, 'auto:phase2_realistic_T33', '')
process.load('RecoLocalFastTime.FTLClusterizer.MTDCPEESProducer_cfi')
process.load("Configuration.StandardSequences.Reconstruction_cff")
#process.load('RecoLocalFastTime.FTLClusterizer.mtdMergedClusterProducer_cfi')

process.mtdTrackingRecHits.barrelClusters = cms.InputTag("mtdMergedClusters", "FTLBarrel")
#process.mtdTrackingRecHits.endcapClusters = cms.InputTag("mtdMergedClusters", "FTLEndcap")

process.load('SimFastTiming.MtdSimMergedClusterProducers.mtdSimMergedClusterProducer_cfi')
process.load('SimFastTiming.MtdAssociatorProducers.mtdRecoMergedClusterToSimMergedClusterAssociatorByHits_cfi')
process.load('SimFastTiming.MtdAssociatorProducers.mtdRecoMergedClusterToSimMergedClusterAssociation_cfi')
process.load('SimGeneral.TrackingAnalysis.simHitTPAssociation_cfi')
process.load('SimFastTiming.MtdAssociatorProducers.mtdSimMergedClusterToTPAssociation_cfi')
process.load('SimFastTiming.MtdAssociatorProducers.mtdSimLayerClusterToTPAssociation_cfi')
process.load('SimFastTiming.MtdAssociatorProducers.mtdSimMergedClusterToTPAssociatorByTrackId_cfi')
process.load('SimFastTiming.MtdAssociatorProducers.mtdSimLayerClusterToTPAssociatorByTrackId_cfi')
process.load('SimFastTiming.MtdAssociatorProducers.mtdRecoClusterToSimLayerClusterAssociatorByHits_cfi')
process.load('SimFastTiming.MtdAssociatorProducers.mtdRecoClusterToSimLayerClusterAssociation_cfi')
process.load('SimTracker.TrackAssociatorProducers.quickTrackAssociatorByHits_cfi')
process.load('SimTracker.TrackerHitAssociation.tpClusterProducer_cfi')
process.load('SimTracker.TrackAssociation.trackingParticleRecoTrackAsssociation_cfi')

process.maxEvents = cms.untracked.PSet( input = cms.untracked.int32(1000) )

#Setup FWK for multithreaded
process.options.numberOfThreads = 4
process.options.numberOfStreams = 0
process.options.numberOfConcurrentLuminosityBlocks = 0
process.options.eventSetup.numberOfConcurrentIOVs = 1

process.MessageLogger.cerr.FwkReport  = cms.untracked.PSet(
    reportEvery = cms.untracked.int32(100),
)

process.source = cms.Source("PoolSource",
    fileNames = cms.untracked.vstring(
        #'file:/eos/user/p/pakrap/MTD/CMSSW_15_0_0_pre2/src/Validation/MtdValidation/29706.0_SinglePiFlatPt0p7To10+Run4D110/1000evt/step3.root'
        'file:/eos/user/p/pakrap/MTD/CMSSW_ASSOCMAP/src/Validation/MtdValidation/PU/step3.root' # single pi
        #'file:/eos/user/p/pakrap/MTD/CMSSW_ASSOCMAP/src/RecoLocalFastTime/FTLClusterizer/output.root' # single pi
    )
)

process.mix.digitizers = cms.PSet()
for a in process.aliases: delattr(process, a)

# --- BTL Validation
process.load("Validation.MtdValidation.btlSimHitsValid_cfi")
process.load("Validation.MtdValidation.btlDigiHitsValid_cfi")
#process.load("Validation.MtdValidation.btlLocalRecoValid_cfi")
#btlValidation = cms.Sequence(process.btlSimHitsValid + process.btlDigiHitsValid + process.btlLocalRecoValid)
btlValidation = cms.Sequence(process.btlSimHitsValid + process.btlDigiHitsValid)

# --- ETL Validation
#process.load("Validation.MtdValidation.etlSimHitsValid_cfi")
#process.load("Validation.MtdValidation.etlDigiHitsValid_cfi")
#process.load("Validation.MtdValidation.etlLocalRecoValid_cfi")
#etlValidation = cms.Sequence(process.etlSimHitsValid + process.etlDigiHitsValid + process.etlLocalRecoValid)

# --- Global Validation
process.load("Validation.MtdValidation.mtdTracksValid_cfi")
#process.load("Validation.MtdValidation.mtdEleIsoValid_cfi")
#process.load("Validation.MtdValidation.vertices4DValid_cff")

# Restrict to barrel
process.mtdTracksValid.trackMinimumEtlEta = cms.double(10.0)
process.mtdTracksValid.trackMaximumEtlEta = cms.double(10.0)
#process.etlLocalRecoValid.recCluTag = cms.InputTag('mtdMergedClusters', 'FTLEndcap')

# process.btlDigiHitsValid.optionalPlots = True
# process.etlDigiHitsValid.optionalPlots = True
# process.btlLocalRecoValid.optionalPlots = True
# process.etlLocalRecoValid.optionalPlots = True
# process.mtdTracksValid.optionalPlots = True
# process.vertices4DValid.optionalPlots = True

process.mtdTracksValid.useMergedClusters = cms.untracked.bool(True)
process.mtdTracksValid.r2sAssociationMapTag = cms.InputTag("mtdRecoMergedClusterToSimMergedClusterAssociation")
process.mtdTracksValid.tp2SimAssociationMapTag = cms.InputTag("mtdSimMergedClusterToTPAssociation")
process.mtdTracksValid.recCluTagBTL = cms.InputTag('mtdMergedClusters', 'FTLBarrel')
process.mtdTracksValid.recCluTagETL = cms.InputTag('mtdMergedClusters', 'FTLEndcap')
#process.validation = cms.Sequence(btlValidation + etlValidation + process.mtdTracksValid + process.mtdEleIsoValid + process.vertices4DValid)

process.validation = cms.Sequence(btlValidation + process.mtdTracksValid)

process.DQMoutput = cms.OutputModule("DQMRootOutputModule",
    dataset = cms.untracked.PSet(
        dataTier = cms.untracked.string('DQMIO'),
        filterName = cms.untracked.string('')
    ),
    fileName = cms.untracked.string('file:step3_inDQM.root'),
    outputCommands = process.DQMEventContent.outputCommands,
    splitLevel = cms.untracked.int32(0)
)

process.p = cms.Path( process.mix + 
                      process.mtdTrackingRecHits +
                      process.simHitTPAssocProducer +
                      process.tpClusterProducer +
                      process.quickTrackAssociatorByHits +
                      process.trackingParticleRecoTrackAsssociation +
                      process.mtdSimLayerClusterToTPAssociatorByTrackId +
                      process.mtdSimLayerClusterToTPAssociation +
                      process.mtdRecoClusterToSimLayerClusterAssociatorByHits +
                      process.mtdRecoClusterToSimLayerClusterAssociation +
                      process.mtdSimMergedClusterProducer +
                      process.mtdRecoMergedClusterToSimMergedClusterAssociatorByHits +
                      process.mtdRecoMergedClusterToSimMergedClusterAssociation +
                      process.mtdSimMergedClusterToTPAssociatorByTrackId + 
                      process.mtdSimMergedClusterToTPAssociation + 
                      process.validation )

process.endjob_step = cms.EndPath(process.endOfProcess)
process.DQMoutput_step = cms.EndPath( process.DQMoutput )

process.schedule = cms.Schedule( process.p , process.endjob_step , process.DQMoutput_step )
