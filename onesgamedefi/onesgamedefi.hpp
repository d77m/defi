#pragma once

#include <eosiolib/crypto.h>

#include <eosiolib/asset.hpp>
#include <eosiolib/eosio.hpp>
#include <eosiolib/singleton.hpp>
#include <eosiolib/time.hpp>
#include <string>
#include <utils.hpp>
#include <vector>

using namespace eosio;
using namespace std;

class [[eosio::contract("onesgamedefi")]] onesgame : public contract
{
public:
    using eosio::contract::contract;

    onesgame(eosio::name self, eosio::name code,
             eosio::datastream<const char *> ds)
        : contract(self, code, ds),
          _defi_liquidity(_self, _self.value),
          _defi_config(_self, _self.value),
          _liquidity_log(_self, _self.value),
          _swap_log(_self, _self.value){};

    ~onesgame(){};

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
    typedef multi_index<"pair"_n, st_defi_pair, indexed_by<"byliquidity"_n, const_mem_fun<st_defi_pair, uint64_t, &st_defi_pair::liquidity_key>>> tb_defi_pair;

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

    struct transfer_args
    {
        name from;
        name to;
        asset quantity;
        string memo;
    };

    struct st_defi_transfer
    {
        checksum256 trx_id;
        name action1;
        transfer_args args1;
        name action2;
        transfer_args args2;
        uint64_t status;
    };
    typedef singleton<"transfer"_n, st_defi_transfer> tb_defi_transfer;

    struct [[eosio::table]] st_defi_pools
    {
        eosio::name account;
        uint64_t liquidity_token;
        eosio::asset quantity1;
        eosio::asset quantity2;
        uint64_t timestamp;

        uint64_t primary_key() const { return account.value; }
    };

    typedef multi_index<"defipools"_n, st_defi_pools> tb_defi_pools;

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
                        indexed_by<"bythirdkey"_n, const_mem_fun<st_swap_log, uint64_t,
                                                                 &st_swap_log::third_key>>>
        tb_swap_log;

    struct swap_t
    {
        asset quantity;
        asset original_quantity;
        uint64_t code;
    };

    struct [[eosio::table]] st_market_info
    {
        uint64_t liquidity_id;
        eosio::name account;

        eosio::asset in_token1;
        eosio::asset in_token2;

        uint64_t liquidity_token;
        symbol liquidity_symbol;

        eosio::asset out_token1;
        eosio::asset out_token2;

        eosio::asset profit;

        uint64_t timestamp;
        uint64_t status;

        uint64_t primary_key() const { return liquidity_id; }
    };

    typedef multi_index<"marketinfo"_n, st_market_info> tb_market_info;

    struct [[eosio::table]] st_market_log
    {
        uint64_t mine_id;
        eosio::name account;
        uint64_t liquidity_id;

        eosio::asset in_token1;
        eosio::asset in_token2;

        uint64_t liquidity_token;

        eosio::asset out_token1;
        eosio::asset out_token2;

        eosio::asset profit;

        uint64_t begin_timestamp;
        uint64_t end_timestamp;

        uint64_t primary_key() const { return mine_id; }
    };

    typedef multi_index<"marketlog"_n, st_market_log> tb_market_log;

public:
    void transfer(name from, name to, asset quantity, string memo);

    [[eosio::action]] void newliquidity(name account, token_t token1, token_t token2);

    [[eosio::action]] void addliquidity(name account, uint64_t liquidity_id);

    [[eosio::action]] void subliquidity(name account, uint64_t liquidity_id, uint64_t liquidity_token);

    [[eosio::action]] void remove(uint64_t id);

    [[eosio::action]] void updateweight(uint64_t liquidity_id, uint64_t type, float weight);

    [[eosio::action]] void marketmine(name account, uint64_t liquidity_id, uint64_t to_liquidity_id, asset quantity1, asset quantity2);
    [[eosio::action]] void marketexit(string memo, uint64_t amount);
    [[eosio::action]] void marketclaim();

    [[eosio::action]] void marketsettle();

private:
    void _addliquidity(name from, name to, asset quantity, string memo);

    void _marketclaim_box();
    void _marketexit_box(uint64_t liquidity_token, symbol symbol_code, string memo);

    void _marketexit_dfs(uint64_t liquidity_token, uint64_t liquidity_id);

    void _handle_box(name from, name to, asset quantity, string memo);
    void _handle_dfs(name from, name to, asset quantity, string memo);
    void _handle_refund(asset quantity);
    void _handle_withdraw(asset quantity);

    void swapmine(name account, uint64_t code, asset quantity, uint64_t liquidity_id);

    void swap(name account, asset quantity, std::vector<std::string> & params);

    swap_t _swap(name account, swap_t & swapin, uint64_t liquidity_id, uint64_t slippage, uint64_t third_id);

    void _swaplog(name account, uint64_t third_id, uint64_t liquidity_id,
                  token_t in_token, token_t out_token, asset in_asset,
                  asset out_asset, asset fee, float_t price);

    void _liquiditylog(name account, uint64_t liquidity_id, string type,
                       token_t in_token, token_t out_token, asset in_asset,
                       asset out_asset, uint64_t liquidity_token,
                       asset in_balance, asset out_balance, uint64_t balance_ltoken);

    void _transfer_to(name to, uint64_t code, asset quantity, string memo);

    uint64_t _get_swap_id();

    uint64_t _get_liquidity_id();

    uint64_t _get_pool_id();

    checksum256 _get_trx_id();

    std::vector<eosio::action> _get_actions();

    void _transfer_to(name to, uint64_t amount, symbol coin_code, string memo);

    void _newpair(uint64_t liquidity_id, const token_t &token1, const token_t &token2);

    checksum256 _getpair_digest(const token_t &token1, const token_t &token2);

    tb_defi_config _defi_config;
    tb_defi_liquidity _defi_liquidity;

    tb_swap_log _swap_log;
    tb_liquidity_log _liquidity_log;

public:
    static uint64_t code;
};