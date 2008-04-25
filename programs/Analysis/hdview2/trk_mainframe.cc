// $Id$
//
//    File: trk_mainframe.cc
// Created: Wed Apr  9 08:11:16 EDT 2008
// Creator: davidl (on Darwin Amelia.local 8.11.1 i386)
//

using namespace std;

#include "trk_mainframe.h"
#include "hdv_mainframe.h"
#include "hdview2.h"
#include "MyProcessor.h"

#include <TGButtonGroup.h>
#include <TGTextEntry.h>
#include <TArrow.h>
#include <TLatex.h>
#include <TGaxis.h>
#include <TColor.h>

// We declare this as a global since putting it in the class would
// require defining the DReferenceTrajectory class to ROOT
static vector<DReferenceTrajectory*> REFTRAJ;

// Ditto
static vector<pair<const DCoordinateSystem*,double> > TRACKHITS;


int colors[]={kBlue, kRed, kMagenta, kGreen, kCyan};
int ncolors=5;


//---------------------------------
// trk_mainframe    (Constructor)
//---------------------------------
trk_mainframe::trk_mainframe(hdv_mainframe *hdvmf, const TGWindow *p, UInt_t w, UInt_t h):TGMainFrame(p,w,h)
{
	this->hdvmf = hdvmf;

	TGLayoutHints *lhints = new TGLayoutHints(kLHintsNormal, 2,2,2,2);
	TGLayoutHints *bhints = new TGLayoutHints(kLHintsBottom|kLHintsCenterX, 2,2,2,2);
	TGLayoutHints *bbhints = new TGLayoutHints(kLHintsTop, 2,2, -8,-8);
	TGLayoutHints *thints = new TGLayoutHints(kLHintsTop|kLHintsCenterX, 2,2,2,2);
	TGLayoutHints *ahints = new TGLayoutHints(kLHintsLeft|kLHintsTop, 2,2,2,2);
	TGLayoutHints *xhints = new TGLayoutHints(kLHintsNormal|kLHintsExpandX, 2,2,2,2);
	//TGLayoutHints *rhints = new TGLayoutHints(kLHintsRight, 2,2,2,2);
	TGLayoutHints *dhints = new TGLayoutHints(kLHintsExpandY|kLHintsCenterY, 2,2,2,2);
	TGLayoutHints *ehints = new TGLayoutHints(kLHintsCenterY, 2,2,2,2);

	TGHorizontalFrame *mainframe = new TGHorizontalFrame(this);
	AddFrame(mainframe, ahints);

	//------- Left side
	TGHorizontalFrame *leftframe = new TGHorizontalFrame(mainframe);
	mainframe->AddFrame(leftframe, ahints);

		//------- Canvas + histo
		TGVerticalFrame *canvasframe = new TGVerticalFrame(leftframe);
		leftframe->AddFrame(canvasframe, bhints);

		int width=325;
		canvas = new TRootEmbeddedCanvas("Track Canvas", canvasframe, width, 2.0*width, kSunkenFrame, GetWhitePixel());
		canvasframe->AddFrame(canvas, lhints);
		canvas->SetScrolling(TGCanvas::kCanvasScrollBoth);
		
		slo = -10.0;
		shi = 437.0;
		resilo = -1000;
		resihi = +1000;
		canvas->GetCanvas()->cd();
		canvas->GetCanvas()->Range(resilo, slo, resihi, shi);
		
		histocanvas = new TRootEmbeddedCanvas("Histo Canvas", canvasframe, width, width/5.0, kSunkenFrame, GetWhitePixel());
		canvasframe->AddFrame(histocanvas, lhints);
		histocanvas->SetScrolling(TGCanvas::kCanvasScrollBoth);
		
		gPad->SetGridy();
		gPad->SetTicky();
		gPad->SetTickx();
		gPad->SetLeftMargin(0);
		gPad->SetRightMargin(0);
		gPad->SetTopMargin(0);
		resi = new TH1D("resi", "", 30, -0.1, 0.1);
		resi->SetStats(0);
		resi->SetFillStyle(3002);
		resi->SetFillColor(kMagenta);
		resi->Draw();

		//------- Hits radio buttons
		TGVButtonGroup *hitsbuttons = new TGVButtonGroup(leftframe, "Hit");
		leftframe->AddFrame(hitsbuttons, thints);
		for(int i=34; i>=0; i--){
			char title[256];
			sprintf(title, "%2d", i);
			TGRadioButton *but = new TGRadioButton(hitsbuttons, title);
			hitsbuttons->AddFrame(but, bbhints);
		}
	
	//------- Right side
	TGVerticalFrame *rightframe = new TGVerticalFrame(mainframe);
	mainframe->AddFrame(rightframe, ahints);
		
		//------- Controls
		TGGroupFrame *controlsframe = new TGGroupFrame(rightframe, "Controls", kHorizontalFrame);
		rightframe->AddFrame(controlsframe, xhints);
		
			//-------- Pan, Zoom, Reset
			TGVerticalFrame *panzoomresetframe = new TGVerticalFrame(controlsframe);
			controlsframe->AddFrame(panzoomresetframe, lhints);
				TGHorizontalFrame *panzoomframe = new TGHorizontalFrame(panzoomresetframe);
				panzoomresetframe->AddFrame(panzoomframe, lhints);
				
					//------- Pan
					TGGroupFrame *panframe = new TGGroupFrame(panzoomframe, "Pan", kHorizontalFrame);
					panzoomframe->AddFrame(panframe, dhints);
						TGTextButton *panleft = new TGTextButton(panframe, "<");
						TGVerticalFrame *updownframe = new TGVerticalFrame(panframe);
							TGTextButton *panup = new TGTextButton(updownframe, "^");
							TGTextButton *pandown = new TGTextButton(updownframe, "v");
							updownframe->AddFrame(panup, dhints);
							updownframe->AddFrame(pandown, dhints);
						TGTextButton *panright = new TGTextButton(panframe, ">");
						panframe->AddFrame(panleft, ehints);
						panframe->AddFrame(updownframe, dhints);
						panframe->AddFrame(panright, ehints);
				
					//------- Zoom
					TGGroupFrame *zoomframe = new TGGroupFrame(panzoomframe, "Zoom", kVerticalFrame);
					panzoomframe->AddFrame(zoomframe, lhints);
						TGTextButton *zoomin = new TGTextButton(zoomframe, " + ");
						TGTextButton *zoomout = new TGTextButton(zoomframe, " - ");
						zoomframe->AddFrame(zoomin, lhints);
						zoomframe->AddFrame(zoomout, lhints);
				
				TGTextButton *reset = new TGTextButton(panzoomresetframe, "reset");
				panzoomresetframe->AddFrame(reset, xhints);
				
			//-------- Event, Info frame
			TGVerticalFrame *eventinfoframe = new TGVerticalFrame(controlsframe);
			controlsframe->AddFrame(eventinfoframe, thints);
		
				//-------- Next, Previous
				TGGroupFrame *prevnextframe = new TGGroupFrame(eventinfoframe, "Event", kHorizontalFrame);
				eventinfoframe->AddFrame(prevnextframe, thints);
					TGTextButton *prev	= new TGTextButton(prevnextframe,	"<-- Prev");
					TGTextButton *next	= new TGTextButton(prevnextframe,	"Next -->");
					prevnextframe->AddFrame(prev, lhints);
					prevnextframe->AddFrame(next, lhints);
				
					next->Connect("Clicked","hdv_mainframe", hdvmf, "DoNext()");
					prev->Connect("Clicked","hdv_mainframe", hdvmf, "DoPrev()");
					next->Connect("Clicked","trk_mainframe", this, "DoNewEvent()");
					prev->Connect("Clicked","trk_mainframe", this, "DoNewEvent()");
					
			//-------- Info
				TGGroupFrame *infoframe = new TGGroupFrame(eventinfoframe, "Info", kVerticalFrame);
				eventinfoframe->AddFrame(infoframe, thints);
		
		//------- Tracks to plot
		for(int i=0; i<4; i++){
			char title[256];
			sprintf(title,"Track %d%s", i+1, i==0 ? " (s-axis source)":"");
			TGGroupFrame *trackframe = new TGGroupFrame(rightframe, title, kHorizontalFrame);
			rightframe->AddFrame(trackframe, xhints);
			
			//------ Color
			TGLabel *lab = new TGLabel(trackframe,"  ");
			trackframe->AddFrame(lab, dhints);
			lab->SetBackgroundColor(gROOT->GetColor(colors[i%ncolors])->GetPixel());
			
			//------ Data type
			TGVerticalFrame *datatypeframe = new TGVerticalFrame(trackframe);
			trackframe->AddFrame(datatypeframe, thints);
			
				TGLabel *datatypelab = new TGLabel(datatypeframe, "Data type:");
				datatypeframe->AddFrame(datatypelab, lhints);

				TGComboBox *datatype	= new TGComboBox(datatypeframe, "DTrackCandidate", i);
				datatypeframe->AddFrame(datatype, lhints);
				this->datatype.push_back(datatype);
				datatype->Resize(120,20);
				datatype->SetUniqueID(i);
				FillDataTypeComboBox(datatype, i==0 ? "DTrack":"<none>");

				datatype->Connect("Selected(Int_t, Int_t)","trk_mainframe", this, "DoTagMenuUpdate(Int_t, Int_t)");
				datatype->Connect("Selected(Int_t)","trk_mainframe", this, "DoRequestFocus(Int_t)");

			//------ Factory tag
			TGVerticalFrame *factorytagframe = new TGVerticalFrame(trackframe);
			trackframe->AddFrame(factorytagframe, thints);
			
				TGLabel *factorytaglab = new TGLabel(factorytagframe, "Factory Tag:");
				factorytagframe->AddFrame(factorytaglab, lhints);

				TGComboBox *factorytag	= new TGComboBox(factorytagframe, "<default>", i);
				factorytagframe->AddFrame(factorytag, lhints);
				this->factorytag.push_back(factorytag);
				factorytag->Resize(100,20);
				factorytag->SetUniqueID(i);
				FillFactoryTagComboBox(factorytag, datatype, "<default>");

				factorytag->Connect("Selected(Int_t, Int_t)","trk_mainframe", this, "DoTrackNumberMenuUpdate(Int_t, Int_t)");
				factorytag->Connect("Selected(Int_t)","trk_mainframe", this, "DoRequestFocus(Int_t)");

			//------ Track Number
			TGVerticalFrame *tracknoframe = new TGVerticalFrame(trackframe);
			trackframe->AddFrame(tracknoframe, thints);
			
				TGLabel *tracknolab = new TGLabel(tracknoframe, "Track:");
				tracknoframe->AddFrame(tracknolab, lhints);

				TGComboBox *trackno	= new TGComboBox(tracknoframe, i==0 ? "0":"", 0);
				tracknoframe->AddFrame(trackno, lhints);
				this->trackno.push_back(trackno);
				trackno->Resize(85, 20);
				trackno->SetUniqueID(i);

				trackno->Connect("Selected(Int_t)","trk_mainframe", this, "DoRequestFocus(Int_t)");
				trackno->Connect("Selected(Int_t)","trk_mainframe", this, "DoRedraw()");
		}


		//------- Track Info
		TGGroupFrame *trackinfoframe = new TGGroupFrame(rightframe, "Track Info", kVerticalFrame);
		rightframe->AddFrame(trackinfoframe, xhints);

		//------- Hit Info
		TGGroupFrame *hitinfoframe = new TGGroupFrame(rightframe, "Hit Info", kVerticalFrame);
		rightframe->AddFrame(hitinfoframe, xhints);


	// Finish up and map the window
	SetWindowName("Hall-D Event Viewer:Track Inspector");
	SetIconName("HDView:TrackInspector");
	MapSubwindows();
	Resize(GetDefaultSize());
	MapWindow();
	
	RequestFocus();
}

