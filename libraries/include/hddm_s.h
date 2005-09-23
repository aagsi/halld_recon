/*
 * hddm_s.h - DO NOT EDIT THIS FILE
 *
 * This file was generated automatically by hddm-c from the file
 * event.xml
 * This header file defines the c structures that hold the data
 * described in the data model (from event.xml). 
 * Any program that needs access to the data described in the model
 * can include this header file, and make use of the input/output
 * services provided in hddm_s.c
 *
 * The hddm data model tool set was written by
 * Richard Jones, University of Connecticut.
 *
 * For more information see the following web site
 *
 * http://zeus.phys.uconn.edu/halld/datamodel/doc
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <rpc/rpc.h>
#include <string.h>
#include <strings.h>
#include <particleType.h>

#define MALLOC(N,S) malloc(N)
#define CALLOC(N,S) calloc(N,1)
#define FREE(P) free(P)

#ifndef SAW_s_Momentum_t
#define SAW_s_Momentum_t

typedef struct {
   float                E;
   float                px;
   float                py;
   float                pz;
} s_Momentum_t;
#endif /* s_Momentum_t */

#ifndef SAW_s_Properties_t
#define SAW_s_Properties_t

typedef struct {
   int                  charge;
   float                mass;
} s_Properties_t;
#endif /* s_Properties_t */

#ifndef SAW_s_Beam_t
#define SAW_s_Beam_t

typedef struct {
   Particle_t           type;
   s_Momentum_t*        momentum;
   s_Properties_t*      properties;
} s_Beam_t;
#endif /* s_Beam_t */

#ifndef SAW_s_Target_t
#define SAW_s_Target_t

typedef struct {
   Particle_t           type;
   s_Momentum_t*        momentum;
   s_Properties_t*      properties;
} s_Target_t;
#endif /* s_Target_t */

#ifndef SAW_s_Product_t
#define SAW_s_Product_t

typedef struct {
   int                  decayVertex;
   Particle_t           type;
   s_Momentum_t*        momentum;
   s_Properties_t*      properties;
} s_Product_t;

typedef struct {
   unsigned int mult;
   s_Product_t in[1];
} s_Products_t;
#endif /* s_Product_t */

#ifndef SAW_s_Origin_t
#define SAW_s_Origin_t

typedef struct {
   float                t;
   float                vx;
   float                vy;
   float                vz;
} s_Origin_t;
#endif /* s_Origin_t */

#ifndef SAW_s_Vertex_t
#define SAW_s_Vertex_t

typedef struct {
   s_Products_t*        products;
   s_Origin_t*          origin;
} s_Vertex_t;

typedef struct {
   unsigned int mult;
   s_Vertex_t in[1];
} s_Vertices_t;
#endif /* s_Vertex_t */

#ifndef SAW_s_Reaction_t
#define SAW_s_Reaction_t

typedef struct {
   int                  type;
   float                weight;
   s_Beam_t*            beam;
   s_Target_t*          target;
   s_Vertices_t*        vertices;
} s_Reaction_t;

typedef struct {
   unsigned int mult;
   s_Reaction_t in[1];
} s_Reactions_t;
#endif /* s_Reaction_t */

#ifndef SAW_s_Hit_t
#define SAW_s_Hit_t

typedef struct {
   float                dE;
   float                t;
} s_Hit_t;

typedef struct {
   unsigned int mult;
   s_Hit_t in[1];
} s_Hits_t;
#endif /* s_Hit_t */

#ifndef SAW_s_CdcPoint_t
#define SAW_s_CdcPoint_t

typedef struct {
   float                dEdx;
   float                dradius;
   float                phi;
   bool_t               primary;
   float                r;
   int                  track;
   float                z;
} s_CdcPoint_t;

typedef struct {
   unsigned int mult;
   s_CdcPoint_t in[1];
} s_CdcPoints_t;
#endif /* s_CdcPoint_t */

#ifndef SAW_s_Straw_t
#define SAW_s_Straw_t

