/*
 * Retroshare file transfer module: ftTransferModule.h
 *
 * Copyright 2008 by Robert Fernie.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License Version 2 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA.
 *
 * Please report all bugs and problems to "retroshare@lunamutt.com".
 *
 */
 
#ifndef FT_TRANSFER_MODULE_HEADER
#define FT_TRANSFER_MODULE_HEADER

/*
 * FUNCTION DESCRIPTION
 *
 * Each Transfer Module is paired up with a single File Creator, and responsible for the transfer of one file.
 * The Transfer Module is responsible for sending requests to peers at the correct data rates, and storing the returned data
 * in a FileCreator.
 * There are multiple Transfer Modules in the File Transfer system. Their requests are multiplexed through the Client Module. * The Transfer Module contains all the algorithms for sensible Data Requests.
 * It must be able to cope with varying data rates and dropped peers without flooding the system with too many requests.
 *
 */

#include <map>
#include <list>
#include <ftFileCreator.h>

const int  PQIPEER_INIT                 = 0x0000;
const int  PQIPEER_NOT_ONLINE           = 0x0001;
const int  PQIPEER_DOWNLOADING          = 0x0002;
const int  PQIPEER_IDLE                 = 0x0004;
const int  PQIPEER_SUSPEND              = 0x0010;

class Request
{
  uint64_t offset;
  uint32_t chunkSize;
};

class peerInfo
{
  std:string peerId;
  uint32_t state;
  uint32_t desiredRate;
  Request lastRequest;
  uint32_t actualRate;
  
  //current file data request
  uint64_t req_loc;
  uint32_t req_size;
  
  time_t lastTS;
};

class ftTransferModule 
{
public:
  ftTransferModule(ftFileCreator *fc,ftClientModule *cm);
  ~ftTransferModule();

  //interface to download controller
  bool setFileSources(std::list<std::string> peerIds);
  bool setPeerState(std::string peerId,uint32_t state,uint32_t maxRate);  //state = ONLINE/OFFLINE
  uint32_t getDataRate(std::string peerId);
  bool stopTransfer();
  bool resumeTransfer();
  bool completeFileTransfer();

  //interface to client module
  bool recvFileData(uint64_t offset, uint32_t chunk_size, void *data);  //called by ftClientModule when data arrives
  void requestData(uint64_t offset, uint32_t chunk_size);

  //interface to file creator , just wrapper functions that call the ftFileCreator
  bool getChunk(uint64_t &offset, uint32_t &chunk_size);
  bool storeData(uint64_t offset, uint32_t chunk_size, void *data);

  //internal used functions
  void queryInactive();
  
  int tick();
  
  /* add by DrBob for interfaces */
  std::string hash() { return mHash; }
  uint64_t    size() { return mSize; }
  
public:
  ftFileCreator  *mFileCreator;
  ftClientModule *mClientModule;

private:
  std::string mHash;
  uint64_t    mSize;
  std::list<std::string>         mFileSources;
  std::map<std::string,peerInfo> mOnlinePeers;
  	
  uint64_t mOffset;
  uint32_t mChunkSize;	
  bool     mFlag;  //1:transfer complete, 0: not complete
};

#endif  //FT_TRANSFER_MODULE_HEADER
