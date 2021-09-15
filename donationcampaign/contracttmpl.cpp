#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>
#include <eosio/singleton.hpp>

using namespace eosio;
using namespace std;

const symbol donate_symbol = symbol(symbol_code("CAT"), 4);
const name governance_designer = "governance23"_n;
const name governance = "community"_n;
const name issuer_account = "vake.c"_n;
const string donate_prefix = "donate";
const string refund_prefix = "refund";
const string transfer_prefix = "transfer";

enum CodeTypeEnum {
    NORMAL = 0,
    POSITION_CONFIG,
    POSITION_APPOINT,
    POSITION_DISMISS,
    BADGE_CONFIG,
    BADGE_ISSUE,
    BADGE_REVOKE,
};

enum ExecutionType {
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
        campaign_table(_self, _self.value) {
            // constructor
        }

    static inline uint128_t build_reference_id(uint64_t reference_id, uint64_t type) {
        return static_cast<uint128_t>(type)  | static_cast<uint128_t>(reference_id) << 64;
    }

    /*  Action   : transfer
        Description: handle fund sent into this contract
        Parameters : from                   - sender
                     to                     - has to be this contract
                     quantity               - amount of token sent
                     memo                   - donor's infor under a fixed format of: donate-<donor_name>
        Return     : none */
    void transfer(name from, name to, asset quantity, string memo);

    /*  Action   : transferfund
        Description: transfer and log fund information
        Parameters : quantity               - amount of token to be funded
                     receiver               - receiver account
        Return     : none */
    ACTION transferfund(asset quantity, name receiver);

    /*  Action   : refund
        Description: refund if donors're revoked 
        Parameters : revoked_account        - revoked account
        Return     : none */
    ACTION refund(name revoked_account);

    /*  Action   : initialize
        Description: initialize campaign information. Executed only once before campaign starts
        Parameters : community_account      - created community's account
                     donor_position_id      - donor-position id
                     start_at               - when funding starts
                     funding_end_at         - when funding ends
                     end_at                 - when fund execution & campaign end
        Return     : none */
    ACTION initialize(name community_account, uint64_t donor_position_id, uint64_t start_at, uint64_t funding_end_at, uint64_t end_at);
    
    /*  Action   : config
        Description: configure campaign information
        Parameters : donor_position_id      - donor-position id
                     start_at               - when funding starts
                     funding_end_at         - when funding ends
                     end_at                 - when fund execution & campaign end
        Return     : none */
    ACTION config(uint64_t donor_position_id, uint64_t start_at, uint64_t funding_end_at, uint64_t end_at);

    // to record donation info for donor-refund if revoked
    TABLE donation_info {
        name donor_name;
        asset token_quantity;
        uint64_t primary_key() const { return donor_name.value; }
        EOSLIB_SERIALIZE( donation_info, (donor_name)(token_quantity));
    };
    typedef eosio::multi_index<"donor.info"_n, donation_info> donation_info_table;

    // to store campaign info
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

    // to access community/governance designer table
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
};

