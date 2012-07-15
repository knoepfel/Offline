//
// Construct and return an Beamline.
//
//
// $Id: BeamlineMaker.cc,v 1.9 2012/07/15 22:06:16 kutschke Exp $
// $Author: kutschke $
// $Date: 2012/07/15 22:06:16 $
//
// Original author Peter Shanahan
//

#include <cmath>
#include <iomanip>
#include <iostream>

// Framework includes
#include "messagefacility/MessageLogger/MessageLogger.h"

// Mu2e includes
#include "BeamlineGeom/inc/Beamline.hh"
#include "BeamlineGeom/inc/BeamlineMaker.hh"
#include "BeamlineGeom/inc/TransportSolenoid.hh"
#include "ConfigTools/inc/SimpleConfig.hh"

#ifndef __CINT__

using namespace std;

namespace mu2e {

  std::auto_ptr<Beamline> BeamlineMaker::make(const SimpleConfig& c) {
    std::auto_ptr<Beamline> res(new Beamline());
    BuildBeamline(c, res.get());
    BuildTS(c, res.get());
    return res;
  }

  void BeamlineMaker::BuildBeamline(const SimpleConfig& c, Beamline* beamline) {
    beamline->_solenoidOffset = c.getDouble("mu2e.solenoidOffset");
  }

  void BeamlineMaker::BuildTS(const SimpleConfig& c, Beamline* beamline) {

    // Build the TransportSolenoid Geometry.

    // Read or calculate data

    double solenoidOffset = beamline->_solenoidOffset;

    double rTorus = c.getDouble("toyTS.rTorus");
    double rVac   = c.getDouble("toyTS.rVac");
    double rCryo  = c.getDouble("toyTS.rCryo");

    double ts1HalfLength = c.getDouble("toyTS1.halfLength");
    double ts3HalfLength = solenoidOffset - rTorus;
    double ts5HalfLength = c.getDouble("toyTS5.halfLength");

    double ts1zOffset    = (-rTorus-ts1HalfLength);
    double ts5zOffset    = ( rTorus+ts5HalfLength);

    double coll1HalfLength = c.getDouble("coll1.halfLength");
    double coll3HalfLength = c.getDouble("coll3.halfLength");
    double coll5HalfLength = c.getDouble("coll5.halfLength");
    double coll3Hole       = c.getDouble("coll3.hole");

    // Save results to the geometry objects

    beamline->_ts._rTorus = rTorus;
    beamline->_ts._rVac   = rVac;
    beamline->_ts._rCryo  = rCryo;

    beamline->_ts._ts1.set(ts1HalfLength,
                            CLHEP::Hep3Vector(solenoidOffset,0.0,ts1zOffset),
                            0 );
    beamline->_ts._ts3.set(ts3HalfLength,
                            CLHEP::Hep3Vector(0.0,0.0,0.0),
                            new CLHEP::HepRotation(CLHEP::HepRotationY(90.0*CLHEP::degree)) );
    beamline->_ts._ts5.set(ts5HalfLength,
                            CLHEP::Hep3Vector(-solenoidOffset,0.0,ts5zOffset),
                            0 );

    beamline->_ts._coll1.set( coll1HalfLength,CLHEP::Hep3Vector(0.0,0.0,-ts1HalfLength+coll1HalfLength));
    beamline->_ts._coll31.set(coll3HalfLength,CLHEP::Hep3Vector(0.0,0.0,-coll3HalfLength-coll3Hole/2));
    beamline->_ts._coll32.set(coll3HalfLength,CLHEP::Hep3Vector(0.0,0.0, coll3HalfLength+coll3Hole/2));
    beamline->_ts._coll5.set( coll5HalfLength,CLHEP::Hep3Vector(0.0,0.0, ts5HalfLength-coll5HalfLength));

  }

} // namespace mu2e

#endif
