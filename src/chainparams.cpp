// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin Core developers
// Copyright (c) 2014-2017 The Helpforcancer Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "chainparams.h"
#include "consensus/merkle.h"

#include "tinyformat.h"
#include "util.h"
#include "utilstrencodings.h"

#include "arith_uint256.h"

#include <assert.h>

#include <boost/assign/list_of.hpp>

#include "chainparamsseeds.h"

static CBlock CreateGenesisBlock(const char* pszTimestamp, const CScript& genesisOutputScript, uint32_t nTime, uint32_t nNonce, uint32_t nBits, int32_t nVersion, const CAmount& genesisReward)
{
        CMutableTransaction txNew;
        txNew.nVersion = 1;
        txNew.vin.resize(1);
        txNew.vout.resize(1);
        txNew.vin[0].scriptSig = CScript() << 486604799 << CScriptNum(4) << std::vector<unsigned char>((const unsigned char*)pszTimestamp, (const unsigned char*)pszTimestamp + strlen(pszTimestamp));
        txNew.vout[0].nValue = genesisReward;
        txNew.vout[0].scriptPubKey = genesisOutputScript;

        CBlock genesis;
        genesis.nTime    = nTime;
        genesis.nBits    = nBits;
        genesis.nNonce   = nNonce;
        genesis.nVersion = nVersion;
        genesis.vtx.push_back(MakeTransactionRef(std::move(txNew)));
        genesis.hashPrevBlock.SetNull();
        genesis.hashMerkleRoot = BlockMerkleRoot(genesis);
        return genesis;
}

static CBlock CreateDevNetGenesisBlock(const uint256 &prevBlockHash, const std::string& devNetName, uint32_t nTime, uint32_t nNonce, uint32_t nBits, const CAmount& genesisReward)
{
        assert(!devNetName.empty());

        CMutableTransaction txNew;
        txNew.nVersion = 1;
        txNew.vin.resize(1);
        txNew.vout.resize(1);
        // put height (BIP34) and devnet name into coinbase
        txNew.vin[0].scriptSig = CScript() << 1 << std::vector<unsigned char>(devNetName.begin(), devNetName.end());
        txNew.vout[0].nValue = genesisReward;
        txNew.vout[0].scriptPubKey = CScript() << OP_RETURN;

        CBlock genesis;
        genesis.nTime    = nTime;
        genesis.nBits    = nBits;
        genesis.nNonce   = nNonce;
        genesis.nVersion = 4;
        genesis.vtx.push_back(MakeTransactionRef(std::move(txNew)));
        genesis.hashPrevBlock = prevBlockHash;
        genesis.hashMerkleRoot = BlockMerkleRoot(genesis);
        return genesis;
}

/**
 * Build the genesis block. Note that the output of its generation
 * transaction cannot be spent since it did not originally exist in the
 * database.
 *
 * CBlock(hash=00000ffd590b14, ver=1, hashPrevBlock=00000000000000, hashMerkleRoot=e0028e, nTime=1390095618, nBits=1e0ffff0, nNonce=28917698, vtx=1)
 *   CTransaction(hash=e0028e, ver=1, vin.size=1, vout.size=1, nLockTime=0)
 *     CTxIn(COutPoint(000000, -1), coinbase 04ffff001d01044c5957697265642030392f4a616e2f3230313420546865204772616e64204578706572696d656e7420476f6573204c6976653a204f76657273746f636b2e636f6d204973204e6f7720416363657074696e6720426974636f696e73)
 *     CTxOut(nValue=50.00000000, scriptPubKey=0xA9037BAC7050C479B121CF)
 *   vMerkleTree: e0028e
 */
static CBlock CreateGenesisBlock(uint32_t nTime, uint32_t nNonce, uint32_t nBits, int32_t nVersion, const CAmount& genesisReward)
{
        const char* pszTimestamp = "Helpforcancer 9th january 2019";
        const CScript genesisOutputScript = CScript() << ParseHex("0411f8793ddb0042ba81b817ed472523b404831212021b6a4de0d1774393f63b02265a24fc89dbf25619922c38b5c6992481428b0727c42975a2f7ee2f96adb418") << OP_CHECKSIG;
        return CreateGenesisBlock(pszTimestamp, genesisOutputScript, nTime, nNonce, nBits, nVersion, genesisReward);
}

static void MineGenesis(CBlockHeader& genesisBlock, const uint256& powLimit, bool noProduction)
{
    if(noProduction)
        genesisBlock.nTime = std::time(0);
    genesisBlock.nNonce = 0;

    printf("NOTE: Genesis nTime = %u \n", genesisBlock.nTime);
    printf("WARN: Genesis nNonce (BLANK!) = %u \n", genesisBlock.nNonce);

    arith_uint256 besthash;
    memset(&besthash,0xFF,32);
    arith_uint256 hashTarget = UintToArith256(powLimit);
    printf("Target: %s\n", hashTarget.GetHex().c_str());
    arith_uint256 newhash = UintToArith256(genesisBlock.GetHash());
    while (newhash > hashTarget) {
        genesisBlock.nNonce++;
        if (genesisBlock.nNonce == 0) {
            printf("NONCE WRAPPED, incrementing time\n");
            ++genesisBlock.nTime;
        }
        // If nothing found after trying for a while, print status
        if ((genesisBlock.nNonce & 0xfff) == 0)
            printf("nonce %08X: hash = %s (target = %s)\n",
                   genesisBlock.nNonce, newhash.ToString().c_str(),
                   hashTarget.ToString().c_str());

        if(newhash < besthash) {
            besthash = newhash;
            printf("New best: %s\n", newhash.GetHex().c_str());
        }
        newhash = UintToArith256(genesisBlock.GetHash());
    }
    printf("Genesis nTime = %u \n", genesisBlock.nTime);
    printf("Genesis nNonce = %u \n", genesisBlock.nNonce);
    printf("Genesis nBits: %08x\n", genesisBlock.nBits);
    printf("Genesis Hash = %s\n", newhash.ToString().c_str());
    printf("Genesis Hash Merkle Root = %s\n", genesisBlock.hashMerkleRoot.ToString().c_str());
}


