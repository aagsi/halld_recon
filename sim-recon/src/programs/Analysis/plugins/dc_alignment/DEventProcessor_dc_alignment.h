// $Id$
//
//    File: DEventProcessor_dc_alignment.h
// Created: Thu Oct 18 17:15:41 EDT 2012
// Creator: staylor (on Linux ifarm1102 2.6.18-274.3.1.el5 x86_64)
//

#ifndef _DEventProcessor_dc_alignment_
#define _DEventProcessor_dc_alignment_

#include <pthread.h>
#include <map>
#include <vector>
#include <deque>
using std::map;

#include <TTree.h>
#include <TFile.h>
#include <TH1.h>
#include <TH2.h>
#include <TH3.h>

#include <JANA/JFactory.h>
#include <JANA/JEventProcessor.h>
#include <JANA/JEventLoop.h>
#include <JANA/JCalibration.h>

#include <PID/DKinematicData.h>
#include <CDC/DCDCTrackHit.h>
#include <FDC/DFDCHit.h>
#include <FDC/DFDCGeometry.h>
#include <BCAL/DBCALShower.h>
#include <FCAL/DFCALShower.h>
#include <FDC/DFDCPseudo.h>
#include <FDC/DFDCIntersection.h>
#include <DMatrixSIMD.h>
#include <DVector2.h>

#include "FDC_branch.h"
#include "CDC_branch.h"

typedef struct{
  DMatrix4x1 S;
  DMatrix4x4 J;
  DMatrix4x1 Skk;
  DMatrix4x4 Ckk;
  double z,t;
  int num_hits;
  unsigned int h_id;
}trajectory_t;

typedef struct{
  DMatrix2x1 res;
  DMatrix4x1 S;
  DMatrix4x4 C;
  DMatrix2x2 R;
  DMatrix4x2 H_T;
  DMatrix2x4 H;
  double doca,t;
  double drift,drift_time;
}update_t;

typedef struct{
  double res,V;
  DMatrix4x1 S;
  DMatrix4x4 C;
  DMatrix4x1 H_T;
  DMatrix1x4 H;
  double doca,t,z;
  double drift_time,drift;
  bool used_in_fit;
}cdc_update_t;

typedef struct{
  unsigned int id;
  DMatrix4x1 S;
  DMatrix4x4 C; 
  DMatrix4x1 H_T;
  DMatrix1x4 H;
  double ures,doca;
  double R;
  double drift,drift_time,t;
}wire_update_t;

typedef struct{
  double xtrack,ytrack,ztrack;
  const DBCALShower *match;
}bcal_match_t;


typedef struct{
  DMatrix3x1 A;
  DMatrix3x3 E;  
}align_t;

typedef struct{
  DMatrix2x1 A;
  DMatrix2x2 E;
  DMatrix2x1 Au;
  DMatrix2x2 Eu;
  DMatrix2x1 Av;
  DMatrix2x2 Ev;
}wire_align_t;

typedef struct{
  DMatrix4x1 A;
  DMatrix4x4 E;
}cdc_align_t;


typedef struct{
  bool matched;
  DMatrix4x1 S;
  vector<const DFDCPseudo *>hits;
}segment_t;

typedef struct{
  bool matched;
  DMatrix4x1 S;
  vector<const DFDCIntersection*>hits;
}intersection_segment_t;


typedef struct{
  bool matched;
  DVector3 dir;
  vector<const DCDCTrackHit *>hits;
}cdc_segment_t;

typedef struct{
  DVector3 dir;
  vector<const DCDCTrackHit *>axial_hits;
  vector<const DCDCTrackHit *>stereo_hits;
}cdc_track_t;

typedef struct{
  const DFDCWire *wire;
  const DFDCHit *hit;
}intersection_hit_t;

#define EPS 1e-3
#define ITER_MAX 20
#define ADJACENT_MATCH_RADIUS 1.0
#define MATCH_RADIUS 20.0
#define CDC_MATCH_RADIUS 5.0

class DEventProcessor_dc_alignment:public jana::JEventProcessor{
 public:
  DEventProcessor_dc_alignment();
  ~DEventProcessor_dc_alignment();
  const char* className(void){return "DEventProcessor_dc_alignment";}

  TDirectory *dir;
  TTree *fdctree;
  FDC_branch fdc;
  FDC_branch *fdc_ptr;
  TBranch *fdcbranch;

  TTree *cdctree;
  CDC_branch cdc;
  CDC_branch *cdc_ptr;
  TBranch *cdcbranch;

