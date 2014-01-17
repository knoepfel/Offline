#ifndef TrackerMC_StrawElectronics_hh
#define TrackerMC_StrawElectronics_hh
//
// StrawElectronics collects the electronics response behavior of a Mu2e straw in
// several functions and parameters
//
// $Id: StrawElectronics.hh,v 1.9 2014/01/17 19:51:50 brownd Exp $
// $Author: brownd $
// $Date: 2014/01/17 19:51:50 $
//
// Original author David Brown, LBNL
//

// C++ includes
#include <iostream>
#include <array>
#include <vector>
#include <utility>

// Mu2e includes
#include "DataProducts/inc/StrawIndex.hh"
#include "RecoDataProducts/inc/StrawEnd.hh"
#include "RecoDataProducts/inc/StrawDigi.hh"
#include "TrackerMC/inc/StrawHitlet.hh"
#include "fhiclcpp/ParameterSet.h"
#include <array>
#include <vector>

namespace mu2e {
  class StrawElectronics {
    public:
      // construct from parameters
      StrawElectronics(fhicl::ParameterSet const& pset);
      virtual ~StrawElectronics();
      // analog response to a single hitlet.  Note that this does NOT include saturation effects,
      // since those are cumulative and cannot be computed per-hitlet
      double hitletResponse(double time,StrawHitlet const& hitlet) const; // mvolts per pCoulomb
      // Given a (linear) total voltage, compute the saturated voltage
      double saturatedResponse(double lineearresponse) const;
      // relative time when hitlet response is maximal
      double maxResponseTime() const { return _tshape; }
  // digization
      unsigned short adcResponse(double mvolts) const; // ADC response to analog inputs
      unsigned long tdcResponse(double time) const; // TDC response to a given time
      void digitizeWaveform(std::vector<double> const& wf,StrawDigi::ADCWaveform& adc) const;
      void digitizeTimes(std::array<double,2> const& times,StrawDigi::TDCValues& tdc) const;
      bool combineEnds(double t1, double t2) const; // are times from 2 ends combined into a single digi?
  // interpretation of digital data
      void tdcTimes(StrawDigi::TDCValues const& tdc, std::array<double,2>& times) const;
      double adcVoltage(unsigned short adcval) const; // mVolts
      double adcCurrent(unsigned short adcval) const; // microAmps
// accessors
      double adcLSB() const { return _ADCLSB; } //LSB in mvolts
      double tdcLSB() const { return _TDCLSB; } //LSB in nseconds
      unsigned short maxADC() const { return _maxADC; }
      unsigned long maxTDC() const { return _maxTDC; }
      unsigned short ADCPedestal() const { return _ADCped; };
      size_t nADCSamples() const { return _nADC; }
      size_t nADCPreSamples() const { return _nADCpre; }
      double adcPeriod() const { return _ADCPeriod; } // period of ADC clock in nsec
      double adcOffset() const { return _ADCOffset; } // offset WRT clock edge for digitization
      unsigned maxDTDC() const { return _maxDTDC; } // maximum TDC difference between ends
      double flashStart() const { return _flashStart; } // time flash blanking starts
      double flashEnd() const { return _flashEnd; } // time flash blanking ends
      void adcTimes(double time, std::vector<double>& adctimes) const; // sampling times of ADC
      double saturationVoltage() const { return _vsat; }
      double maximumVoltage() const { return _vmax; }
      double shapingTime() const { return _tshape; }
      double shapingPower() const { return _tpow; }
      double dispersion(double dlen) const { return _disp*dlen; } // dispersion width is linear in propagation length
      double threshold() const { return _vthresh; }
      double thresholdNoise() const { return _vthreshnoise; }
      double deadTime() const { return _tdead; }
      double clockStart() const { return _clockStart; }
      double clockJitter() const { return _clockJitter; }
    private:
      // scale factor between charge and voltage (milliVolts from picoCoulombs)
      double _dVdI;
      double _tshape, _tpow; // Shaping time and power.  These should depend on hitlet type, FIXME!
      double _teff; // effective shaping time
      double _tdead; // electronics dead time
      // scale factor between current and voltage (milliVolts per microAmps)
      double _norm; // normalization factor, computed from trise and tfall
      double _vmax, _vsat; // saturation parameters.  _vmax is maximum output, _vsat is where saturation starts
      double _disp; // dispersion in ns/mm;
      double _vthresh; // threshold voltage for electronics discriminator (mVolt)
      double _vthreshnoise; // threshold voltage noise width (mVolt)
// add some noise parameter: FIXME!!!
      double _ADCLSB; // least-significant bit of ADC (mVolts)
      unsigned short _maxADC; // maximum ADC value
      unsigned short _ADCped; // ADC pedestal (reading for 0 volts)
      size_t _nADC,_nADCpre; // Number of ADC samples, presamples
      double _ADCPeriod; // ADC period in nsec
      double _ADCOffset; // Offset of 1st ADC sample WRT threshold crossing (nsec)
      double _TDCLSB; // least-significant bit of TDC (nsecs)
      unsigned long _maxTDC; // maximum TDC value
      unsigned _maxDTDC; // maximum TDC difference for digitizer to combine to hits in different ends to the same straw digi
      double _clockStart, _clockJitter; // time TDC clock starts, and its error (common to both ends!!)
      double _flashStart, _flashEnd; // flash blanking period (no digitizations during this time!!!)
  };
}

#endif