//---------------------------------
// ~trk_mainframe    (Destructor)
//---------------------------------
trk_mainframe::~trk_mainframe()
{
	hdvmf->DoClearTrackInspectorPointer();
}

//---------------------------------
// DoNewEvent
//---------------------------------
void trk_mainframe::DoNewEvent(void)
{
	DoUpdateMenus();
	DoRedraw();
}

//---------------------------------
// DoRedraw
//---------------------------------
void trk_mainframe::DoRedraw(void)
{
	// Delete any existing graphics objects
	for(unsigned int i=0; i<graphics.size(); i++)delete graphics[i];
	graphics.clear();

	// Draw axes and scales
	DrawAxes(canvas->GetCanvas(), graphics, "resi (#mum)", "s (cm)");
	
	// Draw hits on main canvas
	DrawHits(graphics);

	// Draw everything
	canvas->GetCanvas()->cd(0);
	for(unsigned int i=0; i<graphics.size(); i++)graphics[i]->Draw();
	canvas->GetCanvas()->Update();
	
	// Update the histogram
	histocanvas->GetCanvas()->cd();
	resi->Fit("gaus","Q");
	//resi->Draw();
	histocanvas->GetCanvas()->Update();
}

//---------------------------------
// DoHitSelect
//---------------------------------
void trk_mainframe::DoHitSelect(void)
{
_DBG__;

}