static CBlock FindDevNetGenesisBlock(const Consensus::Params& params, const CBlock &prevBlock, const CAmount& reward)
{
        std::string devNetName = GetDevNetName();
        assert(!devNetName.empty());

        CBlock block = CreateDevNetGenesisBlock(prevBlock.GetHash(), devNetName.c_str(), prevBlock.nTime + 1, 0, prevBlock.nBits, reward);

        arith_uint256 bnTarget;
        bnTarget.SetCompact(block.nBits);

        for (uint32_t nNonce = 0; nNonce < UINT32_MAX; nNonce++) {
                block.nNonce = nNonce;

                uint256 hash = block.GetHash();
                if (UintToArith256(hash) <= bnTarget)
                        return block;
        }

        // This is very unlikely to happen as we start the devnet with a very low difficulty. In many cases even the first
        // iteration of the above loop will give a result already
        error("FindDevNetGenesisBlock: could not find devnet genesis block for %s", devNetName);
        assert(false);
}

/**
 * Main network
 */
/**
 * What makes a good checkpoint block?
 * + Is surrounded by blocks with reasonable timestamps
 *   (no blocks before with a timestamp after, none after with
 *    timestamp before)
 * + Contains no strange transactions
 */


class CMainParams : public CChainParams {
public:
    CMainParams() {
            strNetworkID = "main";

            consensus.nSubsidyHalvingInterval = 262800; // Note: actual number of blocks per calendar year with DGW v3 is ~200700 (for example 449750 - 249050)
            consensus.nMasternodePaymentsStartBlock = 15; // not true, but it's ok as long as it's less then nMasternodePaymentsIncreaseBlock
            // consensus.nMasternodePaymentsIncreaseBlock = 1569325056; // actual historical value
            // consensus.nMasternodePaymentsIncreasePeriod = 1569325056; // 17280 - actual historical value
            consensus.nInstantSendConfirmationsRequired = 6;
            consensus.nInstantSendKeepLock = 24;
            consensus.nBudgetPaymentsStartBlock = 1; // actual historical value
            consensus.nBudgetPaymentsCycleBlocks = 10; // ~(60*24*30)/2.6, actual number of blocks per month is 200700 / 12 = 16725
            consensus.nBudgetPaymentsWindowBlocks = 100;
            consensus.nSuperblockStartBlock = 30; // The block at which 12.1 goes live (end of final 12.0 budget cycle)
            consensus.nSuperblockCycle = 30; // ~(60*24*30)/2.6, actual number of blocks per month is 200700 / 12 = 16725
            consensus.nSuperblockStartHash = uint256S("");
            consensus.nGovernanceMinQuorum = 10;
            consensus.nGovernanceFilterElements = 20000;
            consensus.nMasternodeMinimumConfirmations = 15;
            consensus.BIP34Height = 20;
            consensus.BIP34Hash = uint256S("0x");
            consensus.BIP65Height = 300;  // 00000000000076d8fcea02ec0963de4abfd01e771fec0863f960c2c64fe6f357
            consensus.BIP66Height = 200; // 00000000000b1fa2dfa312863570e13fae9ca7b5566cb27e55422620b469aefa
            consensus.DIP0001Height = 12096;
            consensus.powLimit = uint256S("00000fffff000000000000000000000000000000000000000000000000000000");
            consensus.nPowTargetTimespan = 24 * 60 * 60; // helpforcancer: 1 day
            consensus.nPowTargetSpacing = 120 ; // helpforcancer: 2 minutes
            consensus.fPowAllowMinDifficultyBlocks = false;
            consensus.fPowNoRetargeting = false;
            consensus.nPowKGWHeight = 551;
            consensus.nPowDGWHeight = 551;

            // Stake information

            consensus.nPosTargetSpacing = 2 * 60; // PoSW: 2 minutes
            consensus.nPosTargetTimespan = 60 * 40; // 40 minutes at max for difficulty adjustment 40 mins
            consensus.nStakeMinAge = 60 * 2;

            consensus.nStakeMaxAge = 60 * 60 * 24; // one day
            consensus.nWSTargetDiff = 0x1e0ffff0; // Genesis Difficulty
            consensus.nPoSDiffAdjustRange = 5;

            // POS hard fork date
            consensus.nLastPoWBlock = 101;

            consensus.nRuleChangeActivationThreshold = 1916; // 95% of 2016
            consensus.nMinerConfirmationWindow = 2016; // nPowTargetTimespan / nPowTargetSpacing
            consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = 28;
            consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = 1199145601; // January 1, 2008
            consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = 1230767999; // December 31, 2008

            // Deployment of BIP68, BIP112, and BIP113.
            consensus.vDeployments[Consensus::DEPLOYMENT_CSV].bit = 0;
            consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nStartTime = 1547018228; // Jan. 9th 2019
            consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nTimeout = 1578554223; // Jan. 9th 2020

            // Deployment of DIP0001
            consensus.vDeployments[Consensus::DEPLOYMENT_DIP0001].bit = 1;
            consensus.vDeployments[Consensus::DEPLOYMENT_DIP0001].nStartTime = 1547018228; // Jan. 9th 2019
            consensus.vDeployments[Consensus::DEPLOYMENT_DIP0001].nTimeout = 1578554223; // Jan. 9th 2020
            consensus.vDeployments[Consensus::DEPLOYMENT_DIP0001].nWindowSize = 4032;
            consensus.vDeployments[Consensus::DEPLOYMENT_DIP0001].nThreshold = 3226; // 80% of 4032

            // Deployment of BIP147
            consensus.vDeployments[Consensus::DEPLOYMENT_BIP147].bit = 2;
            consensus.vDeployments[Consensus::DEPLOYMENT_BIP147].nStartTime = 1547018228; // Jan. 9th 2019
            consensus.vDeployments[Consensus::DEPLOYMENT_BIP147].nTimeout = 1578554223; // Jan. 9th 2020
            consensus.vDeployments[Consensus::DEPLOYMENT_BIP147].nWindowSize = 4032;
            consensus.vDeployments[Consensus::DEPLOYMENT_BIP147].nThreshold = 3226; // 80% of 4032

            // The best chain should have at least this much work.
            consensus.nMinimumChainWork = uint256S("0x0000000000000000000000000000000000000000000000000000000006600660"); //
            // By default assume that the signatures in ancestors of this block are valid.
            consensus.defaultAssumeValid = uint256S("0x00000639e8420921a0e5f8a662ca8abd947662b881110eb61246cf500f75977e"); // 
            /**
             * The message start string is designed to be unlikely to occur in normal data.
             * The characters are rarely used upper ASCII, not valid as UTF-8, and produce
             * a large 32-bit integer with any alignment.
             */
            pchMessageStart[0] = 0x87;
            pchMessageStart[1] = 0xbd;
            pchMessageStart[2] = 0xe8;
            pchMessageStart[3] = 0xfc;
            vAlertPubKey = ParseHex("04e243339bf9bc67a343db0ba0f73851546759320d901c2809cb2ce05eaee36c4501434f4740ae0ad9fb3e3315444c2a203eda65198fc1d45c8e04e9aa47d81af2");
            nDefaultPort = 22323;
            nPruneAfterHeight = 100000;


            genesis = CreateGenesisBlock(1547063449, 2144487, 0x1e0ffff0, 1, 50 * COIN);


            consensus.hashGenesisBlock = genesis.GetHash();
            assert(consensus.hashGenesisBlock == uint256S("0x00000fda77d52e10f93472f871f8c70ba369d3d750626dbb139b6bd7924ea529"));
            assert(genesis.hashMerkleRoot == uint256S("0xa5e78290eb4de6788d84fc8dc0afad17142cb0d627f36bf2759ff863d00b2f5f"));

            vSeeds.push_back(CDNSSeedData("dnsseed.helpforcancer.com", "dnsseed.helpforcancer.com"));
            vSeeds.push_back(CDNSSeedData("dnsseed2.helpforcancer.com", "dnsseed2.helpforcancer.com"));
            vSeeds.push_back(CDNSSeedData("dnsseed3.helpforcancer.com", "dnsseed3.helpforcancer.com"));
            vSeeds.push_back(CDNSSeedData("helpforcancer.seeds.mn.zone", "helpforcancer.seeds.mn.zone"));
            vSeeds.push_back(CDNSSeedData("helpforcancer.mnseeds.com", "helpforcancer.mnseeds.com"));
            vSeeds.push_back(CDNSSeedData("93.186.254.229", "93.186.254.229"));
            vSeeds.push_back(CDNSSeedData("80.211.1.199", "80.211.1.199"));
            vSeeds.push_back(CDNSSeedData("89.46.65.250", "89.46.65.250"));
            vSeeds.push_back(CDNSSeedData("80.211.7.49", "80.211.7.49"));


        // HFCC addresses start with 'X'
           base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,76);
        // HFCC script addresses start with '7'
           base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,16);
        // HFCC private keys start with '7' or 'X'
           base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,204);
        // HFCC BIP32 pubkeys start with 'xpub' (Bitcoin defaults)
           base58Prefixes[EXT_PUBLIC_KEY] = boost::assign::list_of(0x04)(0x88)(0xB2)(0x1E).convert_to_container<std::vector<unsigned char> >();
        // HFCC BIP32 prvkeys start with 'xprv' (Bitcoin defaults)
           base58Prefixes[EXT_SECRET_KEY] = boost::assign::list_of(0x04)(0x88)(0xAD)(0xE4).convert_to_container<std::vector<unsigned char> >();
        // HFCC BIP44 coin type is '5'
           nExtCoinType = 1997;

            vFixedSeeds = std::vector<SeedSpec6>(pnSeed6_main, pnSeed6_main + ARRAYLEN(pnSeed6_main));

            fMiningRequiresPeers = true;
            fDefaultConsistencyChecks = false;
            fRequireStandard = true;
            fMineBlocksOnDemand = false;
            fAllowMultipleAddressesFromGroup = false;
            fAllowMultiplePorts = false;

            nPoolMaxTransactions = 3;
            nFulfilledRequestExpireTime = 60*60; // fulfilled requests expire in 1 hour

            strSporkAddress = "XhxEg5cZXNsYs7jZbASBzNWEXUBKL8ZrsH";

            checkpointData = (CCheckpointData) {
                    boost::assign::map_list_of
                                ( 0, uint256S("0x00000fda77d52e10f93472f871f8c70ba369d3d750626dbb139b6bd7924ea529"))
				( 10, uint256S("0x00000e6f71d898a6f55958a7a2e95af956b169ff6ef40070a1cfd7bf5e6452db"))
				( 50, uint256S("0x0000067d68c2ebe64376c4b452aa75d4009c438d266e33dcd58eec503da9675b"))
				( 90, uint256S("0x000004d79e0978b22612b38a282ab5cbc6c04a367b4125670547bebb6bb64ad6"))
				( 101, uint256S("0x00000639e8420921a0e5f8a662ca8abd947662b881110eb61246cf500f75977e"))


            };
            chainTxData = ChainTxData{
                    1547148431, // * UNIX timestamp of last checkpoint block
                    102,    // * total number of transactions between genesis and last checkpoint
                    //   (the tx=... number in the SetBestChain debug.log lines)
                    0.98        // * estimated number of transactions per day after checkpoint

            };
    }
};
static CMainParams mainParams;

