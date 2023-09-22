//////////////////////////////////////////////////////////////////
//
//   0 block header
//   1 block trailer
//   2 slot header
//
/////////////////////////////////////////////////////////////////////

#include "VLDModule.h"
#include "THaSlotData.h"
#include "TMath.h"

#include <unistd.h>
#include <iostream>
#include <iomanip>
#include <numeric>
#include <cassert>
#include <stdexcept>
#include <map>
#include <sstream>

using namespace std;

//#define DEBUG
//#define WITH_DEBUG

#ifdef DEBUG
#include <fstream>
#endif

namespace Decoder {

using vsiz_t = vector<int>::size_type;

Module::TypeIter_t VLDModule::fgThisType =
        DoRegister( ModuleType( "Decoder::VLDModule" , 493 ));

//_____________________________________________________________________________
VLDModule::VLDModule(UInt_t crate, UInt_t slot)
  : PipeliningModule(crate, slot), 
    vld_header_data{}, vld_data{},
    block_header_found(false), block_trailer_found(false), slot_header_found(false)
{
  IsInit = false;
  VLDModule::Init();
}

//_____________________________________________________________________________
VLDModule::~VLDModule() = default;

//_____________________________________________________________________________
void VLDModule::Clear( Option_t* opt )
{
  // Clear event-by-event data
  PipeliningModule::Clear(opt);
  vld_header_data.clear();
  vld_data.clear();
  
  // Initialize data_type_def to FILLER and data types to false
  data_type_def = 15;
  // Initialize data types to false
  block_header_found = block_trailer_found = slot_header_found = false;
}

//_____________________________________________________________________________
void VLDModule::Init()
{
  Module::Init();
#if defined DEBUG && defined WITH_DEBUG
  // This will make a HUGE output
  delete fDebugFile; fDebugFile = 0;
  fDebugFile = new ofstream;
  fDebugFile->open(string("VLD_debug.txt"));
#endif
  //fDebugFile=0;
  Clear();
  IsInit = kTRUE;
  fName = "VLD";

}

//_____________________________________________________________________________
inline
void VLDModule::DecodeBlockHeader( UInt_t pdat, uint32_t data_type_id )
{
  if( data_type_id ) {
    block_header_found = true;                         // Set to true if found
    vld_header_data.num_vld     = (pdat >> 16) & 0x1F; // Number of VLD that are readout in this block, mask 5 bits
    vld_header_data.read_count  = (pdat >> 8) & 0xFF;  // Number of times data has been read from VLD shared memory, mask 8 bits
    vld_header_data.write_count = (pdat >> 0) & 0xFF;  // Number of times data has been written to VLD shared memory, mask 8 bits
    // Debug output
#ifdef WITH_DEBUG
    if( fDebugFile )
      *fDebugFile << "VLDModule::Decode:: VLD BLOCK HEADER"
		  << " >> data = " << hex << pdat << dec
		  << " >> num_vld = " << vld_header_data.num_vld
		  << " >> read count = " << vld_header_data.read_count
		  << " >> write count = " << vld_header_data.write_count
		  << endl;
#endif
  }
}

//_____________________________________________________________________________
void VLDModule::DecodeBlockTrailer( UInt_t pdat )
{
  block_trailer_found = true;
  vld_header_data.nwords_inblock = (pdat >> 0) & 0xFFFF; // Total number of words in block, mask 16 bits
  // Debug output
#ifdef WITH_DEBUG
  if( fDebugFile )
    *fDebugFile << "VLDModule::Decode:: VLD BLOCK TRAILER"
                << " >> data = " << hex << pdat << dec
                << " >> nwords in block = " << vld_header_data.nwords_inblock
                << endl;
#endif
}
  
//_____________________________________________________________________________
void VLDModule::DecodeSlotHeader( UInt_t pdat , uint32_t data_type_id )
{
  uint32_t connectid, lohi, chnmask, channel;

  if( data_type_id )  { //  word 1
    slot_header_found = true;
    vld_header_data.slotid   = (pdat >> 16) & 0x1F; // Slot number (set by VME64x backplane), mask 5 bits
    vld_header_data.nconnect = (pdat >> 8) & 0xFF;  // Number of connector data words to follow (4 x 2 = current), mask 8 bits
  }
  else { // continuation words
    connectid = (pdat >> 28) & 0x7;    // ID of VLD connector [0, 3], mask 3 bits
    lohi      = (pdat >> 24) & 0x1;    // The Low (0) or High (1) 18 channels of the connector
    chnmask   = (pdat >> 0) & 0x3FFFF; // Mask of channels with VLD pulse enabled

    for( int b=0; b<18; b++) {
      if( (chnmask & (0x1 << b)) == 1 ) {
	if( lohi == 0 )
	  channel = b;
	else
	  channel = b + 18;
      }
    }
    
    channel = channel + (connectid * 36);

    vld_data.connectid.push_back( connectid );
    vld_data.lohi.push_back( lohi );  
    vld_data.chnmask.push_back( chnmask );
    vld_data.channel.push_back( channel );
  }
#ifdef WITH_DEBUG
  if( fDebugFile )
    *fDebugFile << "VLDModule::Decode:: VLD EVENT HEADER"
                << " >> data = " << hex << pdat << dec
                << " >> slot ID = " << vld_header_data.slotid
                << " >> num connector words = " << vld_header_data.nconnect
                << " >> num connector id = " << connectid
                << " >> num lo/hi bit = " << lohi
                << " >> channel mask = " << chnmask
                << endl;
#endif
}

//_____________________________________________________________________________
void VLDModule::UnsupportedType( UInt_t pdat, uint32_t data_type_id )
{

  // Handle unsupported, invalid, or irrelevant/non-decodable data types
#ifdef WITH_DEBUG
  // Data type descriptions
  static const vector<string> what_text{ "UNDEFINED TYPE",
                                         "DATA NOT VALID",
                                         "FILLER WORD",
                                         "INCORRECT DECODING" };
  // Lookup table data_type -> message number
  static const map<uint32_t, uint32_t> what_map = {
    // undefined type
    { 3,  0 },
    { 4,  0 },
    { 5,  0 },
    { 6,  0 },
    { 7,  0 },
    { 8,  0 },
    { 9,  0 },
    { 10, 0 },
    { 11, 0 },
    { 12, 0 },
    { 13, 0 },
    // data not valid
    { 14, 1 },
    // filler word
    { 15, 2 }
  };
  auto idx = what_map.find(data_type_def);
  // Message index. The last message means this function was called when
  // it shouldn't have been called, i.e. coding error in DecodeOneWord
  size_t i = (idx == what_map.end()) ? what_text.size() - 1 : idx->second;
  const string& what = what_text[i];
  ostringstream str;
  str << "VLDModule::Decode:: " << what
      << " >> data = " << hex << pdat << dec
      << " >> data type id = " << data_type_id
      << endl;
  if( fDebugFile )
    *fDebugFile << str.str();
  if( idx == what_map.end() )
    cerr << str.str();
#endif
}

//_____________________________________________________________________________
Int_t VLDModule::Decode( const UInt_t* pdat )
{
  assert(pdat);
  uint32_t data = *pdat;

  uint32_t data_type_id = (data >> 31) & 0x1;  // Data type identification, mask 1 bit
  if( data_type_id == 1 )
    data_type_def = (data >> 27) & 0xF;        // Data type defining words, mask 4 bits

  // Debug output
#ifdef WITH_DEBUG
  if( fDebugFile )
    *fDebugFile << "VLDModule::Decode:: VLD DATA TYPES"
                << " >> data = " << hex << data << dec
                << " >> data word id = " << data_type_id
                << " >> data type = " << data_type_def
                << endl;

#endif

  
  // Acquire data objects depending on the data type defining word
  switch( data_type_def ) {
    case 0: // Block header, indicates the beginning of a block of events
      DecodeBlockHeader(data, data_type_id);
      break;
    case 1: // Block trailer, indicates the end of a block of events
      DecodeBlockTrailer(data);
      break;
    case 2: // Slot header
      DecodeSlotHeader(data, data_type_id);
      break;
    case 3:  // Undefined type
    case 4:  // Undefined type
    case 5:  // Undefined type
    case 6:  // Undefined type
    case 7:  // Undefined type
    case 8:  // Undefined type
    case 9:  // Undefined type
    case 10: // Undefined type
    case 11: // Undefined type
    case 12: // Undefined type
    case 13: // Undefined type
    case 14: // Data not valid
    case 15: // Filler Word, should be ignored
      UnsupportedType(data, data_type_id);
      break;
  default:
    throw logic_error("VLDModule: incorrect masking of data_type_def");
  }  // data_type_def switch
  
#ifdef WITH_DEBUG
  if( fDebugFile )
    *fDebugFile << "**********************************************************************"
                << "\n" << endl;
#endif
  
  return block_trailer_found;
}

//_____________________________________________________________________________
void VLDModule::LoadTHaSlotDataObj( THaSlotData* sldat )
{

  // Load THaSlotData
  
  for( vsiz_t iclus = 0; iclus < vld_data.connectid.size(); iclus++ )
    sldat->loadData("scaler", 0, vld_data.connectid[iclus], vld_data.connectid[iclus]);
  for( vsiz_t iclus = 0; iclus < vld_data.lohi.size(); iclus++ )
    sldat->loadData("scaler", 0, vld_data.lohi[iclus], vld_data.lohi[iclus]);
  for( vsiz_t iclus = 0; iclus < vld_data.chnmask.size(); iclus++ )
    sldat->loadData("scaler", 0, vld_data.chnmask[iclus], vld_data.chnmask[iclus]);
  for( vsiz_t iclus = 0; iclus < vld_data.channel.size(); iclus++ )
    sldat->loadData("scaler", 0, vld_data.channel[iclus], vld_data.channel[iclus]);

}

//_____________________________________________________________________________
UInt_t VLDModule::LoadSlot( THaSlotData* sldat, const UInt_t* evbuffer,
                                const UInt_t* pstop )
{
  // Load from evbuffer between [evbuffer,pstop]
  return LoadSlot(sldat, evbuffer, 0, pstop + 1 - evbuffer);

}

//_____________________________________________________________________________
UInt_t VLDModule::LoadSlot( THaSlotData *sldat, const UInt_t* evbuffer,
                                UInt_t pos, UInt_t len)
{

  // Load from bank data in evbuffer between [pos,pos+len)
  const auto* p = evbuffer + pos;
  const auto* q = p + len;
  while( p != q ) {
    if( Decode(p++) == 1 )
      break;  // block trailer found
  }

  LoadTHaSlotDataObj(sldat);
  
  return fWordsSeen = p - (evbuffer + pos);
}

//_____________________________________________________________________________


} // namespace Decoder

ClassImp(Decoder::VLDModule)
