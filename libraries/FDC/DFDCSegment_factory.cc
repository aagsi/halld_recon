//************************************************************************
// DFDCSegment_factory.cc - factory producing track segments from pseudopoints
//************************************************************************

#include "DFDCSegment_factory.h"
#include "DANA/DApplication.h"
#include <math.h>

#define HALF_CELL 0.5
#define MAX_DEFLECTION 0.15
#define EPS 1e-8
#define KILL_RADIUS 5.0 
#define NOT_CORRECTED 0x8
#define Z_TARGET 65.0
#define MATCH_RADIUS 5.0
#define SIGN_CHANGE_CHISQ_CUT 10.0
#define BEAM_VARIANCE 0.1 // cm^2
#define FDC_X_RESOLUTION 0.02  // 200 microns
#define FDC_Y_RESOLUTION 0.02
#define USED_IN_SEGMENT 0x8

static bool got_deflection_file=true;
static bool warn=true;

bool DFDCSegment_package_cmp(const DFDCPseudo* a, const DFDCPseudo* b) {
  return a->wire->layer>b->wire->layer;
}


// Locate a position in array xx given x
void locate(double *xx,int n,double x,int *j){
  int ju,jm,jl;
  int ascnd;
  
  jl=-1;
  ju=n;
  ascnd=(xx[n-1]>=xx[0]);
  while(ju-jl>1){
    jm=(ju+jl)>>1;
    if (x>=xx[jm]==ascnd)
      jl=jm;
    else
      ju=jm;
  }
  if (x==xx[0]) *j=0;
  else if (x==xx[n-1]) *j=n-2;
  else *j=jl; 
}


// Polynomial interpolation on a grid.
// Adapted from Numerical Recipes in C (2nd Edition), pp. 121-122.
void polint(double *xa, double *ya,int n,double x, double *y,double *dy){
  int i,m,ns=0;
  double den,dif,dift,ho,hp,w;
  
  double *c= new double[n];
  double *d= new double[n];

  dif=fabs(x-xa[0]);
  for (i=0;i<n;i++){
    if ((dift=fabs(x-xa[i]))<dif){
      ns=i;
      dif=dift;
    }
    c[i]=ya[i];
    d[i]=ya[i];
  }
  *y=ya[ns--];

  for (m=1;m<n;m++){
    for (i=1;i<=n-m;i++){
      ho=xa[i-1]-x;
      hp=xa[i+m-1]-x;
      w=c[i+1-1]-d[i-1];
      if ((den=ho-hp)==0.0) return;
      
      den=w/den;
      d[i-1]=hp*den;
      c[i-1]=ho*den;      
    }    
    *y+=(*dy=(2*ns<(n-m) ?c[ns+1]:d[ns--]));
  }
  delete []c;
  delete []d;
}

DFDCSegment_factory::DFDCSegment_factory() {
        logFile = new ofstream("DFDCSegment_factory.log");
        _log = new JStreamLog(*logFile, "SEGMENT");
        *_log << "File initialized." << endMsg;
}


///
/// DFDCSegment_factory::~DFDCSegment_factory():
/// default destructor -- closes log file
///
DFDCSegment_factory::~DFDCSegment_factory() {
        logFile->close();
        delete logFile;
        delete _log;
}
///
/// DFDCSegment_factory::brun():
/// Initialization: read in deflection map, get magnetic field map
///
jerror_t DFDCSegment_factory::brun(JEventLoop* eventLoop, int eventNo) { 
  DApplication* dapp=dynamic_cast<DApplication*>(eventLoop->GetJApplication());
  bfield = dapp->GetBfield();

  ifstream fdc_deflection_file;

  fdc_deflection_file.open("fdc_deflections.dat");
  if (!fdc_deflection_file.is_open()){ 
    *_log << "Failed to open fdc_deflection file." << endMsg;
    got_deflection_file=false;
    return RESOURCE_UNAVAILABLE;
  }

  double bx;
  double bz;

  char buf[80];
  fdc_deflection_file.getline(buf,80,'\n');
  fdc_deflection_file.setf(ios::skipws); 
  for (int i=0;i<LORENTZ_X_POINTS;i++){
    for (int j=0;j<LORENTZ_Z_POINTS;j++){
      fdc_deflection_file >> lorentz_x[i];
      fdc_deflection_file >> lorentz_z[j];
      fdc_deflection_file >> bx;
      fdc_deflection_file >> bz;
      fdc_deflection_file >> lorentz_nx[i][j];
      fdc_deflection_file >> lorentz_nz[i][j];  

    }
  }
  fdc_deflection_file.close();

  *_log << "Table of Lorentz deflections initialized." << endMsg;
  return NOERROR;
}

///
/// DFDCSegment_factory::evnt():
/// Routine where pseudopoints are combined into track segments
///
jerror_t DFDCSegment_factory::evnt(JEventLoop* eventLoop, int eventNo) {
  myeventno=eventNo;

  vector<const DFDCPseudo*>pseudopoints;
  eventLoop->Get(pseudopoints);  

  // Group pseudopoints by package
  vector<DFDCPseudo*>package[4];
  for (vector<const DFDCPseudo*>::iterator i=pseudopoints.begin();
       i!=pseudopoints.end();i++){
    package[((*i)->wire->layer-1)/6].push_back((DFDCPseudo*)(*i));

  }
  for (int j=0;j<4;j++){
    std::sort(package[j].begin(), package[j].end(), DFDCSegment_package_cmp);
    FindSegments(package[j]);
  }

  return NOERROR;
}

// Riemann Line fit: linear regression of s on z to determine the tangent of 
// the dip angle and the z position of the closest approach to the beam line.
// Also returns predicted positions along the helical path.
//
jerror_t DFDCSegment_factory::RiemannLineFit(unsigned int n,DMatrix XYZ0,
					     DMatrix CR,DMatrix &XYZ){
  // Fill matrix of intersection points
  for (unsigned int m=0;m<n;m++){
    double r2=XYZ0(m,0)*XYZ0(m,0)+XYZ0(m,1)*XYZ0(m,1);
    double x_int0,temp,y_int0;
    double denom= N[0]*N[0]+N[1]*N[1];
    double numer=dist_to_origin+r2*N[2];
   
    x_int0=-N[0]*numer/denom;
    y_int0=-N[1]*numer/denom;
    temp=denom*r2-numer*numer;
    if (temp<0){
      temp*=-1.;  
      // I'm not sure this is the correct thing to do but we don't want the 
      // algorithm to fail at this stage!

      // return VALUE_OUT_OF_RANGE;
    }
    temp=sqrt(temp)/denom;
    
    // Choose sign of square root based on proximity to actual measurements
    double diffx1=x_int0+N[1]*temp-XYZ0(m,0);
    double diffy1=y_int0-N[0]*temp-XYZ0(m,1);
    double diffx2=x_int0-N[1]*temp-XYZ0(m,0);
    double diffy2=y_int0+N[0]*temp-XYZ0(m,1);
    if (diffx1*diffx1+diffy1*diffy1 > diffx2*diffx2+diffy2*diffy2){
      XYZ(m,0)=x_int0-N[1]*temp;
      XYZ(m,1)=y_int0+N[0]*temp;
    }
    else{
      XYZ(m,0)=x_int0+N[1]*temp;
      XYZ(m,1)=y_int0-N[0]*temp;
    }
  }
      
  // Linear regression to find z0, tanl   
  double sumv=0.,sumx=0.,sumy=0.,sumxx=0.,sumxy=0.,sperp=0.,Delta;
  for (unsigned int k=0;k<n;k++){
    double diffx=XYZ(k,0)-XYZ(0,0);
    double diffy=XYZ(k,1)-XYZ(0,1);
    double chord=sqrt(diffx*diffx+diffy*diffy);
    double ratio=chord/2./rc; 
    // Make sure the argument for the arcsin does not go out of range...
    if (ratio>1.) 
      sperp=2.*rc*(M_PI/2.);
    else
      sperp=2.*rc*asin(ratio);
    // Assume errors in s dominated by errors in R 
    sumv+=1./CR(k,k);
    sumy+=sperp/CR(k,k);
    sumx+=XYZ(k,2)/CR(k,k);
    sumxx+=XYZ(k,2)*XYZ(k,2)/CR(k,k);
    sumxy+=sperp*XYZ(k,2)/CR(k,k);
  }
  Delta=sumv*sumxx-sumx*sumx;

  // Track parameters z0 and tan(lambda)
  tanl=-Delta/(sumv*sumxy-sumy*sumx); 
  z0=(sumxx*sumy-sumx*sumxy)/Delta*tanl;
  zvertex=z0-sperp*tanl;
  
  // Error in tanl 
  var_tanl=sumv/Delta*(tanl*tanl*tanl*tanl);

  return NOERROR;
}