/**
 * Testnet (v3)
 */
class CTestNetParams : public CChainParams {
public:
    CTestNetParams() {
            strNetworkID = "test";
            consensus.nSubsidyHalvingInterval = 1569325056;
            consensus.nMasternodePaymentsStartBlock = 15; // not true, but it's ok as long as it's less then nMasternodePaymentsIncreaseBlock
            consensus.nMasternodePaymentsIncreaseBlock = 1569325056;
            consensus.nMasternodePaymentsIncreasePeriod = 1569325056;
            consensus.nInstantSendConfirmationsRequired = 2;
            consensus.nInstantSendKeepLock = 6;
            consensus.nBudgetPaymentsStartBlock = 46;
            consensus.nBudgetPaymentsCycleBlocks = 24;
            consensus.nBudgetPaymentsWindowBlocks = 10;
            consensus.nSuperblockStartBlock = 3050; // NOTE: Should satisfy nSuperblockStartBlock > nBudgetPaymentsStartBlock
            // consensus.nSuperblockStartHash = uint256S("000001af046f4ed575a48b919ed28be8a40c6a78df8d7830fbbfd07ec17a1fee");
            consensus.nSuperblockCycle = 24; // Superblocks can be issued hourly on testnet
            consensus.nGovernanceMinQuorum = 1;
            consensus.nGovernanceFilterElements = 500;
            consensus.nMasternodeMinimumConfirmations = 1;
            consensus.BIP34Height = 76;
            consensus.BIP34Hash = uint256S("0x000008ebb1db2598e897d17275285767717c6acfeac4c73def49fbea1ddcbcb6");
            consensus.BIP65Height = 2431; // 0000039cf01242c7f921dcb4806a5994bc003b48c1973ae0c89b67809c2bb2ab
            consensus.BIP66Height = 2075; // 0000002acdd29a14583540cb72e1c5cc83783560e38fa7081495d474fe1671f7
            consensus.DIP0001Height = 5500;
            consensus.powLimit = uint256S("00000fffff000000000000000000000000000000000000000000000000000000");
            consensus.nPowTargetTimespan = 60 * 60 * 24; // helpforcancer: 1 day
            consensus.nPowTargetSpacing = 2 * 60; // helpforcancer: 2 minutes
            consensus.fPowAllowMinDifficultyBlocks = true;
            consensus.fPowNoRetargeting = false;
            consensus.nPowKGWHeight = 4001; // nPowKGWHeight >= nPowDGWHeight means "no KGW"
            consensus.nPowDGWHeight = 4001;

            // Stake info
            consensus.nPosTargetSpacing = 2 * 60; // PoSW: 2 minutes
            consensus.nPosTargetTimespan = 60 * 40;
            consensus.nStakeMinAge = 60; //one minute
            consensus.nStakeMaxAge = 60 * 60 * 24; // one day
            consensus.nLastPoWBlock = 650;
            consensus.nPoSDiffAdjustRange = 1;
            // highest difficulty | 0x1e0ffff0 (?)
            // smallest difficulty | 0x008000
            consensus.nWSTargetDiff = 0x1e0ffff0; // Genesis Difficulty

            consensus.nRuleChangeActivationThreshold = 1512; // 75% for testchains
            consensus.nMinerConfirmationWindow = 2016; // nPowTargetTimespan / nPowTargetSpacing
            consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = 28;
            consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = 1199145601; // January 1, 2008
            consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = 1230767999; // December 31, 2008

            // Deployment of BIP68, BIP112, and BIP113.
            consensus.vDeployments[Consensus::DEPLOYMENT_CSV].bit = 0;
            consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nStartTime = 1506556800; // September 28th, 2017
            consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nTimeout = 1538092800; // September 28th, 2018

            // Deployment of DIP0001
            consensus.vDeployments[Consensus::DEPLOYMENT_DIP0001].bit = 1;
            consensus.vDeployments[Consensus::DEPLOYMENT_DIP0001].nStartTime = 1505692800; // Sep 18th, 2017
            consensus.vDeployments[Consensus::DEPLOYMENT_DIP0001].nTimeout = 1537228800; // Sep 18th, 2018
            consensus.vDeployments[Consensus::DEPLOYMENT_DIP0001].nWindowSize = 100;
            consensus.vDeployments[Consensus::DEPLOYMENT_DIP0001].nThreshold = 50; // 50% of 100

            // Deployment of BIP147
            consensus.vDeployments[Consensus::DEPLOYMENT_BIP147].bit = 2;
            consensus.vDeployments[Consensus::DEPLOYMENT_BIP147].nStartTime = 1517792400; // Feb 5th, 2018
            consensus.vDeployments[Consensus::DEPLOYMENT_BIP147].nTimeout = 1549328400; // Feb 5th, 2019
            consensus.vDeployments[Consensus::DEPLOYMENT_BIP147].nWindowSize = 100;
            consensus.vDeployments[Consensus::DEPLOYMENT_BIP147].nThreshold = 50; // 50% of 100

            // The best chain should have at least this much work.
            consensus.nMinimumChainWork = uint256S("0x"); // 37900
            // By default assume that the signatures in ancestors of this block are valid.
            consensus.defaultAssumeValid = uint256S("0x"); // 37900


            pchMessageStart[0] = 0x8b;
            pchMessageStart[1] = 0xf0;
            pchMessageStart[2] = 0x8f;
            pchMessageStart[3] = 0xd8;
            vAlertPubKey = ParseHex("0411f8793ddb0042ba81b817ed472523b404831212021b6a4de0d1774393f63b02265a24fc89dbf25619922c38b5c6992481428b0727c42975a2f7ee2f96adb418");
            nDefaultPort = 22324;
            nPruneAfterHeight = 1000;
          
        

            genesis = CreateGenesisBlock(1547063449, 2144487, 0x1e0ffff0, 1, 50 * COIN);
            consensus.hashGenesisBlock = genesis.GetHash();
            assert(consensus.hashGenesisBlock == uint256S("0x00000fda77d52e10f93472f871f8c70ba369d3d750626dbb139b6bd7924ea529"));
            assert(genesis.hashMerkleRoot == uint256S("0xa5e78290eb4de6788d84fc8dc0afad17142cb0d627f36bf2759ff863d00b2f5f"));



            // Testnet helpforcancer addresses start with 'y'
            base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,140);
            // Testnet helpforcancer script addresses start with '8' or '9'
            base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,19);
            // Testnet private keys start with '9' or 'c' (Bitcoin defaults)
            base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,239);
            // Testnet helpforcancer BIP32 pubkeys start with 'tpub' (Bitcoin defaults)
            base58Prefixes[EXT_PUBLIC_KEY] = boost::assign::list_of(0x04)(0x35)(0x87)(0xCF).convert_to_container<std::vector<unsigned char> >();
            // Testnet helpforcancer BIP32 prvkeys start with 'tprv' (Bitcoin defaults)
            base58Prefixes[EXT_SECRET_KEY] = boost::assign::list_of(0x04)(0x35)(0x83)(0x94).convert_to_container<std::vector<unsigned char> >();

            // Testnet helpforcancer BIP44 coin type is '1' (All coin's testnet default)
            nExtCoinType = 1;

            fMiningRequiresPeers = true;
            fDefaultConsistencyChecks = false;
            fRequireStandard = false;
            fMineBlocksOnDemand = false;
            fAllowMultipleAddressesFromGroup = false;
            fAllowMultiplePorts = false;

            nPoolMaxTransactions = 3;
            nFulfilledRequestExpireTime = 5*60; // fulfilled requests expire in 5 minutes


            strSporkAddress = "yMCScEFCuhFGQL8aBS8UPXnKriFtjMVWra";

            checkpointData = (CCheckpointData) {
                    boost::assign::map_list_of
                            (   0, uint256S("0x00000fda77d52e10f93472f871f8c70ba369d3d750626dbb139b6bd7924ea529"))


            };
            chainTxData = ChainTxData{
                    1547063449, // * UNIX timestamp of last checkpoint block
                    0,       // * total number of transactions between genesis and last checkpoint
                    //   (the tx=... number in the SetBestChain debug.log lines)
                    0         // * estimated number of transactions per day after checkpoint

            };

    }
};
static CTestNetParams testNetParams;

