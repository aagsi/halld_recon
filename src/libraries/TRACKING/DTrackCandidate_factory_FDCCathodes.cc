// $Id$
//
//    File: DTrackCandidate_factory_FDCCathodes.cc
// Created: Tue Nov  6 13:37:08 EST 2007
// Creator: staylor (on Linux ifarml1.jlab.org 2.4.21-47.0.1.ELsmp i686)
//
/// This factory links segments in the FDC packages into track candidates 
/// by swimming through the field from one package to the next.

#include "DTrackCandidate_factory_FDCCathodes.h"
#include "DANA/DApplication.h"
#include "FDC/DFDCPseudo_factory.h"
#include "FDC/DFDCSegment_factory.h"
#include "DHelicalFit.h"
#include <TROOT.h>
#include <TH1F.h>
#include <TH2F.h>

//#define MATCH_RADIUS(p) (2.79+2.88/(p)/(p))
#define MATCH_RADIUS(p) (2.75+2./(p+0.25)/(p+0.25))
#define MAX_SEGMENTS 20
#define HALF_PACKAGE 6.0
#define FDC_OUTER_RADIUS 50.0 
#define BEAM_VAR 1.0 // cm^2
#define HIT_CHI2_CUT 10.0

///
/// DTrackCandidate_factory_FDCCathodes::brun():
///
jerror_t DTrackCandidate_factory_FDCCathodes::brun(JEventLoop* eventLoop, 
						   int runnumber) {
  DApplication* dapp=dynamic_cast<DApplication*>(eventLoop->GetJApplication());
  bfield = dapp->GetBfield();
  const DGeometry *dgeom  = dapp->GetDGeometry(runnumber);
  
  USE_FDC=true;
  if (!dgeom->GetFDCZ(z_wires)){
    _DBG_<< "FDC geometry not available!" <<endl;
    USE_FDC=false;
  }

  // Get the position of the CDC downstream endplate from DGeometry
  double endplate_dz,endplate_rmin,endplate_rmax;
  dgeom->GetCDCEndplate(endplate_z,endplate_dz,endplate_rmin,endplate_rmax);
  endplate_z+=endplate_dz;
  dgeom->GetTargetZ(TARGET_Z);

  DEBUG_HISTS=false;
  gPARMS->SetDefaultParameter("TRKFIND:DEBUG_HISTS", DEBUG_HISTS);

  APPLY_MOMENTUM_CORRECTION=false;
  gPARMS->SetDefaultParameter("TRKFIND:APPLY_MOMENTUM_CORRECTION",APPLY_MOMENTUM_CORRECTION);
  p_factor1=1.61*M_PI/180.;
  p_factor2=-0.0766;

  string description = "If hit wih largest R is less than this, then a ";
  description += "fake point will be added when fitting the parameters ";
  description += "for the track candidate in the 'FDCCathodes' factory. ";
  description += "The point will be on the beamline (x,y) = (0,0) and ";
  description += "at a z location determined from the geometry center of ";
  description += "target (via DGeometry::GetTargetZ()";
  MAX_R_VERTEX_LIMIT = 10.0;
  gPARMS->SetDefaultParameter("TRKFIND:MAX_R_VERTEX_LIMIT", MAX_R_VERTEX_LIMIT, description);

  if(DEBUG_HISTS) {
    dapp->Lock();
    match_dist_fdc=(TH2F*)gROOT->FindObject("match_dist_fdc");
    if (!match_dist_fdc) 
      match_dist_fdc=new TH2F("match_dist_fdc",
		  "Matching distance for connecting FDC segments",
			      50,0.,7,100,0,25.);
    dapp->Unlock();
  }
    
  // Initialize the stepper
  stepper=new DMagneticFieldStepper(bfield);
  stepper->SetStepSize(1.0);

  return NOERROR;
}


//------------------
// erun
//------------------
jerror_t DTrackCandidate_factory_FDCCathodes::erun(void)
{
  if (stepper) delete stepper;
        return NOERROR;
}
//------------------
// fini
//------------------
jerror_t DTrackCandidate_factory_FDCCathodes::fini(void)
{
  
  if (stepper) delete stepper;
        return NOERROR;
}


// Local routine for sorting segments by charge and curvature
inline bool DTrackCandidate_segment_cmp(const DFDCSegment *a, const DFDCSegment *b){
  //  double k1=a->S(0,0),k2=b->S(0,0);
  //double q1=k1/fabs(k1),q2=k2/fabs(k2);
  //if (q1!=q2) return q1<q2;
  //return fabs(k1)<fabs(k2); 
  if (a->q!=b->q) return a->q<b->q;
  return a->rc>b->rc;
}

