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

  Double_t E() const { return fE; }
  void     SetEnergy(Double_t e) { fE = e; }

  Double_t T() const { return fT; }
  void     SetTime(Double_t t) { fT = t; }

 protected:

  Double_t fE; // Cluster energy deposit

  Double_t fT;  // Cluster time
  
  ClassDef(THcNPSCluster,0)

};

#endif
