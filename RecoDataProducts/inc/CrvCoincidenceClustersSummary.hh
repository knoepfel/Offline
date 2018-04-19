#ifndef RecoDataProducts_CrvCoincidenceClustersSummary_hh
#define RecoDataProducts_CrvCoincidenceClustersSummary_hh
//
// $Id: $
// $Author: ehrlich $
// $Date: 2014/08/07 01:33:41 $
//
// Contact person Ralf Ehrlich
//

#include "MCDataProducts/inc/SimParticle.hh"
#include "RecoDataProducts/inc/CrvRecoPulse.hh"
#include "canvas/Persistency/Common/Ptr.h"
#include "CLHEP/Vector/ThreeVector.h"

#include <vector>

namespace mu2e 
{
  class CrvCoincidenceClustersSummary
  {
    public:

    struct PulseInfo
    {
      art::Ptr<CrvRecoPulse> _crvRecoPulse;
      art::Ptr<SimParticle>  _simParticle;     //MC
      double                 _energyDeposited; //MC
      PulseInfo() {}
      PulseInfo(const art::Ptr<CrvRecoPulse> &crvRecoPulse, const art::Ptr<SimParticle> &simParticle, double energyDeposited) :
                _crvRecoPulse(crvRecoPulse), _simParticle(simParticle), _energyDeposited(energyDeposited) {}
    };

    struct Cluster
    {
      int                _crvSectorType;
      CLHEP::Hep3Vector  _avgCounterPos;
      double             _startTime;
      double             _endTime;
      int                _PEs;
      bool               _hasMCInfo;
      std::vector<PulseInfo> _pulses;
      art::Ptr<SimParticle>  _mostLikelySimParticle; //MC
      double                 _totalEnergyDeposited;  //MC
      double                 _earliestHitTime;       //MC
      CLHEP::Hep3Vector      _earliestHitPos;        //MC

      Cluster() {}
      Cluster(int crvSectorType, const CLHEP::Hep3Vector &avgCounterPos, double startTime, double endTime, int PEs, 
              bool hasMCInfo, const std::vector<PulseInfo> &pulses, const art::Ptr<SimParticle> mostLikelySimParticle, 
              double totalEnergyDeposited, double earliestHitTime, const CLHEP::Hep3Vector &earliestHitPos) :
              _crvSectorType(crvSectorType), _avgCounterPos(avgCounterPos), _startTime(startTime), _endTime(endTime), _PEs(PEs), 
              _hasMCInfo(hasMCInfo), _pulses(pulses), _mostLikelySimParticle(mostLikelySimParticle), 
              _totalEnergyDeposited(totalEnergyDeposited), _earliestHitTime(earliestHitTime), _earliestHitPos(earliestHitPos) {}
    };

    CrvCoincidenceClustersSummary() {}

    const std::vector<Cluster> &GetClusters() const
    {
      return _clusters;
    }

    std::vector<Cluster> &GetClusters()
    {
      return _clusters;
    }

    private:

    std::vector<Cluster> _clusters;
  };
}

#endif /* RecoDataProducts_CrvCoincidenceClustersSummary_hh */
