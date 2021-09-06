// #define IS_TEST
#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>
#include <eosio/singleton.hpp>

using namespace eosio;
using namespace std;

CONTRACT donocamp : public contract {
private:

    asset convertbytes2cat(uint32_t bytes);

    TABLE account {
        asset    balance;
        uint64_t primary_key()const { return balance.symbol.code().raw(); }
    };
    typedef eosio::multi_index< "accounts"_n, account > accounts;

    static asset get_balance( const name& token_contract_account, const name& owner, const symbol_code& sym_code ) {
        accounts accountstable( token_contract_account, owner.value );
        const auto& ac = accountstable.get( sym_code.raw() );
        return ac.balance;
    }

public:
    donocamp(eosio::name receiver, eosio::name code, datastream<const char *> ds) : contract(receiver, code, ds) {}

    void transfer(name from, name to, asset quantity, string memo);

    ACTION claim();

    ACTION refund();

    //ACTION dummy(name test);
    
    /*
	* global singelton table, used for position id building
	* Scope: self
	*/
	TABLE v1global {
		  
		v1global(){}
        symbol core_symbol = symbol(symbol_code("CAT"), 4);
        uint64_t init_ram_amount = 10*1024;
        // init stake net for new community account
        asset init_net = asset(1'0000, this->core_symbol);
        // init statke cpu for new community account
        asset init_cpu = asset(1'0000, this->core_symbol);
        asset min_active_contract = asset(10'0000, this->core_symbol);

		EOSLIB_SERIALIZE( v1global, 
            (core_symbol)
            (init_ram_amount)
            (init_net)
            (init_cpu) 
            (min_active_contract)
        );
	};
    typedef eosio::singleton< "v1.global"_n, v1global> v1_global_table;
    v1global _config;

};

