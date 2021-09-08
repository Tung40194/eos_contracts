#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>

using namespace eosio;
using namespace std;

const symbol system_core_symbol = symbol(symbol_code("CAT"), 4);

struct executor_info {
    name receiver_name;
    asset quantity;
    std::string memo;
};

struct exec_code_data {
        name code_action;
        vector<char> packed_params;
    };

CONTRACT contractTmpl : public contract {

public:

    contractTmpl(eosio::name receiver, eosio::name code, datastream<const char *> ds) : contract(receiver, code, ds), dono_table(_self, _self.value) {}

    void transfer(name from, name to, asset quantity, string memo);

    ACTION transferfund(name community_account, vector<executor_info> executors);

    ACTION refund(name campaign_admin, name revoked_account, name vake_account);

    // table to store donor info
    TABLE donation_info {
        name donor_name;
        asset token_quantity;
        uint64_t primary_key() const { return donor_name.value; }
        EOSLIB_SERIALIZE( donation_info, (donor_name)(token_quantity));
    };
    typedef eosio::multi_index<"dono.info"_n, donation_info> donation_info_table;

    donation_info_table dono_table;
};


void contractTmpl::transfer(name from, name to, asset quantity, string memo) {
    if (from == _self) {
        return;
    }
    
    check((to == _self), "ERR::VERIFY_FAILED::contract is not involved in this transfer");
    check(quantity.symbol.is_valid(), "ERR::VERIFY_FAILED::invalid quantity");
    check(quantity.amount > 0, "ERR::VERIFY_FAILED::only positive quantity allowed");
    
    if (quantity.symbol == system_core_symbol) {
        // record donation info for donor refund if revoked
        dono_table.emplace(get_self(), [&](auto &row) {
            row.donor_name = name{dono_table.available_primary_key()};
            row.token_quantity = quantity;
        });

        // TODO: replace when community account's created
        name community_acc = name{"community2.c"};
        // TODO: replace when Donor position's created
        uint64_t donorpos_id = 1;
        vector<name> sender = {from};
        std::string reason = "appoint donor position to sender";
        uint64_t appointpos_code_id = 6;
        
        exec_code_data exec_code;
        exec_code.code_action = name{"appointpos"};
        exec_code.packed_params = eosio::pack(std::make_tuple(community_acc, donorpos_id, sender, reason));
        vector<exec_code_data> code_actions = {exec_code};

        //campaign contract account should be assigned to right holder of "appointpos"
        action(permission_level{_self, "active"_n},
                "governance23"_n,
                "execcode"_n,
                std::make_tuple(community_acc, _self, appointpos_code_id, code_actions)).send();
    }
}

ACTION contractTmpl::transferfund(name community_account, vector<executor_info> executors) {
    require_auth(community_account);
    name burning_address = "eosio"_n;

    for (auto executor : executors) {
        action( permission_level{_self, "active"_n},
            "eosio.token"_n,
            "transfer"_n,
            std::make_tuple(executor.receiver_name, burning_address, executor.quantity, executor.memo)).send();
    }
}

//TODO: replace campaign admin, vake account when generate
ACTION contractTmpl::refund(name campaign_admin, name revoked_account, name vake_account) {
    require_auth(campaign_admin);
    check(is_account(revoked_account), "ERR::VERIFY_FAILED::wrong donor account");

    auto dono_itr = dono_table.find(revoked_account.value);
    check(dono_itr != dono_table.end(), "ERR::VERIFY_FAILED::account has not donated");

    asset refunded_quantity = dono_itr->token_quantity;
    
    action( permission_level{_self, "active"_n},
            "eosio.token"_n,
            "transfer"_n,
            std::make_tuple(_self, vake_account, refunded_quantity, std::string("refund to revoked account"))).send();
    
    // erase donor
    dono_table.erase(dono_itr);
}


#define EOSIO_ABI_CUSTOM(TYPE, MEMBERS)                                                                          \
    extern "C"                                                                                                   \
    {                                                                                                            \
        void apply(uint64_t receiver, uint64_t code, uint64_t action)                                            \
        {                                                                                                        \
            auto self = receiver;                                                                                \
            if (code == self || code == "eosio.token"_n.value || action == "onerror"_n.value)                    \
            {                                                                                                    \
                if (action == "transfer"_n.value)                                                                \
                {                                                                                                \
                    check(code == "eosio.token"_n.value, "Must transfer Token");                                 \
                }                                                                                                \
                switch (action)                                                                                  \
                {                                                                                                \
                    EOSIO_DISPATCH_HELPER(TYPE, MEMBERS)                                                         \
                }                                                                                                \
                /* does not allow destructor of this contract to run: eosio_exit(0); */                          \
            }                                                                                                    \
        }                                                                                                        \
    }

EOSIO_ABI_CUSTOM(contractTmpl, (transferfund)(refund))
