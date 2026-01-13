#include <oxenc/hex.h>

#include <iostream>
#include <string>

#include "../common/string_util.h"
#include "../cryptonote_basic/cryptonote_basic.h"
#include "../cryptonote_basic/cryptonote_format_utils.h"
#include "../cryptonote_core/cryptonote_core.h"
#include "../cryptonote_core/cryptonote_tx_utils.h"
#include "../serialization/binary_utils.h"
#include "../device/device.hpp"

using namespace cryptonote;

// Generate a new governance wallet and create genesis transaction with 200M coins
int main() {
    try {
        hw::get_device("default");  // Initialize crypto via hardware device

        std::cout << "========================================\n";
        std::cout << "Equilibria Horizon Genesis Generator\n";
        std::cout << "========================================\n\n";

        // Generate new governance wallet
        account_base gov_account;
        gov_account.generate();

        std::string gov_address = get_account_address_as_str(
            network_type::TESTNET, false, gov_account.get_keys().m_account_address);

        std::string gov_spend_key = tools::hex_guts(gov_account.get_keys().m_spend_secret_key);
        std::string gov_view_key = tools::hex_guts(gov_account.get_keys().m_view_secret_key);

        std::cout << "✅ Generated New Governance Wallet:\n";
        std::cout << "   Address: " << gov_address << "\n";
        std::cout << "   Spend Key: " << gov_spend_key << "\n";
        std::cout << "   View Key: " << gov_view_key << "\n\n";

        // Create miner tx context for genesis block
        oxen_miner_tx_context miner_tx_context = oxen_miner_tx_context::miner_block(
            network_type::TESTNET,
            gov_account.get_keys().m_account_address);

        // Create the genesis transaction
        transaction genesis_tx;
        genesis_tx.version = txversion::v1;
        genesis_tx.unlock_time = 0;

        // Construct miner tx - this will use the 200M premine from cryptonote_basic_impl.cpp
        // Genesis blocks always use hf7 version, regardless of network's current hardfork
        std::pair<bool, uint64_t> result = construct_miner_tx(
            0,  // height
            0,  // median_weight
            0,  // already_generated_coins (0 triggers premine)
            0,  // current_block_weight
            0,  // fee
            genesis_tx,
            miner_tx_context,
            {},  // sn_rewards
            "",  // extra_nonce
            hf::hf7);  // Genesis blocks always use hf7

        if (!result.first) {
            throw std::runtime_error("Failed to construct miner tx");
        }

        std::cout << "✅ Generated Genesis Transaction\n";
        std::cout << "   Reward Amount: " << result.second << " atomic units\n";
        std::cout << "   Reward Amount: " << (result.second / oxen::COIN) << " XEQ\n\n";

        // Serialize the transaction
        std::string tx_blob;
        if (!t_serializable_object_to_blob(genesis_tx, tx_blob)) {
            throw std::runtime_error("Failed to serialize genesis transaction");
        }

        std::string genesis_tx_hex = oxenc::to_hex(tx_blob);

        // Create genesis block to verify
        block genesis;
        genesis.major_version = hf::hf7;  // Genesis blocks always use hf7
        genesis.minor_version = static_cast<uint8_t>(hf::hf7);
        genesis.timestamp = 0;
        genesis.prev_id = crypto::hash{};
        genesis.nonce = 12345;  // Must match GENESIS_NONCE in testnet.h
        genesis.miner_tx = genesis_tx;

        // Calculate block hash
        crypto::hash block_hash = get_block_longhash(
            network_type::UNDEFINED,
            randomx_longhash_context(NULL, genesis, 0),
            genesis,
            0,
            0);

        std::cout << "✅ Generated Genesis Block\n";
        std::cout << "   Block Hash: " << oxenc::to_hex(std::string_view{
                             reinterpret_cast<const char*>(block_hash.data()), sizeof(block_hash)}) << "\n\n";

        std::cout << "========================================\n";
        std::cout << "CONFIGURATION VALUES\n";
        std::cout << "========================================\n\n";

        std::cout << "Add this to src/network_config/testnet.h:\n\n";
        std::cout << ".GENESIS_TX = \"" << genesis_tx_hex << "\"sv,\n\n";
        std::cout << ".GOVERNANCE_WALLET_ADDRESS = {\n";
        std::cout << "    \"" << gov_address << "\",  // HF10+\n";
        std::cout << "},\n\n";

        std::cout << "========================================\n";
        std::cout << "WALLET BACKUP INFORMATION\n";
        std::cout << "========================================\n\n";
        std::cout << "⚠️  SAVE THESE KEYS SECURELY!\n\n";
        std::cout << "Governance Address: " << gov_address << "\n";
        std::cout << "Spend Private Key: " << gov_spend_key << "\n";
        std::cout << "View Private Key: " << gov_view_key << "\n\n";

        return 0;
    } catch (const std::exception& e) {
        std::cerr << "❌ Error: " << e.what() << std::endl;
        return 1;
    }
}
