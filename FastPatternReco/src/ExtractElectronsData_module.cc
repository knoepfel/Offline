//
// module that extract Data of the Electrons tracks that came from the targets and put temporary inside the event
//
// $Id: ExtractElectronsData_module.cc,v 1.10 2013/03/14 19:47:45 kutschke Exp $
// $Author: kutschke $
// $Date: 2013/03/14 19:47:45 $
//
// Original author G. Tassielli
//

// C++ includes.
#include <iostream>
#include <string>
#include <memory>
#include <map>
#include <utility>
#include <limits>

#include <boost/shared_ptr.hpp>


// Framework includes.
//#include "art/Framework/Core/EDAnalyzer.h"
#include "art/Framework/Core/EDProducer.h"
#include "art/Framework/Principal/Event.h"
#include "art/Framework/Principal/Run.h"
#include "art/Framework/Core/ModuleMacros.h"
#include "art/Framework/Services/Optional/TFileDirectory.h"
#include "art/Framework/Services/Optional/TFileService.h"
#include "art/Framework/Services/Registry/ServiceHandle.h"
#include "art/Framework/Principal/Handle.h"
#include "art/Framework/Principal/Provenance.h"
#include "fhiclcpp/ParameterSet.h"
#include "messagefacility/MessageLogger/MessageLogger.h"


#include "CLHEP/Vector/TwoVector.h"

// Mu2e includes.
//#include "GeometryService/inc/GeometryService.hh"
//#include "GeometryService/inc/GeomHandle.hh"
//#include "GeometryService/inc/getTrackerOrThrow.hh"
//#include "TrackerGeom/inc/Tracker.hh"
#include "TrackerGeom/inc/Straw.hh"
#include "ITrackerGeom/inc/Cell.hh"
//#include "DataProducts/inc/DPIndexVectorCollection.hh"
#include "MCDataProducts/inc/PtrStepPointMCVectorCollection.hh"
#include "MCDataProducts/inc/GenParticleCollection.hh"
#include "MCDataProducts/inc/PhysicalVolumeInfoCollection.hh"
#include "MCDataProducts/inc/SimParticleCollection.hh"
#include "MCDataProducts/inc/StepPointMCCollection.hh"
#include "MCDataProducts/inc/StrawHitMCTruthCollection.hh"
#include "RecoDataProducts/inc/StrawHitCollection.hh"
//#include "ITrackerGeom/inc/ITracker.hh"
//#include "TTrackerGeom/inc/TTracker.hh"
//#include "FastPatternReco/inc/TTHitPerTrackData.hh"
//#include "FastPatternReco/inc/GenTrackData.hh"
#include "MCDataProducts/inc/GenId.hh"
#include "MCDataProducts/inc/VisibleGenElTrack.hh"
#include "MCDataProducts/inc/VisibleGenElTrackCollection.hh"

// Root includes.
#include "TApplication.h"
#include "TCanvas.h"
#include "TDirectory.h"
#include "TMath.h"
#include "TH1F.h"
#include "TH2F.h"
#include "TClonesArray.h"
#include "TObjArray.h"
#include "TLine.h"
#include "TArrow.h"
#include "TEllipse.h"
#include "TMarker.h"
#include "TBox.h"
#include "TROOT.h"
#include "TStyle.h"
#include "TF1.h"
#include "TSpectrum.h"
#include "TLatex.h"

using namespace std;

namespace mu2e {

typedef art::Ptr<StrawHit> StrawHitPtr;
class Straw;

class ExtractElectronsData : public art::EDProducer {
public:

        explicit ExtractElectronsData(fhicl::ParameterSet const& pset);
        virtual ~ExtractElectronsData() {
                //if (_fakeCanvas)        delete _fakeCanvas;
        }

        virtual void beginJob();
        void endJob();

        // This is called for each event.
        //void analyze(art::Event const& e);
        void produce(art::Event & e);

private:

        // Start: run time parameters

        // The module label of this module.
        std::string _moduleLabel;

        // Label of the G4 module
        std::string _g4ModuleLabel;

        // Name of the tracker StepPoint collection
        std::string _trackerStepPoints;

        // Label of the module that made the hits.
        std::string _makerModuleLabel;

        // Label of the generator.
        std::string _generatorModuleLabel;

        int _debugLvl;

        //    // Number of events to accumulate between prompts.
        //    int _nAccumulate;

        // End: run time parameters

        //TCanvas*      _fakeCanvas;

        // Some ugly but necessary ROOT related bookkeeping:

        // The job needs exactly one instance of TApplication.  See note 1.
        //auto_ptr<TApplication> _application;

        // Save directory from beginJob so that we can go there in endJob. See note 3.
        //TDirectory* _directory;

};

ExtractElectronsData::ExtractElectronsData(fhicl::ParameterSet const& pset) :

      // Run time parameters
      _moduleLabel(pset.get<string>("module_label")),/*@module_label*/
      _g4ModuleLabel(pset.get<string>("g4ModuleLabel")),
      _trackerStepPoints(pset.get<string>("trackerStepPoints","tracker")),
      _makerModuleLabel(pset.get<string>("makerModuleLabel")),
      _generatorModuleLabel(pset.get<std::string>("generatorModuleLabel", "generate")),
      _debugLvl(pset.get<int>("diagLevel",0))
     /*,
    _fakeCanvas(0),

    // Some ugly but necessary ROOT related bookkeeping.
    _application(0),
    _directory(0)*/
{
        // Tell the framework what we make.
        produces<VisibleGenElTrackCollection>();

}

void ExtractElectronsData::beginJob(){

        cout<<"Starting Extract Electrons from targets that hit the tracker job!"<<endl;

        // Get access to the TFile service and save current directory for later use.
        //art::ServiceHandle<art::TFileService> tfs;

        // If needed, create the ROOT interactive environment. See note 1.
        //    if ( !gApplication ){
        //      int    tmp_argc(0);
        //      char** tmp_argv(0);
        //      _application = auto_ptr<TApplication>(new TApplication( "noapplication", &tmp_argc, tmp_argv ));
        //    }
        //
        //    gStyle->SetPalette(1);
        //    gROOT->SetStyle("Plain");

        //_fakeCanvas = new TCanvas("canvas_Fake","double click for next event",300,100);
        //    _fakeCanvas = tfs->make<TCanvas>("canvas_Fake","double click for next event",400,200);

        // See note 3.
        //_directory = gDirectory;

}

//  void ExtractElectronsData::analyze(art::Event const& event ) {
void ExtractElectronsData::produce(art::Event & event ) {

//cout<<"-----------------------1---------------------"<<endl;
        auto_ptr<VisibleGenElTrackCollection> genEltrk(new VisibleGenElTrackCollection);

//        const Tracker& tracker = getTrackerOrThrow();
//        const TTracker &ttr = static_cast<const TTracker&>( tracker );
//        const std::vector<Device> ttrdev = ttr.getDevices();

        art::Handle<StrawHitCollection> pdataHandle;
        event.getByLabel(_makerModuleLabel,pdataHandle);
        StrawHitCollection const* hits = pdataHandle.product();

        // Get the persistent data about the StrawHitsMCTruth.
        //art::Handle<StrawHitMCTruthCollection> truthHandle;
        //event.getByLabel(_makerModuleLabel,truthHandle);
        //StrawHitMCTruthCollection const* hits_truth = truthHandle.product();

        // Get the persistent data about pointers to StepPointMCs
        art::Handle<PtrStepPointMCVectorCollection> mcptrHandle;
        event.getByLabel(_makerModuleLabel,"StrawHitMCPtr",mcptrHandle);
        PtrStepPointMCVectorCollection const* hits_mcptr = mcptrHandle.product();

        if (!(/*hits->size() == hits_truth->size() &&*/
                        hits_mcptr->size() == hits->size() ) ) {
                /*throw cet::exception("RANGE")*/
                mf::LogError("RANGE")
                << "Strawhits: " << hits->size()
                /*<< " MCTruthStrawHits: " << hits_truth->size()*/
                << " MCPtr: " << hits_mcptr->size();
                return;
        }

        // Get handles to the generated and simulated particles.
        art::Handle<GenParticleCollection> genParticles;
        event.getByLabel(_generatorModuleLabel, genParticles);

        //art::Handle<SimParticleCollection> simParticles;
        //event.getByLabel(_g4ModuleLabel, simParticles);

        size_t nStrawPerEvent = hits->size();

        std::vector<mu2e::VisibleGenElTrack>::iterator genEltrk_it;

        double mchittime=0.0;

        bool overlapped = false;
        bool ovrlpByProton = false;
        bool isFirst = false;
        bool addTrack = true;

        double ptMeV, rho;
        //double sel_rho;
        double B=1.0;
        CLHEP::Hep2Vector radDir;
        HepGeom::Point3D<double> CirCenter;
        //cout<<"-----------------------2---------------------"<<endl;

        for (size_t i=0; i<nStrawPerEvent; ++i) {

                // Access data
//                StrawHit        const&      hit(hits->at(i));
//                StrawHitMCTruth const&    truth(hits_truth->at(i));
                //      DPIndexVector   const&    mcptr(hits_mcptr->at(i));
                PtrStepPointMCVector const&    mcptr(hits_mcptr->at(i));

                //      double hitEnergy = hit.energyDep();

                //Skip the straw if the energy of the hit is smaller than the minimum required
                //if (hitEnergy < _minimumEnergy) continue;

//                //Get hit straw
//                StrawIndex si = hit.strawIndex();
//                const Straw & str = tracker.getStraw(si);

                // cout << "Getting informations about cells" << endl;

//                StrawId sid = str.id();
//                int stn     = sid.getStraw();
//                int layern  = sid.getLayer();
//                int devicen = sid.getDevice();
//                int sectorn = sid.getSector();
//
//                const CLHEP::Hep3Vector stMidPoint3 = str.getMidPoint();

                //time of the hit
//                double hitTime = hit.time();

//                //direction of the straw
//                const CLHEP::Hep3Vector stDirection3 = str.getDirection();
//
//                // cout << "Reading MCtruth info" << endl;

                // Get MC truth data
//                double driftTime = truth.driftTime();
//                double driftDistance = truth.driftDistance();

                //Position along the wire using mctruth info
//                double vMC = truth.distanceToMid();

//                const CLHEP::Hep3Vector MCHitPoint = stMidPoint3 + (vMC/stDirection3.mag())*stDirection3;
//
//                CLHEP::Hep2Vector StYNormalDir;
//                StYNormalDir.set(stDirection3.getX(),stDirection3.getY());
//                StYNormalDir.rotate(90.0*CLHEP::degree);
//                StYNormalDir=StYNormalDir.unit();
//                //      cout<<"Normal dir "<<StYNormalDir<<endl;
//                CLHEP::Hep3Vector tmpUprPoint  = driftDistance*StYNormalDir+MCHitPoint;
//                CLHEP::Hep3Vector tmpDownPoint = -driftDistance*StYNormalDir+MCHitPoint;
//
//                //      cout<<"Point "<<MCHitPoint<<endl;
//                //      cout<<"Up Point  "<<tmpUprPoint<<endl;
//                //      cout<<"Dow Point "<<tmpDownPoint<<endl;


                overlapped = false;    // more that one track are crossing the cell/straw in the max drift time
                ovrlpByProton = false; // in case of overlapping, is there a proton?
                isFirst = false;       // is the hit of a track that produces the signal (the first in signal arrival time)?

                //cout<<"-----------------------3---------------------"<<endl;
                for (size_t j = 0; j < mcptr.size(); ++j) {

                        //        StepPointMC const& mchit = (*mchits)[mcptr[j].index];
                        StepPointMC const& mchit = *mcptr[j];

                        // The simulated particle that made this hit.
                        SimParticleCollection::key_type trackId(mchit.trackId());
                        //cout<<"-------------- j "<<j<<" trkId "<<trackId<<" -----------"<<endl;
                        //SimParticle const& sim = simParticles->at(trackId);
                        art::Ptr<SimParticle> const& simptr = mchit.simParticle();
                        art::Ptr<SimParticle>::key_type simKey(simptr.key());
                        SimParticle const& sim = *simptr;
                        //cout<<"-----------------------3.1---------------------"<<endl;

                        isFirst=( j==0 );
                        if ( isFirst ){
                                for ( size_t jj = 1; jj < mcptr.size(); ++jj) {
                                        //                        if ( trackId != (*mchits)[mcptr[jj].index].trackId() ) {
                                        //if ( trackId != (*(mcptr[jj])).trackId() ) {
                                        //if ( !( simptr== mcptr[jj]->simParticle() ) ) {
                                        if ( simKey!=mcptr[jj]->simParticle().key() && trackId != (*(mcptr[jj])).trackId() ) {
                                                overlapped=true;
                                                //if ( (simParticles->at((*(mcptr[jj])).trackId())).pdgId()==2212 ) {
                                                if ( (*(mcptr[jj])).simParticle()->pdgId()==2212 ) {
                                                        ovrlpByProton=true;
                                                        break;
                                                }
                                                //break;
                                        }
                                }
                        }

                        if ( sim.pdgId()==11 && sim.isPrimary() /*sim.fromGenerator()*/ /*&& sim.startMomentum().rho()>=103.0*/ ){

                                addTrack = true;
                                mchittime=mchit.time();

                                genEltrk_it=genEltrk->begin();
                                for ( ; genEltrk_it!=genEltrk->end(); ++genEltrk_it ){
                                        //if ( genEltrk_it->getTrkID()==trackId ) {
                                        if ( genEltrk_it->getTrkKey()==simKey && genEltrk_it->getTrkID()==trackId ) {
                                                genEltrk_it->addHitData( StrawHitPtr( pdataHandle, i), mchittime,overlapped,ovrlpByProton,isFirst,mchit.position(),mchit.momentum()  );
                                                addTrack=false;
                                                break;
                                        }
                                }
                                if (addTrack) {
                                        //genEltrk->push_back( VisibleGenElTrack( trackId, std::pair<CLHEP::Hep3Vector,CLHEP::HepLorentzVector>( sim.startPosition(), sim.startMomentum() ) ) );
                                        genEltrk->push_back( VisibleGenElTrack( simKey, trackId, std::pair<CLHEP::Hep3Vector,CLHEP::HepLorentzVector>( sim.startPosition(), sim.startMomentum() ) ) );
                                        genEltrk->back().addHitData( StrawHitPtr( pdataHandle, i), mchittime,overlapped,ovrlpByProton,isFirst,mchit.position(),mchit.momentum()  );
                                        //GenParticle const& genp = genParticles->at(sim.generatorIndex());
                                        GenParticle const& genp = *sim.genParticle();
                                        if ( genp.generatorId()==GenId::conversionGun ) genEltrk->back().setConversionEl();
                                }

                        }

                }

        }
        //cout<<"-----------------------4---------------------"<<endl;

        //    std::map<SimParticleCollection::key_type,std::pair<CLHEP::Hep3Vector,CLHEP::HepLorentzVector> >::iterator genEl_it     = genElectrons.begin();
        //    /*std::map<SimParticleCollection::key_type,std::pair<CLHEP::Hep3Vector,CLHEP::Hep3Vector> >::iterator*/ firstHitOfGenEl_it = firstHitOfGenEl.begin();

        //TClonesArray *genElDraws = new TClonesArray("TEllipse");
        genEltrk_it = genEltrk->begin();
        //genElDraws->ExpandCreateFast( nGenEl );
        while ( genEltrk_it != genEltrk->end() ){
                genEltrk_it->sort();
                genEltrk_it->FindNTurns();
                VisibleGenElTrack &iEltrk = const_cast<VisibleGenElTrack &>(*genEltrk_it);
                unsigned short &nloops = iEltrk.getNumOfLoops();

                if (_debugLvl>0) {
                        cout<<"Generated el at "<<genEltrk_it->getTrkVertex()<<" of "<<genEltrk_it->getTrkLrntzVec()<<endl;
                        cout<<"Is the conversion el? "<<genEltrk_it->isConversionEl()<<endl;
                        cout<<"First hit in tracker of gen el at "<<genEltrk_it->getFirstHit()._hitPoint<<" of "<<genEltrk_it->getFirstHit()._hitMomentum<<endl;
                        cout<<"N loops "<<nloops<<endl;
                }

                for ( unsigned int ilp=0; ilp<nloops; ilp++ ){
                        GenElHitData& hdil = iEltrk.getithLoopHit(ilp);
                        ptMeV = sqrt( pow(hdil._hitMomentum[0],2) + pow(hdil._hitMomentum[1],2) );
                        rho   = ptMeV/(B*0.3);
                        double helStep = CLHEP::twopi*rho*hdil._hitMomentum[2]/ptMeV;
                        CirCenter.set(hdil._hitPoint.getX(),hdil._hitPoint.getY(),hdil._hitPoint.getZ());
                        radDir.setX(hdil._hitMomentum.getX());
                        radDir.setY(hdil._hitMomentum.getY());
                        radDir.rotate( ( (hdil._hitMomentum.getZ()>=0.0) ? 90.0 : -90.0 )*CLHEP::degree );
                        radDir=radDir.unit();
                        CirCenter=CirCenter+rho*radDir;
                        double phi0;
                        //if (radDir.phi()>0) phi0=radDir.phi()-CLHEP::pi;
                        //else phi0=radDir.phi()+CLHEP::pi;
                        phi0=atan2( -radDir.y(), -radDir.x() );
                        if (_debugLvl>1) {
                                cout<<ilp<<" -th loop: p_t "<<ptMeV<<" rho mm "<<rho<<" P mm "<<helStep<<endl;
                                cout<<"radDir "<<radDir<<endl;
                                cout<<" Hit Pos "<<hdil._hitPoint<<" Circ center "<<CirCenter<<" phi0 "<< phi0/*/CLHEP::deg*/<<" phi0 at z(-1500) "<< phi0 - CLHEP::twopi*(hdil._hitPoint.getZ()+1500.0)/helStep <<endl;
                        }

                }
//                cout<<"Hits of the el track: "<<iEltrk.getNumOfHit() <<" List of hit(Id):"<<endl;
//                for (int ihit=0; ihit<(int)iEltrk.getNumOfHit(); ihit++){
//                        GenElHitData& hdil = iEltrk.getHit(ihit);
//                        if (hdil._isFirst) cout<<"\t"<<hdil._iHit->strawIndex()<<endl;
//                }

                ++genEltrk_it;
        }

        //cout<<"-----------------------5---------------------"<<endl;
        event.put(std::move(genEltrk));


  } // end produce

  void ExtractElectronsData::endJob(){

//
//    // cd() to correct root directory. See note 3.
//    //TDirectory* save = gDirectory;
//    //_directory->cd();
//
//    // Write canvas.  See note 4.
////    _canvas->Write();
//
//    // cd() back to where we were.  See note 3.
//    //save->cd();
//
  }


}  // end namespace mu2e

using mu2e::ExtractElectronsData;
DEFINE_ART_MODULE(ExtractElectronsData);
