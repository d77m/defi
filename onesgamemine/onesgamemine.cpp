#include "onesgamemine.hpp"
#include <eosiolib/transaction.hpp>

#define EOS_TOKEN_SYMBOL symbol("EOS", 4)
#define ONES_TOKEN_SYMBOL symbol("ONES", 4)
#define ONES_TOKEN_ACCOUNT "eosonestoken"
#define ONES_DEFI_ACCOUNT "onesgamedefi"
#define ONES_PLAY_ACCOUNT "onesgameplay"

uint64_t default_swap_time = 1598961600;

#define ACCOUNT_CHECK(account) \
    eosio_assert(is_account(account), "invalid account " #account);

void split(const std::string &str, char delimiter, std::vector<std::string> &params)
{
    std::size_t cur, prev = 0;
    cur = str.find(delimiter);
    while (cur != std::string::npos)
    {
        params.push_back(str.substr(prev, cur - prev));
        prev = cur + 1;
        cur = str.find(delimiter, prev);
    }
    params.push_back(str.substr(prev));
}

void onesgame::issue(name to, asset quantity, string memo)
{
    require_auth(eosio::name(ONES_PLAY_ACCOUNT));

    std::vector<std::string> params;
    split(memo, ',', params);

    std::string action = params.at(0);

    if (action == "issuemarket")
    {
        eosio_assert(quantity.amount == 360000, "invalid quantity");

        eosio_assert(params.size() == 3, "invalid memo");
        uint64_t round = atoll(params.at(1).c_str());
        uint64_t total = atoll(params.at(2).c_str());

        auto it = _defi_market.find(round);
        eosio_assert(it == _defi_market.end(), "invalid round");

        uint64_t cur_time = now();
        st_defi_config defi_config = _defi_config.get();

        uint64_t amount = defi_config.market_quantity; // (defi_config.swap_suply - defi_config.last_swap_suply) / 2;

        // uint64_t amount = 100 * 10000;
        _defi_market.emplace(get_self(), [&](auto &t) {
            t.round = round;
            t.quantity = asset(amount, quantity.symbol);
            t.total = total;
            t.executed = 0;
            t.timestamp = cur_time;
        });

        defi_config.market_time = (cur_time / 3600) * 3600;
        defi_config.market_issue += quantity.amount;
        defi_config.market_suply += amount;
        defi_config.last_swap_suply = defi_config.swap_suply;
        defi_config.market_quantity = 0;

        auto maxit = _defi_market.rbegin();
        if (maxit != _defi_market.rend())
        {
            uint64_t max_round = maxit->round;

            auto itr = _defi_market.begin();
            while (itr != _defi_market.end() && itr->round < (max_round - 100))
            {
                _defi_market.erase(itr);
                itr = _defi_market.begin();
            }
        }

        _defi_config.set(defi_config, _self);
    }
    else if (action == "issueswap")
    {
        eosio_assert(quantity.amount == 720000, "invalid quantity");

        st_defi_config defi_config = _defi_config.get();

        defi_config.swap_issue += quantity.amount;
        _defi_config.set(defi_config, _self);
    }
}

void onesgame::mineswap(name account, asset quantity)
{
    require_auth(eosio::name(ONES_DEFI_ACCOUNT));

    eosio_assert(quantity.symbol == EOS_TOKEN_SYMBOL, "invalid symbol");

    if (quantity.amount < 10000)
    {
        return;
    }
    uint64_t counter = (quantity.amount / 10000);

    st_defi_config defi_config = _defi_config.get();

    uint64_t cur_swap_quantity = defi_config.swap_quantity;
    uint64_t cur_time = now();
    uint64_t increased_swap_quantity = (cur_time - defi_config.swap_time) * 0.02 * 10000;

    uint64_t total_swap_quantity = increased_swap_quantity + cur_swap_quantity;

    uint64_t airdrop_swap_quantity = 0;

    uint64_t t = total_swap_quantity;
    for (uint64_t i = 0; i < counter; i++)
    {
        airdrop_swap_quantity += t * 0.0001;
        t = total_swap_quantity - airdrop_swap_quantity;
    }
    defi_config.swap_counter += counter;
    defi_config.swap_quantity = total_swap_quantity - airdrop_swap_quantity;
    defi_config.swap_suply += airdrop_swap_quantity;
    uint64_t market_quantity = (airdrop_swap_quantity / 2);
    defi_config.market_quantity += market_quantity;
    defi_config.swap_time = cur_time;

    _defi_config.set(defi_config, _self);

    auto it = _defi_account.find(account.value);
    if (it != _defi_account.end())
    {
        _defi_account.modify(it, _self, [&](auto &t) {
            t.swap_quantity += asset(airdrop_swap_quantity, ONES_TOKEN_SYMBOL);
            t.mine_quantity += asset(quantity.amount, EOS_TOKEN_SYMBOL);
        });
    }
    else
    {
        _defi_account.emplace(get_self(), [&](auto &t) {
            t.account = account;
            t.market_quantity = asset(0, ONES_TOKEN_SYMBOL);
            t.quantity = asset(0, ONES_TOKEN_SYMBOL);
            t.swap_quantity = asset(airdrop_swap_quantity, ONES_TOKEN_SYMBOL);
            t.mine_quantity = asset(quantity.amount, EOS_TOKEN_SYMBOL);
            t.timestamp = now();
        });
    }

    this->_transfer_to(account, airdrop_swap_quantity, ONES_TOKEN_SYMBOL, "swap mine");
}

void onesgame::minemarket(name account, uint64_t round, float_t factor)
{
    require_auth(get_self());

    auto defi_market = _defi_market.find(round);
    eosio_assert(defi_market != _defi_market.end(), "invalid round");
    _defi_market.modify(defi_market, _self, [&](auto &t) {
        t.executed += 1;
    });

    auto it = _defi_account.find(account.value);

    uint64_t amount = defi_market->quantity.amount * factor;
    if (it != _defi_account.end())
    {
        eosio_assert(round >= it->market_round, "invalid round");

        _defi_account.modify(it, _self, [&](auto &t) {
            t.quantity += asset(amount, ONES_TOKEN_SYMBOL);
            t.market_round = round;
        });
    }
    else
    {
        eosio_assert(it->market_round != round, "market has been mined");

        _defi_account.emplace(get_self(), [&](auto &t) {
            t.account = account;
            t.quantity = asset(amount, ONES_TOKEN_SYMBOL);
            t.market_quantity = asset(0, ONES_TOKEN_SYMBOL);
            t.swap_quantity = asset(0, ONES_TOKEN_SYMBOL);
            t.mine_quantity = asset(0, EOS_TOKEN_SYMBOL);
            t.market_round = round;
            t.timestamp = now();
        });
    }
}

void onesgame::claim(name account)
{
    require_auth(account);

    auto it = _defi_account.find(account.value);
    eosio_assert((it != _defi_account.end()), "invalid account");

    asset quantity = it->quantity;
    _defi_account.modify(it, _self, [&](auto &t) {
        t.quantity -= quantity;
        t.market_quantity += quantity;
    });

    this->_transfer_to(account, quantity.amount, ONES_TOKEN_SYMBOL, "market mine");
}

void onesgame::init()
{
    require_auth(get_self());
    return;
}

void onesgame::_transfer_to(name to, uint64_t amount, symbol coin_code, string memo)
{
    if (amount == 0)
    {
        return;
    }

    if (coin_code == ONES_TOKEN_SYMBOL)
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
                EOSIO_DISPATCH_HELPER(onesgame, (mineswap)(minemarket)(claim)(init))
            }
            return;
        }

        if ((code == eosio::name(ONES_TOKEN_ACCOUNT).value) && action == eosio::name("issue").value)
        {
            execute_action(eosio::name(receiver), eosio::name(receiver), &onesgame::issue);
            return;
        }

        eosio_exit(0);
    }
}