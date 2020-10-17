#pragma once

#include <eosiolib/crypto.h>

#include <eosiolib/asset.hpp>
#include <eosiolib/eosio.hpp>
#include <eosiolib/singleton.hpp>
#include <eosiolib/time.hpp>
#include <string>
#include <vector>

using namespace eosio;
using namespace std;

// contract
class [[eosio::contract("onesgamemine")]] onesgame : public contract
{
public:
    using eosio::contract::contract;

    onesgame(eosio::name self, eosio::name code,
             eosio::datastream<const char *> ds)
        : contract(self, code, ds),
          _defi_config(_self, _self.value),
          _defi_account(_self, _self.value),
          _defi_market(_self, _self.value){

          };

    ~onesgame(){

    };

    [[eosio::action]] void mineswap(name account, asset quantity);

    [[eosio::action]] void minemarket(uint64_t round_id);

    [[eosio::action]] void minemarkets();

    [[eosio::action]] void claim(name account);

    [[eosio::action]] void init();

    [[eosio::action]] void upgrade();

    [[eosio::action]] void oauth(name account, uint8_t status);

    void issue(name to, asset quantity, string memo);

    void transfer(name from, name to, asset quantity, string memo);

    struct [[eosio::table("config")]] st_defi_config
    {
        uint64_t swap_time;
        uint64_t swap_quantity;
        uint64_t swap_suply;
        uint64_t swap_counter;
        uint64_t swap_issue;
        vector<uint64_t> market_suply;
        uint64_t market_time;
        uint64_t last_swap_suply;
        vector<uint64_t> market_quantity;
        uint64_t market_issue;
    };
    typedef singleton<"config"_n, st_defi_config> tb_defi_config;

    struct [[eosio::table]] st_defi_account
    {
        eosio::name account;
        vector<uint64_t> quantity;
        vector<uint64_t> market_quantity;

        asset swap_quantity;
        asset mine_quantity;
        uint64_t market_round;
        uint64_t timestamp;
        uint8_t oauth;

        uint64_t primary_key() const { return account.value; }
    };

    typedef multi_index<"account"_n, st_defi_account> tb_defi_account;

    struct [[eosio::table]] st_defi_market
    {
        uint64_t round;
        uint64_t amount;
        vector<uint64_t> quantity;
        uint64_t total;
        uint64_t executed;
        uint64_t timestamp;

        uint64_t primary_key() const { return round; }
    };

    typedef multi_index<"market"_n, st_defi_market> tb_defi_market;

    struct [[eosio::table]] st_defi_round
    {
        uint64_t id;
        eosio::name account;
        uint64_t round_id;

        uint64_t amount;

        uint64_t primary_key() const { return id; }
        // uint64_t account_key() const { return account.value; }
        uint64_t round_key() const { return round_id; }
    };

    typedef multi_index<
        "round"_n, st_defi_round,
        // indexed_by<"bykey"_n, const_mem_fun<st_defi_round, uint64_t,
        //  &st_defi_round::round_key>>,
        indexed_by<"byroundkey"_n, const_mem_fun<st_defi_round, uint64_t,
                                                 &st_defi_round::round_key>>>
        tb_defi_round;

    struct token_t
    {
        name address;
        symbol symbol;
        friend bool operator==(const token_t &a, const token_t &b)
        {
            return a.address == b.address && a.symbol == b.symbol;
        }
        EOSLIB_SERIALIZE(token_t, (address)(symbol))
    };

    struct st_defi_pool
    {
        uint64_t pool_id;
        eosio::name account;
        uint64_t liquidity_id;
        uint64_t liquidity_token;
        eosio::asset quantity1;
        eosio::asset quantity2;
        uint64_t timestamp;

        uint64_t primary_key() const { return pool_id; }
        uint64_t account_key() const { return account.value; }
    };

    typedef multi_index<
        "defipool"_n, st_defi_pool,
        indexed_by<"byaccountkey"_n, const_mem_fun<st_defi_pool, uint64_t,
                                                   &st_defi_pool::account_key>>>
        tb_defi_pool;

    struct st_defi_liquidity
    {
        uint64_t liquidity_id;
        token_t token1;
        token_t token2;

        eosio::asset quantity1;
        eosio::asset quantity2;
        uint64_t liquidity_token;
        float_t price1;
        float_t price2;
        uint64_t cumulative1;
        uint64_t cumulative2;
        float_t swap_weight;
        float_t liquidity_weight;
        uint64_t timestamp;

        uint64_t primary_key() const { return liquidity_id; }
    };

    typedef multi_index<"liquidity"_n, st_defi_liquidity> tb_defi_liquidity;

private:
    void _transfer_to(name to, uint64_t amount, symbol coin_code, string memo);

    void _syncmarket(uint64_t round_id, uint64_t & total_amount, uint64_t & total_user);

    void _minemarket(uint64_t round_id, name account, const vector<uint64_t> &quantity, float factor);

private:
    tb_defi_account _defi_account;

    tb_defi_config _defi_config;

    tb_defi_market _defi_market;
};