//------------------
// evnt:  main segment linking routine
//------------------
jerror_t DTrackCandidate_factory_FDCCathodes::evnt(JEventLoop *loop, int eventnumber)
{
  if (!USE_FDC) return NOERROR;

  vector<const DFDCSegment*>segments;
  eventLoop->Get(segments);
  // abort if there are no segments
  if (segments.size()==0.) return NOERROR;

  std::sort(segments.begin(), segments.end(), DTrackCandidate_segment_cmp);

  // Group segments by package
  vector<DFDCSegment*>package[4];
  
  for (unsigned int i=0;i<segments.size();i++){
     const DFDCSegment *segment=segments[i];
 package[(segment->hits[0]->wire->layer-1)/6].push_back((DFDCSegment*)segment);
  }
      
  double zpackage[4];  // z-positions of entrances to FDC packages 
  zpackage[0]=z_wires[0];
  zpackage[1]=z_wires[6];
  zpackage[2]=z_wires[12];
  zpackage[3]=z_wires[18];
  DFDCSegment *match2=NULL;
  DFDCSegment *match3=NULL;
  DFDCSegment *match4=NULL;
  unsigned int match_id=0;

  // Bail if there are too many segments
  /*
  if (package[0].size()+package[1].size()+package[2].size()
  +package[3].size()>MAX_SEGMENTS)
  return UNRECOVERABLE_ERROR; 
  */

  // First deal with tracks with segments in the first package
  vector<int>pack1_matched(package[0].size());
  if (package[0].size()>0){
    // Loop over segments in the first package, matching them to segments in 
    // the second, third, and fourth (most downstream) packages.
    for (unsigned int i=0;i<package[0].size();i++){
      DFDCSegment *segment=package[0][i];
      match2=match3=match4=NULL;

      // Tracking parameters from first segment
      tanl=segment->tanl;
      phi0=segment->phi0;
      z_vertex=segment->z_vertex;
      xc=segment->xc;
      yc=segment->yc;
      rc=segment->rc;
      // Sign of the charge
      q=segment->q;
      stepper->SetCharge(q);

      //double qsum=q;
      //unsigned int num_q=1;

      // Start filling vector of segments belonging to current track    
      vector<DFDCSegment*>mysegments; 
      mysegments.push_back(segment);
 
      // Try matching to package 2
      if (package[1].size()>0 && 
	  (match2=GetTrackMatch(zpackage[1],segment,package[1],match_id))
	  !=NULL){
	// Insert the segment from package 2 into the track 
	mysegments.push_back(match2);
	
	// remove the segment from the list 
	package[1].erase(package[1].begin()+match_id);

	//qsum+=match2->q;
	//num_q++;

	// Try matching to package 3
	if (package[2].size()>0 && 
	    (match3=GetTrackMatch(zpackage[2],match2,package[2],match_id))
	    !=NULL){
	  // Insert the segment from package 3 into the track 
	  mysegments.push_back(match3);

	  // remove the segment from the list 
	  package[2].erase(package[2].begin()+match_id);

	  //qsum+=match3->q;
	  //num_q++;
	  
	  // Try matching to package 4
	  if (package[3].size()>0 && 
	      (match4=GetTrackMatch(zpackage[3],match3,package[3],
	       match_id))!=NULL){
	    // Insert the segment from package 4 into the track 
	    mysegments.push_back(match4);

	    // remove the segment from the list 
	    package[3].erase(package[3].begin()+match_id);

	    //qsum+=match4->q;
	    //num_q++;
	  }
	}
	// No match in package 3, try for 4
	else if(package[3].size()>0 && 
		(match4=GetTrackMatch(zpackage[3],match2,package[3],
		 match_id))!=NULL){
	  // Insert the segment from package 4 into the track 
	  mysegments.push_back(match4);

	  // remove the segment from the list 
	  package[3].erase(package[3].begin()+match_id);

	  //qsum+=match4->q;
	  //num_q++;
	}
      }
      // No match in package 2, try for 3
      else if (package[2].size()>0 && 
	       (match3=GetTrackMatch(zpackage[2],segment,package[2],
		match_id))!=NULL){
	// Insert the segment from package 3 into the track
	mysegments.push_back(match3);

	// remove the segment from the list 
	package[2].erase(package[2].begin()+match_id);

	//qsum+=match3->q;
	//num_q++;

	// Try matching to package 4
	if (package[3].size()>0 && 
	    (match4=GetTrackMatch(zpackage[3],match3,package[3],
	     match_id))!=NULL){
	  // Insert the segment from package 4 into the track 
	  mysegments.push_back(match4);

	  // remove the segment from the list 
	  package[3].erase(package[3].begin()+match_id);

	  //qsum+=match4->q;
	  //num_q++;
	}
      }    
      // No match to package 2 or 3, try 4
      else if (package[3].size()>0 && 
	       (match4=GetTrackMatch(zpackage[3],segment,package[3],
		match_id))!=NULL){
	// Insert the segment from package 4 into the track 
	mysegments.push_back(match4);
	
	// remove the segment from the list 
	package[3].erase(package[3].begin()+match_id);

	//qsum+=match4->q;
	//num_q++;
      }

      //if (qsum>=0) q=1.;
      //else q=-1.;

      // Variables for determining average Bz
      double Bz_avg=0.;
      unsigned int num_hits=segment->hits.size();

      if (mysegments.size()>1){
	pack1_matched[i]=1;

	DHelicalFit fit;
	double max_r=0.;
	if (segment){ 
	  for (unsigned int n=0;n<segment->hits.size();n++){
	    const DFDCPseudo *hit=segment->hits[n];
	    fit.AddHit(hit);
	    Bz_avg-=bfield->GetBz(hit->xy.X(),hit->xy.Y(),
				  hit->wire->origin.z());
	    double r=hit->xy.Mod();
	    if (r>max_r) max_r=r;
	  }
	}
	if (match2){
	  for (unsigned int n=0;n<match2->hits.size();n++){
	    const DFDCPseudo *hit=match2->hits[n];
	    fit.AddHit(hit);
	    Bz_avg-=bfield->GetBz(hit->xy.X(),hit->xy.Y(),
				  hit->wire->origin.z());
	    double r=hit->xy.Mod();
	    if (r>max_r) max_r=r;
	  }
	  num_hits+=match2->hits.size();
	}
	if (match3){
	  for (unsigned int n=0;n<match3->hits.size();n++){
	    const DFDCPseudo *hit=match3->hits[n];
	    fit.AddHit(hit);
	    Bz_avg-=bfield->GetBz(hit->xy.X(),hit->xy.Y(),
				  hit->wire->origin.z());
	    double r=hit->xy.Mod();
	    if (r>max_r) max_r=r;
	  }
	  num_hits+=match3->hits.size();
	}
	if (match4){
	  for (unsigned int n=0;n<match4->hits.size();n++){
	    const DFDCPseudo *hit=match4->hits[n];
	    fit.AddHit(hit);
	    Bz_avg-=bfield->GetBz(hit->xy.X(),hit->xy.Y(),
				  hit->wire->origin.z());
	    double r=hit->xy.Mod();
	    if (r>max_r) max_r=r;
	  }
	  num_hits+=match4->hits.size();
	}
	// Fake point at origin
	if (max_r<MAX_R_VERTEX_LIMIT) fit.AddHitXYZ(0.,0.,TARGET_Z,BEAM_VAR,BEAM_VAR,0.);
	if (fit.FitCircleAndLineRiemann(mysegments[0]->rc)==NOERROR){      
	  // Charge
	  //if (q==0) 
	  //  q=fit.q;
	  mysegments[1]->q=q;
	  // Estimate for azimuthal angle
	  phi0=fit.phi; 
	  mysegments[1]->phi0=phi0;
	  // remaining tracking parameters
	  tanl=fit.tanl;
	  z_vertex=fit.z_vertex;
	
	  mysegments[1]->tanl=tanl;
	  mysegments[1]->z_vertex=z_vertex;
	  xc=mysegments[1]->xc=fit.x0;
	  yc=mysegments[1]->yc=fit.y0;
	  rc=mysegments[1]->rc=fit.r0;

	  //printf("Match %x %x %x %x\n",segment,match2,match3,match4);
	  //printf("xc %f yc %f rc %f\n",xc,yc,rc);
	  
	  // Try to match to package 2 again.
	  if (match2==NULL && package[1].size()>0 &&
	      (match2=GetTrackMatch(zpackage[1],mysegments[1],package[1],
				    match_id))!=NULL){ 
	    // Insert the segment from package 2 into the track
	    mysegments.push_back(match2);

	    // remove the segment from the list 
	    package[1].erase(package[1].begin()+match_id);

	    //    qsum+=match2->q;
	    //num_q++;

	    // Redo the fit with the additional hits from package 2
	    for (unsigned int n=0;n<match2->hits.size();n++){
	      const DFDCPseudo *hit=match2->hits[n];
	      fit.AddHit(hit);
	      Bz_avg-=bfield->GetBz(hit->xy.X(),hit->xy.Y(),
				    hit->wire->origin.z());
	    }
	    num_hits+=match2->hits.size();
	    
	    //if (qsum>=0) q=1.;
	    // else q=-1.;
	    
	    if (fit.FitCircleAndLineRiemann(mysegments[1]->rc)==NOERROR){     
	      // Charge
	      //if (q==0) 
	      //q=fit.q;
	      mysegments[1]->q=q;
	      // Estimate for azimuthal angle
	      phi0=fit.phi;
	      mysegments[1]->phi0=phi0;
	      // remaining tracking parameters
	      tanl=fit.tanl;
	      z_vertex=fit.z_vertex;
	
	      mysegments[1]->tanl=tanl;
	      mysegments[1]->z_vertex=z_vertex;
	      xc=mysegments[1]->xc=fit.x0;
	      yc=mysegments[1]->yc=fit.y0;
	      rc=mysegments[1]->rc=fit.r0;
	      
	      //printf("Match %x %x %x %x\n",segment,match2,match3,match4);
	      //printf("xc %f yc %f rc %f\n",xc,yc,rc);
	  
	    }
	  }
	  
	  // Try to match to package 3 again.
	  if (match3==NULL && package[2].size()>0 &&
	      (match3=GetTrackMatch(zpackage[2],mysegments[1],package[2],
				    match_id))!=NULL){
	    // Insert the segment from package 3 into the track
	    mysegments.push_back(match3);

	    // remove the segment from the list 
	    package[2].erase(package[2].begin()+match_id);
	    
	    //qsum+=match3->q;
	    //num_q++;

	    // Redo the fit with the additional hits from package 3
	    for (unsigned int n=0;n<match3->hits.size();n++){
	      const DFDCPseudo *hit=match3->hits[n];
	      fit.AddHit(hit);
	      Bz_avg-=bfield->GetBz(hit->xy.X(),hit->xy.Y(),
				    hit->wire->origin.z());
	    }
	    num_hits+=match3->hits.size();
	    
	    //if (qsum>=0) q=1.;
	    //else q=-1.;
	 
	    if (fit.FitCircleAndLineRiemann(mysegments[1]->rc)==NOERROR){     
	      // Charge
	      //if (q==0) 
	      //q=fit.q;
	      mysegments[2]->q=q;
	      // Estimate for azimuthal angle
	      phi0=fit.phi;
	      mysegments[2]->phi0=phi0;
	      // remaining tracking parameters
	      tanl=fit.tanl;
	      z_vertex=fit.z_vertex;
	
	      mysegments[2]->tanl=tanl;
	      mysegments[2]->z_vertex=z_vertex;
	      xc=mysegments[2]->xc=fit.x0;
	      yc=mysegments[2]->yc=fit.y0;
	      rc=mysegments[2]->rc=fit.r0;
	      
	      //printf("Match %x %x %x %x\n",segment,match2,match3,match4);
	      //printf("xc %f yc %f rc %f\n",xc,yc,rc);
	      
	      // If we failed to match to package 4, try again.
	      if (match4==NULL && package[3].size()>0 && 
		  (match4=GetTrackMatch(zpackage[3],mysegments[2],package[3],
					match_id))!=NULL){
		// Insert the segment from package 4 into the track
		mysegments.push_back(match4);

		// remove the segment from the list 
		package[3].erase(package[3].begin()+match_id); 
	      }
	    }
	    // Try to match to package 4 again.
	    if (match4==NULL && package[3].size()>0 &&
		(match4=GetTrackMatch(zpackage[3],mysegments[1],package[3],
				      match_id))!=NULL){ 
	      
	      // Insert the segment from package 4 into the track
	      mysegments.push_back(match4);

	      // remove the segment from the list
	      package[3].erase(package[3].begin()+match_id);
	    }
	  }
	}
      
	DVector3 mom,pos;
	Bz_avg/=double(num_hits);

	pos.SetX(segment->hits[0]->xy.X());
	pos.SetY(segment->hits[0]->xy.Y());
	pos.SetZ(segment->hits[0]->wire->origin.z());
	//GetPositionAndMomentum(Bz_avg,pos,mom);
	
	// Create new track, starting with the current segment
	DTrackCandidate *track = new DTrackCandidate;
	//track->setPosition(pos);
	//track->setMomentum(mom);

	if (match2){
	  q=GetCharge(pos,match2);
	}
	else if (match3){
	  q=GetCharge(pos,match3);
	}
	else if (match4){
	  q=GetCharge(pos,match4);
	}
		
	GetPositionAndMomentum(Bz_avg,pos,mom);
	
	// Empirical correction to the momentum
	if (APPLY_MOMENTUM_CORRECTION){
	  double p_mag=mom.Mag();
	  mom.SetMag(p_mag*(1.+p_factor1/mom.Theta()+p_factor2));
	}
	  
	track->setCharge(q);
	track->setPosition(pos);
	track->setMomentum(mom);

	for (unsigned int m=0;m<mysegments.size();m++)
	  track->AddAssociatedObject(mysegments[m]);
	
	_data.push_back(track); 
	
      }
    }
  }

  // Prune segments in package 1 that have been matched to other segments
  vector<DFDCSegment*>pack1_left_over;     
  for (unsigned int i=0;i<package[0].size();i++){
    if (pack1_matched[i]!=1) pack1_left_over.push_back(package[0][i]);
  }

  // Next try to link segments starting at package 2
  if (package[1].size()>0 ){
    // Loop over segments in the 2nd package, matching them to segments in 
    // the third and fourth (most downstream) packages.
    for (unsigned int i=0;i<package[1].size();i++){
      DFDCSegment *segment=package[1][i];
      
      // tracking parameters
      tanl=segment->tanl;
      phi0=segment->phi0; 
      z_vertex=segment->z_vertex;
      xc=segment->xc;
      yc=segment->yc;
      rc=segment->rc;
      // Sign of the charge
      q=segment->q;
      stepper->SetCharge(q);

      //double qsum=q;
      
      // Start filling vector of segments belonging to current track    
      vector<DFDCSegment*>mysegments; 
      mysegments.push_back(segment);

      // Try matching to package 3
      if (package[2].size()>0 && 
	  (match3=GetTrackMatch(zpackage[2],segment,package[2],match_id))
	  !=NULL){
	// Insert the segment from package 3 into the track
	mysegments.push_back(match3);

	// remove the segment from the list 
	package[2].erase(package[2].begin()+match_id);

	//qsum+=match3->q;

	// Try matching to package 4
	if (package[3].size()>0 && 
	    (match4=GetTrackMatch(zpackage[3],match3,package[3],
	     match_id))!=NULL){
	  // Insert the segment from package 4 into the track 
	  mysegments.push_back(match4);

	  // remove the segment from the list 
	  package[3].erase(package[3].begin()+match_id);

	  //qsum+=match4->q;	  
	}	
      }
      // No match in 3, try for 4
      else if (package[3].size()>0 && 
	       (match4=GetTrackMatch(zpackage[3],segment,package[3],
		match_id))!=NULL){
	// Insert the points in the segment from package 4 into the track 
	mysegments.push_back(match4);

	// remove the segment from the list 
	package[3].erase(package[3].begin()+match_id);

	//qsum+=match4->q;
      }
      
      //if (qsum>=0) q=1.;
      //else q=-1.;
 
      // Variables for determining average Bz
      double Bz_avg=0.;
      unsigned int num_hits=segment->hits.size();
 
      if (mysegments.size()>1){
	DHelicalFit fit;
	double max_r=0.;
	if (segment){ 
	  for (unsigned int n=0;n<segment->hits.size();n++){
	    const DFDCPseudo *hit=segment->hits[n];
	    fit.AddHit(hit);
	    Bz_avg-=bfield->GetBz(hit->xy.X(),hit->xy.Y(),
				  hit->wire->origin.z());
	    double r=hit->xy.Mod();
	    if (r>max_r) max_r=r;
	  }
	}
	if (match3){
	  for (unsigned int n=0;n<match3->hits.size();n++){
	    const DFDCPseudo *hit=match3->hits[n];
	    fit.AddHit(hit);
	    Bz_avg-=bfield->GetBz(hit->xy.X(),hit->xy.Y(),
				  hit->wire->origin.z()); 
	    double r=hit->xy.Mod();
	    if (r>max_r) max_r=r;	    
	  }
	  num_hits+=match3->hits.size();
	}
	if (match4){
	  for (unsigned int n=0;n<match4->hits.size();n++){ 
	    const DFDCPseudo *hit=match4->hits[n];
	    double covxx=2.*hit->covxx;
	    double covyy=2.*hit->covyy;
	    double covxy=2.*hit->covxy;
	    double x=hit->xy.X();
	    double y=hit->xy.Y();
	    double z=hit->wire->origin.z();
	    fit.AddHitXYZ(x,y,z,covxx,covyy,covxy);  
	    Bz_avg-=bfield->GetBz(x,y,z);

	    double r=hit->xy.Mod();
	    if (r>max_r) max_r=r;
	  }
	  num_hits+=match4->hits.size();
	}
	// Fake point at origin
	if (max_r<MAX_R_VERTEX_LIMIT) fit.AddHitXYZ(0.,0.,TARGET_Z,BEAM_VAR,BEAM_VAR,0.);
	if (fit.FitCircleAndLineRiemann(mysegments[0]->rc)==NOERROR){
	  // Charge
	  //if (q==0) 
	  //  q=fit.q;
	  mysegments[1]->q=q;
	  // Estimate for azimuthal angle
	  phi0=fit.phi;
	  mysegments[1]->phi0=phi0;
	  // remaining tracking parameters
	  tanl=fit.tanl;
	  z_vertex=fit.z_vertex;

	  mysegments[1]->tanl=tanl;
	  mysegments[1]->z_vertex=z_vertex;
	  xc=mysegments[1]->xc=fit.x0;
	  yc=mysegments[1]->yc=fit.y0;
	  rc=mysegments[1]->rc=fit.r0;
	  
	  // Try to match to package 4 again.
	  if (match4==NULL && package[3].size()>0 &&
	      (match4=GetTrackMatch(zpackage[3],mysegments[1],package[3],match_id))
	      !=NULL){
	    // Insert the segment from package 4 into the track
	    mysegments.push_back(match4);

	    // remove the segment from the list 
	    package[3].erase(package[3].begin()+match_id);
	  }

	  // Try to match to package 1 again	  
	  if (pack1_left_over.size()>0){
	    DFDCSegment *match1=NULL;
	    if ((match1=GetTrackMatch(zpackage[0],mysegments[1],
				      pack1_left_over,
				      match_id))!=NULL){
	      mysegments.push_back(match1);
	      pack1_left_over.erase(pack1_left_over.begin()+match_id);

	      // Refit with additional hits from package 1
	      for (unsigned int n=0;n<match1->hits.size();n++){
		const DFDCPseudo *hit=match1->hits[n];
		fit.AddHit(hit);
		Bz_avg-=bfield->GetBz(hit->xy.X(),hit->xy.Y(),
				      hit->wire->origin.z());
	      }
	      num_hits+=match1->hits.size();

	      if (fit.FitCircleAndLineRiemann(mysegments[1]->rc)==NOERROR){
		// Charge
		//q=fit.q;
		mysegments[1]->q=q;
		// Estimate for azimuthal angle
		phi0=fit.phi;
		mysegments[1]->phi0=phi0;
		// remaining tracking parameters
		tanl=fit.tanl;
		z_vertex=fit.z_vertex;
		
		mysegments[1]->tanl=tanl;
		mysegments[1]->z_vertex=z_vertex;
		xc=mysegments[1]->xc=fit.x0;
		yc=mysegments[1]->yc=fit.y0;
		rc=mysegments[1]->rc=fit.r0;
	      }
	    }
	  }
	}
      } // if mysegments.size()>1
      else{
	for (unsigned int n=0;n<segment->hits.size();n++){
	  Bz_avg-=bfield->GetBz(segment->hits[n]->xy.X(),
				segment->hits[n]->xy.Y(),
				segment->hits[n]->wire->origin.z());
	}
      }

      DVector3 mom,pos;
      Bz_avg/=double(num_hits);
      
      // Try to fix relatively high momentum tracks in the very forward 
      // direction that look like low momentum tracks due to small pt.
      // Assume that the particle came from the center of the target.
      if (rc<1.0 && segment && match3 && match4 
	  && segment->hits[0]->xy.Mod()<10.0 
	  && match3->hits[0]->xy.Mod()<10.0 && match4->hits[0]->xy.Mod()<10.0){
	rc=segment->rc;
	xc=segment->xc;
	yc=segment->yc;
	double ratio=segment->hits[0]->xy.Mod()/(2.*rc);
	if (ratio<1.){
	  double sperp=2.*rc*asin(ratio);
	  tanl=(segment->hits[0]->wire->origin.z()-TARGET_Z)/sperp;
	}
      }		

      pos.SetXYZ(segment->hits[0]->xy.X(),segment->hits[0]->xy.Y(),
		 segment->hits[0]->wire->origin.z());
      //      GetPositionAndMomentum(Bz_avg,pos,mom);

      // Create new track, starting with the current segment
      DTrackCandidate *track = new DTrackCandidate;
      //track->setPosition(pos);
      //track->setMomentum(mom);
     
      if (match3){
	q=GetCharge(pos,match3);
      }
      else if (match4){
	q=GetCharge(pos,match4);
      }
      else{
	q=GetCharge(pos,segment);
      }
      
      GetPositionAndMomentum(Bz_avg,pos,mom);
    
      // Empirical correction to the momentum 
      if (APPLY_MOMENTUM_CORRECTION){
	double p_mag=mom.Mag();
	mom.SetMag(p_mag*(1.+p_factor1/mom.Theta()+p_factor2));
      }

      track->setCharge(q);
      track->setPosition(pos);
      track->setMomentum(mom);


      for (unsigned int m=0;m<mysegments.size();m++)
	track->AddAssociatedObject(mysegments[m]);

      _data.push_back(track); 
    }
  }
  
  // Next try to link segments starting at package 3
  if(package[2].size()>0){
    // Loop over segments in the 3rd package, matching them to segments in 
    // the fourth (most downstream) packages.
    for (unsigned int i=0;i<package[2].size();i++){
      DFDCSegment *segment=package[2][i];
 
      // tracking parameters
      tanl=segment->tanl;
      phi0=segment->phi0;
      z_vertex=segment->z_vertex;  
      xc=segment->xc;
      yc=segment->yc;
      rc=segment->rc;
      // Sign of the charge
      q=segment->q;
      stepper->SetCharge(q);

      // Start filling vector of segments belonging to current track    
      vector<DFDCSegment*>mysegments; 
      mysegments.push_back(segment);
      
      // double qsum=q;
      
      // Try matching to package 4
      if (package[3].size()>0 && 
	  (match4=GetTrackMatch(zpackage[3],segment,package[3],match_id))
	  !=NULL){
	// Insert the segment from package 4 into the track 
	mysegments.push_back(match4);
	
	// remove the segment from the list 
	package[3].erase(package[3].begin()+match_id);
	
	//qsum+=match4->q;
      }	
      
      //if (qsum>0) q=1.;
      //else q=-1.;
 
      // Variables for determining average Bz
      double Bz_avg=0.;
      unsigned int num_hits=segment->hits.size();
          
      if (mysegments.size()>1){
	DHelicalFit fit;
	double max_r=0.;
	for (unsigned int m=0;m<mysegments.size();m++){
	  for (unsigned int n=0;n<mysegments[m]->hits.size();n++){
	    const DFDCPseudo *hit=mysegments[m]->hits[n];
	    fit.AddHit(hit);
	    Bz_avg-=bfield->GetBz(hit->xy.X(),hit->xy.Y(),
				  hit->wire->origin.z());	    
	    double r=hit->xy.Mod();
	    if (r>max_r) max_r=r;
	  }
	  num_hits+=mysegments[m]->hits.size();
	}
	// Fake point at origin
	if (max_r<MAX_R_VERTEX_LIMIT) fit.AddHitXYZ(0.,0.,TARGET_Z,BEAM_VAR,BEAM_VAR,0.);
	if (fit.FitCircleAndLineRiemann(mysegments[0]->rc)==NOERROR){     	
	  // Charge
	  //if (q==0) 
	  //q=fit.q;
	  // Estimate for azimuthal angle
	  phi0=fit.phi;
	  mysegments[0]->phi0=phi0;
	  // remaining tracking parameters
	  tanl=fit.tanl;
	  z_vertex=fit.z_vertex;

	  mysegments[0]->tanl=tanl;
	  mysegments[0]->z_vertex=z_vertex;
	  xc=mysegments[0]->xc=fit.x0;
	  yc=mysegments[0]->yc=fit.y0;
	  rc=mysegments[0]->rc=fit.r0;
	}
      
	// Try to match to package 2 again.
	if (package[1].size()>0 &&
	    (match2=GetTrackMatch(zpackage[1],mysegments[0],package[1],
				  match_id))!=NULL){
	  // Insert the segment from package 2 into the track 
	  mysegments.push_back(match2);
	  
	  // remove the segment from the list 
	  package[1].erase(package[1].begin()+match_id);
	  
	  // Redo the fit with the additional hits from package 2
	  for (unsigned int n=0;n<match2->hits.size();n++){
	    const DFDCPseudo *hit=match2->hits[n];
	    fit.AddHit(hit);
	    Bz_avg-=bfield->GetBz(hit->xy.X(),hit->xy.Y(),
				  hit->wire->origin.z());	    
	  }
	  num_hits+=match2->hits.size();

	  if (fit.FitCircleAndLineRiemann(mysegments[0]->rc)==NOERROR){     
	    // Charge
	    //if (q==0) 
	    //q=fit.q;
	    mysegments[0]->q=q;
	    // Estimate for azimuthal angle
	    phi0=fit.phi;
	    mysegments[0]->phi0=phi0;
	    // remaining tracking parameters
	    tanl=fit.tanl;
	    z_vertex=fit.z_vertex;
	    
	    mysegments[0]->tanl=tanl;
	    mysegments[0]->z_vertex=z_vertex;
	    xc=mysegments[0]->xc=fit.x0;
	    yc=mysegments[0]->yc=fit.y0;
	    rc=mysegments[0]->rc=fit.r0;
	  }
	}

	// Try to match to package 1 again	  
	if (pack1_left_over.size()>0){
	  DFDCSegment *match1=NULL;
	  if ((match1=GetTrackMatch(zpackage[0],mysegments[0],
				    pack1_left_over,
				    match_id))!=NULL){
	    mysegments.push_back(match1);
	    pack1_left_over.erase(pack1_left_over.begin()+match_id);
	  }
	}
      } // if mysegments.size()>1
      else{
	for (unsigned int n=0;n<segment->hits.size();n++){
	  Bz_avg-=bfield->GetBz(segment->hits[n]->xy.X(),
				segment->hits[n]->xy.Y(),
				segment->hits[n]->wire->origin.z());
	}
      }
	
      DVector3 mom,pos;
      Bz_avg/=double(num_hits);

      
      // Try to fix relatively high momentum tracks in the very forward 
      // direction that look like low momentum tracks due to small pt.
      // Assume that the particle came from the center of the target.
      if (rc<1.0 && segment && match4 
	  && segment->hits[0]->xy.Mod()<10.0 
	  && match4->hits[0]->xy.Mod()<10.0){
	rc=segment->rc;
	xc=segment->xc;
	yc=segment->yc;
	double ratio=segment->hits[0]->xy.Mod()/(2.*rc);
	if (ratio<1.){
	  double sperp=2.*rc*asin(ratio);
	  tanl=(segment->hits[0]->wire->origin.z()-TARGET_Z)/sperp;
	}
      }	
     
      pos.SetXYZ(segment->hits[0]->xy.X(),segment->hits[0]->xy.Y(),
		 segment->hits[0]->wire->origin.z());
      //GetPositionAndMomentum(Bz_avg,pos,mom);
      
      // Create new track, starting with the current segment
      DTrackCandidate *track = new DTrackCandidate;
      //track->setPosition(pos);
      //track->setMomentum(mom);      
 
      if (match4){
	q=GetCharge(pos,match4);
      }
      else{
	q=GetCharge(pos,segment);
      }
      
      GetPositionAndMomentum(Bz_avg,pos,mom);

      // Empirical correction to the momentum
      if (APPLY_MOMENTUM_CORRECTION){
	double p_mag=mom.Mag();
	mom.SetMag(p_mag*(1.+p_factor1/mom.Theta()+p_factor2));
      }

      track->setPosition(pos);
      track->setMomentum(mom);      
      track->setCharge(q);

      for (unsigned int m=0;m<mysegments.size();m++)
	track->AddAssociatedObject(mysegments[m]);
      
      _data.push_back(track); 
    }
  }

  // Now collect stray segments in package 4
  for (unsigned int k=0;k<package[3].size();k++){
    DFDCSegment *segment=package[3][k];
    
    DVector3 pos,mom;
    tanl=segment->tanl;
    phi0=segment->phi0;
     // Circle parameters
    xc=segment->xc;
    yc=segment->yc;
    rc=segment->rc;
    // Sign of the charge
    q=segment->q;
    // z-position at "vertex"
    z_vertex=segment->z_vertex;

    // Try to fix relatively high momentum tracks in the very forward 
    // direction that look like low momentum tracks due to small pt.
    // Assume that the particle came from the center of the target.
    if (rc<1.0 && segment->hits[0]->xy.Mod()<10.0 ){
      double ratio=segment->hits[0]->xy.Mod()/(2.*rc);
      if (ratio<1.){
	double sperp=2.*rc*asin(ratio);
	tanl=(segment->hits[0]->wire->origin.z()-TARGET_Z)/sperp;
      }
    }	
    
    double Bz_avg=0.;
    // Compute average magnitic field for the segment
    for (unsigned int m=0;m<segment->hits.size();m++){
      Bz_avg-=bfield->GetBz(segment->hits[m]->xy.X(),segment->hits[m]->xy.Y(),
			    segment->hits[m]->wire->origin.z());
    }
    Bz_avg/=double(segment->hits.size());
  
    pos.SetXYZ(segment->hits[0]->xy.X(),segment->hits[0]->xy.Y(),
		 segment->hits[0]->wire->origin.z());    
    GetPositionAndMomentum(Bz_avg,pos,mom);
    
    // Empirical correction to the momentum 
    if (APPLY_MOMENTUM_CORRECTION){
      double p_mag=mom.Mag();
      mom.SetMag(p_mag*(1.+p_factor1/mom.Theta()+p_factor2));
    }

    // Create new track, starting with the current segment
    DTrackCandidate *track = new DTrackCandidate;
    track->setPosition(pos);
    track->setMomentum(mom);    
    track->setCharge(q);

    track->AddAssociatedObject(segment);

    _data.push_back(track); 
  }

  // Finally, output stray segments in package 1
  for (unsigned int k=0;k<pack1_left_over.size();k++){
    DFDCSegment *segment=pack1_left_over[k];
  
    DVector3 pos,mom;
    tanl=segment->tanl;
    phi0=segment->phi0;
     // Circle parameters
    xc=segment->xc;
    yc=segment->yc;
    rc=segment->rc;
    // Sign of the charge
    q=segment->q;
    // z-position at "vertex"
    z_vertex=segment->z_vertex;

    double Bz_avg=0.;
    // Compute average magnitic field for the segment
    for (unsigned int m=0;m<segment->hits.size();m++){
      Bz_avg-=bfield->GetBz(segment->hits[m]->xy.X(),segment->hits[m]->xy.Y(),
			    segment->hits[m]->wire->origin.z());
    }
    Bz_avg/=double(segment->hits.size());

    pos.SetXYZ(segment->hits[0]->xy.X(),segment->hits[0]->xy.Y(),
	     segment->hits[0]->wire->origin.z());
    GetPositionAndMomentum(Bz_avg,pos,mom);
    
    // Empirical correction to the momentum 
    if (APPLY_MOMENTUM_CORRECTION){   
      double p_mag=mom.Mag();
      mom.SetMag(p_mag*(1.+p_factor1/mom.Theta()+p_factor2));
    }

    // Create new track, starting with the current segment
    DTrackCandidate *track = new DTrackCandidate;
    track->setPosition(pos);
    track->setMomentum(mom);    
    track->setCharge(q);

    track->AddAssociatedObject(segment);

    _data.push_back(track); 
  }


  
  return NOERROR;
}