typedef struct {
   float                phim;
   s_Hits_t*            hits;
   s_CdcPoints_t*       cdcPoints;
} s_Straw_t;

typedef struct {
   unsigned int mult;
   s_Straw_t in[1];
} s_Straws_t;
#endif /* s_Straw_t */

#ifndef SAW_s_Ring_t
#define SAW_s_Ring_t

typedef struct {
   float                radius;
   s_Straws_t*          straws;
} s_Ring_t;

typedef struct {
   unsigned int mult;
   s_Ring_t in[1];
} s_Rings_t;
#endif /* s_Ring_t */

#ifndef SAW_s_CentralDC_t
#define SAW_s_CentralDC_t

typedef struct {
   s_Rings_t*           rings;
} s_CentralDC_t;
#endif /* s_CentralDC_t */

#ifndef SAW_s_Strip_t
#define SAW_s_Strip_t

typedef struct {
   float                u;
   s_Hits_t*            hits;
} s_Strip_t;

typedef struct {
   unsigned int mult;
   s_Strip_t in[1];
} s_Strips_t;
#endif /* s_Strip_t */

#ifndef SAW_s_CathodePlane_t
#define SAW_s_CathodePlane_t

typedef struct {
   float                tau;
   float                z;
   s_Strips_t*          strips;
} s_CathodePlane_t;

typedef struct {
   unsigned int mult;
   s_CathodePlane_t in[1];
} s_CathodePlanes_t;
#endif /* s_CathodePlane_t */

#ifndef SAW_s_FdcPoint_t
#define SAW_s_FdcPoint_t

typedef struct {
   float                dEdx;
   float                dradius;
   bool_t               primary;
   int                  track;
   float                x;
   float                y;
   float                z;
} s_FdcPoint_t;

typedef struct {
   unsigned int mult;
   s_FdcPoint_t in[1];
} s_FdcPoints_t;
#endif /* s_FdcPoint_t */

#ifndef SAW_s_Wire_t
#define SAW_s_Wire_t

typedef struct {
   float                u;
   s_Hits_t*            hits;
   s_FdcPoints_t*       fdcPoints;
} s_Wire_t;

typedef struct {
   unsigned int mult;
   s_Wire_t in[1];
} s_Wires_t;
#endif /* s_Wire_t */

#ifndef SAW_s_AnodePlane_t
#define SAW_s_AnodePlane_t

typedef struct {
   float                tau;
   float                z;
   s_Wires_t*           wires;
} s_AnodePlane_t;

typedef struct {
   unsigned int mult;
   s_AnodePlane_t in[1];
} s_AnodePlanes_t;
#endif /* s_AnodePlane_t */

#ifndef SAW_s_Chamber_t
#define SAW_s_Chamber_t

typedef struct {
   int                  layer;
   int                  module;
   s_CathodePlanes_t*   cathodePlanes;
   s_AnodePlanes_t*     anodePlanes;
} s_Chamber_t;

typedef struct {
   unsigned int mult;
   s_Chamber_t in[1];
} s_Chambers_t;
#endif /* s_Chamber_t */

#ifndef SAW_s_ForwardDC_t
#define SAW_s_ForwardDC_t

typedef struct {
   s_Chambers_t*        chambers;
} s_ForwardDC_t;
#endif /* s_ForwardDC_t */

#ifndef SAW_s_Paddle_t
#define SAW_s_Paddle_t

typedef struct {
   float                phim;
   s_Hits_t*            hits;
} s_Paddle_t;

typedef struct {
   unsigned int mult;
   s_Paddle_t in[1];
} s_Paddles_t;
#endif /* s_Paddle_t */

#ifndef SAW_s_StartPoint_t
#define SAW_s_StartPoint_t

typedef struct {
   float                dEdx;
   float                phi;
   bool_t               primary;
   float                r;
   float                t;
   int                  track;
   float                z;
} s_StartPoint_t;

