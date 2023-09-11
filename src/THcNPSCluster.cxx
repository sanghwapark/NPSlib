#include "THcNPSCluster.h"
#include <iostream>

//______________________________________________________
THcNPSCluster::THcNPSCluster() :
  fE(0)
{
  // Constructor
}

//______________________________________________________
THcNPSCluster::THcNPSCluster(Double_t x, Double_t y, Double_t z, Double_t t, Double_t e)
{  
  fCenter.SetXYZ(x, y, z);
  fT = t;
  fE = e;
}

//______________________________________________________
void THcNPSCluster::Clear( Option_t* )

{
  fCenter.SetXYZ( kBig, kBig, kBig);
  fT = kBig;
  fE = 0;
}

//______________________________________________________
void THcNPSCluster::Print( Option_t* ) const
{
  // Print contents of cluster, XYZ and E

  std::cout << fCenter.X() << " " << fCenter.Y() << " " << fCenter.Z() << " " << fT << " " << fE << std::endl;

}

ClassImp(THcNPSCluster)