// Use predicted positions (propagating from plane 1 using a helical model) to
// update the R and RPhi covariance matrices.
//
jerror_t DFDCSegment_factory::UpdatePositionsAndCovariance(unsigned int n,
     double r1sq,DMatrix &XYZ,DMatrix &CRPhi,DMatrix &CR){
  double delta_x=XYZ(ref_plane,0)-xc; 
  double delta_y=XYZ(ref_plane,1)-yc;
  double r1=sqrt(r1sq);
  double denom=delta_x*delta_x+delta_y*delta_y;

  // Auxiliary matrices for correcting for non-normal track incidence to FDC
  // The correction is 
  //    CRPhi'= C*CRPhi*C+S*CR*S, where S(i,i)=R_i*kappa/2
  //                                and C(i,i)=sqrt(1-S(i,i)^2)  
  DMatrix C(n,n);
  DMatrix S(n,n);

  // Predicted positions
  Phi1=atan2(delta_y,delta_x);
  double z1=XYZ(ref_plane,2);
  double var_R1=CR(ref_plane,ref_plane);
  for (unsigned int k=0;k<n;k++){       
    double sperp=charge*(XYZ(k,2)-z1)/tanl;
    double Phi=atan2(XYZ(k,1),XYZ(k,0));
    double sinPhi=sin(Phi);
    double cosPhi=cos(Phi);
    double dRPhi_dx=Phi*cosPhi-sinPhi;
    double dRPhi_dy=Phi*sinPhi+cosPhi;

    double sinp=sin(Phi1+sperp/rc);
    double cosp=cos(Phi1+sperp/rc);
    XYZ(k,0)=xc+rc*cosp;
    XYZ(k,1)=yc+rc*sinp;
    
    double dx_drho=cosp+sperp/rc*sinp;
    double dx_dx0=1.-rc*sinp*delta_y/denom;
    double dx_dy0=rc*sinp*delta_x/denom;
    double dx_dx1=rc*sinp*delta_y/denom;
    double dx_dy1=-rc*sinp*delta_x/denom;
    double dx_dtanl=sinp*sperp/tanl;
    
    double dy_drho=sinp-sperp/rc*cosp;
    double dy_dx0=rc*cosp*delta_y/denom;
    double dy_dy0=1.-rc*cosp*delta_x/denom;
    double dy_dx1=-rc*cosp*delta_y/denom;
    double dy_dy1=rc*cosp*delta_x/denom;
    double dy_dtanl=-cosp*sperp/tanl;
 
    double dRPhi_dx0=dRPhi_dx*dx_dx0+dRPhi_dy*dy_dx0;
    double dRPhi_dy0=dRPhi_dx*dx_dy0+dRPhi_dy*dy_dy0;
    double dRPhi_dx1=dRPhi_dx*dx_dx1+dRPhi_dy*dy_dx1;
    double dRPhi_dy1=dRPhi_dx*dx_dy1+dRPhi_dy*dy_dy1;
    double dRPhi_drho=dRPhi_dx*dx_drho+dRPhi_dy*dy_drho;
    double dRPhi_dtanl=dRPhi_dx*dx_dtanl+dRPhi_dy*dy_dtanl;

    double dR_dx0=cosPhi*dx_dx0+sinPhi*dy_dx0;
    double dR_dy0=cosPhi*dx_dy0+sinPhi*dy_dy0;
    double dR_dx1=cosPhi*dx_dx1+sinPhi*dy_dx1;
    double dR_dy1=cosPhi*dx_dy1+sinPhi*dy_dy1;
    double dR_drho=cosPhi*dx_drho+sinPhi*dy_drho;
    double dR_dtanl=cosPhi*dx_dtanl+sinPhi*dy_dtanl;
    
    double var_x0=(varN[0][0]+N[0]*N[0]*varN[2][2]/N[2]/N[2]
		   -2.*varN[0][2]*N[0]/N[2])/4./N[2]/N[2];
    double var_y0=(varN[1][1]+N[1]*N[1]*varN[2][2]/N[2]/N[2]
		   -2.*varN[1][2]*N[1]/N[2])/4./N[2]/N[2];
    
    double dr_dn1=xavg[0]/2./N[2]/rc;
    double dr_dn2=xavg[0]/2./N[2]/rc;
    double dr_dn3=-(1.+4.*xavg[0]*N[0]+4.*xavg[1]*N[1])/4./rc/N[2]/N[2];
    double var_r=dr_dn1*dr_dn1*varN[0][0]+dr_dn2*dr_dn2*varN[1][1]
      +dr_dn3*dr_dn3*varN[2][2]+2.*varN[0][1]*dr_dn1*dr_dn2
      +2.*varN[1][2]*dr_dn2*dr_dn3+2.*varN[0][2]*dr_dn1*dr_dn3
      +var_avg/4./rc/rc/N[2]/N[2];
	
    double dc_dn3=-xavg[2];
    double dc_dn2=-xavg[1];
    double dc_dn1=-xavg[0];
    double cdist=dist_to_origin+r1sq*N[2];
    double n2=N[0]*N[0]+N[1]*N[1];

    double ydenom=XYZ(0,1)*n2+N[1]*cdist;
    double dy1_dn3=-(dc_dn3+r1sq)*(N[1]*XYZ(0,1)+cdist)/ydenom;
    double dy1_dr1=-r1*(2.*N[1]*N[2]*XYZ(0,1)+2.*N[2]*cdist-N[0]*N[0])/ydenom;
    double dy1_dn2=(XYZ(0,1)*(N[1]*N[1]-N[0]*N[0])*cdist
		    -XYZ(0,1)*N[1]*n2*dc_dn2+N[1]*cdist*cdist
		    -n2*cdist*dc_dn2-N[0]*N[0]*N[1]*r1sq)/n2/ydenom;
    double var_y1=dy1_dr1*dy1_dr1*var_R1
      +dy1_dn3*dy1_dn3*varN[2][2]+dy1_dn2*dy1_dn2*varN[1][1]
      +2.*dy1_dn2*dy1_dn3*varN[1][2];

    double xdenom=XYZ(0,0)*n2+N[0]*cdist;
    double dx1_dn3=-(dc_dn3+r1sq)*(XYZ(0,0)*N[0]+cdist)/xdenom;
    double dx1_dr1=-r1*(2.*N[0]*N[2]*XYZ(0,0)+2.*N[2]*cdist-N[1]*N[1])/xdenom;
    double dx1_dn1=(XYZ(0,0)*(N[0]*N[0]-N[1]*N[1])*cdist
		    -XYZ(0,0)*N[0]*n2*dc_dn1+N[0]*cdist*cdist
		    -n2*cdist*dc_dn1-N[0]*N[1]*N[1]*r1sq)/n2/xdenom;

    double var_x1=dx1_dr1*dx1_dr1*var_R1
      +dx1_dn3*dx1_dn3*varN[2][2]+dx1_dn1*dx1_dn1*varN[0][0]
      +2.*dx1_dn3*dx1_dn1*varN[0][2];

    CRPhi(k,k)=dRPhi_dx0*dRPhi_dx0*var_x0+dRPhi_dy0*dRPhi_dy0*var_y0
      +dRPhi_dx1*dRPhi_dx1*var_x1+dRPhi_dy1*dRPhi_dy1*var_y1
      +dRPhi_drho*dRPhi_drho*var_r+dRPhi_dtanl*dRPhi_dtanl*var_tanl;
    CR(k,k)=dR_dx0*dR_dx0*var_x0+dR_dy0*dR_dy0*var_y0+dR_dx1*dR_dx1*var_x1
      +dR_dy1*dR_dy1*var_y1+dR_drho*dR_drho*var_r+dR_dtanl*dR_dtanl*var_tanl;
    
    double rtemp=sqrt(XYZ(k,0)*XYZ(k,0)+XYZ(k,1)*XYZ(k,1)); 
    double stemp=rtemp/4./rc;
    double ctemp=1.-stemp*stemp;
    if (ctemp>0){
      S(k,k)=stemp;
      C(k,k)=sqrt(ctemp);
    }
    else{
      S(k,k)=0.;
      C(k,k)=1.;
    }
  }
  
   // Correct the covariance matrices for contributions due to multiple
  // scattering
  DMatrix CRPhi_ms(n,n);
  DMatrix CR_ms(n,n);
  double lambda=atan(tanl);
  double cosl=cos(lambda);
  double sinl=sin(lambda);
  if (cosl<EPS) cosl=EPS;
  if (sinl<EPS) sinl=EPS;
  // We loop over all the points except the point at the target, which is left
  // out because the path length is too long to use the linear approximation
  // we are using to estimate the contributions due to multiple scattering.
  for (unsigned int m=0;m<n-1;m++){
    double Rm=sqrt(XYZ(m,0)*XYZ(m,0)+XYZ(m,1)*XYZ(m,1));
    double zm=XYZ(m,2);
    for (unsigned int k=m;k<n-1;k++){
      double Rk=sqrt(XYZ(k,0)*XYZ(k,0)+XYZ(k,1)*XYZ(k,1));
      double zk=XYZ(k,2);
      unsigned int imin=(k+1>m+1)?k+1:m+1;
      for (unsigned int i=imin;i<n-1;i++){
        double sigma2_ms=GetProcessNoise(i,XYZ);
        double Ri=sqrt(XYZ(i,0)*XYZ(i,0)+XYZ(i,1)*XYZ(i,1));
        double zi=XYZ(i,2);
        CRPhi_ms(m,k)+=sigma2_ms*(Rk-Ri)*(Rm-Ri)/cosl/cosl;
        CR_ms(m,k)+=sigma2_ms*(zk-zi)*(zm-zi)/sinl/sinl/sinl/sinl;
      }
      CRPhi_ms(k,m)=CRPhi_ms(m,k);
      CR_ms(k,m)=CR_ms(m,k);
    }
  }
  CRPhi+=CRPhi_ms;
  CR+=CR_ms;

  // Correction for non-normal incidence of track on FDC 
  CRPhi=C*CRPhi*C+S*CR*S;
 
  return NOERROR;
}