// Swim track from one package to the next and look for a match to a segment
// in the new package
DFDCSegment *DTrackCandidate_factory_FDCCathodes::GetTrackMatch(double z, 
						 DFDCSegment *segment,
						 vector<DFDCSegment*>package,
						 unsigned int &match_id){
  DFDCSegment *match=NULL;
  DVector3 norm(0.,0.,1.);  // normal to FDC planes

  // Get the position and momentum at the exit of the package for the 
  // current segment
  DVector3 pos,mom,origin(0.,0.,z);
  if (GetPositionAndMomentum(segment,pos,mom)!=NOERROR) return NULL;
  if (z<pos.z()) mom=-1.0*mom;

  // magnitude of momentum
  double p=mom.Mag();

  // Match to the next package by swimming the track through the field
  double diff_min=1000.,diff;
  if (stepper->SwimToPlane(pos,mom,origin,norm,NULL)==false){
    for (unsigned int j=0;j<package.size();j++){
      DFDCSegment *segment2=package[j];
      unsigned int index=segment2->hits.size()-1;
 
      double dx=pos.x()-segment2->hits[index]->xy.X();
      double dy=pos.y()-segment2->hits[index]->xy.Y();
      diff=sqrt(dx*dx+dy*dy);
      if (diff<diff_min&&diff<MATCH_RADIUS(p)){
	diff_min=diff;
	match=segment2;
	match_id=j;
      }
    }
  }
  
  // If matching in the forward direction did not work, try swimming and
  // matching backwards...
  if (match==NULL){
    diff_min=1000.;
    for (unsigned int i=0;i<package.size();i++){
      DFDCSegment *segment2=package[i];
      if (GetPositionAndMomentum(segment2,pos,mom)==NOERROR){
        mom=-1.0*mom;
        origin.SetZ(segment->hits[0]->wire->origin.z());
        if (stepper->SwimToPlane(pos,mom,origin,norm,NULL)==false){
          double dx=pos.x()-segment->hits[0]->xy.X();
          double dy=pos.y()-segment->hits[0]->xy.Y();
          diff=sqrt(dx*dx+dy*dy);
          if (diff<diff_min&&diff<MATCH_RADIUS(p)){
	    diff_min=diff;
	    match=segment2;
            match_id=i;
          }
        }	
      }       
    }
  }

  // Since we are assuming that the particle is coming from the target,
  // it is possible that the particle could have undergone more than a full
  // revolution and still end up producing the current segment, in which 
  // case if the vertex position is more or less correct, the dip angle is 
  // too large.  Try matching again with an adjusted tanl.
  if (match==NULL){
    diff_min=1000.;
    double my_tanl=segment->tanl;
    double sperp=(z-segment->z_vertex)/my_tanl;
    segment->tanl=my_tanl*sperp/(sperp+2.*segment->rc*M_PI);
    if (GetPositionAndMomentum(segment,pos,mom)!=NOERROR){
      // Restore old value
      segment->tanl=my_tanl;
      return NULL;
    }
    if (z<pos.z()) mom=-1.0*mom;
    
    for (unsigned int j=0;j<package.size();j++){
      DFDCSegment *segment2=package[j];
      unsigned int index=segment2->hits.size()-1;
 
      double dx=pos.x()-segment2->hits[index]->xy.X();
      double dy=pos.y()-segment2->hits[index]->xy.Y();
      diff=sqrt(dx*dx+dy*dy);
      if (diff<diff_min&&diff<MATCH_RADIUS(p)){
	diff_min=diff;
	match=segment2;
	match_id=j;
      }
    }
    // Restore old value
    segment->tanl=my_tanl;
  }

  // If matching in the forward direction did not work, try swimming and
  // matching backwards... with modified tanl values
  if (match==NULL){
    diff_min=1000.;
    for (unsigned int i=0;i<package.size();i++){
      DFDCSegment *segment2=package[i];
      double my_tanl=segment2->tanl;
      double sperp=(z-segment2->z_vertex)/my_tanl;
      segment2->tanl=my_tanl*sperp/(sperp+2.*segment2->rc*M_PI);
   
      if (GetPositionAndMomentum(segment2,pos,mom)==NOERROR){
        mom=-1.0*mom;
        origin.SetZ(segment->hits[0]->wire->origin.z());
        if (stepper->SwimToPlane(pos,mom,origin,norm,NULL)==false){
          double dx=pos.x()-segment->hits[0]->xy.X();
          double dy=pos.y()-segment->hits[0]->xy.Y();
          diff=sqrt(dx*dx+dy*dy);
          if (diff<diff_min&&diff<MATCH_RADIUS(p)){
	    diff_min=diff;
	    match=segment2;
            match_id=i;
          }
        }	
      } 
      // Restore old value
      segment2->tanl=my_tanl;
    }
  }

  if(DEBUG_HISTS){
    match_dist_fdc->Fill(p,diff_min);
  }
  return match;
}


