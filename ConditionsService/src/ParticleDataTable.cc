//
// Mu2e wrapper around HepPDT::ParticleDataTable
//
//   $Id: ParticleDataTable.cc,v 1.14 2012/07/15 22:06:16 kutschke Exp $
//   $Author: kutschke $
//   $Date: 2012/07/15 22:06:16 $
//
//
// 1) The Geant4 particle table is a superset of this table.  It includes
//    various nuclei and ions.
//
// 2) In changeUnits and improveData I want to loop over all elements in the
//    and modify their contents. The minor complication is that there is no
//    non-const iterator over the table.  But we can stitch one together using
//    two components.  First, there is an accessor by ParticleID to give a
//    non-const pointer to any element in the table.  Also, there is a const
//    iterator over the table.
//
// 3) The table is populated in the destructor of the table builder.  So it is
//    not valid until the table builder goes out of scope.
//
// 4) At present mass_width_2008.mc contains precise values of masses and widths
//    but it is missing a lot of particles and does not contain antiparticles.
//    Moreover if I add anti-particles to the table the code that reads the tables
//    skips them.  On the other hand particle.tbl has all of the particles present
//    but it has crude values for the masses and widths.  This code first reads
//    particle.tbl to make sure that it has all of the particles; this will be the
//    permanent particle data table.  Then the code reads mass_width_2008.mc into
//    an auxillary table. For all particles that are present in both tables, the
//    code copies the masses and widths from the auxillary table to the permanent table.
//    Then the auxillary table goes out of scope.
//
#include <fstream>
#include <iomanip>

// Framework include files.
#include "messagefacility/MessageLogger/MessageLogger.h"
#include "cetlib/exception.h"

// Mu2e include files
#include "ConditionsService/inc/ParticleDataTable.hh"
#include "ConfigTools/inc/ConfigFileLookupPolicy.hh"
#include "ConfigTools/inc/SimpleConfig.hh"
#include "MCDataProducts/inc/PDGCode.hh"

// External include files.
#include "HepPDT/TableBuilder.hh"
#include "HepPDT/TempParticleData.hh"
#include "CLHEP/Units/SystemOfUnits.h"

using namespace std;

using HepPDT::ParticleData;
using HepPDT::ParticleID;
using HepPDT::Measurement;
using HepPDT::TempParticleData;
using CLHEP::GeV;
using CLHEP::second;

namespace mu2e {

  // Default construct an empty table and then fill it.
  ParticleDataTable::ParticleDataTable( std::string const& name,
                                        std::string const& tableFilename ):
    _pdt(name),
    _tableFilename(tableFilename),
    _unitsChanged(false){

    loadTableFromFile();
  }

  ParticleDataTable::ParticleDataTable( SimpleConfig const& config ):
    _pdt("Mu2eParticleData"),
    _unitsChanged(false){

    ConfigFileLookupPolicy findConfig;

    _tableFilename = findConfig(config.getString("particleDataTable.filename",
                                                 "ConditionsService/data/particle.tbl"));
  if( _tableFilename.empty() )
      throw "ParticleDataTable c'tor: find_file failure!";  // TODO: improve exception

  _auxillaryFilename = findConfig(config.getString("particleDataTable.auxillaryFilename",
                                                   "ConditionsService/data/mass_width_2008.mc"));
  if( _auxillaryFilename.empty() )
      throw "ParticleDataTable c'tor: find_file failure!";  // TODO: improve exception

    loadTableFromFile();
  }

  void ParticleDataTable::loadTableFromFile(){

    // See note 3.
    {
      // Construct the table builder.
      HepPDT::TableBuilder  tb(_pdt);

      // Build the table from the data file.
      ifstream in(_tableFilename.c_str());
      if ( !in ) {
        throw cet::exception("FILE")
          << "Unable to open particle data file: "
          << _tableFilename << "\n";
      }
      //HepPDT::addPDGParticles( in, tb);
      HepPDT::addParticleTable( in, tb, true);

    }

    improveData();

    // Make sure masses and widths are in MeV.
    changeUnits();

  }