// Variance of projected multiple scattering angle for a single layer 
double DFDCSegment_factory::GetProcessNoise(unsigned int i,DMatrix XYZ){
  double cosl=cos(atan(tanl));
  double sinl=sin(atan(tanl));
 
  // Try to prevent division-by-zero errors 
  if (sinl<EPS) sinl=EPS;
  if (cosl<EPS) cosl=EPS;

  // Get Bfield
  double x=XYZ(i,0);
  double y=XYZ(i,1);
  double z=XYZ(i,2);
  double Bx,By,Bz,B;
  bfield->GetField(x,y,z,Bx,By,Bz);
  B=sqrt(Bx*Bx+By*By+Bz*Bz);
   
  // Momentum
  double p=0.003*B*rc/cosl;
  // Assume pion
  double beta=p/sqrt(p*p+0.14*0.14);
 
  //Materials: copper, Kapton, Mylar, Air, Argon, CO2
  double thickness[6]={4e-4,50e-4,13e-4,1.0,0.4,0.6};
  double density[6]={8.96,1.42,1.39,1.2931e-3,1.782e-3,1.977e-3};
  double X0[6]={12.86,40.56,39.95,36.66,19.55,36.2};
  double material_sum=0.;
  for (unsigned int j=0;j<6;j++){
    material_sum+=thickness[j]*density[j]/X0[j];
  }
 
  // Variance from multiple scattering
  return (0.0136*0.0136/p/p/beta/beta*material_sum/sinl
	  *(1.+0.038*log(material_sum/sinl))
	  *(1.+0.038*log(material_sum/sinl)));
}

// Calculate the (normal) eigenvector corresponding to the eigenvalue lambda
jerror_t DFDCSegment_factory::CalcNormal(DMatrix A,double lambda,DMatrix &N){
  double sum=0;

  N(0,0)=1.;
  N(1,0)=N(0,0)*(A(1,0)*A(0,2)-(A(0,0)-lambda)*A(1,2))
    /(A(0,1)*A(2,1)-(A(1,1)-lambda)*A(0,2));
  N(2,0)=N(0,0)*(A(2,0)*(A(1,1)-lambda)-A(1,0)*A(2,1))
    /(A(1,2)*A(2,1)-(A(2,2)-lambda)*(A(1,1)-lambda));
  
  // Normalize: n1^2+n2^2+n3^2=1
  for (int i=0;i<3;i++){
    sum+=N(i,0)*N(i,0);
  }
  for (int i=0;i<3;i++){
    N(i,0)/=sqrt(sum);
  }

  return NOERROR;
}