/**
 * Devnet
 */
class CDevNetParams : public CChainParams {
public:
    CDevNetParams() {
            strNetworkID = "dev";
            consensus.nSubsidyHalvingInterval = 210240;
            consensus.nMasternodePaymentsStartBlock = 4010; // not true, but it's ok as long as it's less then nMasternodePaymentsIncreaseBlock
            consensus.nMasternodePaymentsIncreaseBlock = 4030;
            consensus.nMasternodePaymentsIncreasePeriod = 10;
            consensus.nInstantSendConfirmationsRequired = 2;
            consensus.nInstantSendKeepLock = 6;
            consensus.nBudgetPaymentsStartBlock = 4100;
            consensus.nBudgetPaymentsCycleBlocks = 50;
            consensus.nBudgetPaymentsWindowBlocks = 10;
            consensus.nSuperblockStartBlock = 4200; // NOTE: Should satisfy nSuperblockStartBlock > nBudgetPeymentsStartBlock
            consensus.nSuperblockStartHash = uint256(); // do not check this on devnet
            consensus.nSuperblockCycle = 24; // Superblocks can be issued hourly on devnet
            consensus.nGovernanceMinQuorum = 1;
            consensus.nGovernanceFilterElements = 500;
            consensus.nMasternodeMinimumConfirmations = 1;
            consensus.BIP34Height = 1; // BIP34 activated immediately on devnet
            consensus.BIP65Height = 1; // BIP65 activated immediately on devnet
            consensus.BIP66Height = 1; // BIP66 activated immediately on devnet
            consensus.DIP0001Height = 2; // DIP0001 activated immediately on devnet
            consensus.powLimit = uint256S("7fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff"); // ~uint256(0) >> 1
            consensus.nPowTargetTimespan = 24 * 60 * 60; // Helpforcancer: 1 day
            consensus.nPowTargetSpacing = 2.5 * 60; // Helpforcancer: 2.5 minutes
            consensus.fPowAllowMinDifficultyBlocks = true;
            consensus.fPowNoRetargeting = false;
            consensus.nPowKGWHeight = 4001; // nPowKGWHeight >= nPowDGWHeight means "no KGW"
            consensus.nPowDGWHeight = 4001;


            // Stake information

            consensus.nPosTargetSpacing = 2 * 60; // PoSW: 1 minutes
            consensus.nPosTargetTimespan = 60 * 40;
            consensus.nStakeMinAge = 60 * 60;
            consensus.nStakeMaxAge = 60 * 60 * 24; // one day
            consensus.nLastPoWBlock = 180675;



            consensus.nRuleChangeActivationThreshold = 1512; // 75% for testchains
            consensus.nMinerConfirmationWindow = 2016; // nPowTargetTimespan / nPowTargetSpacing
            consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = 28;
            consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = 1199145601; // January 1, 2008
            consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = 1230767999; // December 31, 2008

            // Deployment of BIP68, BIP112, and BIP113.
            consensus.vDeployments[Consensus::DEPLOYMENT_CSV].bit = 0;
            consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nStartTime = 1506556800; // September 28th, 2017
            consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nTimeout = 1538092800; // September 28th, 2018

            // Deployment of DIP0001
            consensus.vDeployments[Consensus::DEPLOYMENT_DIP0001].bit = 1;
            consensus.vDeployments[Consensus::DEPLOYMENT_DIP0001].nStartTime = 1505692800; // Sep 18th, 2017
            consensus.vDeployments[Consensus::DEPLOYMENT_DIP0001].nTimeout = 1537228800; // Sep 18th, 2018
            consensus.vDeployments[Consensus::DEPLOYMENT_DIP0001].nWindowSize = 100;
            consensus.vDeployments[Consensus::DEPLOYMENT_DIP0001].nThreshold = 50; // 50% of 100

            // Deployment of BIP147
            consensus.vDeployments[Consensus::DEPLOYMENT_BIP147].bit = 2;
            consensus.vDeployments[Consensus::DEPLOYMENT_BIP147].nStartTime = 1517792400; // Feb 5th, 2018
            consensus.vDeployments[Consensus::DEPLOYMENT_BIP147].nTimeout = 1549328400; // Feb 5th, 2019
            consensus.vDeployments[Consensus::DEPLOYMENT_BIP147].nWindowSize = 100;
            consensus.vDeployments[Consensus::DEPLOYMENT_BIP147].nThreshold = 50; // 50% of 100

            // The best chain should have at least this much work.
            consensus.nMinimumChainWork = uint256S("0x000000000000000000000000000000000000000000000000000000000000000");

            // By default assume that the signatures in ancestors of this block are valid.
            consensus.defaultAssumeValid = uint256S("0x000000000000000000000000000000000000000000000000000000000000000");

            pchMessageStart[0] = 0xe2;
            pchMessageStart[1] = 0xca;
            pchMessageStart[2] = 0xff;
            pchMessageStart[3] = 0xce;
            vAlertPubKey = ParseHex("04517d8a699cb43d3938d7b24faaff7cda448ca4ea267723ba614784de661949bf632d6304316b244646dea079735b9a6fc4af804efb4752075b9fe2245e14e412");
            nDefaultPort = 19999;
            nPruneAfterHeight = 1000;

         
            
            genesis = CreateGenesisBlock(1417713337, 1096447, 0x207fffff, 1, 50 * COIN);
            consensus.hashGenesisBlock = genesis.GetHash();
            assert(consensus.hashGenesisBlock == uint256S("0x"));
            assert(genesis.hashMerkleRoot == uint256S("0x"));

            devnetGenesis = FindDevNetGenesisBlock(consensus, genesis, 50 * COIN);
            consensus.hashDevnetGenesisBlock = devnetGenesis.GetHash();

            vFixedSeeds.clear();
            vSeeds.clear();
            //vSeeds.push_back(CDNSSeedData("helpforcancerevo.org",  "devnet-seed.helpforcancerevo.org"));

            // Testnet Helpforcancer addresses start with 'y'
            base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,140);
            // Testnet Helpforcancer script addresses start with '8' or '9'
            base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,19);
            // Testnet private keys start with '9' or 'c' (Bitcoin defaults)
            base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,239);
            // Testnet Helpforcancer BIP32 pubkeys start with 'tpub' (Bitcoin defaults)
            base58Prefixes[EXT_PUBLIC_KEY] = boost::assign::list_of(0x04)(0x35)(0x87)(0xCF).convert_to_container<std::vector<unsigned char> >();
            // Testnet Helpforcancer BIP32 prvkeys start with 'tprv' (Bitcoin defaults)
            base58Prefixes[EXT_SECRET_KEY] = boost::assign::list_of(0x04)(0x35)(0x83)(0x94).convert_to_container<std::vector<unsigned char> >();

            // Testnet Helpforcancer BIP44 coin type is '1' (All coin's testnet default)
            nExtCoinType = 1;

            fMiningRequiresPeers = true;
            fDefaultConsistencyChecks = false;
            fRequireStandard = false;
            fMineBlocksOnDemand = false;
            fAllowMultipleAddressesFromGroup = true;
            fAllowMultiplePorts = true;

            nPoolMaxTransactions = 3;
            nFulfilledRequestExpireTime = 5*60; // fulfilled requests expire in 5 minutes

            strSporkAddress = "yjPtiKh2uwk3bDutTEA2q9mCtXyiZRWn55";

            checkpointData = (CCheckpointData) {
                    boost::assign::map_list_of
                            (      0, uint256S("0x"))
                            (      1, devnetGenesis.GetHash())
            };

            chainTxData = ChainTxData{
                    devnetGenesis.GetBlockTime(), // * UNIX timestamp of devnet genesis block
                    2,                            // * we only have 2 coinbase transactions when a devnet is started up
                    0.01                          // * estimated number of transactions per second
            };
    }
};
static CDevNetParams *devNetParams;