  // Accessor by ID that checks that the requested particle exists in the table.
  ParticleDataTable::maybe_ref ParticleDataTable::particle( ParticleID id ) const{
    ParticleData const* p = _pdt.particle(id);
    return p ? maybe_ref(*p): maybe_ref();
  }

  // Accessor by name that checks that the requested particle exists in the table.
  ParticleDataTable::maybe_ref ParticleDataTable::particle( std::string const& name ) const{
    ParticleData const* p = _pdt.particle(name);
    return p ? maybe_ref(*p): maybe_ref();
  }

  void ParticleDataTable::changeUnits(){

    // Electron mass, in MeV.
    double eMassMeV = 0.510999;

    // Electron mass from the table.
    double eMass = particle(PDGCode::e_minus).ref().mass().value();

    // Ratio of two measures of the mass.
    double r = eMass/eMassMeV;
    double log10r = log10(r);

    // Tolerance on the ratio.
    double tolerance = 0.1;

    // Decide if the table is in MeV, GeV or something else?
    int units(0);
    if ( std::abs(log10r) < tolerance ){
      units = 1;
    } else if ( std::abs(log10r+3.) < tolerance ){
      units = 2;
    }

    if ( units == 0 ){
      mf::LogPrint("CONDITIONS")
        << "Did not recognize the units of masses in the particle data table.\n"
        << "The electron mass appears to be: "
        << eMass
        << "  Hope that's OK.\n";
    }

    // Units are GeV, so change them to MeV.
    if ( units == 2 ){
      _unitsChanged = true;

      mf::LogPrint("CONDITIONS")
        << "The HepPDT particle data table has masses in GeV. Changing to MeV.\n"
        << "  ( This leaves the lifetimes in a screwed up state: they are in kilo-seconds."
        << "    This does not affect Geant4 which has its own table of lifetimes. )";

      for ( HepPDT::ParticleDataTable::const_iterator i=_pdt.begin(), e=_pdt.end();
            i!=e; ++i ){

        // Get non-const reference to the particle data.  See Note 2.
        ParticleData& particle = *_pdt.particle(i->first.pid());

        // Extract properties with dimensions of mass.
        Measurement mass   = particle.mass();
        Measurement width  = particle.totalWidth();
        double lowerCutoff = particle.lowerCutoff();
        double upperCutoff = particle.upperCutoff();

        // Compute same quantities in new units.
        Measurement newmass (  mass.value()*GeV,  mass.sigma()*GeV );
        Measurement newwidth( width.value()*GeV, width.sigma()*GeV );

        // Reset the properties in the table.
        particle.setMass(newmass);
        particle.setTotalWidth(newwidth);
        particle.setLowerCutoff( lowerCutoff*GeV );
        particle.setUpperCutoff( upperCutoff*GeV );

      }
    }

  } // end changeUnits.


  // This is a temporary hack.  See note 4.
  void ParticleDataTable::improveData(){

    // Auxillary table that has better masses and widths but is missing anti-particles.
    HepPDT::ParticleDataTable auxTable;

    // Fill the auxillary table. See note 3.
    {
      // Construct the table builder.
      HepPDT::TableBuilder  tb(auxTable);

      // Build the table from the data file.
      ifstream in( _auxillaryFilename.c_str() );
      if ( !in ) {
        throw cet::exception("FILE")
          << "Unable to open auxillary particle data file: "
          << _auxillaryFilename << "\n";
      }
      HepPDT::addPDGParticles( in, tb);
    }

    // Copy masses and widths from the auxillary table to the permanent one.
    for ( HepPDT::ParticleDataTable::const_iterator i=_pdt.begin(), e=_pdt.end();
          i!=e; ++i ){

      // Get non-const access to each particle. See note 2.
      ParticleData* particle = _pdt.particle( i->first.pid() );

      ParticleData* tmp = auxTable.particle( std::abs(i->first.pid()) );
      if ( !tmp ) continue;

      // hadrons, leptons and gamma, Z, W, W-
      bool doit = particle->isHadron() ||  particle->isLepton() ||
        ( std::abs(particle->pid()) > 21 && std::abs(particle->pid()) < 25 );

      if ( doit ){
        particle->setMass(tmp->mass());
        particle->setTotalWidth(tmp->totalWidth());
      }

    }



  } // end improveData

}  // end namespace mu2e
