#ifndef ROOT_THcNPSCluster
#define ROOT_THcNPSCluster

//////////////////////////////////////
//
// THcNPSCluster
//
//////////////////////////////////////

#include "THaCluster.h"
#include "TVector3.h"

class THcNPSCluster : public THaCluster {
 public:
  THcNPSCluster();
  THcNPSCluster(Double_t x, Double_t y, Double_t z, Double_t e, Double_t t);
  virtual ~THcNPSCluster() = default;

  virtual void Clear( Option_t* opt="" );
  virtual void Print( Option_t* opt="" ) const;

  // Coordinate transformation from detector plane to lab
  virtual void RotateToLab(Double_t angle, TVector3& vertex, TVector3& pvect);

  // Getter/Setter functions
  bool       HasVertex()          const { return fHasVertex; }
  Double_t   E()                  const { return fE; }
  void       GetTime()                  { return fT; }
  TVector3&  GetPvect()                 { return fPvect; } 
  TVector3&  GetVertex()                { return fVertex; }
  void       GetTheta()                 { return fPvect.Theta(); } // in lab frame
  void       GetPhi()                   { return fPvect.Phi(); }   // in lab frame

  void       SetEnergy(Double_t energy) { fE = energy; }
  void       SetTime(Double_t time)     { fT = time; }
  void       SetVertex(const TVector3& vertex) { fVertex = vertex; fHasVertex = true; }
  void       SetVertex(Double_t vx, Double_t vy, Double_t vz) { fVertex.SetXYZ(vx, vy, vz); fHasVertex = true; }
  void       SetPvect(const TVector3& pvect) { fPvect = pvect; }

 protected:

  Double_t fE;          // Cluster energy deposit
  Double_t fT;          // Cluster time
  bool     fHasVertex;   
  TVector3 fVertex;     // vertex information from other spectrometer
  TVector3 fPvect;      // momentum vector
  TVector3 fCenterLab;  

  ClassDef(THcNPSCluster,0)

};

#endif
