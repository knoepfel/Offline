///////////////////////////////////////////////////////////////////////////////
// Calorimeter-driven track finding
// P.Murat, G.Pezzullo
// try to order routines alphabetically
///////////////////////////////////////////////////////////////////////////////
#include "fhiclcpp/ParameterSet.h"

#include "CalPatRec/inc/CalTrkFit_module.hh"
#include "CalPatRec/inc/AlgorithmIDCollection.hh"

// framework
#include "art/Framework/Principal/Handle.h"
#include "GeometryService/inc/GeomHandle.hh"
#include "GeometryService/inc/DetectorSystem.hh"
#include "art/Framework/Core/ModuleMacros.h"
#include "art/Framework/Services/Optional/TFileService.h"

// conditions
#include "ConditionsService/inc/AcceleratorParams.hh"
#include "ConditionsService/inc/ConditionsHandle.hh"
#include "ConditionsService/inc/TrackerCalibrations.hh"
#include "GeometryService/inc/getTrackerOrThrow.hh"
#include "TTrackerGeom/inc/TTracker.hh"
#include "CalorimeterGeom/inc/DiskCalorimeter.hh"
#include "ConfigTools/inc/ConfigFileLookupPolicy.hh"

#include "CalPatRec/inc/KalFitResult.hh"
#include "CalPatRec/inc/CprModuleHistBase.hh"
#include "art/Utilities/make_tool.h"

// Mu2e BaBar
#include "BTrk/ProbTools/ChisqConsistency.hh"
#include "BTrk/BbrGeom/BbrVectorErr.hh"
#include "BTrk/KalmanTrack/KalHit.hh"
#include "BTrk/TrkBase/TrkHelixUtils.hh"

//TrkReco
#include "TrkReco/inc/TrkUtilities.hh"

#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics/median.hpp>
#include <boost/accumulators/statistics/stats.hpp>
#include <boost/accumulators/statistics/moment.hpp>
#include <boost/algorithm/string.hpp>

#include "TSystem.h"
#include "TInterpreter.h"
#include "TVector2.h"

using namespace std;
using namespace boost::accumulators;
using CLHEP::HepVector;
using CLHEP::HepSymMatrix;
using CLHEP::Hep3Vector;

namespace mu2e {
  //-----------------------------------------------------------------------------
  // comparison functor for sorting by Z(wire)
  //-----------------------------------------------------------------------------
  struct straw_zcomp : public binary_function<StrawHitIndex,StrawHitIndex,bool> {
    bool operator()(StrawHitIndex const& h1, StrawHitIndex const& h2) {

      mu2e::GeomHandle<mu2e::TTracker> handle;
      const TTracker* t = handle.get();
      const Straw* s1 = &t->getStraw(StrawIndex(h1));
      const Straw* s2 = &t->getStraw(StrawIndex(h2));

      return s1->getMidPoint().z() < s2->getMidPoint().z();
    }
  }; // a semicolumn here is required

