// $Id: PSVacuumMaker.cc,v 1.2 2012/07/15 22:06:18 kutschke Exp $
// $Author: kutschke $
// $Date: 2012/07/15 22:06:18 $
//
// Original author Andrei Gaponenko

#include <sstream>

#include "CLHEP/Vector/ThreeVector.h"
#include "CLHEP/Units/SystemOfUnits.h"

#include "ProductionSolenoidGeom/inc/PSVacuumMaker.hh"
#include "ProductionSolenoidGeom/inc/PSVacuum.hh"

#include "ProductionSolenoidGeom/inc/ProductionSolenoid.hh"
#include "ProductionSolenoidGeom/inc/PSEnclosure.hh"

#include "ConfigTools/inc/SimpleConfig.hh"

namespace mu2e {

  std::auto_ptr<PSVacuum>  PSVacuumMaker::make(const SimpleConfig& c,
                                               const ProductionSolenoid& ps,
                                               const PSEnclosure& pse,
                                               double zmax
                                               )

  {
    const double zmin = ps.psEndRefPoint().z() - 2*pse.shell().halfLength();

    const CLHEP::Hep3Vector originInMu2e(ps.psEndRefPoint().x(),
                                         ps.psEndRefPoint().y(),
                                         (zmin+zmax)/2);

    return std::auto_ptr<PSVacuum>(new PSVacuum(
                                                Tube(
                                                     c.getString("PS.insideMaterialName"),
                                                     originInMu2e,
                                                     0.,
                                                     ps.getVacVesselInnerParamsPtr()->innerRadius(),
                                                     (zmax - zmin)/2
                                                     )
                                                )
                                   );

  }

} // namespace mu2e
