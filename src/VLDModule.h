#ifndef VLDModule_
#define VLDModule_


/////////////////////////////////////////////////////////////////////
//
//   VLDModule
//
/////////////////////////////////////////////////////////////////////

#include "PipeliningModule.h"
#include <string>
#include <cstdint>
#include <cstring>  // for memset

namespace Decoder {

class VLDModule : public PipeliningModule {

public:

   VLDModule() : VLDModule(0,0) {}
   VLDModule(UInt_t crate, UInt_t slot);
   virtual ~VLDModule();

   using PipeliningModule::GetData;
   using PipeliningModule::Init;

   virtual void Clear( Option_t *opt="" );
   virtual void Init();
   virtual UInt_t LoadSlot( THaSlotData* sldat, const UInt_t* evbuffer, const UInt_t* pstop );
   virtual UInt_t LoadSlot( THaSlotData* sldat, const UInt_t* evbuffer, UInt_t pos, UInt_t len );
   virtual Int_t  Decode( const UInt_t* data );

   inline virtual std::vector<UInt_t> GetChannelMask() { return vld_data.chnmask; }
   inline virtual std::vector<UInt_t> GetLoHiBit() { return vld_data.lohi; }
   inline virtual std::vector<UInt_t> GetConnectorID() { return vld_data.connectid; }

   virtual UInt_t LoadBank( THaSlotData* sldat, const UInt_t* evbuffer,
                            UInt_t pos, UInt_t len );
   virtual UInt_t LoadNextEvBuffer( THaSlotData *sldat );


 private:

   struct vld_header_data_struct {
     uint32_t num_vld, read_count, write_count;      // Block header objects
     uint32_t nwords_inblock;                        // Block trailer objects
     uint32_t slotid, nconnect;                      // Slot header objects
     void clear() { memset(this, 0, sizeof(vld_header_data_struct)); }
   } __attribute__((aligned(32))) vld_header_data;
   
   struct vld_data_struct {
     std::vector<uint32_t> connectid, lohi, chnmask;
     void clear() {
       connectid.clear(); lohi.clear(); chnmask.clear();
     }
   } __attribute__((aligned(128))) vld_data;
   
   Bool_t block_header_found, block_trailer_found, slot_header_found;

   void LoadTHaSlotDataObj( THaSlotData* sldat );
   
   void DecodeBlockHeader( UInt_t pdat, uint32_t data_type_id );
   void DecodeBlockTrailer( UInt_t pdat );
   void DecodeSlotHeader( UInt_t pdat, uint32_t data_type_id );
   void UnsupportedType( UInt_t pdat, uint32_t data_type_id );

   static TypeIter_t fgThisType;
   
   ClassDef(VLDModule,0)        // VLD module
     
  };

}

#endif
