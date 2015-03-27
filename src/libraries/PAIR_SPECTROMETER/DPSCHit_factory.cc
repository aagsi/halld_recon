// $Id$
//
//    File: DPSCHit_factory.cc
// Created: Wed Oct 15 16:45:33 EDT 2014
// Creator: staylor (on Linux gluon05.jlab.org 2.6.32-358.18.1.el6.x86_64 x86_64)
//

#include <iostream>
#include <iomanip>
#include <cmath>
#include <vector>
#include <limits>
using namespace std;

#include "DPSCHit_factory.h"
#include "DPSCDigiHit.h"
#include "DPSCTDCDigiHit.h"
#include <DAQ/Df250PulseIntegral.h>
#include <DAQ/DCODAROCInfo.h>
using namespace jana;


//------------------
// init
//------------------
jerror_t DPSCHit_factory::init(void)
{
  DELTA_T_ADC_TDC_MAX = 4.0; // ns
  gPARMS->SetDefaultParameter("PSC:DELTA_T_ADC_TDC_MAX", DELTA_T_ADC_TDC_MAX,
			      "Maximum difference in ns between a (calibrated) fADC time and"
			      " F1TDC time for them to be matched in a single hit");
	
  /// set the base conversion scales
  a_scale    = 0.0001; 
  t_scale    = 0.0625;   // 62.5 ps/count
  t_base     = 0.;    // ns
  tdc_scale  = 0.060;    // 60 ps/count
	
  return NOERROR;
}

