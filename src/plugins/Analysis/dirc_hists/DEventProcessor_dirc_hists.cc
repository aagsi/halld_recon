// -----------------------------------------
// DEventProcessor_dirc_hists.cc
// -----------------------------------------

#include "DEventProcessor_dirc_hists.h"

// Routine used to create our DEventProcessor
extern "C" {
  void InitPlugin(JApplication *app) {
    InitJANAPlugin(app);
    app->AddProcessor(new DEventProcessor_dirc_hists());
  }
}

DEventProcessor_dirc_hists::DEventProcessor_dirc_hists() {
}

DEventProcessor_dirc_hists::~DEventProcessor_dirc_hists() {
}

jerror_t DEventProcessor_dirc_hists::init(void) {
  
  DIRC_TRUTH_BARHIT = false;
  if(gPARMS->Exists("DIRC:TRUTH_BARHIT"))
	  gPARMS->GetParameter("DIRC:TRUTH_BARHIT",DIRC_TRUTH_BARHIT);

  TDirectory *dir = new TDirectoryFile("DIRC","DIRC");
  dir->cd();
 
  // list of particle IDs for histograms (and alternate hypotheses for likelihood diff)
  deque<TString> locLikelihoodName;
  dFinalStatePIDs.push_back(Positron);    locLikelihoodName.push_back("ln L(e+) - ln L(#pi+)");
  dFinalStatePIDs.push_back(Electron);    locLikelihoodName.push_back("ln L(e-) - ln L(#pi-)");
  dFinalStatePIDs.push_back(PiPlus);      locLikelihoodName.push_back("ln L(#pi+) - ln L(K+)");
  dFinalStatePIDs.push_back(PiMinus);     locLikelihoodName.push_back("ln L(#pi-) - ln L(K-)");
  dFinalStatePIDs.push_back(KPlus);       locLikelihoodName.push_back("ln L(#pi+) - ln L(K+)");
  dFinalStatePIDs.push_back(KMinus);      locLikelihoodName.push_back("ln L(#pi-) - ln L(K-)");
  dFinalStatePIDs.push_back(Proton);      locLikelihoodName.push_back("ln L(K+) - ln L(p)");
  dFinalStatePIDs.push_back(AntiProton);  locLikelihoodName.push_back("ln L(K-) - ln L(#bar{p}");

  dMaxChannels = 108*64;

  // plots for each bar
  TDirectory *locBarDir = new TDirectoryFile("PerBarDiagnostic","PerBarDiagnostic");
  locBarDir->cd();
  for(int i=0; i<24; i++) {
	  hDiffBar[i] = new TH2I(Form("hDiff_bar%02d",i), Form("Bar %02d; Channel ID; t_{calc}-t_{measured} [ns]; entries [#]", i), dMaxChannels, 0, dMaxChannels, 400,-20,20);
	  hNphCBar[i] = new TH1I(Form("hNphC_bar%02d",i), Form("Bar %02d; # photons", i), 150, 0, 150);
	  hNphCBarVsP[i] = new TH2I(Form("hNphCVsP_bar%d",i), Form("Bar %02d # photons vs. momentum; p (GeV/c); # photons", i), 120, 0, 12.0, 150, 0, 150);
	  hNphCBarInclusive[i] = new TH1I(Form("hNphCInclusive_bar%02d",i), Form("Bar %02d; # photons", i), 150, 0, 150);
	  hNphCBarInclusiveVsP[i] = new TH2I(Form("hNphCInclusiveVsP_bar%d",i), Form("Bar %02d # photons vs. momentum; p (GeV/c); # photons", i), 120, 0, 12.0, 150, 0, 150);
  }
  dir->cd();
 
  // plots for each hypothesis
  for(uint loc_i=0; loc_i<dFinalStatePIDs.size(); loc_i++) {
	  Particle_t locPID = dFinalStatePIDs[loc_i];
	  string locParticleName = ParticleType(locPID);
	  string locParticleROOTName = ParticleName_ROOT(locPID);
	  
	  TDirectory *locParticleDir = new TDirectoryFile(locParticleName.data(),locParticleName.data());
	  locParticleDir->cd();

	  hExtrapolatedBarHitXY[locPID] = new TH2I(Form("hExtrapolatedBarHitXY_%s",locParticleName.data()), "; Bar Hit X (cm); Bar Hit Y (cm)", 200, -100, 100, 200, -100, 100); 
	  hDiff[locPID] = new TH1I(Form("hDiff_%s",locParticleName.data()), Form("; %s t_{calc}-t_{measured} [ns]; entries [#]", locParticleName.data()), 400,-100,100);
	  hDiffVsChannelDirect[locPID] = new TH2I(Form("hDiffVsChannelDirect_%s",locParticleName.data()), Form("; Channel ID; %s t_{calc}-t_{measured} [ns]; entries [#]",locParticleName.data()), dMaxChannels, 0, dMaxChannels, 400,-20,20);
	  hDiffVsChannelReflected[locPID] = new TH2I(Form("hDiffVsChannelReflected_%s",locParticleName.data()), Form("; Channel ID; %s t_{calc}-t_{measured} [ns]; entries [#]",locParticleName.data()), dMaxChannels, 0, dMaxChannels, 400,-20,20);
	  hNphC[locPID] = new TH1I(Form("hNphC_%s",locParticleName.data()), Form("# photons; %s # photons", locParticleROOTName.data()), 150, 0, 150);
	  hNphCInclusive[locPID] = new TH1I(Form("hNphCInclusive_%s",locParticleName.data()), Form("# photons; %s # photons", locParticleROOTName.data()), 150, 0, 150);
	  hThetaC[locPID] = new TH1I(Form("hThetaC_%s",locParticleName.data()), Form("cherenkov angle; %s #theta_{C} [rad]", locParticleROOTName.data()), 250, 0.6, 1.0);
	  hDeltaThetaC[locPID] = new TH1I(Form("hDeltaThetaC_%s",locParticleName.data()), Form("cherenkov angle; %s #Delta#theta_{C} [rad]", locParticleROOTName.data()), 200,-0.2,0.2);
	  hLikelihood[locPID] = new TH1I(Form("hLikelihood_%s",locParticleName.data()), Form("; %s -lnL; entries [#]", locParticleROOTName.data()),1000,0.,1000.);
	  hLikelihoodDiff[locPID] = new TH1I(Form("hLikelihoodDiff_%s",locParticleName.data()), Form("; %s;entries [#]", locLikelihoodName[loc_i].Data()),100,-200.,200.);


	  hNphCVsP[locPID] = new TH2I(Form("hNphCVsP_%s",locParticleName.data()), Form("# photons vs. momentum; p (GeV/c); %s # photons", locParticleROOTName.data()), 120, 0, 12.0, 150, 0, 150);
	  hNphCInclusiveVsP[locPID] = new TH2I(Form("hNphCInclusiveVsP_%s",locParticleName.data()), Form("# photons vs. momentum; p (GeV/c); %s # photons", locParticleROOTName.data()), 120, 0, 12.0, 150, 0, 150);
	  hThetaCVsP[locPID] = new TH2I(Form("hThetaCVsP_%s",locParticleName.data()),  Form("cherenkov angle vs. momentum; p (GeV/c); %s #theta_{C} [rad]", locParticleROOTName.data()), 120, 0.0, 12.0, 250, 0.75, 0.85);
	  hDeltaThetaCVsP[locPID] = new TH2I(Form("hDeltaThetaCVsP_%s",locParticleName.data()),  Form("cherenkov angle vs. momentum; p (GeV/c); %s #Delta#theta_{C} [rad]", locParticleROOTName.data()), 120, 0.0, 12.0, 200,-0.2,0.2);
	  hLikelihoodDiffVsP[locPID] = new TH2I(Form("hLikelihoodDiffVsP_%s",locParticleName.data()),  Form("; p (GeV/c); %s", locLikelihoodName[loc_i].Data()), 120, 0.0, 12.0, 100, -200, 200);

	  hDeltaTVsP[locPID] = new TH2I(Form("hDeltaTVsP_%s",locParticleName.data()), Form("#Delta T vs. momentum; p (GeV/c); %s #Delta T (ns)", locParticleROOTName.data()), 120, 0.0, 12.0, 200, -100, 100);

	  dir->cd();
  }
  
  int locBar = 3;
  string locParticleName = "PiPlus";
  // occupancy for fixed position and momentum
  TDirectory *locMapDir = new TDirectoryFile("HitMapBar3","HitMapBar3");
  locMapDir->cd();
  for(int locXbin=0; locXbin<40; locXbin++) {
	  double xbin_min = -100.0 + locXbin*5.0;
	  double xbin_max = xbin_min + 5.0;
	 
	  hHitTimeMap[locXbin] = new TH1I(Form("hHitTimeMap_%s_%d_%d",locParticleName.data(),locBar,locXbin), Form("Bar %d, xbin [%0.0f,%0.0f]; t_{measured} [ns]; entries [#]",locBar,xbin_min,xbin_max), 100,0,100); 
	  hPixelHitMap[locXbin] = new TH2S(Form("hPixelHit_%s_%d_%d",locParticleName.data(),locBar,locXbin), Form("Bar %d, xbin [%0.0f,%0.0f]; pixel rows; pixel columns", locBar,xbin_min,xbin_max), 144, -0.5, 143.5, 48, -0.5, 47.5);
	  hPixelHitMapReflected[locXbin] = new TH2S(Form("hPixelHitReflected_%s_%d_%d",locParticleName.data(),locBar,locXbin), Form("Bar %d, xbin [%0.0f,%0.0f]; pixel rows; pixel columns", locBar,xbin_min,xbin_max), 144, -0.5, 143.5, 48, -0.5, 47.5);
  }

  gDirectory->cd("/");
 
  return NOERROR;
}

