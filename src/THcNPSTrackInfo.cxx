// THcNPSTrackInfo

#include "THcNPSTrackInfo.h"
#include "THcNPSCluster.h"
#include "THcNPSCalorimeter.h"
#include "THcNPSApparatus.h"
#include "THcReactionPoint.h"
#include "THcParmList.h"
#include "THcGlobals.h"
#include "VarDef.h"
#include "VarType.h"

//_____________________________________________________________________
THcNPSTrackInfo::THcNPSTrackInfo( const char* name, const char* description,
				  const char* nps_apparatus,
				  const char* vertex_module) :
  THaPhysicsModule(name, description),
  fSpectroName(nps_apparatus),
  fVertexModuleName(vertex_module),
  fSpectro(nullptr),
  fNPSCalo(nullptr),
  fVertexModule(nullptr)
{
  // Constructor
  fHasVertex = false;
  fVertex.SetXYZ(kBig, kBig, kBig);
}

//_____________________________________________________________________
THcNPSTrackInfo::~THcNPSTrackInfo()
{
  // Destructor
  DefineVariables( kDelete );
}

//_____________________________________________________________________
void THcNPSTrackInfo::Clear( Option_t* opt )
{
  THaPhysicsModule::Clear(opt);
  fNPSTrk.clear();
  fVertex.SetXYZ(kBig, kBig, kBig);
  fHasVertex = false;
}

//_____________________________________________________________________
THaAnalysisObject::EStatus THcNPSTrackInfo::Init( const TDatime& run_time )
{

  fStatus = kOK;

  // Find NPS Apparatus
  fSpectro = dynamic_cast<THcNPSApparatus*>
    ( FindModule( fSpectroName.Data(), "THcNPSApparatus") );    
  if(!fSpectro) {
    fStatus = kInitError;
    return fStatus;
  }

  // Get NPS calorimeter detector
  fNPSCalo = dynamic_cast<THcNPSCalorimeter*>(fSpectro->GetDetector("cal"));
  fNPSAngle = fSpectro->GetNPSAngle();

  // HMS Vertex Module (ReactionPoint)
  fVertexModule = dynamic_cast<THcReactionPoint*>
    ( FindModule( fVertexModuleName.Data(), "THcReactionPoint") );
  if(!fVertexModule) {
    fStatus = kInitError;
    return fStatus;
  }

  if( (fStatus =THaPhysicsModule::Init( run_time )) != kOK ) {
    return fStatus;
  }

  return fStatus;
}

//_____________________________________________________________________
Int_t THcNPSTrackInfo::DefineVariables( EMode mode )
{

  if( mode == kDefine && fIsSetup ) return kOK;
  fIsSetup = ( mode == kDefine );

  // vx,vy,vz are same as H.react.x/y/z -- saved for xcheck
  // trk.x and trk.y are same as the clusX, clusY from NPSCalorimeter -- saved for xcheck
  RVarDef vars[] = {
    {"vx",    "Vertex x from HMS track",              "fVertex.X()"},
    {"vy",    "Vertex y from HMS track",              "fVertex.Y()"},
    {"vz",    "Vertex z from HMS track",              "fVertex.Z()"},
    {"px",    "Lab momentum px (GeV)",                "fNPSTrk.fPx" },
    {"py",    "Lab momentum py (GeV)",                "fNPSTrk.fPy" },    
    {"pz",    "Lab momentum pz (GeV)",                "fNPSTrk.fPz" },
    {"p",     "Lab momentum (GeV)",                   "fNPSTrk.fP" },
    {"mult",  "Cluster size",                         "fNPSTrk.fMult"},
    {"x",     "x coordinate in detector front plane", "fNPSTrk.fX"},
    {"y",     "y coordinate in detector front plane", "fNPSTrk.fY"},
    { 0 }
  };

  return DefineVarsFromList( vars, mode );
}

//_____________________________________________________________________ 
Int_t THcNPSTrackInfo::Process( const THaEvData& )
{

  if( !IsOK() ) return -1;

  fNPSTrk.clear();  

  if( fNPSCalo->GetNClusters() == 0 ) return 1;

  if( fVertexModule->HasVertex() ) {
    fVertex = fVertexModule->GetVertex();
    fHasVertex = true;
  }
  
  TVector3 pvect;
  for( auto& cluster : fNPSCalo->GetClusters() ) {

    if(fHasVertex) {
      cluster.RotateToLab(fNPSAngle, fVertex, pvect);
      cluster.SetVertexFlag(true);
    }
    else {
      // if there is no vertex found from the HMS, it assumes (0,0,0) in THcNPSCluster,
      // in case one wants to reconstruct the photon momentum vector for NPS single data
      TVector3 Vtx_tmp(0,0,0);
      cluster.RotateToLab(fNPSAngle, Vtx_tmp, pvect);
      cluster.SetVertexFlag(false);
    }

    fNPSTrk.push_back( { pvect.Mag(), pvect.X(), pvect.Y(), pvect.Z(), cluster.X(), cluster.Y(), cluster.GetSize() } );

  }
  return 0;
}

//_____________________________________________________________________ 
ClassImp(THcNPSTrackInfo)
