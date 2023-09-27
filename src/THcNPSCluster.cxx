#include "THcNPSCluster.h"
#include <iostream>

//______________________________________________________
THcNPSCluster::THcNPSCluster() :
  fE(kBig),
  fT(kBig),
  fP(kBig),
  fHasVertex(false)
{
  // Constructor

  // Initialize momentum and vertex vectors
  fPvect.SetXYZ(0., 0., 0.);
  fVertex.SetXYZ(0., 0., 0.);
  fCenterLab.SetXYZ(0., 0., 0.);
}

//______________________________________________________
THcNPSCluster::THcNPSCluster(Double_t x, Double_t y, Double_t z, Double_t t, Double_t e)
{  
  fCenter.SetXYZ(x, y, z);
  fE = e;
  fT = t;
  fP = e; // assume photon

  fPvect.SetXYZ(0., 0., 0.);
  fVertex.SetXYZ(0., 0., 0.);
  fCenterLab.SetXYZ(0., 0., 0.);
  fHasVertex = false;
}

//______________________________________________________
void THcNPSCluster::Clear( Option_t* )
{
  fCenter.SetXYZ(kBig, kBig, kBig);
  fPvect.SetXYZ( kBig, kBig, kBig);
  fVertex.SetXYZ( kBig, kBig, kBig);
  fCenterLab.SetXYZ( kBig, kBig, kBig);
  fHasVertex = false;
  fE = fT = fP = kBig;
}

//______________________________________________________
void THcNPSCluster::RotateToLab(Double_t angle, TVector3& vertex, TVector3& pvect)
{
  // Set vertex vector
  fVertex = vertex;

  // Rotate along y-axis, correct for vertex
  Double_t x_lab = fCenter.X()*cos(angle)  + fCenter.Z()*sin(angle) - fVertex.X();
  Double_t y_lab = fCenter.Y() - fVertex.Y();
  Double_t z_lab = -fCenter.X()*sin(angle) + fCenter.Z()*cos(angle) - fVertex.Z();

  TVector3 rvect(x_lab, y_lab, z_lab);
  Double_t th = rvect.Theta();
  Double_t ph = rvect.Phi();
  
  fCenterLab = rvect;
  // TVector3: phi is polar angle and theta is azimuth
  pvect.SetXYZ(fE * cos(ph) * sin(th),
	       fE * sin(ph) * sin(th),
	       fE * cos(th));
  
  fPvect = pvect;
}

//______________________________________________________
void THcNPSCluster::Print( Option_t* ) const
{
  // Print contents of cluster, XYZ and E

  std::cout << fCenter.X() << " " << fCenter.Y() << " " << fCenter.Z() << " " << fT << " " << fE << std::endl;

}

ClassImp(THcNPSCluster)
