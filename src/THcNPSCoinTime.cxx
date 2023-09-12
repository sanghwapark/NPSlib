#include "THcNPSCoinTime.h"
#include "THcNPSCluster.h"

#include <iostream>

using namespace std;

//________________________________________________________________________
THcNPSCoinTime::THcNPSCoinTime( const char* name, const char* description,
				const char* elecArmName, const char* hadArmName,
				const char* coinname) :
  THaPhysicsModule(name, description),
  fCoinDetName(coinname),
  fCoinDet(nullptr),
  felecArmName(eleArmName),
  fhadArmName(hadArmName),
  fHMSSpect(nullptr),
  fNPSSpect(nullptr)
{
  // Constructor  
}
  
//________________________________________________________________________
THcNPSCoinTime::~THcNPSCoinTime()
{
  // Destructor
  RemoveVariables();
}

//________________________________________________________________________
THcNPSCoinTime::Clear( Option_t* opt )
{

  fROC1_epCoinTime = kBig;
  fROC_RAW_CoinTime = fNPS_RAW_CoinTime = fHMS_RAW_CoinTime = kBig;
  elec_coinCorr = had_coinCorr_Proton = kBig;
}

//________________________________________________________________________
THcNPSCoinTime::Reset( Option_t* opt )
{
  Clear(opt);
}

//________________________________________________________________________
THaAnalysisObject::Estatus THcNPSCoinTime::Init( const TDatime& run_time )
{

  // Locate the spectrometer appratus and save pointer to it
  cout << "*************************************************" << endl;
  cout << "Initializing THcNPSCointTime Physics Modue" << endl;
  cout << "Hadron Arm   -------> " << fhadArmName << endl;
  cout << "Electron Arm -------> " << felecArmName << endl;
  cout << "TrigDet  -------> " << fCoinDetName << endl;
  cout << "**************************************************" << endl;

  fStatus = kOK;

  if (THaPhysicsModule::Init( run_time ) != kOK )
    return fStatus;

  // Get spectrometers
  if (felecArmName == "H") {
    // HMS
    fHMSSpect = dynamic_cast<THaSpectrometer*>
      ( FindModule( felecArmName.Data(), "THcHallCSpectrometer"));
    // NPS
    fNPSSpect = dynamic_cast<THcNPSApparatus*>
      ( FindModule( fhadArmName.Data(), "THcNPSApparatus"));
  }
  else {
    fHMSSpect = dynamic_cast<THaSpectrometer*>
      ( FindModule( fhadArmName.Data(), "THcHallCSpectrometer"));
    
    fNPSpect = dynamic_cast<THcNPSApparatus*>
      ( FindModule( felecArmName.Data(), "THcNPSApparatus"));
  }

  if( !fHMSSpect || !fNPSSpect )
    fStatus = kInitError;
  
  fCoinDet = dynamic_cast<THcTrigDet*>
    ( FindModule( fCoinDetName.Data(), "THcTrigDet"));
  if( !fCoinDet ) {
    cout << "THcCoinTime module  Cannnot find TrigDet = " << fCoinDetName.Data() << endl;
    fStatus = kInitError;
    return fStatus;
  }
  
  // Get NPS calorimeter
  fNPSCalo = dynamic_cast<THcNPSCalorimeter*>(fNPSSpect->GetDetector("cal"));
  if( !fNPSCalo ) {
    cout <<
    fStatus = kInitError;
  }

  // NPS angle   
  fNPSAngle = TMath::DegToRad()*fNPSSpect->GetNPSAngle();
  
  return fStatus;
}