typedef struct {
   unsigned int mult;
   s_StartPoint_t in[1];
} s_StartPoints_t;
#endif /* s_StartPoint_t */

#ifndef SAW_s_StartCntr_t
#define SAW_s_StartCntr_t

typedef struct {
   s_Paddles_t*         paddles;
   s_StartPoints_t*     startPoints;
} s_StartCntr_t;
#endif /* s_StartCntr_t */

#ifndef SAW_s_Shower_t
#define SAW_s_Shower_t

typedef struct {
   float                E;
   float                t;
} s_Shower_t;

typedef struct {
   unsigned int mult;
   s_Shower_t in[1];
} s_Showers_t;
#endif /* s_Shower_t */

#ifndef SAW_s_Upstream_t
#define SAW_s_Upstream_t

typedef struct {
   s_Showers_t*         showers;
} s_Upstream_t;
#endif /* s_Upstream_t */

#ifndef SAW_s_Downstream_t
#define SAW_s_Downstream_t

typedef struct {
   s_Showers_t*         showers;
} s_Downstream_t;
#endif /* s_Downstream_t */

#ifndef SAW_s_Cone_t
#define SAW_s_Cone_t

typedef struct {
   int                  sector;
   s_Upstream_t*        upstream;
   s_Downstream_t*      downstream;
} s_Cone_t;

typedef struct {
   unsigned int mult;
   s_Cone_t in[1];
} s_Cones_t;
#endif /* s_Cone_t */

#ifndef SAW_s_Shell_t
#define SAW_s_Shell_t

typedef struct {
   int                  layer;
   s_Cones_t*           cones;
} s_Shell_t;

typedef struct {
   unsigned int mult;
   s_Shell_t in[1];
} s_Shells_t;
#endif /* s_Shell_t */

#ifndef SAW_s_Mod_t
#define SAW_s_Mod_t

typedef struct {
   int                  module;
   s_Shells_t*          shells;
} s_Mod_t;

typedef struct {
   unsigned int mult;
   s_Mod_t in[1];
} s_Mods_t;
#endif /* s_Mod_t */

#ifndef SAW_s_BarrelShower_t
#define SAW_s_BarrelShower_t

typedef struct {
   float                E;
   float                phi;
   bool_t               primary;
   float                r;
   float                t;
   int                  track;
   float                z;
} s_BarrelShower_t;

typedef struct {
   unsigned int mult;
   s_BarrelShower_t in[1];
} s_BarrelShowers_t;
#endif /* s_BarrelShower_t */

#ifndef SAW_s_BarrelEMcal_t
#define SAW_s_BarrelEMcal_t

typedef struct {
   s_Mods_t*            mods;
   s_BarrelShowers_t*   barrelShowers;
} s_BarrelEMcal_t;
#endif /* s_BarrelEMcal_t */

#ifndef SAW_s_Flash_t
#define SAW_s_Flash_t

typedef struct {
   float                pe;
   float                t;
} s_Flash_t;

typedef struct {
   unsigned int mult;
   s_Flash_t in[1];
} s_Flashes_t;
#endif /* s_Flash_t */

#ifndef SAW_s_Section_t
#define SAW_s_Section_t

typedef struct {
   float                phim;
   s_Flashes_t*         flashes;
} s_Section_t;

typedef struct {
   unsigned int mult;
   s_Section_t in[1];
} s_Sections_t;
#endif /* s_Section_t */

#ifndef SAW_s_CerenkovPoint_t
#define SAW_s_CerenkovPoint_t

typedef struct {
   float                E;
   bool_t               primary;
   float                px;
   float                py;
   float                pz;
   float                t;
   int                  track;
   float                x;
   float                y;
   float                z;
} s_CerenkovPoint_t;

typedef struct {
   unsigned int mult;
   s_CerenkovPoint_t in[1];
} s_CerenkovPoints_t;
#endif /* s_CerenkovPoint_t */

#ifndef SAW_s_Cerenkov_t
#define SAW_s_Cerenkov_t