  enum track_type{
    kWireBased,
    kTimeBased,
  };
  enum state_vector{
    state_x,
    state_y,
    state_tx,
    state_ty,
  };

  enum fdc_align_parms{
    kPhi,
    kU,
  };

  enum align_parms{
    kDx,
    kDy,
    kDPhi,
  };
  enum cdc_align_parms{
    dX,
    dY,
    dVx,
    dVy,
  };
  enum cdc_align_parms2{
    k_dXu,
    k_dYu,
    k_dXd,
    k_dYd,
  };

  
 private:
  jerror_t init(void);						///< Called once at program start.
  jerror_t brun(jana::JEventLoop *eventLoop, int runnumber);	///< Called everytime a new run number is detected.
  jerror_t evnt(jana::JEventLoop *eventLoop, int eventnumber);	///< Called every event.
  jerror_t erun(void);						///< Called everytime run number changes, provided brun has been called.
  jerror_t fini(void);						///< Called after last event of last event source has been processed.
  
  DMatrix4x1 FitLine(vector<const DFDCPseudo*> &fdchits);
  DMatrix4x1 FitLine(vector<const DFDCPseudo*> &fdchits,
		     double &var_x,double &cov_x_tx,
		     double &var_tx,double &chi2x,
		     double &var_y,double &cov_y_ty,
		     double &var_ty,double &chi2y);
  DMatrix4x1 FitLine(vector<const DFDCIntersection*> &fdchits);

  DMatrix4x1 GuessForStateVector(const cdc_track_t &track,double x,double y);

  jerror_t DoFilter(DMatrix4x1 &S,vector<const DFDCPseudo*> &fdchits);
  jerror_t DoFilter(DMatrix4x1 &S,vector<const DCDCTrackHit *>&hits);
  jerror_t DoFilter(DMatrix4x1 &S,vector<const DFDCIntersection *>&intersections);

  jerror_t KalmanFilter(double anneal_factor,
			DMatrix4x1 &S,DMatrix4x4 &C,
			vector<const DFDCPseudo *>&hits,
			deque<trajectory_t>&trajectory,
			vector<update_t>&updates,
			double &chi2,unsigned int &ndof);	
  jerror_t KalmanFilter(double anneal_factor,
			DMatrix4x1 &S,DMatrix4x4 &C,
			vector<const DCDCTrackHit *>&hits,
			deque<trajectory_t>&trajectory,
			vector<cdc_update_t>&updates,
			double &chi2,unsigned int &ndof,
			bool timebased=false);
  jerror_t KalmanFilter(double anneal_factor,
			DMatrix4x1 &S,DMatrix4x4 &C,
			vector<intersection_hit_t>&hits,
			deque<trajectory_t>&trajectory,
			vector<wire_update_t>&updates,
			double &chi2,unsigned int &ndof);

  jerror_t Smooth(DMatrix4x1 &Ss,DMatrix4x4 &Cs,
		  deque<trajectory_t>&trajectory,
		  vector<const DFDCPseudo *>&hits,
		  vector<update_t>updates,
		  vector<update_t>&smoothed_updates);
  jerror_t Smooth(DMatrix4x1 &Ss,DMatrix4x4 &Cs,
		  deque<trajectory_t>&trajectory,
		  vector<const DCDCTrackHit *>&hits,
		  vector<cdc_update_t>&updates,
		  vector<cdc_update_t>&smoothed_updates);
  jerror_t Smooth(DMatrix4x1 &Ss,DMatrix4x4 &Cs,
		  deque<trajectory_t>&trajectory,
		  vector<intersection_hit_t>&hits,
		  vector<wire_update_t>updates,
		  vector<wire_update_t>&smoothed_updates);

  jerror_t SetReferenceTrajectory(double z,DMatrix4x1 &S,
				  deque<trajectory_t>&trajectory,
				  vector<const DFDCPseudo *>&wires);
  jerror_t SetReferenceTrajectory(double z,DMatrix4x1 &S,
				  deque<trajectory_t>&trajectory,
				  const DCDCTrackHit *last_cdc); 
  jerror_t SetReferenceTrajectory(double z,DMatrix4x1 &S,
				  deque<trajectory_t>&trajectory,
				  vector<intersection_hit_t>&hits);

  jerror_t FindSegments(vector<const DFDCIntersection*>&points,
			vector<intersection_segment_t>&segments);  

