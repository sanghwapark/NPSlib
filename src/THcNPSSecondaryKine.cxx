// THcNPSSecondaryKine
//
// This module is to calculate the kinematics
// for NPS DVCS analysis using detected photon
// instead of track info in THcSecondaryKine

/*
  NPS cluster positions are defined in the coordinate system
  +x=horizontal,beam left, +y=up, z=along the central ray
  where the origin is at (XFront=0, YFront=0, Z)
  Note that it's not defined in transport coordinate system

  To get the position in lab frame,
  we just need to rotate along y (assume phi=0)

  X_lab         X_cls
  Y_lab = (R_y) Y_cls
  Z_lab         Z_cls

  R_y = cos(th)  0  sin(th)
        0        1     0
	-sin(th) 0  cos(th)

  Vertex corrections:
  X_lab_corr = X_lab - Vertex(X)  
  Y_lab_corr = Y_lab - Vertex(Y)  
  Z_lab_corr = Z_lab - Vertex(Z)  
 */

#include "THcReactionPoint.h"
#include "THcPrimaryKine.h"
#include "THcNPSApparatus.h"
#include "THcNPSCalorimeter.h"
#include "THcNPSCluster.h"
#include "THcNPSSecondaryKine.h"
#include "THcGlobals.h"
#include "THcParmList.h"
#include "VarDef.h"
#include "VarType.h"
#include "TVector3.h"


#include <iostream>

ClassImp(THcNPSSecondaryKine)

//__________________________________________________________________
THcNPSSecondaryKine::THcNPSSecondaryKine( const char* name, const char* description,
			const char* secondary_spectro,
			const char* primary_kine,
			Double_t secondary_mass,
           	        const char* vertex_module) :
  THcSecondaryKine(name, description, secondary_spectro, primary_kine, secondary_mass),
  fApparatName("NPS"),
  fApparatus(nullptr),
  fNPSCalo(nullptr),
  fVertexModuleName(vertex_module),
  fVertexModule(nullptr),
  fNPSAngle(0)
{
  // Constructor
}

//__________________________________________________________________
THcNPSSecondaryKine::~THcNPSSecondaryKine()
{
  DefineVariables( kDelete );
}

//__________________________________________________________________
THaAnalysisObject::EStatus THcNPSSecondaryKine::Init( const TDatime& run_time )
{

  fStatus = kOK;

  // NPS not HallC spectrometer, use THcApparatus instead
  fApparatus = dynamic_cast<THcNPSApparatus*>
    ( FindModule( fApparatName.Data(), "THcNPSApparatus"));
  if(!fApparatus) {
    fStatus = kInitError;
    return fStatus;
  }

  fPrimary = dynamic_cast<THcPrimaryKine*>
    ( FindModule( fPrimaryName.Data(), "THcPrimaryKine"));
  if(!fPrimary) {
    fStatus = kInitError;
    return fStatus;
  }

  fVertexModule = dynamic_cast<THcReactionPoint*>
    ( FindModule( fVertexModuleName.Data(), "THcReactionPoint"));
  if(!fVertexModule) {
    fStatus = kInitError;
    return fStatus;
  }

  // We get NPS Calorimeter cluster info from this
  // std::cout << "Init THcNPSSecondaryKine: fApparatus->GetDetector("cal")" << std::endl;
  fNPSCalo = dynamic_cast<THcNPSCalorimeter*>(fApparatus->GetDetector("cal"));

  if( (fStatus=THaPhysicsModule::Init( run_time )) != kOK ) {
    return fStatus;
  }

  return fStatus;

}