  //-----------------------------------------------------------------------------
  // module constructor, parameter defaults are defiend in CalPatRec/fcl/prolog.fcl
  //-----------------------------------------------------------------------------
  CalTrkFit::CalTrkFit(fhicl::ParameterSet const& pset) :
    _diagLevel       (pset.get<int>                ("diagLevel"                      )),
    _debugLevel      (pset.get<int>                ("debugLevel"                     )),
    _printfreq       (pset.get<int>                ("printFrequency"                 )),
    _useAsFilter     (pset.get<int>                ("useAsFilter"                    )),    
    _addhits         (pset.get<bool>               ("addhits"                        )),
    _zsave           (pset.get<vector<double> >("ZSavePositions",
						vector<double>{-1522.0,0.0,1522.0}   )), // front, middle and back of the tracker
    _shLabel         (pset.get<string>             ("StrawHitCollectionLabel"        )),
    _shDigiLabel     (pset.get<string>             ("StrawDigiCollectionLabel"       )),
    _shpLabel        (pset.get<string>             ("StrawHitPositionCollectionLabel")),
    _shfLabel        (pset.get<string>             ("StrawHitFlagCollectionLabel"    )),
    _trkseedLabel    (pset.get<string>             ("TrackSeedModuleLabel"           )),
    _maxdtmiss       (pset.get<double>             ("MaxDtMiss"                      )),
    _maxadddoca      (pset.get<double>             ("MaxAddDoca"                     )),
    _maxaddchi       (pset.get<double>             ("MaxAddChi"                      )),
    _goodseed        (pset.get<vector<string> >("GoodKalSeedFitBits",vector<string>{})),
    _tpart           ((TrkParticle::type)(pset.get<int>("fitparticle"               ))),
    _fdir            ((TrkFitDirection::FitDirection) (pset.get<int>("fitdirection"))),
    _fitter          (pset.get<fhicl::ParameterSet>   ("Fitter",fhicl::ParameterSet())),
    _result          ()
  {
    produces<KalRepCollection       >();
    produces<KalRepPtrCollection    >();
    produces<AlgorithmIDCollection  >();
    produces<StrawHitFlagCollection >();
    produces<KalSeedCollection      >();

    // fHackData = new THackData("HackData","Hack Data");
    // gROOT->GetRootFolder()->Add(fHackData);
//-----------------------------------------------------------------------------
// provide for interactive diagnostics
//-----------------------------------------------------------------------------
    if (_debugLevel != 0) _printfreq = 1;

    if (_diagLevel  != 0) {
      _hmanager = art::make_tool<CprModuleHistBase>(pset.get<fhicl::ParameterSet>("histograms"));
      fhicl::ParameterSet ps1 = pset.get<fhicl::ParameterSet>("Fitter.DoubletAmbigResolver");
      //      fhicl::ParameterSet ps2 = pset.get<fhicl::ParameterSet>("doubletAmbigResolver");
      _dar      = new DoubletAmbigResolver(ps1,0,0,0);
      _data.listOfDoublets = new std::vector<Doublet>;
    }
    else {
      _hmanager = std::make_unique<CprModuleHistBase>();
      _dar                 = NULL;
      _data.listOfDoublets = NULL;
    }

  }

  //-----------------------------------------------------------------------------
  // destructor
  //-----------------------------------------------------------------------------
  CalTrkFit::~CalTrkFit() {
    if (_dar) {
      delete _data.listOfDoublets;
      delete _dar;
    }
  }

