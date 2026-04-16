#pragma once

#include "JsonUtils.h"
#include "BlockchainUtils.h"

uint32_t getEra();
uint64_t getTip();
bool getGenesisBlockHash(BlockchainUtils *blockchainUtils, std::string *blockHash);

bool getNonce(BlockchainUtils *blockchainUtils, char *ss58Address, uint64_t *payloadNonce);
bool getBlockHash(BlockchainUtils *blockchainUtils, int block_number, std::string *blockHash);
bool extractRuntimeVersions(BlockchainUtils *blockchainUtils, uint32_t *specVersion, uint32_t *transactionVersion);
bool getRuntimeInfo(BlockchainUtils *blockchainUtils, JSONVar *runtimeInfo);
bool getRuntimeInfo(const std::string &parentBlockHash, BlockchainUtils *blockchainUtils, JSONVar *runtimeInfo);
bool getChainHead(BlockchainUtils *blockchainUtils, std::string *chainHead);
bool getParentBlockHash(const std::string &chainHead, BlockchainUtils *blockchainUtils, std::string *parentBlockHash);