//------------------
// brun
//------------------
jerror_t DPSCHit_factory::brun(jana::JEventLoop *eventLoop, int runnumber)
{
  // Only print messages for one thread whenever run number change
  static pthread_mutex_t print_mutex = PTHREAD_MUTEX_INITIALIZER;
  static set<int> runs_announced;
  pthread_mutex_lock(&print_mutex);
  bool print_messages = false;
  if(runs_announced.find(runnumber) == runs_announced.end()){
    print_messages = true;
    runs_announced.insert(runnumber);
  }
  pthread_mutex_unlock(&print_mutex);	

  /// Read in calibration constants
  if(print_messages) jout << "In DPSCHit_factory, loading constants..." << endl;
	
  // F1TDC tframe(ns) and rollover count
  map<string,int> tdc_parms;
  tframe=-1;
  int rollover_count=0;
  if (eventLoop->GetCalib("/F1TDC/rollover",tdc_parms))
    jout << "Error loading /F1TDC/rollover !" <<endl;
  if (tdc_parms.find("tframe")!=tdc_parms.end())
    tframe=tdc_parms["tframe"];
  else 
    jerr << "Unable to get tframe from /F1TDC/rollover !" <<endl;
  if (tdc_parms.find("count")!=tdc_parms.end())
    rollover_count=tdc_parms["count"];
  else 
    jerr << "Unable to get rollover count from /F1TDC/rollover !" <<endl;

  // extract the PS Geometry
  vector<const DPSGeometry*> psGeomVect;
  eventLoop->Get( psGeomVect );
  if (psGeomVect.size() < 1)
    return OBJECT_NOT_AVAILABLE;
  const DPSGeometry& psGeom = *(psGeomVect[0]);

  // load scale factors
  map<string,double> scale_factors;
  if (eventLoop->GetCalib("/PHOTON_BEAM/pair_spectrometer/digi_scales", scale_factors))
    jout << "Error loading /PHOTON_BEAM/pair_spectrometer/digi_scales !" << endl;
  if (scale_factors.find("PSC_ADC_ASCALE") != scale_factors.end())
    a_scale = scale_factors["PSC_ADC_ASCALE"];
  else
    jerr << "Unable to get PSC_ADC_ASCALE from /PHOTON_BEAM/pair_spectrometer/digi_scales !" 
	 << endl;
  if (scale_factors.find("PSC_ADC_TSCALE") != scale_factors.end())
    t_scale = scale_factors["PSC_ADC_TSCALE"];
  else
    jerr << "Unable to get PSC_ADC_TSCALE from /PHOTON_BEAM/pair_spectrometer/digi_scales !" 
	 << endl;
  if (scale_factors.find("PSC_TDC_SCALE") != scale_factors.end())
    tdc_scale = scale_factors["PSC_TDC_SCALE"];
  else
    jerr << "Unable to get PSC_TDC_SCALE from /PHOTON_BEAM/pair_spectrometer/digi_scales !" 
	 << endl;

  // load base time offset
  map<string,double> base_time_offset;
  if (eventLoop->GetCalib("/PHOTON_BEAM/pair_spectrometer/base_time_offset",base_time_offset))
    jout << "Error loading /PHOTON_BEAM/pair_spectrometer/base_time_offset !" << endl;
  if (base_time_offset.find("PS_COARSE_BASE_TIME_OFFSET") != base_time_offset.end())
    t_base = base_time_offset["PS_COARSE_BASE_TIME_OFFSET"];
  else
    jerr << "Unable to get PS_COARSE_BASE_TIME_OFFSET from /PHOTON_BEAM/pair_spectrometer/base_time_offset !" << endl;

  // tdc_scale
  // By default we will use the F1TDC entries in the ccdb to find tdc_scale
  if (tframe>0 && rollover_count>0){ 
    tdc_scale=double(tframe)/double(rollover_count);
  }
  else {
    jerr << "Unable to get TDC_SCALE from database !" 
	 << endl;
  }
  if (runnumber==0) { // use hard-coded values when runnumber is 0
    tframe = 3744;
    rollover_count = 64466; 
    tdc_scale = double(tframe)/double(rollover_count);
  }
  if(print_messages) jout << "TDC scale = " << tdc_scale << " ns/count" << endl;

  /// Read in calibration constants
  vector<double> raw_adc_pedestals;
  vector<double> raw_adc_gains;
  vector<double> raw_adc_offsets;
  vector<double> raw_tdc_offsets;

  // load constant tables
  if(eventLoop->GetCalib("/PHOTON_BEAM/pair_spectrometer/coarse/adc_pedestals", raw_adc_pedestals))
    jout << "Error loading /PHOTON_BEAM/pair_spectrometer/coarse/adc_pedestals !" << endl;
  if(eventLoop->GetCalib("/PHOTON_BEAM/pair_spectrometer/coarse/adc_gain_factors", raw_adc_gains))
    jout << "Error loading /PHOTON_BEAM/pair_spectrometer/coarse/adc_gain_factors !" << endl;
  if(eventLoop->GetCalib("/PHOTON_BEAM/pair_spectrometer/coarse/adc_timing_offsets", raw_adc_offsets))
    jout << "Error loading /PHOTON_BEAM/pair_spectrometer/coarse/adc_timing_offsets !" << endl;
  if(eventLoop->GetCalib("/PHOTON_BEAM/pair_spectrometer/coarse/tdc_timing_offsets", raw_tdc_offsets))
    jout << "Error loading /PHOTON_BEAM/pair_spectrometer/coarse/tdc_timing_offsets !" << endl;


  FillCalibTable(adc_pedestals, raw_adc_pedestals, psGeom);
  FillCalibTable(adc_gains, raw_adc_gains, psGeom);
  FillCalibTable(adc_time_offsets, raw_adc_offsets, psGeom);
  FillCalibTable(tdc_time_offsets, raw_tdc_offsets, psGeom);

  return NOERROR;
}

