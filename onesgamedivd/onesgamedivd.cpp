#include "onesgamedivd.hpp"
#include <eosiolib/transaction.hpp>

#define EOS_TOKEN_SYMBOL symbol("EOS", 4)
#define EOS_TOKEN_ACCOUNT "eosio.token"

#define ONES_TOKEN_SYMBOL symbol("ONES", 4)
#define ONES_TOKEN_ACCOUNT "eosonestoken"

#define ACCOUNT_CHECK(account) \
    eosio_assert(is_account(account), "invalid account " #account);

void onesgame::transfer(name from, name to, asset quantity, string memo)
{

    ACCOUNT_CHECK(from);
    ACCOUNT_CHECK(to);

    require_auth(from);

    if (from == get_self() || to != get_self())
    {
        return;
    }

    eosio_assert(quantity.is_valid(), "invalid quantity");
    eosio_assert(quantity.amount > 0, "must transfer positive quantity");
    if (memo == "stake")
    {
        eosio_assert(quantity.amount % 100000 == 0, "must transfer 10*n quantity");

        this->_stake(from, quantity);
        return;
    }
    else if (memo == "swap divd fee")
    {
        this->_addbonus(quantity);
    }
}
void onesgame::_stake(name account, asset quantity)
{
    auto it = _defi_account.find(account.value);
    if (it != _defi_account.end())
    {
        _defi_account.modify(it, _self, [&](auto &t) {
            t.stake_quantity += quantity;
        });
    }
    else
    {
        _defi_account.emplace(get_self(), [&](auto &t) {
            t.account = account;
            t.stake_quantity = quantity;
            t.reward_quantity = asset(0, EOS_TOKEN_SYMBOL);
            t.unclaim_quantity = asset(0, EOS_TOKEN_SYMBOL);
            t.retire_quantity = asset(0, ONES_TOKEN_SYMBOL);
        });
    }
    uint64_t stake_id = _get_stake_id();
    _defi_stake.emplace(get_self(), [&](auto &t) {
        t.stake_id = stake_id;
        t.account = account;
        t.quantity = quantity;
        t.timestamp = now();
    });

    this->_add(quantity);
}

void onesgame::_addbonus(asset quantity)
{
    if (quantity.symbol != EOS_TOKEN_SYMBOL)
    {
        return;
    }
    st_defi_config defi_config = _defi_config.get();
    defi_config.bonus_quantity += quantity;
    _defi_config.set(defi_config, _self);
}

void onesgame::claim(name account)
{
    require_auth(account);

    auto it = _defi_account.find(account.value);
    eosio_assert((it != _defi_account.end()), "invalid account");
    eosio_assert((it->unclaim_quantity.amount > 0), "unclaim is zero");

    uint64_t amount = it->unclaim_quantity.amount;
    _defi_account.modify(it, _self, [&](auto &t) {
        t.unclaim_quantity = asset(0, EOS_TOKEN_SYMBOL);
    });
    this->_transfer_to(account, amount, EOS_TOKEN_SYMBOL, "bonus");
}

void onesgame::unstake(name account, uint64_t stake_id)
{
    require_auth(account);

    auto it = _defi_stake.find(stake_id);
    eosio_assert((it != _defi_stake.end()), "stake_id account");
    eosio_assert((account == it->account), "invalid account");

    auto accountit = _defi_account.find(account.value);

    auto account_index = _defi_account.get_index<"bystake"_n>();
    auto iter = account_index.rbegin();

    uint64_t num = 0;
    bool isTop100 = false;
    while (iter != account_index.rend())
    {
        if (iter->account == account)
        {
            isTop100 = true;
        }
        iter++;
        num++;
        if (num >= 100)
        {
            break;
        }
    }

    uint64_t retire_amount = 0;
    if (isTop100)
    {
        uint64_t day = ((now() - it->timestamp) / 86400);
        if (day < 30)
        {
            retire_amount = 0.2 * it->quantity.amount;
        }
        else if (day >= 30 && day < 60)
        {
            retire_amount = 0.3 * it->quantity.amount;
        }
        else if (day >= 60 && day < 90)
        {
            retire_amount = 0.4 * it->quantity.amount;
        }
        else
        {
            retire_amount = 0.5 * it->quantity.amount;
        }
    }

    uint64_t remain_amount = it->quantity.amount - retire_amount;
    _defi_account.modify(accountit, _self, [&](auto &t) {
        t.stake_quantity -= it->quantity;
        t.retire_quantity += asset(retire_amount, it->quantity.symbol);
    });

    this->_sub(it->quantity, asset(retire_amount, it->quantity.symbol));
    _defi_stake.erase(it);
    this->_transfer_to(account, remain_amount, ONES_TOKEN_SYMBOL, "refund");
}

void onesgame::bonus()
{
    require_auth(get_self());

    auto account_index = _defi_account.get_index<"bystake"_n>();
    auto it = account_index.rbegin();
    uint64_t num = 0;
    uint64_t total = 0;

    vector<name> accounts;
    while (it != account_index.rend())
    {
        accounts.push_back(it->account);
        total += it->stake_quantity.amount;
        num++;
        if (num > 100)
        {
            break;
        }
        it++;
    }

    st_defi_config defi_config = _defi_config.get();
    asset reward_quantity = asset(defi_config.bonus_quantity.amount * 0.01, defi_config.bonus_quantity.symbol);
    defi_config.bonus_quantity -= reward_quantity;
    defi_config.reward_quantity += reward_quantity;
    defi_config.bonus_id += 1;
    _defi_config.set(defi_config, _self);

    num = 0;
    for (uint64_t i = 0; i < accounts.size(); i++)
    {
        name account = accounts[i];
        auto it = _defi_account.find(account.value);

        float factor = (1.0 * it->stake_quantity.amount) / total;
        uint64_t reward_amount = reward_quantity.amount * factor;

        _defi_account.modify(it, _self, [&](auto &t) {
            t.reward_quantity += asset(reward_amount, it->unclaim_quantity.symbol);
            t.unclaim_quantity += asset(reward_amount, it->unclaim_quantity.symbol);
        });
    }

    tb_defi_bonus _defi_bonus(get_self(), _self.value);

    _defi_bonus.emplace(get_self(), [&](auto &t) {
        t.bonus_id = defi_config.bonus_id;
        t.bonus_quantity = reward_quantity;
        t.stake_quantity = asset(total, ONES_TOKEN_SYMBOL);
        t.timestamp = now();
    });
}

void onesgame::init()
{

    require_auth(get_self());

    st_defi_config defi_config = _defi_config.get_or_create(
        _self,
        st_defi_config{
            .stake_quantity = asset(0, ONES_TOKEN_SYMBOL),
            .retire_quantity = asset(0, ONES_TOKEN_SYMBOL),
            .reward_quantity = asset(0, EOS_TOKEN_SYMBOL),
            .bonus_quantity = asset(18568261, EOS_TOKEN_SYMBOL),
            .state = 0,
            .stake_id = 0,
            .bonus_id = 0});
    return;
}

uint64_t onesgame::_get_stake_id()
{
    st_defi_config defi_config = _defi_config.get();
    defi_config.stake_id += 1;
    _defi_config.set(defi_config, _self);
    return defi_config.stake_id;
}

void onesgame::_sub(asset stake_quantity, asset retire_quantity)
{
    st_defi_config defi_config = _defi_config.get();
    defi_config.stake_quantity -= stake_quantity;
    defi_config.retire_quantity += retire_quantity;
    _defi_config.set(defi_config, _self);
}

void onesgame::_add(asset stake_quantity)
{
    st_defi_config defi_config = _defi_config.get();
    defi_config.stake_quantity += stake_quantity;
    _defi_config.set(defi_config, _self);
}

void onesgame::_transfer_to(name to, uint64_t amount, symbol coin_code, string memo)
{
    if (amount == 0)
    {
        return;
    }

    if (coin_code == EOS_TOKEN_SYMBOL)
    {
        eosio::action(permission_level{get_self(), "active"_n},
                      eosio::name(EOS_TOKEN_ACCOUNT),
                      "transfer"_n,
                      make_tuple(get_self(), to, asset(amount, EOS_TOKEN_SYMBOL), memo))
            .send();
    }
    else if (coin_code == ONES_TOKEN_SYMBOL)
    {
        eosio::action(permission_level{get_self(), "active"_n},
                      eosio::name(ONES_TOKEN_ACCOUNT),
                      "transfer"_n,
                      make_tuple(get_self(), to, asset(amount, ONES_TOKEN_SYMBOL), memo))
            .send();
    }
}

extern "C"
{
    void apply(uint64_t receiver, uint64_t code, uint64_t action)
    {
        if (action == eosio::name("onerror").value)
        {
            eosio_assert(code == eosio::name("eosio").value, "onerror action’s are only valid from the eosio");
        }

        if (code == receiver)
        {
            switch (action)
            {
                EOSIO_DISPATCH_HELPER(onesgame, (claim)(unstake)(bonus)(init))
            }
            return;
        }

        if ((code == eosio::name(EOS_TOKEN_ACCOUNT).value || code == eosio::name(ONES_TOKEN_ACCOUNT).value) && action == eosio::name("transfer").value)
        {
            execute_action(eosio::name(receiver), eosio::name(receiver), &onesgame::transfer);
            return;
        }

        eosio_exit(0);
    }
}