// Obtain position and momentum at the exit of a given package using the 
// helical track model.
//
jerror_t DTrackCandidate_factory_FDCCathodes::GetPositionAndMomentum(DFDCSegment *segment,
					      DVector3 &pos, DVector3 &mom){
  // Position of track segment at last hit plane of package
  double x=segment->xc+segment->rc*cos(segment->Phi1);
  double y=segment->yc+segment->rc*sin(segment->Phi1);
  double z=segment->hits[0]->wire->origin.z();
  pos.SetXYZ(x,y,z);

  // Make sure that the position makes sense!
  //  if (sqrt(x*x+y*y)>FDC_OUTER_RADIUS) return VALUE_OUT_OF_RANGE;

  // Track parameters
  //double kappa=segment->q/(2.*segment->rc);
  double my_phi0=segment->phi0;
  double my_tanl=segment->tanl;
  double z0=segment->z_vertex;

  // Useful intermediate variables
  double cosp=cos(my_phi0);
  double sinp=sin(my_phi0);
  // double twoks=2.*kappa*(z-z0)/my_tanl;
  double twoks=segment->q*(z-z0)/(my_tanl*segment->rc);
  double sin2ks=sin(twoks);
  double cos2ks=cos(twoks); 

  // Get Bfield
  double Bz=fabs(bfield->GetBz(x,y,z));

  // Momentum
  double pt=0.003*Bz*segment->rc;
  mom.SetXYZ(pt*(cosp*cos2ks-sinp*sin2ks),pt*(sinp*cos2ks+cosp*sin2ks),
	     pt*my_tanl);

  return NOERROR;
}