//------------------
// evnt
//------------------
jerror_t DPSCHit_factory::evnt(JEventLoop *loop, int eventnumber)
{
  /// Generate DPSCHit object for each DPSCDigiHit object.
  /// This is where the first set of calibration constants
  /// is applied to convert from digitzed units into natural
  /// units.
  ///
  /// Note that this code does NOT get called for simulated
  /// data in HDDM format. The HDDM event source will copy
  /// the precalibrated values directly into the _data vector.

  // extract the PS Geometry
  vector<const DPSGeometry*> psGeomVect;
  eventLoop->Get(psGeomVect);
  if (psGeomVect.size() < 1)
    return OBJECT_NOT_AVAILABLE;
  const DPSGeometry& psGeom = *(psGeomVect[0]);

  // First, make hits out of all fADC250 hits
  vector<const DPSCDigiHit*> digihits;
  loop->Get(digihits);
  char str[256];

  for (unsigned int i=0; i < digihits.size(); i++){
    const DPSCDigiHit *digihit = digihits[i];

    DPSCHit *hit = new DPSCHit;

    // Make sure channel id is in valid range
    const int PSC_MAX_CHANNELS = psGeom.NUM_COARSE_COLUMNS*psGeom.NUM_ARMS;
    if( (digihit->counter_id <= 0) && (digihit->counter_id > PSC_MAX_CHANNELS)) {
      sprintf(str, "DPSCDigiHit sector out of range! sector=%d (should be 1-%d)", 
	      digihit->counter_id, PSC_MAX_CHANNELS);
      throw JException(str);
    }
		
    // The translation table has PSC channels labaled as paddles 1-16
    // The PSCHit class labels hits as
    //   arm:     North/South (0/1)
    //   module:  1-8
    if( digihit->counter_id <= psGeom.NUM_COARSE_COLUMNS ) {
      hit->arm     = DPSGeometry::kNorth;
      hit->module  = digihit->counter_id;
    } else {
      hit->arm     = DPSGeometry::kSouth;
      hit->module  = digihit->counter_id - psGeom.NUM_COARSE_COLUMNS;
    }

    // Get pedestal, prefer associated event pedestal if it exists,
    // otherwise, use the average pedestal from CCDB
    double pedestal = GetConstant(adc_pedestals,digihit,psGeom);
    const Df250PulseIntegral* PIobj = NULL;
    digihit->GetSingle(PIobj);
    if (PIobj != NULL) {
      // the measured pedestal must be scaled by the ratio of the number
      // of samples used to calculate the integral and the pedestal          
      // Changed to conform to D. Lawrence changes Dec. 4 2014
      double ped_sum = (double)PIobj->pedestal;
      double nsamples_integral = (double)PIobj->nsamples_integral;
      double nsamples_pedestal = (double)PIobj->nsamples_pedestal;
      pedestal          = ped_sum * nsamples_integral/nsamples_pedestal;
    }

    // Apply calibration constants here
    double A = (double)digihit->pulse_integral;
    A -= pedestal;
    double T = (double)digihit->pulse_time;

    hit->integral = A;
    hit->npe_fadc = A * a_scale * GetConstant(adc_gains, digihit, psGeom);
    hit->time_fadc = t_scale * (T - GetConstant(adc_time_offsets, digihit, psGeom)) + t_base;
    hit->t = hit->time_fadc;
    hit->time_tdc = numeric_limits<double>::quiet_NaN();
    hit->has_fADC = true;
    hit->has_TDC  = false; // will get set to true below if appropriate

    hit->AddAssociatedObject(digihit);
                
    _data.push_back(hit);
  }

  // Second, loop over TDC hits, matching them to the
  // existing fADC hits where possible and updating
  // their time information. If no match is found, then
  // create a new hit with just the TDC info.
  vector<const DPSCTDCDigiHit*> tdcdigihits;
  loop->Get(tdcdigihits);
  vector<const DCODAROCInfo*> codarocinfos;
  loop->Get(codarocinfos);

  int tref = 0;
  for(unsigned int i=0;i<codarocinfos.size();i++) {
      if(codarocinfos[i]->rocid==95) {
	  uint64_t cycles = (uint64_t)(4*codarocinfos[i]->timestamp/tframe);
	  tref=4*codarocinfos[i]->timestamp - cycles*tframe; // in ns
	  break;
      }
  }
  if (tref > 0) {
    for(unsigned int i=0; i<tdcdigihits.size(); i++) {
      const DPSCTDCDigiHit *digihit = tdcdigihits[i];
	    
      // calculate geometry information as described above
      DPSGeometry::Arm arm;
      int module = -1;
      if( digihit->counter_id <= psGeom.NUM_COARSE_COLUMNS ) {
	arm     = DPSGeometry::kNorth;
	module  = digihit->counter_id;
      } else {
	arm     = DPSGeometry::kSouth;
	module  = digihit->counter_id - psGeom.NUM_COARSE_COLUMNS;
      }

      // Take care of rollover
      double tdiff = tdc_scale*digihit->time - tref; // in ns
      if (tdiff < 0) tdiff += tframe;
      else if (tdiff > tframe) tdiff -= tframe;
      // Apply calibration constants here
      double T = tdiff - tdc_scale*GetConstant(tdc_time_offsets, digihit, psGeom) + t_base; 
      // Look for existing hits to see if there is a match
      // or create new one if there is no match
      DPSCHit *hit = FindMatch(arm, module, T);
      if (!hit) {
	hit = new DPSCHit;
	hit->arm    = arm;
	hit->module = module;
	hit->time_fadc = numeric_limits<double>::quiet_NaN();
	hit->integral = numeric_limits<double>::quiet_NaN();
	hit->npe_fadc = numeric_limits<double>::quiet_NaN();
	hit->has_fADC = false;
	_data.push_back(hit);
      }
      hit->time_tdc = T;
      hit->has_TDC = true;
      // apply time-walk corrections?
      hit->t = T;
                
      hit->AddAssociatedObject(digihit);
    }
  }

  return NOERROR;
}