//---------------------------------
// DoUpdateMenus
//---------------------------------
void trk_mainframe::DoUpdateMenus(void)
{
	for(unsigned int i=0; i<factorytag.size(); i++){
		FillTrackNumberComboBox(trackno[i], datatype[i], factorytag[i], i!=0);
	}
}

//---------------------------------
// DoTagMenuUpdate
//---------------------------------
void trk_mainframe::DoTagMenuUpdate(Int_t widgetId, Int_t id)
{
	if(widgetId>=0 && widgetId<(int)datatype.size()){
		FillFactoryTagComboBox(factorytag[widgetId], datatype[widgetId], factorytag[widgetId]->GetTextEntry()->GetText());
		factorytag[widgetId]->EmitVA("Selected(Int_t, Int_t)", 2, widgetId, id);
	}
}

//---------------------------------
// DoTrackNumberMenuUpdate
//---------------------------------
void trk_mainframe::DoTrackNumberMenuUpdate(Int_t widgetId, Int_t id)
{
	if(widgetId>=0 && widgetId<(int)datatype.size()){
		FillTrackNumberComboBox(trackno[widgetId], datatype[widgetId], factorytag[widgetId], widgetId!=0);
		DoRedraw();
	}
}

//---------------------------------
// DoRequestFocus
//---------------------------------
void trk_mainframe::DoRequestFocus(Int_t id)
{
	RequestFocus();
}