// Routine to return momentum and position given the helical parameters and the
// z-component of the magnetic field
jerror_t DTrackCandidate_factory_FDCCathodes::GetPositionAndMomentum(const double Bz_avg,DVector3 &pos,DVector3 &mom){
  double pt=0.003*Bz_avg*rc;
  double phi1=atan2(pos.y()-yc,pos.x()-xc);
  double dphi_s=q*(pos.z()-endplate_z)/(rc*tanl);
  double dphi1=phi1-dphi_s;// was -
  double x=xc+rc*cos(dphi1);
  double y=yc+rc*sin(dphi1);
  pos.SetXYZ(x,y,endplate_z);

  dphi1*=-1.;
  if (q<0) dphi1+=M_PI;

  //  printf("q %f\n",q);

  double px=pt*sin(dphi1);
  double py=pt*cos(dphi1);
  double pz=pt*tanl;
  mom.SetXYZ(px,py,pz);

  return NOERROR;
}

double DTrackCandidate_factory_FDCCathodes::GetCharge(const DVector3 &pos,
						 const DFDCSegment *segment
						      ){
  // Magnitude of change in phi to go from the reference plane to another point
  // in the segment
  double dphi=(segment->hits[0]->wire->origin.z()-pos.z())/(rc*tanl);

  // Phi at reference plane
  double Phi1=atan2(pos.y()-yc,pos.x()-xc);

  // Positive and negative changes in phi
  double phiplus=Phi1+dphi;
  double phiminus=Phi1-dphi;
  DVector2 plus(xc+rc*cos(phiplus),yc+rc*sin(phiplus));
  DVector2 minus(xc+rc*cos(phiminus),yc+rc*sin(phiminus));

  // Compute differences 
  double d2plus=(plus-segment->hits[0]->xy).Mod2();
  double d2minus=(minus-segment->hits[0]->xy).Mod2();

  //  printf("z0 %f z %f d+ %f d- %f \n",pos.z(),
  //	 segment->hits[0]->wire->origin.z(),d2plus,d2minus);
  // Look for smallest difference to determine q
  if (d2minus<d2plus){
    return -1.;
  }

  return 1.;
}