//------------------
// FindMatch
//------------------
DPSCHit* DPSCHit_factory::FindMatch(DPSGeometry::Arm arm, int module, double T)
{
  DPSCHit* best_match = NULL;
	
  // Loop over existing hits (from fADC) and look for a match
  // in both the sector and the time.
  for(unsigned int i=0; i<_data.size(); i++) {
    DPSCHit *hit = _data[i];
                
    if(!hit->has_fADC) continue; // only match to fADC hits, not bachelor TDC hits
    if(hit->arm != arm) continue;
    if(hit->module != module) continue;
                
    //double delta_T = fabs(hit->t - T);
    double delta_T = fabs(T - hit->t);
    if(delta_T > DELTA_T_ADC_TDC_MAX) continue;

    return hit;
                
    // if there are multiple hits, pick the one that is closest in time
    if(best_match != NULL) {
      if(delta_T < fabs(best_match->t - T))
	best_match = hit;
    } else {
      best_match = hit;
    }

  }
        
  return best_match;
}


//------------------
// erun
//------------------
jerror_t DPSCHit_factory::erun(void)
{
  return NOERROR;
}

//------------------
// fini
//------------------
jerror_t DPSCHit_factory::fini(void)
{
  return NOERROR;
}

//------------------
// FillCalibTable
//------------------
void DPSCHit_factory::FillCalibTable(psc_digi_constants_t &table, vector<double> &raw_table, 
                                     const DPSGeometry &psGeom)
{
  char str[256];
  const int PSC_MAX_CHANNELS = psGeom.NUM_COARSE_COLUMNS*psGeom.NUM_ARMS;
  int channel = 0;
 
  // reset the table before filling it
  table.clear();

  // initialize table
  for(int column=0; column<psGeom.NUM_COARSE_COLUMNS; column++)
    table.push_back( pair<double,double>() );

  // the constants are stored in the CCDB as a 1D array
  // with the first 8 channels being the north arm,
  // and the second 8 channels being the south arm
  for(int column=0; column<psGeom.NUM_COARSE_COLUMNS; column++) {
    if( column+psGeom.NUM_COARSE_COLUMNS > PSC_MAX_CHANNELS) {  // sanity check
      sprintf(str, "Too many channels for PSC table! channel=%d (should be %d)", 
	      channel, PSC_MAX_CHANNELS);
      cerr << str << endl;
      throw JException(str);
    }

    table[column] = pair<double,double>(raw_table[channel],raw_table[channel+psGeom.NUM_COARSE_COLUMNS]);
    channel += 2;
  }

  // check to make sure that we loaded enough channels
  if(channel != PSC_MAX_CHANNELS) { 
    sprintf(str, "Not enough channels for PSC table! channel=%d (should be %d)", 
	    channel, PSC_MAX_CHANNELS);
    cerr << str << endl;
    throw JException(str);
  }
}

//------------------------------------
// GetConstant
//   Allow a few different interfaces
//
//   PSC Geometry as defined in the Translation Table:
//       arm:     North/South (0/1)
//       module:  1-8
//   Note the different counting schemes used
//------------------------------------
const double DPSCHit_factory::GetConstant( const psc_digi_constants_t &the_table, 
					   const DPSGeometry::Arm in_arm, const int in_module,
					   const DPSGeometry &psGeom ) const
{
  char str[256];
        
  if( (in_arm != DPSGeometry::kNorth) && (in_arm != DPSGeometry::kSouth)) {
    sprintf(str, "Bad arm requested in DPSCHit_factory::GetConstant()! requested=%d , should be 0-%d", 
	    static_cast<int>(in_arm), static_cast<int>(DPSGeometry::kSouth));
    cerr << str << endl;
    throw JException(str);
  }
  if( (in_module <= 0) || (in_module > psGeom.NUM_COARSE_COLUMNS)) {
    sprintf(str, "Bad module # requested in DPSCHit_factory::GetConstant()! requested=%d , should be 1-%d", in_module, psGeom.NUM_COARSE_COLUMNS);
    cerr << str << endl;
    throw JException(str);
  }

  // the tables are indexed by module, with the different values for the two arms
  // stored in the two fields of the pair
  if(in_arm == DPSGeometry::kNorth) {
    return the_table[in_module-1].first;
  } else {
    return the_table[in_module-1].second;
  }
}