// Riemann Circle fit:  points on a circle in x,y project onto a plane cutting
// the circular paraboloid surface described by (x,y,x^2+y^2).  Therefore the
// task of fitting points in (x,y) to a circle is transormed to the taks of
// fitting planes in (x,y, w=x^2+y^2) space
//
jerror_t DFDCSegment_factory::RiemannCircleFit(unsigned int n, DMatrix XYZ,
					       DMatrix CRPhi){
  DMatrix X(n,3);
  DMatrix Xavg(1,3);
  DMatrix A(3,3);
  double B0,B1,B2,Q,Q1,R,sum,diff;
  double theta,lambda_min=1.e8,lambda[3];
  int smallest_eigenvalue=0;
  // Column and row vectors of ones
  DMatrix Ones(n,1),OnesT(1,n);
  DMatrix W_sum(1,1);
  DMatrix W(n,n);
  // Eigenvectors
  DMatrix N1(3,1);
  DMatrix N2(3,1);
  DMatrix N3(3,1);
  DMatrix VN(3,3);

  // The goal is to find the eigenvector corresponding to the smallest 
  // eigenvalue of the equation
  //            lambda=n^T (X^T W X - W_sum Xavg^T Xavg)n
  // where n is the normal vector to the plane slicing the cylindrical 
  // paraboloid described by the parameterization (x,y,w=x^2+y^2),
  // and W is the weight matrix, assumed for now to be diagonal.
  // In the absence of multiple scattering, W_sum is the sum of all the 
  // diagonal elements in W.

  for (unsigned int i=0;i<n;i++){
    X(i,0)=XYZ(i,0);
    X(i,1)=XYZ(i,1);
    X(i,2)=XYZ(i,0)*XYZ(i,0)+XYZ(i,1)*XYZ(i,1);
    Ones(i,0)=OnesT(0,i)=1.;
  }

  // Check that CRPhi is invertible 
  TDecompLU lu(CRPhi);
  if (lu.Decompose()==false){
    *_log << "RiemannCircleFit: Singular matrix,  event "<< myeventno << endMsg;
    
    return UNRECOVERABLE_ERROR; // error placeholder
  }
  W=DMatrix(DMatrix::kInverted,CRPhi);
  W_sum=OnesT*(W*Ones);
  Xavg=(1./W_sum(0,0))*(OnesT*(W*X));
  // Store in private array for use in other routines
  xavg[0]=Xavg(0,0);
  xavg[1]=Xavg(0,1);
  xavg[2]=Xavg(0,2);
  var_avg=1./W_sum(0,0);
  
  A=DMatrix(DMatrix::kTransposed,X)*(W*X)
    -W_sum(0,0)*(DMatrix(DMatrix::kTransposed,Xavg)*Xavg);

  // The characteristic equation is 
  //   lambda^3+B2*lambda^2+lambda*B1+B0=0 
  //
  B2=-(A(0,0)+A(1,1)+A(2,2));
  B1=A(0,0)*A(1,1)-A(1,0)*A(0,1)+A(0,0)*A(2,2)-A(2,0)*A(0,2)+A(1,1)*A(2,2)
    -A(2,1)*A(1,2);
  B0=-A.Determinant();

  // The roots of the cubic equation are given by 
  //        lambda1= -B2/3 + S+T
  //        lambda2= -B2/3 - (S+T)/2 + i sqrt(3)/2. (S-T)
  //        lambda3= -B2/3 - (S+T)/2 - i sqrt(3)/2. (S-T)
  // where we define some temporary variables:
  //        S= (R+sqrt(Q^3+R^2))^(1/3)
  //        T= (R-sqrt(Q^3+R^2))^(1/3)
  //        Q=(3*B1-B2^2)/9
  //        R=(9*B2*B1-27*B0-2*B2^3)/54
  //        sum=S+T;
  //        diff=i*(S-T)
  // We divide Q and R by a safety factor to prevent multiplying together 
  // enormous numbers that cause unreliable results.

  Q=(3.*B1-B2*B2)/9.e4; 
  R=(9.*B2*B1-27.*B0-2.*B2*B2*B2)/54.e6;
  Q1=Q*Q*Q+R*R;
  if (Q1<0) Q1=sqrt(-Q1);
  else{
    return VALUE_OUT_OF_RANGE;
  }

  // DeMoivre's theorem for fractional powers of complex numbers:  
  //      (r*(cos(theta)+i sin(theta)))^(p/q)
  //                  = r^(p/q)*(cos(p*theta/q)+i sin(p*theta/q))
  //
  double temp=100.*pow(R*R+Q1*Q1,0.16666666666666666667);
  theta=atan2(Q1,R)/3.;
  sum=2.*temp*cos(theta);
  diff=-2.*temp*sin(theta);
 
  // First root
  lambda[0]=-B2/3.+sum;
  if (lambda[0]<lambda_min&&lambda[0]>EPS){
    lambda_min=lambda[0];
    smallest_eigenvalue=0;
  }

  // Second root
  lambda[1]=-B2/3.-sum/2.-sqrt(3.)/2.*diff;
  if (lambda[1]<lambda_min&&lambda[1]>EPS){
    lambda_min=lambda[1];
    smallest_eigenvalue=1;
  }

  // Third root
  lambda[2]=-B2/3.-sum/2.+sqrt(3.)/2.*diff;
  if (lambda[2]<lambda_min&&lambda[2]>EPS){
    lambda_min=lambda[2];
    smallest_eigenvalue=2;
  }

  // Normal vector to plane
  CalcNormal(A,lambda_min,N1);
  // Copy N1 to private array
  N[0]=N1(0,0);
  N[1]=N1(1,0);
  N[2]=N1(2,0);

  // Error estimate for N, currently zeroes...
  for (int i=0;i<3;i++)
    for (int j=0;j<3;j++)
      varN[i][j]=0.;
      
  // Distance to origin
  dist_to_origin=-(N[0]*Xavg(0,0)+N[1]*Xavg(0,1)+N[2]*Xavg(0,2));

  // Center and radius of the circle
  xc=-N[0]/2./N[2];
  yc=-N[1]/2./N[2];
  rc=sqrt(1.-N[2]*N[2]-4.*dist_to_origin*N[2])/2./fabs(N[2]);

  return NOERROR;
}