  //-----------------------------------------------------------------------------
  void CalTrkFit::beginJob(){
    art::ServiceHandle<art::TFileService> tfs;
    _hmanager->bookHistograms(tfs,&_hist);
  }

//-----------------------------------------------------------------------------
  bool CalTrkFit::beginRun(art::Run& ) {
    mu2e::GeomHandle<mu2e::TTracker> th;
    _tracker      = th.get();
    _data.tracker = _tracker;  // cache for passing around to histogramming code

    mu2e::GeomHandle<mu2e::Calorimeter> ch;
    _calorimeter = ch.get();
    // calibrations

    mu2e::ConditionsHandle<TrackerCalibrations> tcal("ignored");
    _trackerCalib = tcal.operator ->();

    _fitter.setTracker(_tracker);
    _fitter.setTrackerCalib(_trackerCalib);
    _fitter.setCalorimeter(_calorimeter);

    // ConditionsHandle<AcceleratorParams> accPar("ignored");
    // _data.mbtime = accPar->deBuncherPeriod;

    return true;
  }

//-----------------------------------------------------------------------------
// find the input data products
//-----------------------------------------------------------------------------
  bool CalTrkFit::findData(const art::Event& evt) {

    art::Handle<mu2e::StrawHitCollection> strawhitsH;
    if (evt.getByLabel(_shLabel,strawhitsH)) {
      _shcol = strawhitsH.product();
    }
    else {
      _shcol  = 0;
      printf(" >>> ERROR in CalTrkFit::findData: StrawHitCollection with label=%s not found.\n",
             _shLabel.data());
    }

    art::Handle<mu2e::StrawHitPositionCollection> shposH;
    if (evt.getByLabel(_shpLabel,shposH)) {
      _shpcol = shposH.product();
    }
    else {
      _shpcol = 0;
      printf(" >>> ERROR in CalTrkFit::findData: StrawHitPositionCollection with label=%s not found.\n",
             _shpLabel.data());
    }

    art::Handle<mu2e::StrawHitFlagCollection> shflagH;
    if (evt.getByLabel(_shfLabel,shflagH)) {
      _shfcol = shflagH.product();
    }
    else {
      _shfcol = 0;
      printf(" >>> ERROR in CalTrkFit::findData: StrawHitFlagCollection with label=%s not found.\n",
             _shfLabel.data());
    }

    _trkseeds = 0;
    art::Handle<KalSeedCollection> trkseedsHandle;
    if (evt.getByLabel(_trkseedLabel, trkseedsHandle)) _trkseeds = trkseedsHandle.product();
//-----------------------------------------------------------------------------
// done
//-----------------------------------------------------------------------------
    return (_shcol != 0) && (_shfcol != 0) && (_shpcol != 0) && (_trkseeds != 0);
  }

//-----------------------------------------------------------------------------
// event entry point
//-----------------------------------------------------------------------------
  bool CalTrkFit::filter(art::Event& event ) {
    const char*               oname = "CalTrkFit::produce";
    //    bool                      findkal (false);
    int                       nseeds;

    KalRep*                 krep;
    const KalSeed*            kalSeed(0);

    _data.event      = &event;
    _data.result     = &_result;
    _data.ntracks[0] = 0;	        // _ntracks doesn't seem to be used
    _data.ntracks[1] = 0;
					// reset the fit iteration counter
    _fitter.setNIter(0);
					// event printout
    _eventid = event.event();
    _iev     = event.id().event();

    if ((_iev%_printfreq) == 0) printf("[%s] : START event number %8i\n", oname,_iev);
//-----------------------------------------------------------------------------
// output collections should always be created
//-----------------------------------------------------------------------------
    art::ProductID   kalRepsID(getProductID<KalRepCollection>(event));

    unique_ptr<KalRepCollection>       tracks   (new KalRepCollection        );
    unique_ptr<KalRepPtrCollection>    trackPtrs(new KalRepPtrCollection     );
    unique_ptr<AlgorithmIDCollection>  algs     (new AlgorithmIDCollection   );
    unique_ptr<KalSeedCollection>      kscol    (new KalSeedCollection()     );
    unique_ptr<StrawHitFlagCollection> shfcol   (new StrawHitFlagCollection());
 
    if (! findData(event)) goto END;

    shfcol.reset(new StrawHitFlagCollection(*_shfcol));

    _result._fitType     = 1;               // final fit
    _result._event       = &event ;
    _result._shcol       = _shcol ;
    _result._shpos       = _shpcol;
    _result._shfcol      = _shfcol;
    _result._tpart       = _tpart ;
    _result._fdir        = _fdir  ;
    _result._shDigiLabel = _shDigiLabel;
//-----------------------------------------------------------------------------
// loop over track "seeds" found by CalSeedFit
//-----------------------------------------------------------------------------
    nseeds = _trkseeds->size();
    
    for (int iseed=0; iseed<nseeds; iseed++) {
      kalSeed = &_trkseeds->at(iseed);
      int n_seed_hits = kalSeed->hits().size();

      if (! kalSeed->status().hasAllProperties(_goodseed)) continue;
      if ( n_seed_hits < _fitter.minNStraws())             continue;
//-----------------------------------------------------------------------------
// check the seed has the same basic parameters as this module expects and has
// at least one segment
//-----------------------------------------------------------------------------
      if(kalSeed->particle() != _tpart || kalSeed->fitDirection() != _fdir ) {
	throw cet::exception("RECO")<<"mu2e::KalFinalFit: wrong particle or direction"<< endl;
      }

      if(kalSeed->segments().size() < 1) {
	throw cet::exception("RECO")<<"mu2e::KalFinalFit: no segments"<< endl;
      }
//-----------------------------------------------------------------------------
// find the segment at the 0 flight
//-----------------------------------------------------------------------------
      double flt0 = kalSeed->flt0();
      auto kseg = kalSeed->segments().end();
      for(auto iseg= kalSeed->segments().begin(); iseg != kalSeed->segments().end(); ++iseg) {
	if(iseg->fmin() <= flt0 && iseg->fmax() > flt0){
	  kseg = iseg;
	  break;
	}
      }
      
      if (kseg == kalSeed->segments().end()) {
	printf("Helix segment range doesn't cover flt0 = %10.3f\n",flt0) ;

	for (auto iseg= kalSeed->segments().begin(); iseg != kalSeed->segments().end(); ++iseg) {
	  printf("segment fmin, fmax: %10.3f %10.f \n",iseg->fmin(),iseg->fmax());
	}
	kseg = kalSeed->segments().begin();
      }
//-----------------------------------------------------------------------------
// form a list of hits starting KalSeed hits
//-----------------------------------------------------------------------------
      _result._hitIndices->clear();

      for (auto ihit=kalSeed->hits().begin(); ihit!=kalSeed->hits().end(); ihit++) {
	if (ihit->flag().hasAllProperties(StrawHitFlag::active)) {
	StrawHitIndex hi = ihit->index();
	_result._hitIndices->push_back(hi);
	}
      }

      _result._caloCluster = kalSeed->caloCluster().get();
//-----------------------------------------------------------------------------
// create a trajectory from the seed. This should be a general utility function that
// can work with multi-segment seeds FIXME!
// create CLHEP objects from seed native members.  This will
// go away when we switch to SMatrix FIXME!!!
//-----------------------------------------------------------------------------
      HepVector    pvec(5,0);
      HepSymMatrix pcov(5,0);
      kseg->helix().hepVector(pvec);
      kseg->covar().symMatrix(pcov);
					// create trajectory from these

      if (_result._helixTraj == 0) _result._helixTraj = new HelixTraj(pvec, pcov);
      else                        *_result._helixTraj = HelixTraj(pvec, pcov);
//--------------------------------------------------------------------------------
// 2015-03-23 G. Pezzu: fill info about the doca
//--------------------------------------------------------------------------------
      if (_debugLevel > 0) printf("[CalTrkFit::produce] calling _fitter.makeTrack\n");

      _fitter.makeTrack(_result);

      if (_debugLevel > 0) {
	printf("[CalTrkFit::produce] after _fitter.makeTrack fit status = %i\n", _result._fit.success());
	_fitter.printHits(_result,"CalTrkFit::produce kalfit_001");
      }

      if (_result._fit.success()) {
	if (_addhits) {
//-----------------------------------------------------------------------------
// this is the default. First, add back the hits on this track
// if successfull, try to add missing hits, at this point external errors were
// set to zero
// assume this is the last iteration
//-----------------------------------------------------------------------------
	  //	      int last_iteration = _fitter.maxIteration();
	  int last_iteration   = -1;
	  _result._nunweediter = 0;
	  _fitter.unweedHits(_result,_maxaddchi);
	  if (_debugLevel > 0) _fitter.printHits(_result,"CalTrkFit::produce after unweedHits");
	      
	  findMissingHits(_result);
//-----------------------------------------------------------------------------
// if new hits have been added, add them and refit the track.
// Otherwise - just refit the track one last time in both cases
//-----------------------------------------------------------------------------
	  if (_result._missingHits.size() > 0) {
	    _fitter.addHits(_result,_maxaddchi);
	  }
	  else {
	    _fitter.fitIteration(_result,last_iteration);
	  }

	  if (_debugLevel > 0) _fitter.printHits(_result,"CalTrkFit::produce after addHits");
//-----------------------------------------------------------------------------
// and weed hits again to insure that addHits doesn't add junk
//-----------------------------------------------------------------------------
	  _fitter.weedHits(_result,last_iteration);
	}
//-----------------------------------------------------------------------------
// now evaluate the T0 and its error using the straw hits
//-----------------------------------------------------------------------------
	_fitter.updateT0(_result);
	    
	if (_debugLevel > 0) _fitter.printHits(_result,"CalTrkFit::produce : final, after weedHits");
      }

      if (_result._fit.success()) {
//-----------------------------------------------------------------------------
// fill 'per-track' histograms (mode=1)
//-----------------------------------------------------------------------------
	if (_diagLevel > 0) {
	  _dar->findDoublets(_result._krep,_data.listOfDoublets);
	  _hmanager->fillHistograms(1,&_data,&_hist);
	}
//-----------------------------------------------------------------------------
// save successful kalman fits in the event.
// start from _result, as stealTrack clears the hit pointers
// _result doesn't own anything
//-----------------------------------------------------------------------------
        krep = _result.stealTrack();
//-----------------------------------------------------------------------------
// flag all hits as belonging to this track this is probably obsolete, FIXME!
//-----------------------------------------------------------------------------
	if (size_t(iseed) < StrawHitFlag::_maxTrkId) {
	  for (auto ihit = krep->hitVector().begin(); ihit != krep->hitVector().end(); ++ihit) {
	    TrkStrawHit* hit = static_cast<TrkStrawHit*> (*ihit);
	    if (hit->isActive()) {
	      int idx = hit->index();
	      shfcol->at(idx).merge(StrawHitFlag::trackBit(iseed));
	    }
	  }
	}

        tracks->push_back(krep);
        int index = tracks->size()-1;
        trackPtrs->emplace_back(kalRepsID, index, event.productGetter(kalRepsID));

	int best = AlgorithmID::CalPatRecBit;
	int mask = 1 << AlgorithmID::CalPatRecBit;

	algs->push_back(AlgorithmID(best,mask));

	// convert successful fits into 'seeds' for persistence.  Start with the input
	KalSeed fseed(*kalSeed);
	// reference the seed fit in this fit
	art::Handle<KalSeedCollection> ksH;
	event.getByLabel(_trkseedLabel, ksH);
	fseed._kal = art::Ptr<KalSeed>(ksH, iseed);
	// fill other information
	fseed._t0 = krep->t0();
	fseed._flt0 = krep->flt0();
	fseed._status.merge(TrkFitFlag::kalmanOK);
	if(krep->fitStatus().success()==1) fseed._status.merge(TrkFitFlag::kalmanConverged);
	TrkUtilities::fillHitSeeds(krep, fseed._hits);
//-----------------------------------------------------------------------------
// sample the fit at the requested z positions.  This should
// be in terms of known positions (front of tracker, ...) FIXME!
//-----------------------------------------------------------------------------
	for(auto zpos : _zsave) {
	  // compute the flightlength for this z
	  double fltlen = TrkUtilities::zFlight(krep->pieceTraj(),zpos);
	  // sample the momentum at this flight.  This belongs in a separate utility FIXME
	  BbrVectorErr momerr = krep->momentumErr(fltlen);
	  // sample the helix
	  double locflt(0.0);
	  const HelixTraj* htraj = dynamic_cast<const HelixTraj*>(krep->localTrajectory(fltlen,locflt));
	  // fill the segment
	  KalSegment kseg;
	  TrkUtilities::fillSegment(*htraj,momerr,kseg);
	  fseed._segments.push_back(kseg);
	}
	kscol->push_back(fseed);
      }
      else {
//-----------------------------------------------------------------------------
// fit failed, just delete the track
//-----------------------------------------------------------------------------
        _result.deleteTrack();
      }
    }
//-----------------------------------------------------------------------------
// histogramming
//-----------------------------------------------------------------------------
  END:;
    if (_diagLevel > 0) {
      _data.ntracks[0] = tracks->size();
      _hmanager->fillHistograms(0,&_data,&_hist);
    }
//-----------------------------------------------------------------------------
// put reconstructed tracks into the event record
//-----------------------------------------------------------------------------
    int     ntracks = trackPtrs->size();

    event.put(std::move(tracks)   );
    event.put(std::move(trackPtrs));
    event.put(std::move(kscol    ));
    event.put(std::move(shfcol   ));
    event.put(std::move(algs     ));

    if (_useAsFilter == 0) return true;
    else                   return (ntracks > 0);
  }