  jerror_t FindSegments(vector<const DFDCPseudo*>&pseudos,
			vector<segment_t>&segments);
  jerror_t FindSegments(vector<const DCDCTrackHit*>&hits,
			vector<cdc_segment_t>&segments);
  jerror_t LinkSegments(vector<cdc_segment_t>&axial_segments,
			vector<cdc_segment_t>&stereo_segments,
			vector<cdc_track_t>&LinkedSegments);
  jerror_t LinkSegments(vector<segment_t>segments[4], 
			vector<vector<const DFDCPseudo *> >&LinkedSegments);
  jerror_t LinkSegments(vector<intersection_segment_t>segments[4], 
			vector<vector<const DFDCIntersection*> >&LinkedSegments);
  jerror_t FindOffsets(vector<const DFDCPseudo *>&hits,
		       vector<update_t>&smoothed_updates);
  jerror_t FindOffsets(vector<const DCDCTrackHit*>&hits,
		       vector<cdc_update_t>&updates);  
  jerror_t FindOffsets(vector<intersection_hit_t>&hits,
		       vector<wire_update_t>&smoothed_updates);

  bool MatchOuterDetectors(vector<const DFCALShower *>&fcalshowers,
			   vector<const DBCALShower *>&bcalshowers,
			   const DMatrix4x1 &S);
  bool MatchOuterDetectors(const cdc_track_t &track,
			   vector<const DFCALShower *>&fcalshowers,
			   vector<const DBCALShower *>&bcalshowers,
			   DMatrix4x1 &S);
  jerror_t EstimateT0(vector<update_t>&updates,
		      vector<const DFDCPseudo*>&hits);
  
  double cdc_variance(double t);
  double cdc_drift_distance(double t);
  double FindDoca(double z,const DMatrix4x1 &S,const DVector3 &vhat,
		  const DVector3 &origin);

  jerror_t GetProcessNoise(double dz,
			   double chi2c_factor,
			   double chi2a_factor,
			   double chi2a_corr,
			   const DMatrix4x1 &S,
			   DMatrix4x4 &Q);

  double GetDriftDistance(double t);
  double GetDriftVariance(double t);
  void UpdateWireOriginAndDir(unsigned int ring,unsigned int straw,
			      DVector3 &origin,DVector3 &wdir);
  void ComputeGMatrices(double s,double t,double scale,
			double tx,double ty,double tdir2,double one_over_d,
			double wx,double wy,double wdir2,
			double tdir_dot_wdir,
			double tdir_dot_diff,
			double wdir_dot_diff,
			double dx0,double dy0,
			double diffx,double diffy,
			double diffz,
			DMatrix1x4 &G,DMatrix4x1 &G_T);
    
  pthread_mutex_t mutex;

  TH1F *Hprob,*Hprelimprob,*Hbeta,*HdEdx,*Hmatch,*Hcdc_match;
  TH1F *Hintersection_match;
  TH1F *Hcdc_prob,*Hcdc_prelimprob;
  TH2F *Hbcalmatch,*Hcdcdrift_time;
  TH2F *Hures_vs_layer,*HdEdx_vs_beta;	
  TH2F *Hdrift_time,*Hcdcres_vs_drift_time;
  TH2F *Hres_vs_drift_time,*Hvres_vs_layer;
  TH2F *Hdv_vs_dE,*Hbcalmatchxy;
  TH1F *Hztarg;

  double mT0;
  double target_to_fcal_distance;
  double fdc_drift_table[140];
  DMatrix4x1 Zero4x1;
  DMatrix4x4 Zero4x4;

  double one_over_zrange;
  double endplate_z;
  int myevt;

  double mMinTime,mOuterTime,mOuterZ,mBeta;
  unsigned int mMinTimeID;
  
  bool COSMICS,USE_DRIFT_TIMES,READ_LOCAL_FILE;

  // Geometry
  const DGeometry *dgeom;

  vector<align_t>alignments;
  vector<vector<cdc_align_t> >cdc_alignments;
  vector<wire_align_t>fdc_alignments;
  vector<vector<DFDCWire*> >fdcwires;
  DMatrix3x1 fdc_drift_parms;
};


// Smearing function derived from fitting residuals
inline double DEventProcessor_dc_alignment::cdc_variance(double t){ 
  //return CDC_VARIANCE;
  if (t<0.0) t=0.0;
  
  double sigma=0.14/(t+4.9)+0.0068+0.39e-5*t;
  //sigma+=0.02;
  return sigma*sigma;
}

// Convert time to distance for the cdc
inline double DEventProcessor_dc_alignment::cdc_drift_distance(double t){
  double d=0.017;
  if (t>0.0) d+=0.030*sqrt(t);
  return d;
}

#endif // _DEventProcessor_dc_alignment_

