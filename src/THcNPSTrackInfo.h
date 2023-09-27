#ifndef ROOT_THcNPSTrackInfo
#define ROOT_THcNPSTrackInfo

#include "THaPhysicsModule.h"
#include "TVector3.h"

class THcNPSApparatus;
class THcNPSCalorimeter;
class THcReactionPoint;
class THcNPSCluster;

class THcNPSTrackInfo : public THaPhysicsModule {

 public:
  THcNPSTrackInfo( const char* name, const char* description,
	       const char* spectro = "",
	       const char* vertex_module = "");
  
  virtual ~THcNPSTrackInfo();

  virtual EStatus   Init( const TDatime& run_time );
  virtual void      Clear( Option_t* opt="" );
  virtual Int_t     Process( const THaEvData& );
          void      SetSpectrometer( const char* name ) { fSpectroName = name; } // Treat it as a spectrometer assuming we make it so in the future
          TVector3& GetVertex()                            { return fVertex; }
          bool      HasVertex()                      const { return fHasVertex; }
          void      SetVertex( Double_t vx, Double_t vy, Double_t vz )
          { fVertex.SetXYZ(vx, vy, vz); fHasVertex = true; }
          void      SetVertex( const TVector3& vertex )    { fVertex = vertex; fHasVertex = true; }

 protected:
  
  virtual Int_t DefineVariables( EMode mode = kDefine );
  //virtual Int_t ReadRunDatabase( const TDatime& date );
  
  TString fSpectroName; // not actually spectrometer, but in case we make the NPS as THcHallCSpectrometer in the future..
  TString fVertexModuleName;
  THcNPSApparatus*   fSpectro;
  THcNPSCalorimeter* fNPSCalo;
  THcReactionPoint*  fVertexModule;
  
  bool     fHasVertex;
  TVector3 fVertex;
  Double_t fNPSAngle;

  class NPSTrack {
  public:
    Double_t fP;    // same as NPS cluster Energy
    Double_t fPx;   // Momentum in lab
    Double_t fPy;   // Momentum in lab
    Double_t fPz;   // Momentum in lab 
    Double_t fX;    // position on the detector plane 
    Double_t fY;    // position on the detector plane
    Int_t    fMult; // cluster size
  };
  std::vector<NPSTrack> fNPSTrk;

 public:
  std::vector<NPSTrack> GetTracks() { return fNPSTrk; }


  ClassDef(THcNPSTrackInfo,0)

};

#endif /* ROOT_THcNPSTrackInfo */