const double DPSCHit_factory::GetConstant( const psc_digi_constants_t &the_table, 
					   const DPSCHit *in_hit, const DPSGeometry &psGeom ) const
{
  char str[256];
        
  if( (in_hit->arm != DPSGeometry::kNorth) && (in_hit->arm != DPSGeometry::kSouth)) {
    sprintf(str, "Bad arm requested in DPSCHit_factory::GetConstant()! requested=%d , should be 0-%d", 
	    static_cast<int>(in_hit->arm), static_cast<int>(DPSGeometry::kSouth));
    cerr << str << endl;
    throw JException(str);
  }
  if( (in_hit->module <= 0) || (in_hit->module > psGeom.NUM_COARSE_COLUMNS)) {
    sprintf(str, "Bad module # requested in DPSCHit_factory::GetConstant()! requested=%d , should be 1-%d", in_hit->module, psGeom.NUM_COARSE_COLUMNS);
    cerr << str << endl;
    throw JException(str);
  }

  // the tables are indexed by module, with the different values for the two arms
  // stored in the two fields of the pair
  if(in_hit->arm == DPSGeometry::kNorth) {
    return the_table[in_hit->module-1].first;
  } else {
    return the_table[in_hit->module-1].second;
  }
}

const double DPSCHit_factory::GetConstant( const psc_digi_constants_t &the_table, 
					   const DPSCDigiHit *in_digihit, const DPSGeometry &psGeom) const
{
  char str[256];

  // calculate geometry information 
  DPSGeometry::Arm arm;
  int module = -1;
  if( in_digihit->counter_id <= psGeom.NUM_COARSE_COLUMNS ) {
    arm     = DPSGeometry::kNorth;
    module  = in_digihit->counter_id;
  } else {
    arm     = DPSGeometry::kSouth;
    module  = in_digihit->counter_id - psGeom.NUM_COARSE_COLUMNS;
  }
        
  if( (module <= 0) || (module > psGeom.NUM_COARSE_COLUMNS)) {
    sprintf(str, "Bad module # requested in DPSCHit_factory::GetConstant()! requested=%d , should be 1-%d", module, psGeom.NUM_COARSE_COLUMNS);
    cerr << str << endl;
    throw JException(str);
  }

  // the tables are indexed by module, with the different values for the two arms
  // stored in the two fields of the pair
  if(arm == DPSGeometry::kNorth) {
    return the_table[module-1].first;
  } else {
    return the_table[module-1].second;
  }
}

const double DPSCHit_factory::GetConstant( const psc_digi_constants_t &the_table, 
					   const DPSCTDCDigiHit *in_digihit, const DPSGeometry &psGeom ) const
{
  char str[256];
        
  // calculate geometry information 
  DPSGeometry::Arm arm;
  int module = -1;
  if( in_digihit->counter_id <= psGeom.NUM_COARSE_COLUMNS ) {
    arm     = DPSGeometry::kNorth;
    module  = in_digihit->counter_id;
  } else {
    arm     = DPSGeometry::kSouth;
    module  = in_digihit->counter_id - psGeom.NUM_COARSE_COLUMNS;
  }
        
  if( (module <= 0) || (module > psGeom.NUM_COARSE_COLUMNS)) {
    sprintf(str, "Bad module # requested in DPSCHit_factory::GetConstant()! requested=%d , should be 1-%d", module, psGeom.NUM_COARSE_COLUMNS);
    cerr << str << endl;
    throw JException(str);
  }

  // the tables are indexed by module, with the different values for the two arms
  // stored in the two fields of the pair
  if(arm == DPSGeometry::kNorth) {
    return the_table[module-1].first;
  } else {
    return the_table[module-1].second;
  }

}

