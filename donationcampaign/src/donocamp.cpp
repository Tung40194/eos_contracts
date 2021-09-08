#include "../include/donocamp.hpp"

void donocamp::transfer(name from, name to, asset quantity, string memo) {
    eosio::print("\n>>>donocamp::mark1");
    if (from == _self) {
        return;
    }
    eosio::print("\n>>>donocamp::mark2");
    check((to == _self), "ERR::VERIFY_FAILED::contract is not involved in this transfer");
    check(quantity.symbol.is_valid(), "ERR::VERIFY_FAILED::invalid quantity");
    check(quantity.amount > 0, "ERR::VERIFY_FAILED::only positive quantity allowed");
    eosio::print("\n>>>donocamp::mark3");
    if (quantity.symbol == system_core_symbol)
    {
        // testing
        // action(
        //     permission_level{_self, "active"_n},
        //     "governance23"_n,
        //     "dummy"_n,
        //     std::make_tuple(0))
        //     .send();
        eosio::print("\n>>>donocamp::mark4");
        vector<name> donors = {from};
        eosio::print("\n>>>donocamp::mark5");
        action(
            permission_level{_self, "active"_n},
            "governance23"_n,
            "appointpos"_n,
            std::make_tuple("community5.c"_n, 6, donors, "testing"))
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