//________________________________________________________________________
Int_t THcNPSCoinTime::ReadDatabase( const TDatime& date )
{
  
  DBRequest list[] = {
    {"eHadCoinTime_Offset", &feHad_CT_Offset,    kDouble, 0, 1}, // coin time offset for ep coincidence
    {"HMS_CentralPathLen",  &fHMScentralPathLen, kDouble, 0, 1},
    {"NPS_CentralPathLen",  &fNPScentralPathLen, kDouble, 0, 1},
    {0}
  };

  // Default values if not read from param file
  feHad_CT_Offset = 0.0;
  fHMScentralPathLen = 22.*100;
  fNPScentralPathLen = 18.1*100; // FIXME: update value for NPS!

  gHcParms->LoadParmValues((DBRequest*)&list, "");

  return kOK;
}

//________________________________________________________________________
Int_t THcNPSCoinTime::DefineVariables( EMode mode )
{

  if( mode == kDefine && fIsSetup ) return kOK;
  fIsSetup = ( mode == kDefine );

  RVarDef vars[] = {
    {"epCoinTime_ROC1",    "ROC1 Corrected ep Coincidence Time", "fROC1_epCoinTime"},
    {"epCoinTime_NPS",     "NPS Corrected ep Coincidence Time",  "fNPS_epCoinTime"},
    {"epCoinTime_HMS",     "HMS Corrected ep Coincidence Time",  "fHMS_epCoinTime"},
    
    {"CoinTime_RAW_ROC1",  "ROC1 RAW Coincidence Time",          "fROC1_RAW_CoinTime"},
    {"CoinTime_RAW_NPS",   "NPS RAW Coincidence Time",           "fNPS_RAW_CoinTime"},
    {"CoinTime_RAW_HMS",   "HMS RAW Coincidence Time",           "fHMS_RAW_CoinTime"},

    {"DeltaHMSPathLength", "DeltaHMSpathLength (cm)",            "fDeltaHMSpathLength"},

    {"elec_coinCorr",          "",  "elec_coinCorr"},
    {"had_coinCorr_proton",    "",  "had_coinCorr_Proton"},
    {0}
  };

  return DefineVarsFromList( vars, mode );
}

