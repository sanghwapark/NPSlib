#ifndef ROOT_THcNPSCluster
#define ROOT_THcNPSCluster

//////////////////////////////////////
//
// THcNPSCluster
//
//////////////////////////////////////

#include "THaCluster.h"

class THcNPSCluster : public THaCluster {
 public:
  THcNPSCluster();
  THcNPSCluster(Double_t x, Double_t y, Double_t z, Double_t t, Double_t e);
  virtual ~THcNPSCluster() = default;

  virtual void Clear( Option_t* opt="" );
  virtual void Print( Option_t* opt="" ) const;

  // Coordinate transformation from detector plane to lab
  virtual void RotateToLab(Double_t angle, TVector3& vertex, TVector3& pvect);

  // Getter/Setter functions
  bool       HasVertex()          const { return fHasVertex; }
  Double_t   E()                  const { return fE; }
  Double_t   T()                  const { return fT; }
  Double_t   GetTime()            const { return fT; }
  TVector3&  GetPvect()                 { return fPvect; } 
  TVector3&  GetVertex()                { return fVertex; }
  Double_t   GetTheta()                 { return fPvect.Theta(); } // in lab frame
  Double_t   GetPhi()                   { return fPvect.Phi(); }   // in lab frame
  Double_t   GetP()               const { return fP; }
  Int_t      GetSize()            const { return fSize; } // cluster size
  void       SetSize(Int_t nhits)            { fSize = nhits; } // cluster size
  void       SetVertexFlag(bool vertex_flag) { fHasVertex = vertex_flag; }
  void       SetEnergy(Double_t energy)      { fE = energy; }
  void       SetTime(Double_t time)          { fT = time; }
  void       SetMomentum( Double_t p )       { fP = p; }
  void       SetPvect(const TVector3& pvect) { fPvect = pvect; }
  void       SetVertex(const TVector3& vertex) 
  { fVertex = vertex; fHasVertex = true; }
  void       SetVertex(Double_t vx, Double_t vy, Double_t vz) 
  { fVertex.SetXYZ(vx, vy, vz); fHasVertex = true; }

 protected:

  Double_t fE;          // Cluster energy deposit
  Double_t fT;          // Cluster time
  Double_t fP;          // momentum
  bool     fHasVertex;   
  TVector3 fVertex;     // vertex information from other spectrometer
  TVector3 fPvect;      // momentum vector
  TVector3 fCenterLab;  
  Int_t    fSize;       // Cluster size

  ClassDef(THcNPSCluster,0)

};

#endif