// Riemann Helical Fit based on transforming points on projection to x-y plane 
// to a circular paraboloid surface combined with a linear fit of the arc 
// length versus z. Uses RiemannCircleFit and RiemannLineFit.
//
jerror_t DFDCSegment_factory::RiemannHelicalFit(vector<DFDCPseudo*>points,
						DMatrix &CR,DMatrix &XYZ){
  double Phi;
  unsigned int num_points=points.size()+1;
  DMatrix CRPhi(num_points,num_points); 
  // DMatrix XYZ(num_points,3);
  DMatrix XYZ0(num_points,3);

  // Clear supplemental track info vector
  fdc_track.clear();
  fdc_track_t temp;
  temp.hit_id=0;
  temp.dx=temp.dy=temp.s=temp.chi2=0.;
  fdc_track.assign(num_points-1,temp);
  
  // Fill initial matrices for R and RPhi measurements
  XYZ(num_points-1,2)=XYZ0(num_points-1,2)=Z_TARGET;
  for (unsigned int m=0;m<points.size();m++){
    XYZ0(m,0)=points[m]->x;
    XYZ0(m,1)=points[m]->y;
    XYZ(m,2)=XYZ0(m,2)=points[m]->wire->origin(2);
    Phi=atan2(points[m]->y,points[m]->x);
    CRPhi(m,m)
      =(Phi*cos(Phi)-sin(Phi))*(Phi*cos(Phi)-sin(Phi))*points[m]->cov(0,0)
      +(Phi*sin(Phi)+cos(Phi))*(Phi*sin(Phi)+cos(Phi))*points[m]->cov(1,1)
      +2.*(Phi*sin(Phi)+cos(Phi))*(Phi*cos(Phi)-sin(Phi))*points[m]->cov(1,0);

    CR(m,m)=cos(Phi)*cos(Phi)*points[m]->cov(0,0)
      +sin(Phi)*sin(Phi)*points[m]->cov(1,1)
      +2.*sin(Phi)*cos(Phi)*points[m]->cov(1,0);
  }
  CR(points.size(),points.size())=BEAM_VARIANCE;
  CRPhi(points.size(),points.size())=BEAM_VARIANCE;
  
  // Reference track:
  jerror_t error=NOERROR;  
  // First find the center and radius of the projected circle
  error=RiemannCircleFit(num_points,XYZ0,CRPhi); 
  if (error!=NOERROR) return error;
  
  // Get reference track estimates for z0 and tanl and intersection points
  // (stored in XYZ)
  error=RiemannLineFit(num_points,XYZ0,CR,XYZ);
  if (error!=NOERROR) return error;

  // Guess particle charge (+/-1);
  charge=GetCharge(points.size(),XYZ0,CR,CRPhi);

  double r1sq=XYZ(ref_plane,0)*XYZ(ref_plane,0)
    +XYZ(ref_plane,1)*XYZ(ref_plane,1);
  UpdatePositionsAndCovariance(num_points,r1sq,XYZ,CRPhi,CR);

  // Preliminary circle fit 
  error=RiemannCircleFit(num_points,XYZ0,CRPhi); 
  if (error!=NOERROR) return error;

  // Preliminary line fit
  error=RiemannLineFit(num_points,XYZ0,CR,XYZ);
  if (error!=NOERROR) return error;

  // Guess particle charge (+/-1);
  charge=GetCharge(points.size(),XYZ,CR,CRPhi);
  
  r1sq=XYZ(ref_plane,0)*XYZ(ref_plane,0)+XYZ(ref_plane,1)*XYZ(ref_plane,1);
  UpdatePositionsAndCovariance(num_points,r1sq,XYZ,CRPhi,CR);

  // Final circle fit 
  error=RiemannCircleFit(num_points,XYZ0,CRPhi); 
  if (error!=NOERROR) return error;

  // Final line fit
  error=RiemannLineFit(num_points,XYZ0,CR,XYZ);
  if (error!=NOERROR) return error;

  // Guess particle charge (+/-1)
  charge=GetCharge(points.size(),XYZ,CR,CRPhi);
  
  // Final update to covariance matrices
  r1sq=XYZ(ref_plane,0)*XYZ(ref_plane,0)+XYZ(ref_plane,1)*XYZ(ref_plane,1);
  UpdatePositionsAndCovariance(num_points,r1sq,XYZ,CRPhi,CR);
  
  // Store residuals and path length for each measurement
  chisq=0.;
  for (unsigned int m=0;m<points.size();m++){
    fdc_track_t temp;
    temp.hit_id=m;
    double sperp=charge*(XYZ(m,2)-XYZ(ref_plane,2))/tanl; 
    double sinp=sin(Phi1+sperp/rc);
    double cosp=cos(Phi1+sperp/rc);
    XYZ(m,0)=xc+rc*cosp;
    XYZ(m,1)=yc+rc*sinp;
    temp.dx=XYZ(m,0)-XYZ0(m,0); // residuals
    temp.dy=XYZ(m,1)-XYZ0(m,1);
    temp.s=(XYZ(m,2)-zvertex)/sin(atan(tanl)); // path length 
    temp.chi2=(temp.dx*temp.dx+temp.dy*temp.dy)/CR(m,m);
    fdc_track[m]=temp;	
    chisq+=temp.chi2;
  }
  return NOERROR;
}


// DFDCSegment_factory::FindSegments
//  Associate nearest neighbors within a package with track segment candidates.
// Provide guess for (seed) track parameters
//
jerror_t DFDCSegment_factory::FindSegments(vector<DFDCPseudo*>points){
  // We need at least 3 points to define a circle, so bail if we don't have 
  // enough points.
  if (points.size()<3) return RESOURCE_UNAVAILABLE;
 
  // Put indices for the first point in each plane before the most downstream
  // plane in the vector x_list.
  double old_z=points[0]->wire->origin(2);
  vector<unsigned int>x_list;
  for (unsigned int i=0;i<points.size();i++){
    if (points[i]->wire->origin(2)!=old_z){
      x_list.push_back(i);
    }
    old_z=points[i]->wire->origin(2);
  }
  x_list.push_back(points.size()); 

  // Now loop over the list of track segment end points
  for (unsigned int i=0;i<x_list[0];i++){
    if ((points[i]->status&USED_IN_SEGMENT)==0){
      // Creat a new segment
      DFDCSegment *segment = new DFDCSegment;
      segment->chisq=1.e8;
      DMatrix Seed(5,1);
      DMatrix Cov(5,5);
    
      // Clear track parameters
      kappa=tanl=D=z0=phi0=Phi1=xc=yc=rc=0.;
      charge=1.;
      
      // Point in the last plane in the package 
      double x=points[i]->x;
      double y=points[i]->y; 
      
      // Create list of nearest neighbors
      vector<DFDCPseudo*>neighbors;
      neighbors.push_back(points[i]);
      unsigned int match=0;
      double delta,delta_min=1000.,xtemp,ytemp;
      for (unsigned int k=0;k<x_list.size()-1;k++){
	delta_min=1000.;
	match=0;
	for (unsigned int m=x_list[k];m<x_list[k+1];m++){
	  xtemp=points[m]->x;
	  ytemp=points[m]->y;
	  delta=sqrt((x-xtemp)*(x-xtemp)+(y-ytemp)*(y-ytemp));
	  if (delta<delta_min && delta<MATCH_RADIUS){
	    delta_min=delta;
	    match=m;
	  }
	}	
	if (match!=0){
	  x=points[match]->x;
	  y=points[match]->y;
	  points[match]->status|=USED_IN_SEGMENT;
	  neighbors.push_back(points[match]);	  
	}
      }
      // Look for hits adjacent to the ones we have in our segment candidate
      unsigned int num_neighbors=neighbors.size();
      bool do_sort=false;
      for (unsigned int k=0;k<points.size();k++){
	for (unsigned int j=0;j<num_neighbors;j++){
	  if (abs(neighbors[j]->wire->wire-points[k]->wire->wire)==1
	      && neighbors[j]->wire->origin(2)==points[k]->wire->origin(2)){
	    points[k]->status|=USED_IN_SEGMENT;
	    neighbors.push_back(points[k]);
	    do_sort=true;
	  }      
	}
      }
      // Sort if we added another point
      if (do_sort)
	std::sort(neighbors.begin(),neighbors.end(),DFDCSegment_package_cmp);
    
      // Matrix of points on track 
      DMatrix XYZ(neighbors.size()+1,3);
      DMatrix CR(neighbors.size()+1,neighbors.size()+1);
   
      // Arc lengths in helical model are referenced relative to the plane
      // ref_plane within a segment.  For a 6 hit segment, ref_plane=2 is 
      // roughly in the center of the segment.
      ref_plane=2;  
   
      // Perform the Riemann Helical Fit on the track segment
      jerror_t error=RiemannHelicalFit(neighbors,CR,XYZ);   /// initial hit-based fit
       
      // Correct for the Lorentz effect given DOCAs
      if (error==NOERROR){
	// Correct for the Lorentz effect given DOCAs
	vector<DFDCPseudo> neighbors_old;
	for (int i = 0; i < neighbors.size(); i++) {
	  neighbors_old.push_back(*(neighbors[i]));
	}
	CorrectPoints(neighbors,XYZ);
	for (int i = 0; i < neighbors.size(); i++) {
	  float xcorr = neighbors[i]->x - neighbors_old[i].x;
	  float ycorr = neighbors[i]->y - neighbors_old[i].y;
	  cout << "xcorr = " << xcorr << " ycorr = " << ycorr << endl;
	  neighbors[i]->xcorr = xcorr;
	  neighbors[i]->ycorr = ycorr;
	}

	ref_plane=0; 

	// Fit to "space" points
	error=RiemannHelicalFit(neighbors,CR,XYZ); /// time-based fit with corrected points

	// Final correction 
	if (error==NOERROR){
	  vector<DFDCPseudo> neighbors_old;
	  for (int i = 0; i < neighbors.size(); i++) {
	    neighbors_old.push_back(*(neighbors[i]));
	  }
	  CorrectPoints(neighbors,XYZ); /// correct again based on time-based result
	  for (int i = 0; i < neighbors.size(); i++) {
	    float xcorr = neighbors[i]->x - neighbors_old[i].x;
	    float ycorr = neighbors[i]->y - neighbors_old[i].y;
	    cout << "xcorrf = " << xcorr << " ycorrf = " << ycorr << endl;
	  }
	  for (unsigned int m=0;m<neighbors.size();m++){
	    fdc_track_t temp;
	    temp.hit_id=m;
	    double sperp=charge*(XYZ(m,2)-XYZ(ref_plane,2))/tanl; 
	    double sinp=sin(Phi1+sperp/rc);
	    double cosp=cos(Phi1+sperp/rc);
	    XYZ(m,0)=xc+rc*cosp;
	    XYZ(m,1)=yc+rc*sinp;
	    temp.dx=XYZ(m,0)-neighbors[m]->x; // residuals
	    temp.dy=XYZ(m,1)-neighbors[m]->y;
	    temp.s=(XYZ(m,2)-zvertex)/sin(atan(tanl)); // path length
	    temp.chi2=(temp.dx*temp.dx+temp.dy*temp.dy)/CR(m,m);
	    fdc_track[m]=temp;	
	  } 
	}
      }

      if (rc>0.){
	// guess for curvature
	kappa=charge/2./rc;  
	
	// Estimate for azimuthal angle
	phi0=atan2(-xc,yc); 
	if (charge<0) phi0+=M_PI;
	// Look for distance of closest approach nearest to target
	D=-charge*rc-xc/sin(phi0);
      }
      
      // Initialize seed track parameters
      Seed(0,0)=kappa;    // Curvature 
      Seed(1,0)=phi0;      // Phi
      Seed(2,0)=D;       // D=distance of closest approach to origin   
      Seed(3,0)=tanl;     // tan(lambda), lambda=dip angle
      Seed(4,0)=zvertex;       // z-position at closest approach to origin
      for (unsigned int i=0;i<5;i++) Cov(i,i)=1.;
      
      segment->S.ResizeTo(Seed);
      segment->S=Seed;
      segment->cov.ResizeTo(Cov);
      segment->cov=Cov;
      segment->hits=neighbors;
      segment->track=fdc_track;
      segment->xc=xc;
      segment->yc=yc;
      segment->rc=rc;
      segment->Phi1=Phi1;
      segment->chisq=chisq;
      
      _data.push_back(segment);
    }
  }

  return NOERROR;
}

