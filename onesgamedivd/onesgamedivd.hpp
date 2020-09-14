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
class [[eosio::contract("onesgamedivd")]] onesgame : public contract
{
public:
    using eosio::contract::contract;

    onesgame(eosio::name self, eosio::name code,
             eosio::datastream<const char *> ds)
        : contract(self, code, ds),
          _defi_config(_self, _self.value),
          _defi_account(_self, _self.value),
          _defi_stake(_self, _self.value){

          };

    ~onesgame(){

    };

    [[eosio::action]] void claim(name account);

    [[eosio::action]] void unstake(name account, uint64_t stake_id);

    [[eosio::action]] void init();

    [[eosio::action]] void bonus();

    void transfer(name from, name to, asset quantity, string memo);

    struct [[eosio::table("config")]] st_defi_config
    {
        asset stake_quantity;
        asset retire_quantity;
        asset reward_quantity;
        asset bonus_quantity;
        uint64_t state;
        uint64_t stake_id;
        uint64_t bonus_id;
    };
    typedef singleton<"config"_n, st_defi_config> tb_defi_config;

    struct [[eosio::table]] st_defi_account
    {
        eosio::name account;
        asset stake_quantity;
        asset reward_quantity;
        asset unclaim_quantity;
        asset retire_quantity;

        uint64_t primary_key() const { return account.value; }
        uint64_t stake_key() const { return stake_quantity.amount; }
    };

    typedef multi_index<"accounts"_n, st_defi_account,
                        indexed_by<"bystake"_n, const_mem_fun<st_defi_account, uint64_t, &st_defi_account::stake_key>>>
        tb_defi_account;

    struct [[eosio::table]] st_defi_stake
    {
        uint64_t stake_id;
        eosio::name account;
        asset quantity;
        uint64_t timestamp;

        uint64_t primary_key() const { return stake_id; }
        uint64_t account_key() const { return account.value; }
    };

    typedef multi_index<"stakelog"_n, st_defi_stake,
                        indexed_by<"byaccount"_n, const_mem_fun<st_defi_stake, uint64_t, &st_defi_stake::account_key>>>
        tb_defi_stake;

    struct [[eosio::table]] st_defi_bonus
    {
        uint64_t bonus_id;
        asset bonus_quantity;
        asset stake_quantity;
        uint64_t timestamp;

        uint64_t primary_key() const { return bonus_id; }
    };

    typedef multi_index<"bonuslog"_n, st_defi_bonus> tb_defi_bonus;

private:
    uint64_t _get_stake_id();

    void _stake(name account, asset quantity);

    void _addbonus(asset quantity);

    void _sub(asset stake_quantity, asset retire_quantity);
    void _add(asset stake_quantity);
    void _transfer_to(name to, uint64_t amount, symbol coin_code, string memo);

private:
    tb_defi_account _defi_account;

    tb_defi_config _defi_config;
    tb_defi_stake _defi_stake;
};