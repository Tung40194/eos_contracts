// #define IS_TEST
#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>

using namespace eosio;
using namespace std;

const symbol system_core_symbol = symbol(symbol_code("CAT"), 4);

CONTRACT donocamp : public contract {

public:

    donocamp(eosio::name receiver, eosio::name code, datastream<const char *> ds) : contract(receiver, code, ds) {}

    void transfer(name from, name to, asset quantity, string memo);

    ACTION burnandlog(name community_account, asset quantity, std::string log);

    ACTION claim(name community_account, name executor, asset quantity);

    ACTION refund();
};

