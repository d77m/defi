#pragma once

#include <eosiolib/asset.hpp>
#include <eosiolib/crypto.h>
#include <eosiolib/eosio.hpp>
#include <eosiolib/singleton.hpp>
#include <eosiolib/time.hpp>
#include <string>
#include <vector>

#include <utils.hpp>

using namespace eosio;
using namespace std;

// typedef std::vector<uint64_t> uint64Array;

//contract
class [[eosio::contract("onesgamedefi")]] onesgame : public contract
{
public:
    using eosio::contract::contract;

    onesgame(eosio::name self, eosio::name code,
             eosio::datastream<const char *> ds)
        : contract(self, code, ds),
          _defi_liquidity(_self, _self.value),
          _defi_pool(_self, _self.value),
          _defi_config(_self, _self.value),
          _liquidity_log(_self, _self.value),
          _swap_log(_self, _self.value){

          };

    ~onesgame(){

    };

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

    struct [[eosio::table("config")]] st_defi_config
    {
        uint64_t swap_id;
        uint64_t liquidity_id;
        uint64_t pool_id;
    };
    typedef singleton<"config"_n, st_defi_config> tb_defi_config;

    struct currency_stats
    {
        asset supply;
        asset max_supply;
        name issuer;

        uint64_t primary_key() const { return supply.symbol.code().raw(); }
    };
    typedef eosio::multi_index<"stat"_n, currency_stats> stats;

    struct [[eosio::table]] st_defi_pair
    {
        checksum256 digest;
        uint64_t liquidity_id;

        uint64_t primary_key() const { return utils::uint64_hash(digest); }
        uint64_t liquidity_key() const { return liquidity_id; }
    };
    typedef multi_index<"pair"_n, st_defi_pair,
                        indexed_by<"byliquidity"_n, const_mem_fun<st_defi_pair, uint64_t, &st_defi_pair::liquidity_key>>>
        tb_defi_pair;

    struct [[eosio::table]] st_defi_liquidity
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

    struct [[eosio::table]] st_defi_pool
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

    typedef multi_index<"defipool"_n, st_defi_pool,
                        indexed_by<"byaccountkey"_n, const_mem_fun<st_defi_pool, uint64_t, &st_defi_pool::account_key>>>
        tb_defi_pool;

    struct [[eosio::table]] st_liquidity_log
    {
        uint64_t log_id;
        eosio::name account;
        uint64_t liquidity_id;
        uint64_t liquidity_token;
        token_t in_token;
        token_t out_token;
        eosio::asset in_asset;
        eosio::asset out_asset;
        string type;
        uint64_t timestamp;
        checksum256 trx_id;

        uint64_t primary_key() const { return log_id; }
    };

    typedef multi_index<"liquiditylog"_n, st_liquidity_log> tb_liquidity_log;

    struct [[eosio::table]] st_swap_log
    {
        uint64_t swap_id;
        uint64_t third_id;
        eosio::name account;
        uint64_t liquidity_id;
        token_t in_token;
        token_t out_token;
        eosio::asset in_asset;
        eosio::asset out_asset;
        float_t price;
        uint64_t timestamp;
        checksum256 trx_id;

        uint64_t primary_key() const { return swap_id; }
        uint64_t third_key() const { return third_id; }
    };

    typedef multi_index<"swaplog"_n, st_swap_log,
                        indexed_by<"bythirdkey"_n, const_mem_fun<st_swap_log, uint64_t, &st_swap_log::third_key>>>
        tb_swap_log;

    struct swap_t
    {
        asset quantity;
        asset original_quantity;
        uint64_t code;
    };

public:
    void transfer(name from, name to, asset quantity, string memo);

    [[eosio::action]] void newliquidity(name account, token_t token1, token_t token2);

    [[eosio::action]] void addliquidity(name account, uint64_t liquidity_id);

    [[eosio::action]] void subliquidity(name account, uint64_t liquidity_id, uint64_t liquidity_token);

    [[eosio::action]] void remove(uint64_t type, uint64_t id);

    [[eosio::action]] void updateweight(uint64_t liquidity_id, uint64_t type, float weight);

    void mine(name account, asset quantity, uint64_t liquidity_id);

    void swap(name account, asset quantity, std::vector<std::string> & params);

    swap_t _swap(name account, swap_t & swapin, uint64_t liquidity_id, uint64_t third_id);

    void _swaplog(name account, uint64_t third_id, uint64_t liquidity_id, token_t in_token, token_t out_token, asset in_asset, asset out_asset, asset fee, float_t price);

    void _liquiditylog(name account, uint64_t liquidity_id, string type, token_t in_token, token_t out_token, asset in_asset, asset out_asset, uint64_t liquidity_token);

    void _transfer_to(name to, uint64_t code, asset quantity, string memo);

    uint64_t get_swap_id();

    uint64_t get_liquidity_id();

    uint64_t get_pool_id();

private:
    void
    _transfer_to(name to, uint64_t amount, symbol coin_code, string memo);

    void _newpair(uint64_t liquidity_id, const token_t &token1, const token_t &token2);

public:
    static uint64_t code;

private:
    tb_defi_config _defi_config;
    tb_defi_liquidity _defi_liquidity;
    tb_defi_pool _defi_pool;

    tb_swap_log _swap_log;
    tb_liquidity_log _liquidity_log;
};