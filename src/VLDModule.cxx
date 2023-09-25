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
#include "CodaDecoder.h"  // for coda_format_error
#include "CodaDecoder.h"  // for coda_format_error

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
  if( data_type_id )  { //  word 1
    slot_header_found = true;
    vld_header_data.slotid   = (pdat >> 16) & 0x1F; // Slot number (set by VME64x backplane), mask 5 bits
    vld_header_data.nconnect = (pdat >> 8) & 0xFF;  // Number of connector data words to follow (4 x 2 = current), mask 8 bits
#ifdef WITH_DEBUG
    if( fDebugFile )
      *fDebugFile << "VLDModule::Decode:: VLD SLOT HEADER"
                  << " >> data = " << hex << pdat << dec
                  << " >> slot ID = " << vld_header_data.slotid
                  << " >> num connector words = " << vld_header_data.nconnect
                  << endl;
#endif
  }
  else { // continuation words
    uint32_t connectid = (pdat >> 28) & 0x7;     // ID of VLD connector [0, 3], mask 3 bits
    uint32_t lohi      = (pdat >> 24) & 0x1;     // The Low (0) or High (1) 18 channels of the connector
    uint32_t chnmask   = (pdat >> 0)  & 0x3FFFF; // Mask of channels with VLD pulse enabled

    vld_data.connectid.push_back( connectid );
    vld_data.lohi.push_back( lohi );  
    vld_data.chnmask.push_back( chnmask );
#ifdef WITH_DEBUG
  if( fDebugFile )
      *fDebugFile << "VLDModule::Decode:: VLD SLOT DATA"
                << " >> data = " << hex << pdat << dec
                << " >> slot ID = " << vld_header_data.slotid
                << " >> num connector words = " << vld_header_data.nconnect
                << " >> num connector id = " << connectid
                << " >> num lo/hi bit = " << lohi
                << " >> channel mask = " << chnmask
                << endl;
#endif
  }
}

//_____________________________________________________________________________
void VLDModule::UnsupportedType( UInt_t pdat, uint32_t data_type_id )
{

  // Handle unsupported, invalid, or irrelevant/non-decodable data types
#ifdef WITH_DEBUG
  // Data type descriptions
  ostringstream str;
  str << "VLDModule::Decode:: UNDEFINED TYPE"
      << " >> data = " << hex << pdat << dec
      << " >> data type id = " << data_type_id
      << endl;
  if( fDebugFile )
    *fDebugFile << str.str();

  cerr << str.str() << endl;
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
    default:
#ifdef WITH_DEBUG
      UnsupportedType(data, data_type_id);
#endif
      break;
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
// Helper function for debugging
static void PrintBlock( const uint32_t* codabuffer, uint32_t pos, uint32_t len )
{
  size_t idx = pos;
  while( idx < pos+len ) {
    while( idx < pos+len && !TESTBIT(codabuffer[idx], 31) )
      ++idx;
    if( idx == pos+len )
      break;
    uint32_t data = codabuffer[idx];
    uint32_t type = (data >> 27) & 0xF;
    switch( type ) {
      case 0:
        cout << "Block header"
             << " idx = " << idx
             << " num_vld = " << ((data >> 16) & 0x1F)
             << " read = " << ((data >> 8) & 0xFF)
             << " write = " << (data & 0xFF);
        break;
      case 1:
        cout << "Block trailer"
             << " idx = " << idx
             << " nwords = " << (data & 0x3FFFFF);
        break;
      case 2:
        cout << " Slot header"
             << " idx = " << idx
             << " slot = " << ((data >> 16) & 0x1F)
             << " nwords = " << (data & 0xFF);
        break;
      default:
        cout << "  Type = " << type
             << " idx = " << idx;
        break;
    }
    cout << endl;
    ++idx;
  }
}

//_____________________________________________________________________________
UInt_t VLDModule::LoadBank( THaSlotData* sldat, const UInt_t* evbuffer,
                            UInt_t pos, UInt_t len )
{
  // Load event block. The VLD module does not provide multi-block data.

  if( fDebug > 1 )  // Set fDebug via module config string in db_cratemap.dat
    PrintBlock(evbuffer, pos, len);

  // Find block header. The VLDModule block header does not contain the slot;
  // instead, all slot data are sent in a single block, preceded by slot headers.
  auto ibeg = FindIDWord(evbuffer, pos, len, kBlockHeader);
  if( ibeg == -1 )
    // No block header - something is quite wrong ...
    throw CodaDecoder::coda_format_error(
      "VLDModule::LoadBank: Missing block header. Call expert.");

  fBlockHeader = evbuffer[ibeg];  // save for convenience

  // Find end of block and let the module decode the event
  Long64_t iend = ibeg+1;
  while( true ) {
    iend = FindIDWord(evbuffer, iend, len + pos - iend, kBlockTrailer);
    if( (iend = VerifyBlockTrailer(evbuffer, pos, len, ibeg, iend)) > 0 )
      break;
    if( iend == 0 )
      return 0;
    iend = -iend;
  }
  assert( ibeg >= pos && iend > ibeg && iend < pos+len ); // trivially

  return LoadSlot(sldat, evbuffer, ibeg, iend+1-ibeg);
}

//_____________________________________________________________________________
UInt_t VLDModule::LoadNextEvBuffer( THaSlotData* sldat )
{
  // This method should never be called for this module type
  throw logic_error("VLDModule::LoadNextEvBuffer called. This module "
                    "does not support multi-block mode. Call expert.");
}

//_____________________________________________________________________________


} // namespace Decoder

ClassImp(Decoder::VLDModule)