//---------------------------------
// FillDataTypeComboBox
//---------------------------------
void trk_mainframe::FillDataTypeComboBox(TGComboBox* cb, const string &def)
{
	/// Ideally, this would feel out the factories whose
	/// data types are based on DKinematicData. This is
	/// not currently possbile with JANA though so we have
	/// to simply add DTrack, DTrackCandidate, and DMCThrown
	/// explicitly.

	vector<string> facnames;
	if(def=="<none>")facnames.push_back(def);
	facnames.push_back("DTrack");
	facnames.push_back("DTrackCandidate");
	facnames.push_back("DMCThrown");
	
	cb->RemoveAll();
	for(unsigned int i=0; i<facnames.size(); i++){
		cb->AddEntry(facnames[i].c_str(), i);
		if(def==facnames[i]){
			cb->Select(i, kTRUE);
			cb->GetTextEntry()->SetText(def.c_str());
		}
	}
}

//---------------------------------
// FillFactoryTagComboBox
//---------------------------------
void trk_mainframe::FillFactoryTagComboBox(TGComboBox* cb, TGComboBox* datanamecb, const string &def)
{
	/// Fill the given TGComboBox with the tags of the factories that
	/// provide data of the type currently selected in the "datanamecb"
	/// combo box.
	
	// Get the data type that is currently selected
	string dataname = datanamecb->GetTextEntry()->GetText();

	// Get list of all factories
	vector<JFactory_base*> factories;
	gMYPROC->GetFactories(factories);
	
	// Loop over all factories, looking for ones with the desired
	// data name. Add thier tags to the list
	vector<string> tags;
	tags.push_back("<default>");
	for(unsigned int i=0; i<factories.size(); i++){
		if(dataname == factories[i]->GetDataClassName()){
			string tag = factories[i]->Tag();
			if(tag!="")tags.push_back(tag);
		}
	}
	
	cb->RemoveAll();
	for(unsigned int i=0; i<tags.size(); i++){
		cb->AddEntry(tags[i].c_str(), i);
		if(def==tags[i] || (def=="" && tags[i]=="<default>")){
			cb->Select(i, kTRUE);
			cb->GetTextEntry()->SetText(tags[i].c_str());
		}
	}
	
	// If "<none>" is chosen for the data type then clear the
	// displayed area to make it more obvious that it doesn't matter
	if(dataname=="<none>"){
		cb->GetTextEntry()->SetText("");
	}else{
		string str = cb->GetTextEntry()->GetText();
		if(str=="")cb->GetTextEntry()->SetText("<default>");
	}
}

