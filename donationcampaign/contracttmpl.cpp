#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>
#include <eosio/singleton.hpp>

using namespace eosio;
using namespace std;

const symbol system_core_symbol = symbol(symbol_code("CAT"), 4);
const name governance_designer = "community"_n;
const name issuer_account = "vake.c"_n;

enum CodeTypeEnum {
    NORMAL = 0,
    POSITION_CONFIG,
    POSITION_APPOINT,
    POSITION_DISMISS,
    BADGE_CONFIG,
    BADGE_ISSUE,
    BADGE_REVOKE,
};

enum ExecutionType
{
    SOLE_DECISION = 0,
    COLLECTIVE_DECISION,
    BOTH
};

struct CodeType {
    uint8_t type;
    uint64_t refer_id;
};

struct exec_code_data {
        name code_action;
        vector<char> packed_params;
    };


CONTRACT contracttmpl : public contract {
public:

    contracttmpl(eosio::name receiver, eosio::name code, datastream<const char *> ds) : 
        contract(receiver, code, ds), 
        donor_table(_self, _self.value), 
        campaign_table(_self, _self.value),
        governance_v1_code(governance_designer, "community2.c"_n.value) {
            // constructor
        }

    static inline uint128_t build_reference_id(uint64_t reference_id, uint64_t type) {
        return static_cast<uint128_t>(type)  | static_cast<uint128_t>(reference_id) << 64;
    }

    void transfer(name from, name to, asset quantity, string memo);

    ACTION transferfund(name community_account, asset quantity, string memo);

    ACTION refund(name community_account, name revoked_account);

    ACTION initialize(name community_account, uint64_t donor_position_id, uint64_t start_at, uint64_t funding_end_at, uint64_t end_at);

    ACTION config(name community_account, uint64_t donor_position_id, uint64_t start_at, uint64_t funding_end_at, uint64_t end_at);

    // table to store donor info
    TABLE donation_info {
        name donor_name;
        asset token_quantity;
        uint64_t primary_key() const { return donor_name.value; }
        EOSLIB_SERIALIZE( donation_info, (donor_name)(token_quantity));
    };
    typedef eosio::multi_index<"donor.info"_n, donation_info> donation_info_table;

    // table to store campaign info
    TABLE campaign {
        name communityAccount = name{};
        uint64_t donorPositionId = 0;
        uint64_t startAt = 0;
        uint64_t fundingEndAt = 0;
        uint64_t endAt = 0;
        EOSLIB_SERIALIZE( campaign, (communityAccount)(donorPositionId)(startAt)(fundingEndAt)(endAt));
    };
    typedef eosio::singleton<"campaign.inf"_n, campaign> campaign_info_table;
    typedef eosio::multi_index<"campaign.inf"_n, campaign> fcampaign_info_table;

    //to access community/governance designer table
    TABLE v1_code {
        uint64_t code_id;
        name code_name;
        name contract_name;
        vector<name> code_actions;
        uint8_t code_exec_type = ExecutionType::SOLE_DECISION;
        uint8_t amendment_exec_type = ExecutionType::SOLE_DECISION;
        CodeType code_type;

        uint64_t primary_key() const { return code_id; }
        uint64_t by_code_name() const { return code_name.value; }
        uint128_t by_reference_id() const { return build_reference_id(code_type.refer_id, code_type.type); }
        
        EOSLIB_SERIALIZE( v1_code, (code_id)(code_name)(contract_name)(code_actions)(code_exec_type)(amendment_exec_type)(code_type));
    };

    typedef eosio::multi_index<"v1.code"_n, v1_code, 
        indexed_by< "by.code.name"_n, const_mem_fun<v1_code, uint64_t, &v1_code::by_code_name>>,
        indexed_by< "by.refer.id"_n, const_mem_fun<v1_code, uint128_t, &v1_code::by_reference_id>>
        > v1_code_table;

    donation_info_table donor_table;
    campaign_info_table campaign_table;
    v1_code_table governance_v1_code;
};