typedef struct {
   s_Sections_t*        sections;
   s_CerenkovPoints_t*  cerenkovPoints;
} s_Cerenkov_t;
#endif /* s_Cerenkov_t */

#ifndef SAW_s_Left_t
#define SAW_s_Left_t

typedef struct {
   s_Hits_t*            hits;
} s_Left_t;
#endif /* s_Left_t */

#ifndef SAW_s_Right_t
#define SAW_s_Right_t

typedef struct {
   s_Hits_t*            hits;
} s_Right_t;
#endif /* s_Right_t */

#ifndef SAW_s_Hcounter_t
#define SAW_s_Hcounter_t

typedef struct {
   float                y;
   s_Left_t*            left;
   s_Right_t*           right;
} s_Hcounter_t;

typedef struct {
   unsigned int mult;
   s_Hcounter_t in[1];
} s_Hcounters_t;
#endif /* s_Hcounter_t */

#ifndef SAW_s_Top_t
#define SAW_s_Top_t

typedef struct {
   s_Hits_t*            hits;
} s_Top_t;
#endif /* s_Top_t */

#ifndef SAW_s_Bottom_t
#define SAW_s_Bottom_t

typedef struct {
   s_Hits_t*            hits;
} s_Bottom_t;
#endif /* s_Bottom_t */

#ifndef SAW_s_Vcounter_t
#define SAW_s_Vcounter_t

typedef struct {
   float                x;
   s_Top_t*             top;
   s_Bottom_t*          bottom;
} s_Vcounter_t;

typedef struct {
   unsigned int mult;
   s_Vcounter_t in[1];
} s_Vcounters_t;
#endif /* s_Vcounter_t */

#ifndef SAW_s_TofPoint_t
#define SAW_s_TofPoint_t

typedef struct {
   bool_t               primary;
   float                t;
   int                  track;
   float                x;
   float                y;
   float                z;
} s_TofPoint_t;

typedef struct {
   unsigned int mult;
   s_TofPoint_t in[1];
} s_TofPoints_t;
#endif /* s_TofPoint_t */

#ifndef SAW_s_TofHit_t
#define SAW_s_TofHit_t

typedef struct {
   float                e;
   int                  orientation;
   float                t;
   int                  track;
   float                x;
   float                y;
   float                z;
} s_TofHit_t;

typedef struct {
   unsigned int mult;
   s_TofHit_t in[1];
} s_TofHits_t;
#endif /* s_TofHit_t */

#ifndef SAW_s_ForwardTOF_t
#define SAW_s_ForwardTOF_t

typedef struct {
   s_Hcounters_t*       hcounters;
   s_Vcounters_t*       vcounters;
   s_TofPoints_t*       tofPoints;
   s_TofHits_t*         tofHits;
} s_ForwardTOF_t;
#endif /* s_ForwardTOF_t */

#ifndef SAW_s_Column_t
#define SAW_s_Column_t

typedef struct {
   float                x;
   s_Showers_t*         showers;
} s_Column_t;

typedef struct {
   unsigned int mult;
   s_Column_t in[1];
} s_Columns_t;
#endif /* s_Column_t */

#ifndef SAW_s_Row_t
#define SAW_s_Row_t

typedef struct {
   float                y;
   s_Columns_t*         columns;
} s_Row_t;

typedef struct {
   unsigned int mult;
   s_Row_t in[1];
} s_Rows_t;
#endif /* s_Row_t */

#ifndef SAW_s_ForwardShower_t
#define SAW_s_ForwardShower_t

typedef struct {
   float                E;
   bool_t               primary;
   float                t;
   int                  track;
   float                x;
   float                y;
   float                z;
} s_ForwardShower_t;

typedef struct {
   unsigned int mult;
   s_ForwardShower_t in[1];
} s_ForwardShowers_t;
#endif /* s_ForwardShower_t */

#ifndef SAW_s_ForwardEMcal_t
#define SAW_s_ForwardEMcal_t

