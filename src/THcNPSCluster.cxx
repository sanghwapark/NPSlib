#include "THcNPSCluster.h"
#include <iostream>

//______________________________________________________
THcNPSCluster::THcNPSCluster() :
  fE(0),
  fT(0),
  fHasVertex(false),
{
  // Constructor

  // Initialize momentum and vertex vectors
  fPvect.SetXYZ(0., 0., 0.);
  fVertex.SetXYZ(0., 0., 0.);
  fCenterLab.SetXYZ(0., 0., 0.,);
}

//______________________________________________________
THcNPSCluster::THcNPSCluster(Double_t x, Double_t y, Double_t z, Double_t e, Double_t t)
{  
  fCenter.SetXYZ(x, y, z);
  fE = e;
  fT = t;

  fPvect.SetXYZ(0., 0., 0.);
  fVertex.SetXYZ(0., 0., 0.);
  fCenterLab.SetXYZ(0., 0., 0.,);
  fHasVertex = false;
}

//______________________________________________________
void THcNPSCluster::Clear( Option_t* )
{
  fCenter.SetXYZ( kBig, kBig, kBig);
  fPvect.SetXYZ( kBig, kBig, kBig);
  fVertex.SetXYZ( kBig, kBig, kBig);
  fCenterLab.SetXYZ( kBig, kBig, kBig);
  fHasVertex = false;
  fE = 0;
  fT = 0;
}

//______________________________________________________
virtual void THcNPSCluster::RotateToLab(Double_t angle, TVector3& vertex, TVector3& pvect)
{
  // Set vertex vector
  fVertex = vertex;
  fHasVertex = true;

  // Rotate along y-axis, correct for vertex
  Double_t x_lab = fCenter.X()*cos(angle)  + fCenter.Z()*sin(angle) - vertex.X();
  Double_t y_lab = fCenter.Y() - vertex.Y();
  Double_t z_lab = -fCenter.X()*sin(angle) + fCenter.Z()*cos(angle) - vertx.Z();

  TVector3 rvect(x_lab, y_lab, z_lab);
  Double_t th = rvect.Theta();
  Double_t ph = rvect.Phi();
  
  fCenterLab = rvect;
  fPvect.SetXYZ(fE * cos(th) * sin(ph),
	    fE * sin(th) * sin(ph),
	    fE * cos(ph));

  pvect = fPvect;
}

//______________________________________________________
void THcNPSCluster::Print( Option_t* ) const
{
  // Print contents of cluster, XYZ and E

  std::cout << fCenter.X() << " " << fCenter.Y() << " " << fCenter.Z() << " " << fE << std::endl;

}

ClassImp(THcNPSCluster)
