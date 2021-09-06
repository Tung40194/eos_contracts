#include "../include/donocamp.hpp"
#include "exchange_state.cpp"
const symbol ramcore_symbol = symbol(symbol_code("RAMCORE"), 4);
const symbol ram_symbol = symbol(symbol_code("RAM"), 0);

asset donocamp::convertbytes2cat(uint32_t bytes) {
    v1_global_table config(_self, _self.value);
    _config = config.exists() ? config.get() : v1global{};
    const symbol core_symbol = _config.core_symbol;

    eosiosystem::rammarket _rammarket("eosio"_n, "eosio"_n.value);
    auto itr = _rammarket.find(ramcore_symbol.raw());
    auto tmp = *itr;
    auto eosout = tmp.convert(asset(bytes, ram_symbol), core_symbol);
    return eosout;
}

void donocamp::transfer(name from, name to, asset quantity, string memo) {
    if (from == _self)
    {
        return;
    }

    v1_global_table config(_self, _self.value);
    _config = config.exists() ? config.get() : v1global{};
    const symbol system_core_symbol = _config.core_symbol;
    const uint64_t init_ram_amount = _config.init_ram_amount;
    const asset min_active_contract = _config.min_active_contract;
    const asset init_cpu = _config.init_cpu;
    const asset init_net = _config.init_net;

    print("\n>>>balance: ", get_balance("eosio.token"_n, get_self(), system_core_symbol.code()));
    check(to == _self, "ERR::VERIFY_FAILED::contract is not involved in this transfer");
    check(quantity.symbol.is_valid(), "ERR::VERIFY_FAILED::invalid quantity");
    check(quantity.amount > 0, "ERR::VERIFY_FAILED::only positive quantity allowed");
    check(quantity.amount > 0, "ERR::VERIFY_FAILED::must transfer positive quantity");
    check(get_balance("eosio.token"_n, get_self(), system_core_symbol.code()) >= min_active_contract, "ERR::VERIFY_FAILED::Deposit at least 10 CAT to active creating commnity feature");

    if (quantity.symbol == system_core_symbol)
    {
        const asset ram_fee = convertbytes2cat(init_ram_amount);
        check(quantity >= init_cpu + init_net + ram_fee, "ERR::VERIFY_FAILED::insuffent balance to create new account");
        const asset remain_balance = quantity - init_cpu - init_net - ram_fee;

        if (remain_balance.amount > 0)
        {
            action(
                permission_level{_self, "active"_n},
                "eosio.token"_n,
                "transfer"_n,
                std::make_tuple(_self, from, remain_balance, std::string("return remain amount kakaka")))
                .send();
        }

        action(
                permission_level{_self, "active"_n},
                "governance23"_n,
                "dummy"_n,
                std::make_tuple()
              ).send();
    }
            
}

ACTION donocamp::claim() {
    
}

ACTION donocamp::refund() {
    
}

// ACTION donocamp::dummy(name test) {
//     require_auth(test);
//     eosio::print("\n>>>CALLING DUMMY FUNCTION");
// }

//void donocamp::releasefund(name receiver, asset quantity, string memo) {
//}

#define EOSIO_ABI_CUSTOM(TYPE, MEMBERS)                                                                          \
    extern "C"                                                                                                   \
    {                                                                                                            \
        void apply(uint64_t receiver, uint64_t code, uint64_t action)                                            \
        {                                                                                                        \
            auto self = receiver;                                                                                \
            if (code == self || code == "eosio.token"_n.value || action == "onerror"_n.value)                    \
            {                                                                                                    \
                eosio::print("\n###receiver is: ", receiver);                                                    \
                eosio::print("\n###Code is: ", code);                                                            \
                eosio::print("\n###action is: ", action);                                                        \
                if (action == "transfer"_n.value)                                                                \
                {                                                                                                \
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

EOSIO_ABI_CUSTOM(donocamp, (transfer))
