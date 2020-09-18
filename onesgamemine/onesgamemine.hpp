#pragma once

#include <eosiolib/asset.hpp>
#include <eosiolib/crypto.h>
#include <eosiolib/eosio.hpp>
#include <eosiolib/singleton.hpp>
#include <eosiolib/time.hpp>
#include <string>
#include <vector>

using namespace eosio;
using namespace std;

// typedef std::vector<uint64_t> uint64Array;

//contract
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

    [[eosio::action]] void minemarket(name account, uint64_t round, float_t factor);

    [[eosio::action]] void claim(name account);

    [[eosio::action]] void init();

    void issue(name to, asset quantity, string memo);

    struct [[eosio::table("config")]] st_defi_config
    {
        uint64_t swap_time;
        uint64_t swap_quantity;
        uint64_t swap_suply;
        uint64_t swap_counter;
        uint64_t swap_issue;
        uint64_t market_suply;
        uint64_t market_time;
        uint64_t last_swap_suply;
        uint64_t market_quantity;
        uint64_t market_issue;
    };
    typedef singleton<"config"_n, st_defi_config> tb_defi_config;

    struct [[eosio::table]] st_defi_account
    {
        eosio::name account;
        asset quantity;
        asset market_quantity;
        asset swap_quantity;
        asset mine_quantity;
        uint64_t market_round;
        uint64_t timestamp;

        uint64_t primary_key() const { return account.value; }
    };

    typedef multi_index<"account"_n, st_defi_account> tb_defi_account;

    struct [[eosio::table]] st_defi_market
    {
        uint64_t round;
        asset quantity;
        uint64_t total;
        uint64_t executed;
        uint64_t timestamp;

        uint64_t primary_key() const { return round; }
    };

    typedef multi_index<"market"_n, st_defi_market> tb_defi_market;

private:
    void _transfer_to(name to, uint64_t amount, symbol coin_code, string memo);

private:
    tb_defi_account _defi_account;

    tb_defi_config _defi_config;
    tb_defi_market _defi_market;
};