//________________________________________________________________________
Int_t THcNPSCoinTime::Process( const THaEvData& evdata )
{

  if( !IsOK() ) return -1;

  // Get HMS tracks
  THaTrackInfo* hms_trkifo = fHMSSpect->GetTrackInfo();
  if( !hms_trkifo ) return 1;

  // Get NPS clusters
  if( fNPSCalo->GetNClusters() == 0 ) return 1;

  theHMSTrack = fHMSSpect->GetGoldenTrack();
  if( !theHMSTrack )
    return 1;

  // Loop over all clusters
  // We don't have a "Golden cluster", use highest energy cluster for now
  Double_t ClustMaxE = 0.;
  Double_t NPS_FPtime = kBig;
  TVector3 pvect;
  TVector3 vertex = theHMSTrack->GetVertex();  // Use vertex from HMS
  for(auto& cluster : fNPSCalo->GetClusters()) {
    if( cluster.E() > ClusterMaxE ) {
      ClusterMaxE = cluster.E();

      // Get P vector in lab frame
      cluster.RotateToLab(fNPSAngle, vertex, pvect);      

      // Cluster time
      NPS_FPtime = cluster.GetTime(); 
    }// if found higher energy cluster
  }

  // HMS
  Double_t hms_xptar = theHMSTrack->GetTTheta();
  Double_t hms_dP = theHMSTrack->GEtDp();
  Double_t hms_xfp = theHMSTrack->GEtX();
  Double_t hms_xpfp = theHMSTrack->GetTheta();
  Double_t hms_ypfp = theHMSTrack->GetPhi();
  Double_t HMS_FPtime = theHMSTrack->GetFPTime();
  
  // Assume these are values when the variable is initialized
  if (NPS_FPtime == -2000 || HMS_FPtime == -2000) return 1;
  if (NPS_FPtime == -1000 || HMS_FPtime == -1000) return 1;

  // We only use ROC1
  pNPS_TdcTime_ROC1 = fCoinDet->Get_CT_Trigtime(0);  // pTRIG1_ROC1
  pHMS_TdcTime_ROC1 = fCoinDet->Get_CT_Trigtime(1);  // pTRIG3_ROC1
  
  fDeltaHMSpathLength = -1.0*(12.462*hms_xpfp + 0.1138*hms_xpfp*hms_xfp - 0.0154*hms_xfp - 72.292*hms_xpfp*hms_xpfp - 0.0000544*hms_xfp*had_xfp - 116.52*hms_ypfp*hms_ypfp);
  fDeltaHMSpathLength = (.12*hms_xptar*1000 +0.17*hms_dP/100.);
  
  // First assume NPS is the electron arm
  Double_t ElecPathLength = fNPScentralPathLen;
  Double_t HadPathLength  = fHMScentralPathLen + fDeltaHMSpathLength;
  Double_t elec_P = pvect.Mag();
  Double_t had_P = theHMSTrack->GetP();
  Int_t sign= -1;
  if(felecArmName == "H") {
    ElecPathLength = fHMScentralPathLen + fDeltaHMSpathLength;
    HadPathLength  = fNPScentralPathLen;    // NPS dP = 0
    elec_P = theHMSTrack->GetP();                     // HMS golden track momentum
    had_P = pvect.Mag();                             // NPS golden cluster momentum
    sign = 1;
  }
  
  // beta calculations beta = v/c = p/E
  elecArm_BetaCalc = elec_P / sqrt(elec_P*elec_P + MASS_ELECTRON*MASS_ELECTRON);
  hadArm_BetaCalc_proton = had_P / sqrt(had_P*had_P + MASS_PROTON*MASS_PROTON);

  //Coincidence Corrections
  elec_coinCorr = (ElecPathLength) / (LIGHTSPEED * elecArm_BetaCalc );
  had_coinCorr_proton = (HadPathLength) / (LIGHTSPEED * hadArm_BetaCalc_proton );

  // Comment out other hadron parts, keeping here in case someone needs them.
  /*
  hadArm_BetaCalc_Kaon = had_P / sqrt(had_P*had_P + kaonMass*kaonMass);
  hadArm_BetaCalc_Pion = had_P / sqrt(had_P*had_P + pionMass*pionMass);	
  hadArm_BetaCalc_Positron = had_P / sqrt(had_P*had_P + positronMass*positronMass);

  had_coinCorr_Kaon =  (HadPathLength)/ (lightSpeed * hadArm_BetaCalc_Kaon );
  had_coinCorr_Pion =  (HadPathLength)/ (lightSpeed * hadArm_BetaCalc_Pion );
  had_coinCorr_Positron = (HadPathLength) / (lightSpeed SHMSadArm_BetaCalc_PSHMStron );
  */  

  //Raw, Uncorrected Coincidence Time
  fROC1_RAW_CoinTime =  (pNPS_TdcTime_ROC1 + NPS_FPtime) - (pHMS_TdcTime_ROC1 + HMS_FPtime);

  // Not sure if we want to keep those
  fNPS_RAW_CoinTime = NPS_FPtime - HMS_FPtime; // essentially same as fHMS_RAW_CoinTime
  fHMS_RAW_CoinTime = NPS_FPtime - HMS_FPtime;

  // Corrected Coincidence Time for ROC1
  fROC1_epCoinTime = fROC1_RAW_CoinTime + sign*( elec_coinCorr-had_coinCorr_proton) - feHad_CT_Offset; 
  fNPS_epCoinTime = fNPS_RAW_CoinTime + sign*( elec_coinCorr-had_coinCorr_proton) - feHad_CT_Offset; // essentially same as fHMS_epCoinTime
  fHMS_epCoinTime = fHMS_RAW_CoinTime + sign*( elec_coinCorr-had_coinCorr_proton) - feHad_CT_Offset; 

  return 0;
}

ClassImp(THcNPSCoinTime)