//---------------------------------
// FillTrackNumberComboBox
//---------------------------------
void trk_mainframe::FillTrackNumberComboBox(TGComboBox* cb, TGComboBox* datanamecb, TGComboBox* tagcb, bool add_best_match_option)
{
	/// Fill the given TGComboBox with the track numbers for the factory
	/// currently selected in the "datanamecb" with the tag in the "tagcb"
	/// combo boxes.
	
	// Get the data type and tag that are currently selected
	string dataname = datanamecb->GetTextEntry()->GetText();
	string tag = tagcb->GetTextEntry()->GetText();
	
	
	// Get the number of rows for this factory for this event
	int Nrows = gMYPROC->GetNrows(dataname, tag=="<default>" ? "":tag);

	// Get the currently selected value for the trackno 
	// so we can recycle it if possible.
	string deftrackno = cb->GetTextEntry()->GetText();

	// Add "Best Match" option if specified
	cb->RemoveAll();
	if(add_best_match_option){
		cb->AddEntry("Best Match", 1001);
		cb->Select(1001, kTRUE);
		cb->GetTextEntry()->SetText("Best Match");
	}
	
	// Add track(row) numbers
	for(int i=0; i<Nrows; i++){
		char str[256];
		sprintf(str, "%d", i);
		cb->AddEntry(str, i);
		if(deftrackno==str){
			cb->Select(i, kTRUE);
			cb->GetTextEntry()->SetText(str);
		}
	}
	
	// If "<none>" is chosen for the data type then clear the
	// displayed area to make it more obvious that it doesn't matter
	if(dataname=="<none>" || Nrows==0){
		cb->GetTextEntry()->SetText("");
	}
}

//-------------------
// DrawAxes
//-------------------
void trk_mainframe::DrawAxes(TCanvas *c, vector<TObject*> &graphics, const char *xlab, const char *ylab)
{
	/// Create arrows indicating x and y axes with labels on the specified canvas
	/// and add them to the specified container of graphics objects to be draw later.
	double x1 = c->GetX1();
	double x2 = c->GetX2();
	double y1 = c->GetY1();
	double y2 = c->GetY2();
	double deltax = x2-x1;
	//deltax *= c->GetYsizeReal()/c->GetXsizeReal();
	double deltay = y2-y1;
	double xlo = x1+0.025*deltax;
	double xhi = xlo + 0.06*deltax;
	double ylo = y1+0.015*deltay;
	double yhi = ylo + 0.03*deltay;
	TArrow *yarrow = new TArrow(xlo, ylo, xlo, yhi, 0.02, ">");
	yarrow->SetLineWidth(1.5);
	graphics.push_back(yarrow);
	
	TLatex *ylabel = new TLatex(xlo, yhi+0.005*deltay, ylab);
	ylabel->SetTextAlign(12);
	ylabel->SetTextAngle(90.0);
	ylabel->SetTextSize(0.04);
	graphics.push_back(ylabel);
	
	TArrow *xarrow = new TArrow(xlo, ylo, xhi, ylo, 0.02, ">");
	xarrow->SetLineWidth(1.5);
	graphics.push_back(xarrow);
	
	TLatex *xlabel = new TLatex(xhi+0.005*deltax, ylo, xlab);
	xlabel->SetTextAlign(12);
	xlabel->SetTextSize(0.04);
	graphics.push_back(xlabel);
	
	// Left axis (and grid lines)
	xlo = x1+0.08*deltax;
	ylo = y1+0.05*deltay;
	yhi = y1+0.95*deltay;
	TGaxis *axis = new TGaxis(xlo , ylo, xlo, yhi, ylo, yhi, 510, "-RW");
	graphics.push_back(axis);

	// Right axis
	xhi = x1+0.95*deltax;
	axis = new TGaxis(xhi , ylo, xhi, yhi, ylo, yhi, 510, "+-U");
	graphics.push_back(axis);

	// Top axis
	axis = new TGaxis(xlo , yhi, xhi, yhi, xlo, xhi, 510, "-R");
	graphics.push_back(axis);

	// Bottom axis
	axis = new TGaxis(xlo , ylo, xhi, ylo, xlo, xhi, 510, "+UW");
	graphics.push_back(axis);
	axis = new TGaxis(xlo+0.3*deltax, ylo, xhi, ylo, xlo+0.3*deltax, xhi, 507, "+L");
	graphics.push_back(axis);
}