typedef struct {
   s_Rows_t*            rows;
   s_ForwardShowers_t*  forwardShowers;
} s_ForwardEMcal_t;
#endif /* s_ForwardEMcal_t */

#ifndef SAW_s_UpvLeft_t
#define SAW_s_UpvLeft_t

typedef struct {
   s_Showers_t*         showers;
} s_UpvLeft_t;
#endif /* s_UpvLeft_t */

#ifndef SAW_s_UpvRight_t
#define SAW_s_UpvRight_t

typedef struct {
   s_Showers_t*         showers;
} s_UpvRight_t;
#endif /* s_UpvRight_t */

#ifndef SAW_s_UpvRow_t
#define SAW_s_UpvRow_t

typedef struct {
   float                y;
   s_UpvLeft_t*         upvLeft;
   s_UpvRight_t*        upvRight;
} s_UpvRow_t;

typedef struct {
   unsigned int mult;
   s_UpvRow_t in[1];
} s_UpvRows_t;
#endif /* s_UpvRow_t */

#ifndef SAW_s_UpvPaddle_t
#define SAW_s_UpvPaddle_t

typedef struct {
   float                y;
   float                z;
   s_UpvLeft_t*         upvLeft;
   s_UpvRight_t*        upvRight;
} s_UpvPaddle_t;

typedef struct {
   unsigned int mult;
   s_UpvPaddle_t in[1];
} s_UpvPaddles_t;
#endif /* s_UpvPaddle_t */

#ifndef SAW_s_UpvShower_t
#define SAW_s_UpvShower_t

typedef struct {
   float                E;
   bool_t               primary;
   float                t;
   int                  track;
   float                x;
   float                y;
   float                z;
} s_UpvShower_t;

typedef struct {
   unsigned int mult;
   s_UpvShower_t in[1];
} s_UpvShowers_t;
#endif /* s_UpvShower_t */

#ifndef SAW_s_UpstreamEMveto_t
#define SAW_s_UpstreamEMveto_t

typedef struct {
   s_UpvRows_t*         upvRows;
   s_UpvPaddles_t*      upvPaddles;
   s_UpvShowers_t*      upvShowers;
} s_UpstreamEMveto_t;
#endif /* s_UpstreamEMveto_t */

#ifndef SAW_s_HitView_t
#define SAW_s_HitView_t

typedef struct {
   s_CentralDC_t*       centralDC;
   s_ForwardDC_t*       forwardDC;
   s_StartCntr_t*       startCntr;
   s_BarrelEMcal_t*     barrelEMcal;
   s_Cerenkov_t*        Cerenkov;
   s_ForwardTOF_t*      forwardTOF;
   s_ForwardEMcal_t*    forwardEMcal;
   s_UpstreamEMveto_t*  upstreamEMveto;
} s_HitView_t;
#endif /* s_HitView_t */

#ifndef SAW_s_PhysicsEvent_t
#define SAW_s_PhysicsEvent_t

typedef struct {
   int                  eventNo;
   int                  runNo;
   s_Reactions_t*       reactions;
   s_HitView_t*         hitView;
} s_PhysicsEvent_t;

typedef struct {
   unsigned int mult;
   s_PhysicsEvent_t in[1];
} s_PhysicsEvents_t;
#endif /* s_PhysicsEvent_t */

#ifndef SAW_s_HDDM_t
#define SAW_s_HDDM_t

typedef struct {
   s_PhysicsEvents_t*   physicsEvents;
} s_HDDM_t;
#endif /* s_HDDM_t */