void contracttmpl::transfer(name from, name to, asset quantity, string memo) {
    if (from == _self) {
        return;
    }

    check((to == _self), "ERR::VERIFY_FAILED::contract is not involved in this transfer.");
    check(quantity.symbol.is_valid(), "ERR::VERIFY_FAILED::invalid quantity.");
    check(quantity.amount > 0, "ERR::VERIFY_FAILED::only positive quantity allowed.");

    check(campaign_table.exists(), "ERR::VERIFY_FAILED::campaign has not been initialized, please run initialize function first.");
    auto campaign_info = campaign_table.get();
    const name created_community = campaign_info.communityAccount;
    uint64_t donor_pos_id = campaign_info.donorPositionId;
    //bool isInFundingPeriod = (campaign_info.startAt <= current_time_point().sec_since_epoch()) && (current_time_point().sec_since_epoch() < campaign_info.fundingEndAt);
    //check(isInFundingPeriod, "ERR::VERIFY_FAILED::not in voting period.");
    
    const std::size_t first_break = memo.find("-");
    
    std::string memo_prefix = memo.substr(0, first_break);
    bool equal = (donate_prefix.compare(memo_prefix) == 0);
    check(equal, "ERR::VERIFY_FAILED::unsupported memo format.");

    check((first_break != std::string::npos), "ERR::VERIFY_FAILED::unsupported memo format.");
    std::string donor_str = memo.substr(first_break + 1);
    name donor = name{donor_str};
    require_auth(donor);
    
    if (quantity.symbol == donate_symbol) {
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

        vector<name> donors = {donor};
        std::string reason = "appoint donor-position to " + donor_str;

        v1_code_table governance_v1_code(governance, created_community.value);
        auto getByCodeReferId = governance_v1_code.get_index<"by.refer.id"_n>();
        uint128_t appointpos_code = build_reference_id(donor_pos_id, CodeTypeEnum::POSITION_APPOINT);
        auto appointpos_code_itr = getByCodeReferId.find(appointpos_code);
        uint64_t appointpos_code_id = appointpos_code_itr->code_id;

        // packing appointpos action for execcode
        exec_code_data exec_code;
        exec_code.code_action = name{"appointpos"};
        exec_code.packed_params = eosio::pack(std::make_tuple(created_community, donor_pos_id, donors, reason));
        vector<exec_code_data> code_actions = {exec_code};

        //campaign contract account should be assigned to right holder of "appointpos"
        action(permission_level{_self, "active"_n},
                governance_designer,
                "execcode"_n,
                std::make_tuple(created_community, _self, appointpos_code_id, code_actions)).send();
    }
}

ACTION contracttmpl::transferfund(asset quantity, name receiver) {
    check(campaign_table.exists(), "ERR::VERIFY_FAILED::campaign has not been initialized, please run initialize function first.");
    auto campaign_info = campaign_table.get();
    bool isInFundExecutingPeriod = (campaign_info.fundingEndAt <= current_time_point().sec_since_epoch()) && (current_time_point().sec_since_epoch() < campaign_info.endAt);
    check(isInFundExecutingPeriod, "ERR::VERIFY_FAILED::not in fund-executing period.");

    require_auth(campaign_info.communityAccount);
    string memo = transfer_prefix + "-" + receiver.to_string();

    action( permission_level{_self, "active"_n},
        "eosio.token"_n,
        "transfer"_n,
        std::make_tuple(_self, issuer_account, quantity, memo)).send();
}

ACTION contracttmpl::refund(name revoked_account) {
    check(is_account(revoked_account), "ERR::VERIFY_FAILED::wrong donor account.");

    check(campaign_table.exists(), "ERR::VERIFY_FAILED::campaign has not been initialized, please run initialize function first.");
    auto campaign_info = campaign_table.get();
    bool isInFundingPeriod = (campaign_info.startAt <= current_time_point().sec_since_epoch()) && (current_time_point().sec_since_epoch() < campaign_info.fundingEndAt);
    check(isInFundingPeriod, "ERR::VERIFY_FAILED::not in voting period.");

    require_auth(campaign_info.communityAccount);

    auto dono_itr = donor_table.find(revoked_account.value);
    check(dono_itr != donor_table.end(), "ERR::VERIFY_FAILED::account has not donated.");

    asset refunded_quantity = dono_itr->token_quantity;
    string memo = refund_prefix + "-" + revoked_account.to_string();
    
    action( permission_level{_self, "active"_n},
            "eosio.token"_n,
            "transfer"_n,
            std::make_tuple(_self, issuer_account, refunded_quantity, memo)).send();
    
    donor_table.erase(dono_itr);
}

ACTION contracttmpl::initialize(name community_account, 
                                uint64_t donor_position_id, 
                                uint64_t start_at, 
                                uint64_t funding_end_at, 
                                uint64_t end_at) {
    check((campaign_table.exists() == false), "ERR::VERIFY_FAILED:: initialization function can only be executed once.");
    require_auth(_self);
    campaign_table.set(campaign{community_account, donor_position_id, start_at, funding_end_at, end_at}, _self);
}

ACTION contracttmpl::config(uint64_t donor_position_id, 
                            uint64_t start_at, 
                            uint64_t funding_end_at, 
                            uint64_t end_at) {

    check(campaign_table.exists(), "ERR::VERIFY_FAILED::campaign has not been initialized, please run initialize function first.");

    auto campaign_info = campaign_table.get();
    //require_auth(campaign_info.communityAccount);

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
                    check(code == "eosio.token"_n.value || code == "vake.t"_n.value , "Must transfer Token");                \
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