// Get state vector in translated coordinate system
jerror_t DFDCSegment_factory::GetStateVector(double xold, double yold,
    double old_z,double x, double y,double z,DMatrix S,DMatrix &S1){
  double phi=S(1,0);
  double D=S(2,0);
  double dx=x-xold+D*sin(phi);
  double dy=y-yold-D*cos(phi);
  double dz=z-old_z;
  double kappa=S(0,0);
  double xc=dx-(D+1./2./kappa)*sin(phi);
  double yc=dy+(D+1./2./kappa)*cos(phi);
  double q=kappa/fabs(kappa);
  double rc=sqrt(xc*xc+yc*yc);
  double tanl=S(3,0);
  double z0=S(4,0);
  double phi1=atan2(-xc,yc);

  if (q<0) phi1+=M_PI;

  S1(0,0)=S(0,0);
  S1(1,0)=phi1;
  S1(2,0)=q*rc-1./2./kappa;
  S1(3,0)=tanl;
  S1(4,0)=z0+dz+tanl*(phi1-phi)/2./kappa;
  
  return NOERROR;
}


// Computation of transport matrix for S
jerror_t DFDCSegment_factory:: GetStateTransportMatrix(double xold, 
	double yold, double x, double y,DMatrix S, DMatrix &F){
  // Old version of state vector 
  double tanl=S(3,0);      
  double kappa=S(0,0);
  double phi=S(1,0);
  double D=S(2,0);

  double dx=x-xold+D*sin(phi);
  double dy=y-yold-D*cos(phi);
  double xc=dx-(D+1./2./kappa)*sin(phi);
  double yc=dy+(D+1./2./kappa)*cos(phi);
  double phi1=atan2(-xc,yc);
  double q=kappa/fabs(kappa);
  double rc2=xc*xc+yc*yc;
  double rc=sqrt(rc2);

  if (q<0) phi1+=M_PI;

  // Some temperorary variables for computing F
  double a=yc*sin(phi)+xc*cos(phi);
  double c=yc*cos(phi)-xc*sin(phi);
  
  // Transport matrix for S
  F(0,0)=F(3,3)=F(4,4)=1.;
  F(1,0)=-a/2./kappa/kappa/rc2;
  F(1,1)= (D+1./2./kappa)*c/rc2;
  F(1,2)=a/rc2;
  F(2,0)=(1.-q*c/rc)/2./kappa/kappa;
  F(2,1)=-q*a/rc*(D+1./2./kappa);
  F(2,2)=q*c/rc;
  F(4,0)=tanl/2./kappa*((phi-phi1)/kappa+F(0,1));
  F(4,1)=tanl/2./kappa*(F(1,1)-1.);
  F(4,2)=tanl/2./kappa*F(1,2);
  F(4,3)=(phi1-phi)/2./kappa;

  return NOERROR;

}

// Get covariance matrix due to multiple scattering
jerror_t DFDCSegment_factory::GetProcessNoiseCovariance(double x, double y,
  double z, DMatrix S,vector<DFDCPseudo*>points,double mass_hyp, DMatrix &Q){
  // Track parameters
  double tanl=S(3,0);
  double cosl=cos(atan(tanl));
  double sinl=sin(atan(tanl));
  double kappa=S(0,0);
  double D=S(2,0);
  double phi=S(1,0);
  double sigma2_ms=0;  	
  
  // Get Bfield
  double Bx,By,Bz,B;
  bfield->GetField(x,y,z,Bx,By,Bz);
  B=sqrt(Bx*Bx+By*By+Bz*Bz);
  
  // Momentum
  double p=0.003*B/2./fabs(kappa)/cosl;
  double beta=p/sqrt(p*p+mass_hyp*mass_hyp);

  //Materials: copper, Kapton, Mylar, Air, Argon, CO2, Rohacell
  double thickness[7]={4e-4,50e-4,13e-4,1.0,0.4,0.6,0.9894};
  double density[7]={8.96,1.42,1.39,1.2931e-3,1.782e-3,1.977e-3,0.032};
  double X0[7]={12.86,40.56,39.95,36.66,19.55,36.2,41.04};
  double material_sum=0.;
  for (unsigned int i=0;i<7;i++){
    material_sum+=thickness[i]*density[i]/X0[i];
  }
  // RMS from multiple scattering
  sigma2_ms=0.0136*0.0136/p/p/beta/beta*material_sum/sinl
    *(1.+0.038*log(material_sum/sinl))*(1.+0.038*log(material_sum/sinl));

  Q(0,0)=kappa*kappa*tanl*tanl;
  Q(0,3)=Q(3,0)=kappa*tanl*(1.+tanl*tanl);
  Q(1,1)=1./cosl/cosl;
  Q(2,2)=D*D/tan(phi)/tan(phi)/cosl/cosl;
  Q(3,3)=(1.+tanl*tanl)*(1.+tanl*tanl);
  
  Q*=sigma2_ms;
  
  return NOERROR;
}