#ifdef __cplusplus
extern "C" {
#endif

s_HDDM_t* make_s_HDDM();

s_PhysicsEvents_t* make_s_PhysicsEvents(int n);

s_Reactions_t* make_s_Reactions(int n);

s_Beam_t* make_s_Beam();

s_Momentum_t* make_s_Momentum();

s_Properties_t* make_s_Properties();

s_Target_t* make_s_Target();

s_Vertices_t* make_s_Vertices(int n);

s_Products_t* make_s_Products(int n);

s_Origin_t* make_s_Origin();

s_HitView_t* make_s_HitView();

s_CentralDC_t* make_s_CentralDC();

s_Rings_t* make_s_Rings(int n);

s_Straws_t* make_s_Straws(int n);

s_Hits_t* make_s_Hits(int n);

s_CdcPoints_t* make_s_CdcPoints(int n);

s_ForwardDC_t* make_s_ForwardDC();

s_Chambers_t* make_s_Chambers(int n);

s_CathodePlanes_t* make_s_CathodePlanes(int n);

s_Strips_t* make_s_Strips(int n);

s_AnodePlanes_t* make_s_AnodePlanes(int n);

s_Wires_t* make_s_Wires(int n);

s_FdcPoints_t* make_s_FdcPoints(int n);

s_StartCntr_t* make_s_StartCntr();

s_Paddles_t* make_s_Paddles(int n);

s_StartPoints_t* make_s_StartPoints(int n);

s_BarrelEMcal_t* make_s_BarrelEMcal();

s_Mods_t* make_s_Mods(int n);

s_Shells_t* make_s_Shells(int n);

s_Cones_t* make_s_Cones(int n);

s_Upstream_t* make_s_Upstream();

s_Showers_t* make_s_Showers(int n);

s_Downstream_t* make_s_Downstream();

s_BarrelShowers_t* make_s_BarrelShowers(int n);

s_Cerenkov_t* make_s_Cerenkov();

s_Sections_t* make_s_Sections(int n);

s_Flashes_t* make_s_Flashes(int n);

s_CerenkovPoints_t* make_s_CerenkovPoints(int n);

s_ForwardTOF_t* make_s_ForwardTOF();

s_Hcounters_t* make_s_Hcounters(int n);

s_Left_t* make_s_Left();

s_Right_t* make_s_Right();

s_Vcounters_t* make_s_Vcounters(int n);

s_Top_t* make_s_Top();

s_Bottom_t* make_s_Bottom();

s_TofPoints_t* make_s_TofPoints(int n);

s_TofHits_t* make_s_TofHits(int n);

s_ForwardEMcal_t* make_s_ForwardEMcal();

s_Rows_t* make_s_Rows(int n);

s_Columns_t* make_s_Columns(int n);

s_ForwardShowers_t* make_s_ForwardShowers(int n);

s_UpstreamEMveto_t* make_s_UpstreamEMveto();

s_UpvRows_t* make_s_UpvRows(int n);

s_UpvLeft_t* make_s_UpvLeft();

s_UpvRight_t* make_s_UpvRight();

s_UpvPaddles_t* make_s_UpvPaddles(int n);

s_UpvShowers_t* make_s_UpvShowers(int n);

#ifdef __cplusplus
}
#endif

#ifndef s_DocumentString
#define s_DocumentString

extern char HDDM_s_DocumentString[];

#ifdef INLINE_PREPEND_UNDERSCORES
#define inline __inline
#endif

#endif /* s_DocumentString */

#ifndef HDDM_STREAM_INPUT
#define HDDM_STREAM_INPUT -91
#define HDDM_STREAM_OUTPUT -92

struct popNode_s {
   void* (*unpacker)(XDR*, struct popNode_s*);
   int inParent;
   int popListLength;
   struct popNode_s* popList[99];
};
typedef struct popNode_s popNode;

typedef struct {
   FILE* fd;
   int iomode;
   char* filename;
   XDR* xdrs;
   popNode* popTop;
} s_iostream_t;

#endif /* HDDM_STREAM_INPUT */

#ifdef __cplusplus
extern "C" {
#endif

s_HDDM_t* read_s_HDDM(s_iostream_t* fp);

int flush_s_HDDM(s_HDDM_t* this1,s_iostream_t* fp);

s_iostream_t* open_s_HDDM(char* filename);

s_iostream_t* init_s_HDDM(char* filename);

void close_s_HDDM(s_iostream_t* fp);

#ifdef __cplusplus
}
#endif
