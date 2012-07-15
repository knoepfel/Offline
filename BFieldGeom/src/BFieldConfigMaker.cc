#include "BFieldGeom/inc/BFieldConfigMaker.hh"

#include <string>
#include <iostream>
#include <set>

#include "cetlib/exception.h"

#include "ConfigTools/inc/SimpleConfig.hh"

#include "BFieldGeom/inc/BFieldConfig.hh"
#include "BeamlineGeom/inc/Beamline.hh"
#include "ConfigTools/inc/ConfigFileLookupPolicy.hh"

// CLHEP includes
#include "CLHEP/Units/SystemOfUnits.h"

using namespace std;

namespace mu2e {

  namespace {
    void noLegacyFormatMapSpecs(const SimpleConfig& config) {
      vector<string> keysToCheck;
      keysToCheck.push_back("bfield.files");
      keysToCheck.push_back("bfield.dsFile");
      keysToCheck.push_back("bfield.psFile");
      keysToCheck.push_back("bfield.tsuFile");
      keysToCheck.push_back("bfield.tsdFile");

      for(vector<string>::const_iterator i = keysToCheck.begin(); i != keysToCheck.end(); ++i) {
        if(config.hasName(*i)){
          throw cet::exception("GEOM")
            << "Found obsolete config file parameter "<<*i
            <<".  Please use bfield.innerMaps and bfield.outerMaps to specify G4BL maps."
            << "Maps are not loaded.\n";
        }
      }
    }
  }

  BFieldConfigMaker::BFieldConfigMaker(const SimpleConfig& config, const Beamline& beamg)
    : bfconf_(new BFieldConfig())
  {
    bfconf_->writeBinaries_ = config.getBool("bfield.writeG4BLBinaries", false);
    bfconf_->verbosityLevel_ = config.getInt("bfield.verbosityLevel");
    bfconf_->flipBFieldMaps_ = config.getBool("bfield.flipMaps", false);

    bfconf_->scaleFactor_ = config.getDouble("bfield.scaleFactor",1.0);

    bfconf_->dsFieldForm_ = BFieldConfig::DSFieldModel(config.getInt("detSolFieldForm", BFieldConfig::dsModelUniform));

    const double bz = config.getDouble("toyDS.bz", 0.);
    bfconf_->dsUniformValue_ = CLHEP::Hep3Vector( 0., 0., bz * bfconf_->scaleFactor_);

    const double grad = config.getDouble("toyDS.gradient", 0.);
    bfconf_->dsGradientValue_ = CLHEP::Hep3Vector( 0., 0., grad * bfconf_->scaleFactor_);

    const string format = config.getString("bfield.format","GMC");
    if( format=="GMC" ) {

      // These maps require torus radius of 2926 mm
      const double requiredTorusRadius = 2926.0 * CLHEP::mm;
      if( fabs(beamg.getTS().torusRadius() - requiredTorusRadius)>0.1 ){
        throw cet::exception("GEOM")
          << "The GMC magnetic field files require torus radius of 2926 mm."
          << " The value used by geometry is " <<beamg.getTS().torusRadius()
          << " Maps are not loaded.\n";
      }
      bfconf_->mapType_ = BFMapType::GMC;

      // Add the DS, TS and PS field maps.
      addGMC(config, bfconf_.get(), "bfield.dsFile", "bfield.dsDimensions");
      addGMC(config, bfconf_.get(), "bfield.tsFile", "bfield.tsDimensions");
      addGMC(config, bfconf_.get(), "bfield.psFile", "bfield.psDimensions");

    }
    else if( format=="G4BL" ) {

      // These maps require torus radius of 2929 mm
      const double requiredTorusRadius = 2929.0 * CLHEP::mm;
      if( fabs(beamg.getTS().torusRadius() - requiredTorusRadius)>0.1 ){
        throw cet::exception("GEOM")
          << "The G4BL magnetic field files require torus radius of 2929 mm."
          << " The value used by geometry is " <<beamg.getTS().torusRadius()
          << " Maps are not loaded.\n";
      }
      bfconf_->mapType_ = BFMapType::G4BL;

      // Make sure we don't have any leftover bits from old geometry format,
      // to avoid user confusion.
      noLegacyFormatMapSpecs(config);

      config.getVectorString("bfield.innerMaps", bfconf_->innerMapFiles_);
      config.getVectorString("bfield.outerMaps", bfconf_->outerMapFiles_);

    }
    else {
      throw cet::exception("GEOM")
        << "Unknown format of file with magnetic field maps: " << format << "\n";
    }

  }

  // Parse the config file to learn about one magnetic field map.
  // Create an empty map and call the code to load the map from the file.
  void BFieldConfigMaker::addGMC(const SimpleConfig& config,
                                 BFieldConfig *bfconf,
                                 const std::string& fileKey,
                                 const std::string& dimensionKey) {

    if ( ! config.hasName(fileKey) ){
      cout << "No magnetic field file specified for: "
           << fileKey
           << "   Hope that's OK." << endl;
      return;
    }

    // Get filename and expected dimensions.
    string filename = config.getString(fileKey);
    vector<int> dim;
    config.getVectorInt(dimensionKey,dim, 3);

    // FIXME: can we use inner maps?
    bfconf->outerMapFiles_.push_back(filename);
    bfconf->gmcDimensions_.push_back(dim);
  }

} // namespace mu2e