//__________________________________________________________________
Int_t THcNPSSecondaryKine::DefineVariables( EMode mode )
{

  if( mode == kDefine && fIsSetup ) return kOK;
  fIsSetup = ( mode == kDefine );

  RVarDef vars[] = {
    { "th_xq",    "Polar angle of detected particle with q (rad)",
                  "fTheta_xq" },
    { "ph_xq",    "Azimuth of detected particle with scattering plane (rad)",
                  "fPhi_xq" },
    { "th_bq",    "Polar angle of recoil system with q (rad)", "fTheta_bq" },
    { "ph_bq",    "Azimuth of recoil system with scattering plane (rad)",
                  "fPhi_bq" },
    { "xangle",   "Angle of detected particle with scattered electron (rad)",
                  "fXangle" },
    { "pmiss",    "Missing momentum magnitude (GeV), nuclear physics "
                  "definition (-pB)", "fPmiss" },
    { "pmiss_x",  "x-component of p_miss wrt q (GeV)", "fPmiss_x" },
    { "pmiss_y",  "y-component of p_miss wrt q (GeV)", "fPmiss_y" },
    { "pmiss_z",  "z-component of p_miss, along q (GeV)", "fPmiss_z" },
    { "emiss_nuc",    "Missing energy (GeV), nuclear physics definition "
                  "omega-Tx-Tb", "fEmiss_nuc" },
    { "emiss",    "Missing energy (GeV), ENGINE definition "
                  "omega-Mt-Ex", "fEmiss" },
    { "Mrecoil",  "Invariant mass of recoil system (GeV)", "fMrecoil" },
    { "Erecoil",  "Total energy of recoil system (GeV)", "fErecoil" },
    { "Prec_x",   "x-component of recoil system in lab (GeV/c)", "fB.X()" },
    { "Prec_y",   "y-component of recoil system in lab (GeV/c)", "fB.Y()" },
    { "Prec_z",   "z-component of recoil system in lab (GeV/c)", "fB.Z()" },
    { "tx",       "Kinetic energy of detected particle (GeV)", "fTX" },
    { "tb",       "Kinetic energy of recoil system (GeV)", "fTB" },
    { "px_cm",    "Magnitude of X momentum in CM system (GeV)", "fPX_cm" },
    { "thx_cm",   "Polar angle of X in CM system wrt q (rad)", "fTheta_x_cm" },
    { "phx_cm",   "Azimuth of X in CM system wrt q (rad)", "fPhi_x_cm" },
    { "thb_cm",   "Polar angle of recoil systm in CM wrt q (rad)",
                  "fTheta_b_cm" },
    { "phb_cm",   "Azimuth of recoil system in CM wrt q (rad)", "fPhi_b_cm" },
    { "tx_cm",    "Kinetic energy of X in CM (GeV)", "fTX_cm" },
    { "tb_cm",    "Kinetic energy of B in CM (GeV)", "fTB_cm" },
    { "t_tot_cm", "Total CM kinetic energy", "fTtot_cm" },
    { "MandelS",  "Mandelstam s for secondary vertex (GeV^2)", "fMandelS" },
    { "MandelT",  "Mandelstam t for secondary vertex (GeV^2)", "fMandelT" },
    { "MandelU",  "Mandelstam u for secondary vertex (GeV^2)", "fMandelU" },
    { 0 }
  };

  return DefineVarsFromList( vars, mode );

}