void contracttmpl::transfer(name from, name to, asset quantity, string memo) {
    check(campaign_table.exists(), "ERR::VERIFY_FAILED::campaign has not been init, please init it first");
    auto campaign_info = campaign_table.get();
    bool isInFundingPeriod = (campaign_info.startAt <= current_time_point().sec_since_epoch() < campaign_info.fundingEndAt);
    check(isInFundingPeriod, "ERR::VERIFY_FAILED::not in voting period");

    if (from == _self) {
        return;
    }
    
    // memo fixed format: donate-<donor_name>
    const std::size_t first_break = memo.find("-");
    check((first_break != std::string::npos), "ERR::VERIFY_FAILED::un supported memo format");
    std::string donor_str = memo.substr(first_break + 1);
    name donor = name{donor_str};
    //require_auth(donor);

    check((to == _self), "ERR::VERIFY_FAILED::contract is not involved in this transfer");
    check(quantity.symbol.is_valid(), "ERR::VERIFY_FAILED::invalid quantity");
    check(quantity.amount > 0, "ERR::VERIFY_FAILED::only positive quantity allowed");
    
    if (quantity.symbol == system_core_symbol) {

        // record donation info for donor refund if revoked
        auto donor_itr = donor_table.find(donor.value);
        if (donor_itr == donor_table.end()) {
            donor_table.emplace(get_self(), [&](auto &row) {
                row.donor_name = donor;
                row.token_quantity = quantity;
            });
        } else {
            donor_table.modify(donor_itr, get_self(), [&](auto& row) {
                row.token_quantity += quantity;
            });
        }

        // TODO: replace when community account's created
        name community_acc = name{"community2.c"};
        // TODO: replace when Donor position's created
        uint64_t donor_position_id = 1;
        vector<name> donors = {donor};
        std::string reason = "appoint donor-position to " + donor_str;

        auto getByCodeReferId = governance_v1_code.get_index<"by.refer.id"_n>();
        uint128_t appointpos_code_id_test = build_reference_id(donor_position_id, CodeTypeEnum::POSITION_APPOINT);
        auto issue_badge_code_itr = getByCodeReferId.find(appointpos_code_id_test);
        uint64_t appointpos_code_id = issue_badge_code_itr->code_id;

        eosio::print("\n>>>appointpos_code_id_test: ", appointpos_code_id);
        check((0 == 1), "\n#stop_debug");

        exec_code_data exec_code;
        exec_code.code_action = name{"appointpos"};
        exec_code.packed_params = eosio::pack(std::make_tuple(community_acc, donor_position_id, donors, reason));
        vector<exec_code_data> code_actions = {exec_code};

        //campaign contract account should be assigned to right holder of "appointpos"
        action(permission_level{_self, "active"_n},
                "governance23"_n,
                "execcode"_n,
                std::make_tuple(community_acc, _self, appointpos_code_id, code_actions)).send();
    }
}

// memo fixed format: donate-<donor_name>
ACTION contracttmpl::transferfund(name community_account, asset quantity, string memo) {
    check(campaign_table.exists(), "ERR::VERIFY_FAILED::campaign has not been init, please init it first");
    auto campaign_info = campaign_table.get();
    bool isInFundExecutingPeriod = (campaign_info.fundingEndAt <= current_time_point().sec_since_epoch() < campaign_info.endAt);
    check(isInFundExecutingPeriod, "ERR::VERIFY_FAILED::not in fund-executing period");

    require_auth(community_account);

    action( permission_level{_self, "active"_n},
        "eosio.token"_n,
        "transfer"_n,
        std::make_tuple(_self, issuer_account, quantity, memo)).send();
}

ACTION contracttmpl::refund(name community_account, name revoked_account) {
    require_auth(community_account);
    check(is_account(revoked_account), "ERR::VERIFY_FAILED::wrong donor account");

    auto dono_itr = donor_table.find(revoked_account.value);
    check(dono_itr != donor_table.end(), "ERR::VERIFY_FAILED::account has not donated");

    asset refunded_quantity = dono_itr->token_quantity;
    string memo = "refund-" + revoked_account.to_string();
    
    action( permission_level{_self, "active"_n},
            "eosio.token"_n,
            "transfer"_n,
            std::make_tuple(_self, issuer_account, refunded_quantity, memo)).send();
    
    // erase donor
    donor_table.erase(dono_itr);
}

ACTION contracttmpl::initialize(name community_account, 
                                uint64_t donor_position_id, 
                                uint64_t start_at, 
                                uint64_t funding_end_at, 
                                uint64_t end_at) {

    // if campaign_table has been init, do not init again
    check((campaign_table.exists() == false), "ERR::VERIFY_FAILED::no re-executing initialization function");
    require_auth(_self);
    campaign_table.set(campaign{community_account, donor_position_id, start_at, funding_end_at, end_at}, _self);
}

ACTION contracttmpl::config(name community_account, 
                            uint64_t donor_position_id, 
                            uint64_t start_at, 
                            uint64_t funding_end_at, 
                            uint64_t end_at) {

    require_auth(community_account);

    auto campaign_info = campaign_table.get();
    campaign_info.donorPositionId = donor_position_id;
    campaign_info.startAt = start_at;
    campaign_info.fundingEndAt = funding_end_at;
    campaign_info.endAt = end_at;
    campaign_table.set(campaign_info, _self);
}

#define EOSIO_ABI_CUSTOM(TYPE, MEMBERS)                                                                                      \
    extern "C"                                                                                                               \
    {                                                                                                                        \
        void apply(uint64_t receiver, uint64_t code, uint64_t action)                                                        \
        {                                                                                                                    \
            auto self = receiver;                                                                                            \
            if (code == "vake.t"_n.value || code == self || code == "eosio.token"_n.value || action == "onerror"_n.value)    \
            {                                                                                                                \
                if (action == "transfer"_n.value)                                                                            \
                {                                                                                                            \
                    check(code == "eosio.token"_n.value, "Must transfer Token");                                             \
                }                                                                                                            \
                switch (action)                                                                                              \
                {                                                                                                            \
                    EOSIO_DISPATCH_HELPER(TYPE, MEMBERS)                                                                     \
                }                                                                                                            \
                /* does not allow destructor of this contract to run: eosio_exit(0); */                                      \
            }                                                                                                                \
        }                                                                                                                    \
    }

EOSIO_ABI_CUSTOM(contracttmpl, (transfer)(transferfund)(refund)(initialize)(config))