// Compute track projection matrix
jerror_t DFDCSegment_factory::GetTrackProjectionMatrix(double z,
						       DMatrix S,DMatrix &H){
  double tanl=S(3,0);      
  double kappa=S(0,0);
  double phi=S(1,0);
  double D=S(2,0);
  double z0=S(4,0);
  double sperp=(z-z0)/tanl;

  H(0,0)=1./2./kappa/kappa*(cos(phi)*(2.*kappa*sperp*cos(2.*kappa*sperp)
				      -sin(2.*kappa*sperp))
			    +sin(phi)*(1.-cos(2.*kappa*sperp)
				       -2.*kappa*sperp*sin(2.*kappa*sperp)));
  H(1,0)=1./2./kappa/kappa*(sin(phi)*(2.*kappa*sperp*cos(2.*kappa*sperp)
				      -sin(2.*kappa*sperp))
			    -cos(phi)*(1.-cos(2.*kappa*sperp)
				       -2.*kappa*sperp*sin(2.*kappa*sperp)));
  H(0,1)=-D*cos(phi)-1./2./kappa*sin(phi)*sin(2.*kappa*sperp)
    -1./2./kappa*cos(phi)*(1.-cos(2.*kappa*sperp));  
  H(1,1)=-D*sin(phi)+1./2./kappa*cos(phi)*sin(2.*kappa*sperp)
    -1./2./kappa*sin(phi)*(1.-cos(2.*kappa*sperp));
  H(0,2)=-sin(phi);
  H(1,2)=cos(phi);
  H(0,3)=(sin(phi)*sin(2.*kappa*sperp)-cos(phi)*cos(2.*kappa*sperp))*sperp
    /tanl;
  H(1,3)=-(sin(phi)*sin(2.*kappa*sperp)+cos(phi)*cos(2.*kappa*sperp))*sperp
    /tanl;
  H(0,4)=(sin(phi)*sin(2.*kappa*sperp)-cos(phi)*cos(2.*kappa*sperp))/tanl;
  H(1,4)=-(sin(phi)*sin(2.*kappa*sperp)+cos(phi)*cos(2.*kappa*sperp))/tanl;
  
  return NOERROR;
}

// Track position using Riemann Helical fit parameters
jerror_t DFDCSegment_factory::GetHelicalTrackPosition(double z,
                                            const DFDCSegment *segment,
                                                      double &xpos,
                                                      double &ypos){
  double charge=segment->S(0,0)/fabs(segment->S(0,0));
  double sperp=charge*(z-segment->hits[0]->wire->origin(2))/segment->S(3,0);
 
  xpos=segment->xc+segment->rc*cos(segment->Phi1+sperp/segment->rc);
  ypos=segment->yc+segment->rc*sin(segment->Phi1+sperp/segment->rc);
 
  return NOERROR;
}


jerror_t DFDCSegment_factory::GetHelicalTrackPosition(double z,DMatrix S,
			  double  &xpos, double &ypos){
  // Track parameters
  double kappa=S(0,0);
  double phi=S(1,0);
  double D=S(2,0);
  double tanl=S(3,0);
  double z0=S(4,0);
  double sperp=(z-z0)/tanl;
  
  xpos=-D*sin(phi)+(1./2./kappa)*cos(phi)*sin(2.*kappa*sperp)
    -(1./2./kappa)*sin(phi)*(1.-cos(2.*kappa*sperp)); 
  ypos=D*cos(phi)+(1./2./kappa)*sin(phi)*sin(2.*kappa*sperp)
    +(1./2./kappa)*cos(phi)*(1.-cos(2.*kappa*sperp));

  return NOERROR;
}

// Correct avalanche position along wire and incorporate drift data for 
// coordinate away from the wire using results of preliminary hit-based fit
//
jerror_t DFDCSegment_factory::CorrectPoints(vector<DFDCPseudo*>points,
					    DMatrix XYZ){
  for (unsigned int m=0;m<points.size();m++){
    DFDCPseudo *point=points[m];

    double z=point->wire->origin(2);
    double cosangle=point->wire->udir(1);
    double sinangle=point->wire->udir(0);
    //double x=point->x;
    // double y=point->y;
    double x=XYZ(m,0);
    double y=XYZ(m,1);
    double delta_x=0,delta_y=0;   
    // Variance based on expected resolution
    double sigx2=FDC_X_RESOLUTION*FDC_X_RESOLUTION;
    // Place holder for variance for position along wire
    double sigy2=MAX_DEFLECTION*MAX_DEFLECTION/3.;
      
    // Find difference between point on helical path and wire
    double w=x*cosangle-y*sinangle-point->w;
     // .. and use it to determine which sign to use for the drift time data
    double sign=(w>0?1.:-1.);

    // dip angle
    double lambda=atan(tanl);  

    // Get Bfield, needed to guess particle momentum
    double Bx,By,Bz,B;
    bfield->GetField(x,y,z,Bx,By,Bz);
    B=sqrt(Bx*Bx+By*By+Bz*Bz);
    // Momentum and beta
    double p=0.002998*B*rc/cos(lambda);
    double beta=p/sqrt(p*p+0.140*0.140);

    // Correct the drift time for the flight path and convert to distance units
    // assuming the particle is a pion
    delta_x=sign*(point->time-fdc_track[m].s/beta/29.98)*55E-4;

    // Next find correction to y from table of deflections
    if (got_deflection_file){
      double tanr,tanz,r=sqrt(x*x+y*y);
      double phi=atan2(y,x);
      double ytemp[LORENTZ_X_POINTS],ytemp2[LORENTZ_X_POINTS],dummy;
      int imin,imax, ind,ind2;
    
      // Locate positions in x and z arrays given r and z
      locate(lorentz_x,LORENTZ_X_POINTS,r,&ind);
      locate(lorentz_z,LORENTZ_Z_POINTS,z,&ind2);
    
      // First do interpolation in z direction 
      imin=PACKAGE_Z_POINTS*(ind2/PACKAGE_Z_POINTS); // Integer division...
      for (int j=0;j<LORENTZ_X_POINTS;j++){
	polint(&lorentz_z[imin],&lorentz_nx[j][imin],PACKAGE_Z_POINTS,z,
	       &ytemp[j],&dummy);
	polint(&lorentz_z[imin],&lorentz_nz[j][imin],PACKAGE_Z_POINTS,z,
	       &ytemp2[j],&dummy);
      }
      // Then do final interpolation in x direction 
      imin=(ind>0)?(ind-1):0;
      imax=(ind<LORENTZ_X_POINTS-2)?(ind+2):(LORENTZ_X_POINTS-1);
      polint(&lorentz_x[imin],ytemp,imax-imin+1,r,&tanr,&dummy);
      polint(&lorentz_x[imin],ytemp2,imax-imin+1,r,&tanz,&dummy);
      
      // Compute correction
      double alpha=M_PI/2.-lambda;
      delta_y=tanr*delta_x*cos(alpha)-tanz*delta_x*sin(alpha)*cos(phi);

      // Variance based on expected resolution
      sigy2=FDC_Y_RESOLUTION*FDC_Y_RESOLUTION;
    }
    else if (warn){
      fprintf(stderr,
	      "DFDCSegment_factory: No Lorentz deflection file found!\n");
      fprintf(stderr,"DFDCSegment_factory: --> Pseudo-points will not be corrected for Lorentz effect.");
      warn=false;            
    }
      
    // Fill x and y elements with corrected values
    point->ds =-delta_y;     
    point->dw =delta_x;
    point->x=(point->w+point->dw)*cosangle+(point->s+point->ds)*sinangle;
    point->y=-(point->w+point->dw)*sinangle+(point->s+point->ds)*cosangle;
    point->w_x=point->dw*cosangle;
    point->w_y=-point->dw*sinangle;
    point->s_x=point->ds*sinangle;
    point->s_y=point->ds*cosangle;
    point->cov(0,0)=sigx2*cosangle*cosangle+sigy2*sinangle*sinangle;
    point->cov(1,1)=sigx2*sinangle*sinangle+sigy2*cosangle*cosangle;
    point->cov(0,1)=point->cov(1,0)=(sigy2-sigx2)*sinangle*cosangle;

  }
  return NOERROR;
}
  