//__________________________________________________________________
Int_t THcNPSSecondaryKine::Process( const THaEvData& )
{

  // Calculate secondary kinematics for e+N --> e'+gamma+X
  // Replace tracks with clusters for calculating secondary kinematics
  // Rest is same as THcSecondaryKine::Process

  if( !IsOK() ) return -1;
  
  // Require valid input data
  if( !fPrimary->DataValid() ) return 2;

  // No valid cluster
  if(fNPSCalo->GetNClusters() == 0) return 1;

  // Find vertex using HMS
  TVector3 vertex;
  if(fVertexModule->HasVertex())
    vertex = fVertexModule->GetVertex();
  else
    vertex.SetXYZ(0,0,0);

  // Set photon 4-momentum in lab frame (z = beam, y = up)
  //
  // In case we have multi clusters we need to set criteria to choose "golden" cluster
  // or calcualte kinematics for all identified clusters, store, use it as a cut
  // For now, use a cluster with highest energy deposit
  Double_t ClusterMaxE = 0;
  TVector3 pvect;
  for(auto& cluster : fNPSCalo->GetClusters())
    if( cluster.E() > ClusterMaxE )
      {
	ClusterMaxE = cluster.E();
	// vertex correction and get P vector
	cluster.RotateToLab(fNPSAngle, vertex, pvect);
	if( vertex.Mag() < 1.e-38 ) cluster.SetVertexFlag(false);
      }

  // 4-momentum of X
  fX.SetVectM( pvect, fMX );

  // 4-momenta of the the primary interaction
  const TLorentzVector* pA  = fPrimary->GetA();  // Initial target
  const TLorentzVector* pA1 = fPrimary->GetA1(); // Final target, fA1 = fA + fQ
  const TLorentzVector* pQ  = fPrimary->GetQ();  // Momentum xfer
  const TLorentzVector* pP1 = fPrimary->GetP1(); // Final electron
  Double_t omega      = fPrimary->GetOmega(); // Energy xfer

  // 4-momentum of undetected recoil system B
  fB = *pA1 - fX;

 // Angle of X with scattered primary particle
  fXangle = fX.Angle( pP1->Vect());

  // Angles of X and B wrt q-vector 
  // xq and bq are the 3-momentum vectors of X and B expressed in
  // the coordinate system where q is the z-axis and the x-axis
  // lies in the scattering plane (defined by q and e') and points
  // in the direction of e', so the out-of-plane angle lies within
  // -90<phi_xq<90deg if X is detected on the downstream/forward side of q.
  TRotation rot_to_q;
  rot_to_q.SetZAxis( pQ->Vect(), pP1->Vect()).Invert();
  TVector3 xq = fX.Vect();
  TVector3 bq = fB.Vect();
  xq *= rot_to_q;  
  bq *= rot_to_q;
  fTheta_xq = xq.Theta();   //"theta_xq"
  fPhi_xq   = xq.Phi();     //"out-of-plane angle", "phi"
  fTheta_bq = bq.Theta();
  fPhi_bq   = bq.Phi();

  // Missing momentum and components wrt q-vector
  // The definition of p_miss as the negative of the undetected recoil
  // momentum is the standard nuclear physics convention.
  TVector3 p_miss = -bq;
  fPmiss   = p_miss.Mag();  //=fB.P()
  //The missing momentum components in the q coordinate system.
  //FIXME: just store p_miss in member variable
  fPmiss_x = p_miss.X();
  fPmiss_y = p_miss.Y();
  fPmiss_z = p_miss.Z();

  // Invariant mass of the recoil system, a.k.a. "missing mass".
  // This invariant mass equals MB(ground state) plus any excitation energy.
  fMrecoil = fB.M();

  // Kinetic energies of X and B
  fTX = fX.E() - fMX;
  fTB = fB.E() - fMrecoil;
  
  // Standard nuclear physics definition of "missing energy":
  // binding energy of X in the target (= removal energy of X).
  // NB: If X is knocked out of a lower shell, the recoil system carries
  // a significant excitation energy. This excitation is included in Emiss
  // here, as it should, since it results from the binding of X.
  fEmiss_nuc = omega - fTX - fTB;
  // Target Mass + Beam Energy - scatt ele energy - hadron total energy
  fEmiss = omega + pA->M() - fX.E();

  // In production reactions, the "missing energy" is defined 
  // as the total energy of the undetected recoil system.
  // This is the "missing mass", Mrecoil, plus any kinetic energy.
  fErecoil = fB.E();

  // Calculate some interesting quantities in the CM system of A'.
  // NB: If the target is initially at rest, the A'-vector (spatial part)
  // is the same as the q-vector, so we could just reuse the q rotation
  // matrix. The following is completely general, i.e. allows for a moving 
  // target.

  // Boost of the A' system.
  // NB: ROOT's Boost() boosts particle to lab if the boost vector points
  // along the particle's lab velocity, so we need the negative here to 
  // boost from the lab to the particle frame.
  Double_t beta = pA1->P()/(pA1->E());
  TVector3 boost(0.,0.,-beta); 

  // CM vectors of X and B. 
  // Express X and B in the frame where q is along the z-axis 
  // - the typical head-on collision picture.
  TRotation rot_to_A1;
  rot_to_A1.SetZAxis( pA1->Vect(), pP1->Vect()).Invert();
  TVector3 x_cm_vect = fX.Vect();
  x_cm_vect *= rot_to_A1;
  TLorentzVector x_cm(x_cm_vect,fX.E());
  x_cm.Boost(boost);
  TLorentzVector b_cm(-x_cm.Vect(), pA1->E()-x_cm.E());
  fPX_cm = x_cm.P();
  // pB_cm, by construction, is the same as pX_cm.

  // CM angles of X and B in the A' frame
  fTheta_x_cm = x_cm.Theta();
  fPhi_x_cm   = x_cm.Phi();
  fTheta_b_cm = b_cm.Theta();
  fPhi_b_cm   = b_cm.Phi();

  // CM kinetic energies of X and B and total kinetic energy
  fTX_cm = x_cm.E() - fMX;
  fTB_cm = b_cm.E() - fMrecoil;
  fTtot_cm = fTX_cm + fTB_cm;
    
  // Mandelstam variables for the secondary vertex.
  // These variables are defined for the reaction gA->XB,
  // where g is the virtual photon (with momentum q), and A, X, and B
  // are as before.

  fMandelS = (*pQ+*pA).M2();
  fMandelT = (*pQ-fX).M2();
  fMandelU = (*pQ-fB).M2();

  fDataValid = true;
  return 0;
  
}

//_____________________________________________________________________________
Int_t THcNPSSecondaryKine::ReadDatabase( const TDatime& date )
{

  // NPS angle
  fNPSAngle = TMath::DegToRad()*fApparatus->GetNPSAngle();

  // cout << "In THcSecondaryKine::ReadDatabase() " << endl;

  // Ignore prefix for now
  /*
  char prefix[2];
  prefix[0] = tolower(GetName()[0]);
  prefix[1] = '\0';
  */
  fOopCentralOffset = 0.0;
  DBRequest list[]={
    {"nps_oopcentral_offset",&fOopCentralOffset, kDouble, 0, 1},
    {"ppartmass",            &fMX, kDouble, 0, 1},
    {0}
  };

  gHcParms->LoadParmValues((DBRequest*)&list,"");
  cout << "THcSecondaryKine particleMASS: " << fMX << endl; 

  // default value
  fMX = 0.0;
  fOopCentralOffset = 0.;

  return kOK;
}

//_____________________________________________________________________________
void THcNPSSecondaryKine::SetApparatus( const char* name )
{
  // this in fact sets the name of THcApparatus instead of Spectrometer
  if( !IsInit() )
    fApparatName = name;
  else
    PrintInitError("SetSpectroemter()");
}