  //-----------------------------------------------------------------------------
  //
  //-----------------------------------------------------------------------------
  void CalTrkFit::endJob(){
    // does this cause the file to close?
    art::ServiceHandle<art::TFileService> tfs;
  }


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
  void CalTrkFit::findMissingHits(KalFitResultNew& KRes) {

    const char* oname = "CalTrkFit::findMissingHits";

    //  Trajectory info
    Hep3Vector tdir;
    HepPoint   tpos;
    double     dt;

    KalRep* krep =  KRes._krep;

    krep->pieceTraj().getInfo(0.0,tpos,tdir);

    const TrkDifPieceTraj* reftraj = krep->referenceTraj();

    if (_debugLevel > 0) printf("[%s]      shId    sec     panel       doca        drift     dr added\n",oname);

    KRes._missingHits.clear();
    KRes._doca.clear();

    int nstrs = KRes._shcol->size();
    for (int istr=0; istr<nstrs; ++istr) {
//----------------------------------------------------------------------
// 2015-02-11 change the selection bit for searching for missed hits
//----------------------------------------------------------------------
      StrawHit const& sh = _shcol->at(istr);
//-----------------------------------------------------------------------------
// I think, we want to check the radial bit: if it is set, than at least one of
// the two measured times is wrong...
//-----------------------------------------------------------------------------
//      int radius_ok = KRes._shfcol->at(istr).hasAllProperties(StrawHitFlag::radsel);
      dt        = KRes._shcol->at(istr).time()-krep->t0()._t0;

      //      if (radius_ok && (fabs(dt) < _maxdtmiss)) {
      if (fabs(dt) < _maxdtmiss) {

					// make sure we haven't already used this hit
	TrkStrawHit  *tsh, *closest(NULL);
	bool found = false;

	Straw const&      straw = _tracker->getStraw(sh.strawIndex());
	CLHEP::Hep3Vector hpos  = straw.getMidPoint();

	double dz_max(1.e12) ; // closest_z(1.e12);

	double zhit = hpos.z();

	for (std::vector<TrkHit*>::iterator it=krep->hitVector().begin(); it!=krep->hitVector().end(); it++) {
	  tsh = static_cast<TrkStrawHit*> (*it);
	  int tsh_index = tsh->index();
	  if (tsh_index == istr) {
	    found = true;
	    break;
	  }
					// check proximity in Z
          Straw const&  trk_straw = _tracker->getStraw(tsh->strawHit().strawIndex());
          double        ztrk      = trk_straw.getMidPoint().z();

	  double dz  = ztrk-zhit;
	  if (fabs(dz) < fabs(dz_max)) {
	    closest   = tsh;
	    dz_max    = dz;
	    //	    closest_z = ztrk;
	  }
	}

        if (! found) {
					// estimate trajectory length to hit 
	  double hflt = 0;
	  TrkHelixUtils::findZFltlen(*reftraj,zhit,hflt);

          // good in-time hit.  Compute DOCA of the wire to the trajectory
	  // also estimate the drift time if this hit were on the track to get hte hit radius

	  TrkT0 hitt0 = closest->hitT0();

	  double s    = closest->fltLen();
	  double mom  = krep->momentum(s).mag();
	  double beta = krep->particleType().beta(mom);

	  double tflt = (hflt-s)/(beta*CLHEP::c_light);
	  hitt0._t0  += tflt;

          CLHEP::Hep3Vector hdir  = straw.getDirection();
          // convert to HepPoint to satisfy antique BaBar interface: FIXME!!!
          HepPoint          spt(hpos.x(),hpos.y(),hpos.z());
          TrkLineTraj       htraj(spt,hdir,-20,20);
          // estimate flightlength along track.  This assumes a constant BField!!!

          double      fltlen = (hpos.z()-tpos.z())/tdir.z();

          TrkPoca     hitpoca(krep->pieceTraj(),fltlen,htraj,0.0);

	  double      doca, rdrift, dr, hit_error(0.2);

	  TrkStrawHit hit(sh,straw,istr,hitt0,hflt,hit_error,10);
	  
	  ConditionsHandle<TrackerCalibrations> tcal("ignored");

	  double tdrift=hit.time()-hit.hitT0()._t0;

	  T2D t2d;

	  tcal->TimeToDistance(straw.index(),tdrift,tdir,t2d);


	  rdrift = t2d._rdrift;

	  doca = hitpoca.doca();
	  if (doca > 0) dr = doca-rdrift;
	  else          dr = doca+rdrift;

//-----------------------------------------------------------------------------
// flag hits with small residuals
//-----------------------------------------------------------------------------
	  int added (0);
          if (fabs(dr) < _maxadddoca) {
            KRes._missingHits.push_back(istr);
            KRes._doca.push_back(doca);
	    added = 1;
          }

          if (_debugLevel > 0) {
            printf("[CalTrkFit::findMissingHits] %8i  %6i  %8i  %10.3f %10.3f %10.3f   %3i\n",
                   straw.index().asInt(),
                   straw.id().getPlane(),
                   straw.id().getPanel(),
                   doca, rdrift, dr,added);
          }
        }
      }
      else {
	if (_debugLevel > 0) {
	  printf("[%s] rejected hit: i, index, flag, dt: %5i %5i %s %10.3f\n",
		 oname,istr,sh.strawIndex().asInt(),
		 KRes._shfcol->at(istr).hex().data(),sh.dt());
	}
      }
    }
  }

}
using mu2e::CalTrkFit;
DEFINE_ART_MODULE(CalTrkFit);