//-------------------
// DrawHits
//-------------------
void trk_mainframe::DrawHits(vector<TObject*> &graphics)
{
_DBG__;
	// Find the factory name, tag, and track number for the prime track
	string dataname = datatype[0]->GetTextEntry()->GetText();
	string tag = factorytag[0]->GetTextEntry()->GetText();
	string track = trackno[0]->GetTextEntry()->GetText();
	if(track=="")return;
	if(tag=="<default>")tag="";
	unsigned int index = atoi(track.c_str());
	
	// Clear out any existing reference trajectories
	for(unsigned int i=0; i<REFTRAJ.size(); i++)delete REFTRAJ[i];
	REFTRAJ.clear();
	this->resi->Reset();
	
	// Get the reference trajectory for the prime track
	DReferenceTrajectory *rt=NULL;
	gMYPROC->GetDReferenceTrajectory(dataname, tag, index, rt);
	if(rt==NULL)return;
	
	REFTRAJ.push_back(rt);

	// Get a list of ALL wire hits for this event
	vector<pair<const DCoordinateSystem*,double> > allhits;
	gMYPROC->GetAllWireHits(allhits);
	
	// Draw prime track
	DrawHitsForOneTrack(graphics, allhits, rt, 0);
	
	// Draw other tracks
	for(unsigned int i=1; i<datatype.size(); i++){
		dataname = datatype[i]->GetTextEntry()->GetText();
		tag = factorytag[i]->GetTextEntry()->GetText();
		track = trackno[i]->GetTextEntry()->GetText();
		if(track=="")continue;
		if(tag=="<default>")tag="";
		unsigned int index = atoi(track.c_str());
		if(track=="Best Match"){
			// Need to implement algorithm to find the best match
		}
		
		// Get reference trajectory for this track
		DReferenceTrajectory *myrt=NULL;
		gMYPROC->GetDReferenceTrajectory(dataname, tag, index, myrt);
		if(myrt){
			REFTRAJ.push_back(myrt);
			DrawHitsForOneTrack(graphics, allhits, myrt, i);
		}
	}
}

//-------------------
// DrawHitsForOneTrack
//-------------------
void trk_mainframe::DrawHitsForOneTrack(
	vector<TObject*> &graphics,
	vector<pair<const DCoordinateSystem*,double> > &allhits,
	DReferenceTrajectory *rt,
	int index)
{
_DBG_<<"index="<<index<<endl;
	// Clear current hits list
	if(index==0)TRACKHITS.clear();
	
	vector<pair<const DCoordinateSystem*,double> > &hits = index==0 ? allhits:TRACKHITS;
	
	// Loop over all hits and create graphics objects for each
	for(unsigned int i=0; i<hits.size(); i++){
		const DCoordinateSystem *wire = hits[i].first;
		double dist = hits[i].second;
		DVector3 pos_doca, mom_doca;
		double s;
		if(wire==NULL){_DBG_<<"wire==NULL!!"<<endl; continue;}
		if(rt==NULL){_DBG_<<"rt==NULL!!"<<endl; continue;}
		rt->DistToRT(wire, &s);
		rt->GetLastDOCAPoint(pos_doca, mom_doca);
		DVector3 shift = wire->udir.Cross(mom_doca);
		
		// The magnitude of "dist" is based on the drift time
		// which does not yet subtract out the TOF. This can add
		// 50-100 microns to the resolution.
		//
		// What is really needed here is to try different hypotheses
		// as to the particle type. For now, we just assume its a pion
		double mass = 0.13957;
		double beta = 1.0/sqrt(1.0 + pow(mass/mom_doca.Mag(), 2.0))*2.998E10;
		double tof = s/beta/1.0E-9; // in ns
		dist -= tof*55.0E-4;
		shift.SetMag(dist);

		// See comments in DTrack_factory_ALT1.cc::LeastSquaresB
		double u = rt->GetLastDistAlongWire();
		DVector3 pos_wire = wire->origin + u*wire->udir;
		DVector3 pos_diff = pos_doca-pos_wire;
		if(shift.Dot(pos_diff)<0.0)shift = -shift;
		
		// OK. Finally, we can decide on a sign for the residual.
		// We do this by taking the dot product of the shift with
		// the vector pointing to the center of the wire.
		//double sign = (shift.Dot(pos_wire)<0.0) ? -1.0:+1.0;
		double resi = pos_diff.Mag()-dist;
		
		// If the residual is reasonably small, consider this hit to be
		// on this track and record it (if this is the prime track)
		if(index==0)TRACKHITS.push_back(hits[i]);
		
		
_DBG_<<"resi="<<resi<<"  s="<<s<<"   resi*10E4="<<resi*1.0E4<<endl;
		
		if(index==0){
			TMarker *m = new TMarker(resi*1.0E4, s, 20);
			m->SetMarkerSize(1.6);
			m->SetMarkerColor(kYellow);
			graphics.push_back(m);
			
			this->resi->Fill(resi);
		}
		
		TMarker *m = new TMarker(resi*1.0E4, s, 20);
		m->SetMarkerSize(0.8);
		m->SetMarkerColor(colors[index%ncolors]);
		graphics.push_back(m);
	}
}