/**
 * Regression test
 */
class CRegTestParams : public CChainParams {
public:
    CRegTestParams() {
            strNetworkID = "regtest";
            consensus.nSubsidyHalvingInterval = 150;
            consensus.nMasternodePaymentsStartBlock = 240;
            consensus.nMasternodePaymentsIncreaseBlock = 350;
            consensus.nMasternodePaymentsIncreasePeriod = 10;
            consensus.nInstantSendConfirmationsRequired = 2;
            consensus.nInstantSendKeepLock = 6;
            consensus.nBudgetPaymentsStartBlock = 25;
            consensus.nBudgetPaymentsCycleBlocks = 50;
            consensus.nBudgetPaymentsWindowBlocks = 10;
            consensus.nSuperblockStartBlock = 1500;
            consensus.nSuperblockStartHash = uint256(); // do not check this on regtest
            consensus.nSuperblockCycle = 10;
            consensus.nGovernanceMinQuorum = 1;
            consensus.nGovernanceFilterElements = 100;
            consensus.nMasternodeMinimumConfirmations = 1;
            consensus.BIP34Height = 100000000; // BIP34 has not activated on regtest (far in the future so block v1 are not rejected in tests)
            consensus.BIP34Hash = uint256();
            consensus.BIP65Height = 1351; // BIP65 activated on regtest (Used in rpc activation tests)
            consensus.BIP66Height = 1251; // BIP66 activated on regtest (Used in rpc activation tests)
            consensus.DIP0001Height = 2000;
            consensus.powLimit = uint256S("7fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
            consensus.nPowTargetTimespan = 24 * 60 * 60; // helpforcancer: 1 day
            consensus.nPowTargetSpacing = 120; // helpforcancer: 2.5 minutes
            consensus.fPowAllowMinDifficultyBlocks = true;
            consensus.fPowNoRetargeting = true;
            consensus.nPowKGWHeight = 15200; // same as mainnet
            consensus.nPowDGWHeight = 34140; // same as mainnet
            consensus.nRuleChangeActivationThreshold = 108; // 75% for testchains
            consensus.nMinerConfirmationWindow = 144; // Faster than normal for regtest (144 instead of 2016)
            consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = 28;
            consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = 0;
            consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = 999999999999ULL;
            consensus.vDeployments[Consensus::DEPLOYMENT_CSV].bit = 0;
            consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nStartTime = 0;
            consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nTimeout = 999999999999ULL;
            consensus.vDeployments[Consensus::DEPLOYMENT_DIP0001].bit = 1;
            consensus.vDeployments[Consensus::DEPLOYMENT_DIP0001].nStartTime = 0;
            consensus.vDeployments[Consensus::DEPLOYMENT_DIP0001].nTimeout = 999999999999ULL;
            consensus.vDeployments[Consensus::DEPLOYMENT_BIP147].bit = 2;
            consensus.vDeployments[Consensus::DEPLOYMENT_BIP147].nStartTime = 0;
            consensus.vDeployments[Consensus::DEPLOYMENT_BIP147].nTimeout = 999999999999ULL;

            // Stake info
            consensus.nPosTargetSpacing = 30; // PoSW: 1 minutes
            consensus.nPosTargetTimespan = 60 * 40;
            consensus.nStakeMinAge = 60;
            consensus.nStakeMaxAge = 60 * 60 * 24; // one day
            consensus.nLastPoWBlock = 25;
            // highest difficulty | 0x1e0ffff0 (?)
            // smallest difficulty | 0x008000
            consensus.nWSTargetDiff = 0x1e0ffff0; // Genesis Difficulty

            // The best chain should have at least this much work.
            consensus.nMinimumChainWork = uint256S("0x00");

            // By default assume that the signatures in ancestors of this block are valid.
            consensus.defaultAssumeValid = uint256S("0x00");

            pchMessageStart[0] = 0xd4;
            pchMessageStart[1] = 0xf1;
            pchMessageStart[2] = 0xb9;
            pchMessageStart[3] = 0x99;
            nDefaultPort = 19994;
            nPruneAfterHeight = 1000;
                  //   startNewChain = true;
            
            genesis = CreateGenesisBlock(1547064844, 0, 0x1e0ffff0, 1, 50 * COIN);
       // if (startNewChain)
       //     MineGenesis(genesis, consensus.powLimit, true);
            consensus.hashGenesisBlock = genesis.GetHash();
            assert(consensus.hashGenesisBlock == uint256S("0x5be620e2dced13314a1651463b2980ee8b38164d3835c6ddccb96811d387f2b8"));
            assert(genesis.hashMerkleRoot == uint256S("0xa5e78290eb4de6788d84fc8dc0afad17142cb0d627f36bf2759ff863d00b2f5f"));


            vFixedSeeds.clear(); //!< Regtest mode doesn't have any fixed seeds.
            vSeeds.clear();      //!< Regtest mode doesn't have any DNS seeds.

            fMiningRequiresPeers = false;
            fDefaultConsistencyChecks = true;
            fRequireStandard = false;
            fMineBlocksOnDemand = true;
            fAllowMultipleAddressesFromGroup = true;
            fAllowMultiplePorts = true;

            nFulfilledRequestExpireTime = 5*60; // fulfilled requests expire in 5 minutes

            // privKey: cP4EKFyJsHT39LDqgdcB43Y3YXjNyjb5Fuas1GQSeAtjnZWmZEQK
            strSporkAddress = "yj949n1UH6fDhw6HtVE5VMj2iSTaSWBMcW";

            checkpointData = (CCheckpointData){
                    boost::assign::map_list_of
                            ( 0, uint256S("0x5be620e2dced13314a1651463b2980ee8b38164d3835c6ddccb96811d387f2b8"))
            };

            chainTxData = ChainTxData{
                    1547064844,
                    0,
                    0
            };

            base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,140);
            // Regtest helpforcancer script addresses start with '8' or '9'
            base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,19);
            // Regtest private keys start with '9' or 'c' (Bitcoin defaults)
            base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,239);
            // Regtest helpforcancer BIP32 pubkeys start with 'tpub' (Bitcoin defaults)
            base58Prefixes[EXT_PUBLIC_KEY] = boost::assign::list_of(0x04)(0x35)(0x87)(0xCF).convert_to_container<std::vector<unsigned char> >();
            // Regtest helpforcancer BIP32 prvkeys start with 'tprv' (Bitcoin defaults)
            base58Prefixes[EXT_SECRET_KEY] = boost::assign::list_of(0x04)(0x35)(0x83)(0x94).convert_to_container<std::vector<unsigned char> >();

            // Regtest helpforcancer BIP44 coin type is '1' (All coin's testnet default)
            nExtCoinType = 1;
    }

    void UpdateBIP9Parameters(Consensus::DeploymentPos d, int64_t nStartTime, int64_t nTimeout)
    {
            consensus.vDeployments[d].nStartTime = nStartTime;
            consensus.vDeployments[d].nTimeout = nTimeout;
    }
};
static CRegTestParams regTestParams;

static CChainParams *pCurrentParams = 0;

const CChainParams &Params() {
        assert(pCurrentParams);
        return *pCurrentParams;
}

CChainParams& Params(const std::string& chain)
{
        if (chain == CBaseChainParams::MAIN)
                return mainParams;
        else if (chain == CBaseChainParams::TESTNET)
                return testNetParams;
        else if (chain == CBaseChainParams::DEVNET) {
                assert(devNetParams);
                return *devNetParams;
        } else if (chain == CBaseChainParams::REGTEST)
                return regTestParams;
        else
                throw std::runtime_error(strprintf("%s: Unknown chain %s.", __func__, chain));
}

void SelectParams(const std::string& network)
{
        if (network == CBaseChainParams::DEVNET) {
                devNetParams = new CDevNetParams();
        }

        SelectBaseParams(network);
        pCurrentParams = &Params(network);
}

void UpdateRegtestBIP9Parameters(Consensus::DeploymentPos d, int64_t nStartTime, int64_t nTimeout)
{
        regTestParams.UpdateBIP9Parameters(d, nStartTime, nTimeout);
}