// Routine that performs the main loop of the Kalman engine
jerror_t DFDCSegment_factory::KalmanLoop(vector<DFDCPseudo*>points,
					 double mass_hyp,
			  DMatrix Seed,DMatrix &S,DMatrix &C,double &chisq){
  DMatrix M(2,1);  // measurement vector
  DMatrix M_pred(2,1); // prediction for hit position
  DMatrix H(2,5);  // Track projection matrix
  DMatrix H_T(5,2); // Transpose of track projection matrix
  DMatrix F(5,5);  // State vector propagation matrix
  DMatrix F_T(5,5); // Transpose of state vector propagation matrix
  DMatrix Q(5,5);  // Process noise covariance matrix
  DMatrix K(5,2);  // Kalman gain matrix
  DMatrix V(2,2);  // Measurement covariance matrix
  DMatrix X(2,1);  // Position on helical track
  DMatrix R(2,1);  // Filtered resididual
  DMatrix R_T(1,2);  // ...and its transpose
  DMatrix RC(2,2);  // Covariance of filtered residual

  double x,y,z,old_z,old_x,old_y;
  z=points[0]->wire->origin(2);
  chisq=0;

  // First guess for state vector comes from the seed track
  S=Seed;
  // Initialization of state vector covariance matrix 
  for (int j=0;j<5;j++){
    C(j,j)=1.;
  }

  // Loop over list of points, updating the state vector at each step 
  for (unsigned int k=0;k<points.size();k++){      
    // The next measurement 
    M(0,0)=points[k]->x;
    M(1,0)=points[k]->y; 
    
    // ... and its covariance matrix  
    V=points[k]->cov;
     
    // Z-positions
    old_z=z;
    z=points[k]->wire->origin(2);
   
    // old position on the seed trajectory
    GetHelicalTrackPosition(old_z,Seed,old_x,old_y);  
    X(0,0)=old_x;
    X(1,0)=old_y;

    // Compute Process noise covariance matrix
    GetProcessNoiseCovariance(old_x,old_y,old_z,Seed,points,mass_hyp,Q);    

    // Current position on seed trajectory
    GetHelicalTrackPosition(z,Seed,x,y); 
  
    // Get state transport matrix describing how S evolves
    GetStateTransportMatrix(old_x,old_y,x,y,Seed,F);
    
      // Transport the covariance matrix
    F_T=DMatrix(DMatrix::kTransposed,F);
    C=F*(C*F_T)+Q;
	
    // Get predicted estimate for S
    S=Seed+F*(S-Seed);
    
    // Get the projection matrix to propagate the track to the next z-plane
    GetTrackProjectionMatrix(z,Seed,H);
    // Get transpose of projection matrix
    H_T = DMatrix(DMatrix::kTransposed,H);
    
    // Get prediction for next measurement
    M_pred=X+H*(S-Seed);
    
    // Compute new V and check that it is invertible
    DMatrix V_new(2,2);
    V_new=V+H*(C*H_T);
    TDecompLU lu(V_new);
    if (lu.Decompose()==false){
      *_log << "Singular matrix"<< endMsg;
      continue;
    }

    // Compute Kalman gain matrix
    K=C*(H_T*DMatrix(DMatrix::kInverted,V_new));
    
    // Update the state vector 
    S=S+K*(M-M_pred); 
      
    // Update state vector covariance matrix
    C=C-K*(H*C);
    
    // filtered residual and its covariance matrix
    GetHelicalTrackPosition(z,S,x,y); 
    R(0,0)=M(0,0)-x;
    R(1,0)=M(1,0)-y;
    R_T=DMatrix(DMatrix::kTransposed,R);
    RC=V-H*(C*H_T);

    // Update chi2 for this segment
    chisq+=(R_T*((DMatrix(DMatrix::kInverted,RC))*R))(0,0);
  }

  return NOERROR;
}


// Linear regression to find charge
double DFDCSegment_factory::GetCharge(unsigned int n,DMatrix XYZ, DMatrix CR, 
				      DMatrix CRPhi){
  double var=0.; 
  double sumv=0.;
  double sumy=0.;
  double sumx=0.;
  double sumxx=0.,sumxy=0,Delta;
  double slope,r2;
  double phi_old=atan2(XYZ(0,1),XYZ(0,0));
  for (unsigned int k=0;k<n;k++){   
    double tempz=XYZ(k,2);
    double phi_z=atan2(XYZ(k,1),XYZ(k,0));
    // Check for problem regions near +pi and -pi
    if (fabs(phi_z-phi_old)>M_PI){  
      if (phi_old<0) phi_z-=2.*M_PI;
      else phi_z+=2.*M_PI;
    }
    r2=XYZ(k,0)*XYZ(k,0)+XYZ(k,1)*XYZ(k,1);
    var=(CRPhi(k,k)+phi_z*phi_z*CR(k,k))/r2;
    sumv+=1./var;
    sumy+=phi_z/var;
    sumx+=tempz/var;
    sumxx+=tempz*tempz/var;
    sumxy+=phi_z*tempz/var;
    phi_old=phi_z;
  }
  Delta=sumv*sumxx-sumx*sumx;
  slope=(sumv*sumxy-sumy*sumx)/Delta; 
  
  // Guess particle charge (+/-1);
  if (slope<0.) return -1.;
  return 1.;
}



//------------------
// toString
//------------------
const string DFDCSegment_factory::toString(void)
{
        // Ensure our Get method has been called so _data is up to date
        Get();
        if(_data.size()<=0)return string(); // don't print anything if we have no data!

	return string();

}
