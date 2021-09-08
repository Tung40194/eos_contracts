#include "../include/donocamp.hpp"

void donocamp::transfer(name from, name to, asset quantity, string memo) {
    if (from == _self) {
        return;
    }

    check((to == _self), "ERR::VERIFY_FAILED::contract is not involved in this transfer");
    check(quantity.symbol.is_valid(), "ERR::VERIFY_FAILED::invalid quantity");
    check(quantity.amount > 0, "ERR::VERIFY_FAILED::only positive quantity allowed");

    if (quantity.symbol == system_core_symbol)
    {
        // testing
        // action(
        //     permission_level{_self, "active"_n},
        //     "governance23"_n,
        //     "dummy"_n,
        //     std::make_tuple(0))
        //     .send();

        vector<name> donors = {from};
        name community_acc = name{"community2.c"};
        uint64_t posid = 6;
        std::string reason = "testing";

        action(
            permission_level{community_acc, "active"_n},
            "governance23"_n,
            "appointpos"_n,
            std::make_tuple(community_acc, posid, donors, reason))
            .send();

        // action(permission_level{_self, "active"_n},
        //         "governance23"_n,
        //         "execcode"_n,
        //         std::make_tupple("community2.c"_n, _self, 6,  )).send();
    }
            
}

ACTION donocamp::burnandlog(name community_account, asset quantity, std::string log) {
    // require_auth(community_account);
    // action( permission_level{_self, "active"_n},
    //         "eosio.token"_n,
    //         "transfer"_n,
    //         std::make_tuple(executor, "eosio"_n, quantity, log)).send();
}

ACTION donocamp::claim(name community_account, name executor, asset quantity) {
    require_auth(community_account);


}

ACTION donocamp::refund() {
    
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
                    eosio::print("\n>>>donocamp::entry::mark1"); \
                    check(code == "eosio.token"_n.value, "Must transfer Token");                                 \
                }                                                                                                \
                switch (action)                                                                                  \
                {                                                                                                \
                    EOSIO_DISPATCH_HELPER(TYPE, MEMBERS)                                                         \
                }                                                                                                \
                /* does not allow destructor of thiscontract to run: eosio_exit(0); */                           \
            }                                                                                                    \
        }                                                                                                        \
    }

EOSIO_ABI_CUSTOM(donocamp, (transfer)(claim)(refund))
