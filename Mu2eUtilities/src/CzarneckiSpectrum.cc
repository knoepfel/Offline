//
// Read the table with data about DIO spectrum, and
// merge the spectrum with the analytic expression
// in the endpoint region taken from Czarnecki spectrum
// Czarneckki et al 10.1103/PhysRevD.84.013006
//
// $Id: CzarneckiSpectrum.cc,v 1.7 2012/07/15 22:06:17 kutschke Exp $
// $Author: kutschke $
// $Date: 2012/07/15 22:06:17 $
//

#include "Mu2eUtilities/inc/CzarneckiSpectrum.hh"

#include "CLHEP/Units/PhysicalConstants.h"
#include "ConfigTools/inc/ConfigFileLookupPolicy.hh"
#include "MCDataProducts/inc/PDGCode.hh"
#include "cetlib/pow.h"
#include <cmath>
#include <fstream>
#include <iostream>

using namespace std;

using cet::cube;
using cet::pow;
using cet::square;

namespace mu2e {

  CzarneckiSpectrum::CzarneckiSpectrum(int atomicZ):
  //atomic number of the foil material
    _znum ( atomicZ )
  {
    readTable();
    checkTable();
  }

  CzarneckiSpectrum::~CzarneckiSpectrum()
  {
  }

  double CzarneckiSpectrum::operator()(double E) {

    vector<Value>::iterator it = _table.begin();
    //    cout << "Searching for " << E << endl;
    while ((it != _table.end()) && (it->energy > E)) {
      //    cout << "In the table I have " << it->first << endl;
      it++;
    }

    if (it == _table.end()) {
      return 0;
    } 

    if (it->energy <= E) { 
      if ( it==_table.begin() || (it+1)==_table.end()) {
	return it->weight;
      } else {
	// cout << "Interpulating" << endl;
	return interpulate(E, (it+1)->energy, (it+1)->weight,
			   it->energy, it->weight,
			   (it-1)->energy, (it-1)->weight);
      }
    }
    return 0;
  }

  void CzarneckiSpectrum::readTable() {

    ConfigFileLookupPolicy findConfig;

    fstream intable(findConfig("ConditionsService/data/czarnecki.tbl").c_str(),ios::in);
    if (!(intable.is_open())) {
      throw cet::exception("ProductNotFound")
        << "No Tabulated spectrum table file found";
    }
    double en, prob;
    while (!(intable.eof())) {
      intable >> en >> prob;
      Value valueToAdd;
      valueToAdd.energy = en;
      valueToAdd.weight = prob;
      if (!(intable.eof())) {
	_table.push_back(valueToAdd);
      }
    }
  }
  
  void CzarneckiSpectrum::checkTable() {

    double valueToCompare = (_table.at(0).energy) + 1e9; 
    //order check
    for ( vector<Value>::iterator it =  _table.begin(); it != _table.end(); ++it) {
      if (it->energy >= valueToCompare) {
      throw cet::exception("Format")
        << "Wrong value in the czernacki table: " << it->energy;
      }
    }
    //    unsigned tablesize = _table.size();

  }


  /*  double CzarneckiSpectrum::FitCzarnecki(double E) {
      
  double delta = 105.194 - E - E*E/(2*25133);
  
  double valueIs = (8.6434e-17)*pow(delta,5) + (1.16874e-17)*pow(delta,6) 
  - (1.87828e-19)*pow(delta,7) + (9.16327e-20)*pow(delta,8);
  
  return valueIs;
  
  }*/ 
  //Maybe in a later step we might want to use the fit function (valid from 85 MeV on)

  double CzarneckiSpectrum::interpulate(double E, double e1, double p1,
                                        double e2, double p2, double e3, double p3) {
    
    double discr = e1*e1*e2 + e1*e3*e3 + e2*e2*e3 - e3*e3*e2 - e1*e1*e3 - e1*e2*e2;

    double A = (p1*e2 + p3*e1 + p2*e3 - p3*e2 - p1*e3 - p2*e1) / discr;

    double B = (e1*e1*p2 + e3*e3*p1 + e2*e2*p3 - e3*e3*p2 - e1*e1*p3 - e2*e2*p1) / discr;

    double C = (e1*e1*e2*p3 + e3*e3*e1*p2 + e2*e2*e3*p1 -
                e3*e3*e2*p1 - e1*e1*e3*p2 - e2*e2*e1*p3) / discr;

    return (A*E*E + B*E + C);

  }
  
}

