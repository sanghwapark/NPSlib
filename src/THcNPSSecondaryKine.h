#ifndef ROOT_THcNPSSecondaryKine
#define ROOT_THcNPSSecondaryKine

#include "THcSecondaryKine.h"

class THcNPSApparatus;
class THcPrimaryKine;
class THcReactionPoint;
class THcNPSCalorimeter;

class THcNPSSecondaryKine : public THcSecondaryKine {
 public:
  THcNPSSecondaryKine( const char* name, const char* description = "",
		       const char* secondary_spectro = "",
		       const char* primary_kine = "",
		       Double_t secondary_mass = 0.0,
		       const char* vertex_module = "");

  virtual ~THcNPSSecondaryKine();
  virtual EStatus Init( const TDatime& run_time );
  virtual Int_t   ReadDatabase( const TDatime& date );

  // not sure if we ever want to override this..
  virtual Int_t   Process( const THaEvData& );
  
  void SetApparatus( const char* name );

protected:
  TString               fApparatName;
  THcNPSApparatus*      fApparatus;
  THcNPSCalorimeter*    fNPSCalo;
  TString               fVertexModuleName;
  THcReactionPoint*     fVertexModule;

  Double_t fNPSAngle;

  virtual Int_t DefineVariables( EMode mode = kDefine );

  ClassDef(THcNPSSecondaryKine,0)
};

#endif /* ROOT_THcNPSSecondaryKine */