jerror_t DEventProcessor_dirc_hists::brun(jana::JEventLoop *loop, int32_t runnumber)
{
   // get PID algos
   const DParticleID* locParticleID = NULL;
   loop->GetSingle(locParticleID);
   dParticleID = locParticleID;

   vector<const DDIRCGeometry*> locDIRCGeometry;
   loop->Get(locDIRCGeometry);
   dDIRCGeometry = locDIRCGeometry[0];

   // Initialize DIRC LUT
   loop->GetSingle(dDIRCLut);

   return NOERROR;
}

jerror_t DEventProcessor_dirc_hists::evnt(JEventLoop *loop, uint64_t eventnumber) {

  // check trigger type
  const DTrigger* locTrigger = NULL;
  loop->GetSingle(locTrigger);
  if(!locTrigger->Get_IsPhysicsEvent())
	  return NOERROR;

  // retrieve tracks and detector matches 
  vector<const DTrackTimeBased*> locTimeBasedTracks;
  loop->Get(locTimeBasedTracks);

  vector<const DDIRCPmtHit*> locDIRCPmtHits;
  loop->Get(locDIRCPmtHits);

  const DDetectorMatches* locDetectorMatches = NULL;
  loop->GetSingle(locDetectorMatches);
  DDetectorMatches locDetectorMatch = (DDetectorMatches)locDetectorMatches[0];

  // plot DIRC LUT variables for specific tracks  
  for (unsigned int loc_i = 0; loc_i < locTimeBasedTracks.size(); loc_i++){

	  const DTrackTimeBased* locTrackTimeBased = locTimeBasedTracks[loc_i];

	  // require well reconstructed tracks for initial studies
	  int locDCHits = locTrackTimeBased->Ndof + 5;
	  double locTheta = locTrackTimeBased->momentum().Theta()*180/TMath::Pi();
	  double locP = locTrackTimeBased->momentum().Mag();
	  if(locDCHits < 15 || locTheta < 1.0 || locTheta > 12.0 || locP > 12.0)
		  continue;

	  // require has good match to TOF hit for cleaner sample
	  shared_ptr<const DTOFHitMatchParams> locTOFHitMatchParams;
	  bool foundTOF = dParticleID->Get_BestTOFMatchParams(locTrackTimeBased, locDetectorMatches, locTOFHitMatchParams);
	  if(!foundTOF || locTOFHitMatchParams->dDeltaXToHit > 10.0 || locTOFHitMatchParams->dDeltaYToHit > 10.0)
		  continue;

	  Particle_t locPID = locTrackTimeBased->PID();
	  double locMass = ParticleMass(locPID);

	  // get DIRC match parameters (contains LUT information)
	  shared_ptr<const DDIRCMatchParams> locDIRCMatchParams;
	  bool foundDIRC = dParticleID->Get_DIRCMatchParams(locTrackTimeBased, locDetectorMatches, locDIRCMatchParams);
	 
	  if(foundDIRC) {

		  TVector3 posInBar = locDIRCMatchParams->dExtrapolatedPos; 
		  TVector3 momInBar = locDIRCMatchParams->dExtrapolatedMom;
		  double locExpectedThetaC = locDIRCMatchParams->dExpectedThetaC;
		  double locExtrapolatedTime = locDIRCMatchParams->dExtrapolatedTime;
		  int locBar = dDIRCGeometry->GetBar(posInBar.Y());
		  if(locBar > 23) continue; // skip north box for now

		  japp->RootFillLock(this); //ACQUIRE ROOT FILL LOCK
                  hExtrapolatedBarHitXY[locPID]->Fill(posInBar.X(), posInBar.Y());
        	  japp->RootFillUnLock(this); //RELEASE ROOT FILL LOCK          

		  double locAngle = dDIRCLut->CalcAngle(momInBar, locMass);
		  map<Particle_t, double> locExpectedAngle = dDIRCLut->CalcExpectedAngles(momInBar);

		  // get map of DIRCMatches to PMT hits
		  map<shared_ptr<const DDIRCMatchParams>, vector<const DDIRCPmtHit*> > locDIRCTrackMatchParamsMap;
		  locDetectorMatch.Get_DIRCTrackMatchParamsMap(locDIRCTrackMatchParamsMap);
		  map<Particle_t, double> logLikelihoodSum;

	          int locPhotonInclusive = 0;

		  // loop over associated hits for LUT diagnostic plots
		  for(uint loc_i=0; loc_i<locDIRCPmtHits.size(); loc_i++) {
			  vector<pair<double, double>> locDIRCPhotons = dDIRCLut->CalcPhoton(locDIRCPmtHits[loc_i], locExtrapolatedTime, posInBar, momInBar, locExpectedAngle, locAngle, locPID, logLikelihoodSum);
			  double locHitTime = locDIRCPmtHits[loc_i]->t - locExtrapolatedTime;
			  int locChannel = locDIRCPmtHits[loc_i]->ch%dMaxChannels;
			  if(locHitTime > 0 && locHitTime < 100) locPhotonInclusive++;

			  int pixel_row = dDIRCGeometry->GetPixelRow(locChannel);
			  int pixel_col = dDIRCGeometry->GetPixelColumn(locChannel);

			  // if find track which points to relevant bar, fill photon yield and matched
			  int locXbin = (int)(posInBar.X()/5.0) + 19;
			  if(locXbin >= 0 && locXbin < 40 && locBar == 3 && locPID == PiPlus && momInBar.Mag() > 4.0) {
				  
				  japp->RootFillLock(this); //ACQUIRE ROOT FILL LOCK
				  hHitTimeMap[locXbin]->Fill(locHitTime);
				  if(locHitTime < 38)
					  hPixelHitMap[locXbin]->Fill(pixel_row, pixel_col);
				  else
					  hPixelHitMapReflected[locXbin]->Fill(pixel_row, pixel_col);	  
				  japp->RootFillUnLock(this); //RELEASE ROOT FILL LOCK
			  }

			  if(locDIRCPhotons.size() > 0) {

				  // loop over candidate photons
				  for(uint loc_j = 0; loc_j<locDIRCPhotons.size(); loc_j++) {
					  double locDeltaT = locDIRCPhotons[loc_j].first - locHitTime;
					  double locThetaC = locDIRCPhotons[loc_j].second;

					  japp->RootFillLock(this); //ACQUIRE ROOT FILL LOCK
					  
					  if(fabs(locThetaC-locExpectedThetaC)<0.05) {
						  hDiff[locPID]->Fill(locDeltaT);
						  if(locHitTime < 38)
							  hDiffVsChannelDirect[locPID]->Fill(locChannel,locDeltaT);
						  else 
							  hDiffVsChannelReflected[locPID]->Fill(locChannel,locDeltaT);
						  if(locPID == PiPlus || locPID == PiMinus)
							  hDiffBar[locBar]->Fill(locChannel,locDeltaT);
					  }
					  
					  // fill histograms for candidate photons in timing cut
					  if(fabs(locDeltaT) < 100.0) {
						  hThetaC[locPID]->Fill(locThetaC);
						  hDeltaThetaC[locPID]->Fill(locThetaC-locExpectedThetaC);
						  hDeltaThetaCVsP[locPID]->Fill(momInBar.Mag(), locThetaC-locExpectedThetaC);

					  }
					  
					  japp->RootFillUnLock(this); //RELEASE ROOT FILL LOCK
				  }
			  }
		  }
		  
		  // remove final states not considered
		  if(std::find(dFinalStatePIDs.begin(),dFinalStatePIDs.end(),locPID) == dFinalStatePIDs.end())
			  continue;
		    
		  japp->RootFillLock(this); //ACQUIRE ROOT FILL LOCK

		  // fill histograms with per-track quantities
		  hNphC[locPID]->Fill(locDIRCMatchParams->dNPhotons);
		  hNphCInclusive[locPID]->Fill(locPhotonInclusive);
		  hNphCVsP[locPID]->Fill(momInBar.Mag(), locDIRCMatchParams->dNPhotons);
		  hNphCInclusiveVsP[locPID]->Fill(momInBar.Mag(), locPhotonInclusive);
		  hThetaCVsP[locPID]->Fill(momInBar.Mag(), locDIRCMatchParams->dThetaC); 
		  hDeltaTVsP[locPID]->Fill(momInBar.Mag(), locDIRCMatchParams->dDeltaT);

		  if(locPID == PiPlus || locPID == PiMinus) {
			  hNphCBar[locBar]->Fill(locDIRCMatchParams->dNPhotons);
			  hNphCBarInclusive[locBar]->Fill(locPhotonInclusive);
			  hNphCBarVsP[locBar]->Fill(momInBar.Mag(), locDIRCMatchParams->dNPhotons);
			  hNphCBarInclusiveVsP[locBar]->Fill(momInBar.Mag(), locPhotonInclusive);
		  }

		  // for likelihood and difference for given track mass hypothesis
		  if(locPID == Positron || locPID == Electron) {
			  hLikelihood[locPID]->Fill(-1. * locDIRCMatchParams->dLikelihoodElectron);
			  hLikelihoodDiff[locPID]->Fill(locDIRCMatchParams->dLikelihoodElectron - locDIRCMatchParams->dLikelihoodPion);
			  hLikelihoodDiffVsP[locPID]->Fill(locP, locDIRCMatchParams->dLikelihoodElectron - locDIRCMatchParams->dLikelihoodPion);
		  }
		  else if(locPID == PiPlus || locPID == PiMinus) {
			  hLikelihood[locPID]->Fill(-1. * locDIRCMatchParams->dLikelihoodPion);
			  hLikelihoodDiff[locPID]->Fill(locDIRCMatchParams->dLikelihoodPion - locDIRCMatchParams->dLikelihoodKaon);
			  hLikelihoodDiffVsP[locPID]->Fill(locP, locDIRCMatchParams->dLikelihoodPion - locDIRCMatchParams->dLikelihoodKaon);
		  }
		  else if(locPID == KPlus || locPID == KMinus) {
			  hLikelihood[locPID]->Fill(-1. * locDIRCMatchParams->dLikelihoodKaon);
			  hLikelihoodDiff[locPID]->Fill(locDIRCMatchParams->dLikelihoodPion - locDIRCMatchParams->dLikelihoodKaon);
			  hLikelihoodDiffVsP[locPID]->Fill(locP, locDIRCMatchParams->dLikelihoodPion - locDIRCMatchParams->dLikelihoodKaon);
		  }
		  else if(locPID == Proton) {
			  hLikelihood[locPID]->Fill(-1. * locDIRCMatchParams->dLikelihoodProton);
			  hLikelihoodDiff[locPID]->Fill(locDIRCMatchParams->dLikelihoodProton - locDIRCMatchParams->dLikelihoodKaon);
			  hLikelihoodDiffVsP[locPID]->Fill(locP, locDIRCMatchParams->dLikelihoodProton - locDIRCMatchParams->dLikelihoodKaon);
		  }

		  japp->RootFillUnLock(this); //RELEASE ROOT FILL LOCK
	  }
  }
  
  return NOERROR;
}

jerror_t DEventProcessor_dirc_hists::erun(void) {
  return NOERROR;
}

jerror_t DEventProcessor_dirc_hists::fini(void) {
  return NOERROR;
}
