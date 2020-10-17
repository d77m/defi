#include "onesgamemine.hpp"

#include <eosiolib/transaction.hpp>

#define EOS_TOKEN_SYMBOL symbol("EOS", 4)
#define ONES_TOKEN_SYMBOL symbol("ONES", 4)
#define ONES_TOKEN_ACCOUNT "eosonestoken"
#define ONES_DEFI_ACCOUNT "onesgamedefi"
#define ONES_PLAY_ACCOUNT "onesgameplay"

#define BOX_TOKEN_ACCOUNT "token.defi"
#define BOX_TOKEN_SYMBOL symbol("BOX", 6)

#define DFS_TOKEN_ACCOUNT "minedfstoken"
#define DFS_TOKEN_SYMBOL symbol("DFS", 4)

const symbol token_symbols[] = {ONES_TOKEN_SYMBOL, BOX_TOKEN_SYMBOL,
                                DFS_TOKEN_SYMBOL};

uint64_t default_swap_time = 1598961600;

const uint64_t TOKENNUM = 3;

#define ACCOUNT_CHECK(account) \
    eosio_assert(is_account(account), "invalid account " #account);

void split(const std::string &str, char delimiter,
           std::vector<std::string> &params)
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

uint64_t get_token_offset(symbol type)
{
    for (int i = 0; i < TOKENNUM; i++)
    {
        if (token_symbols[i] == type)
        {
            return i;
        }
    }
    eosio_assert(false, "invalid token type");
    return 99999999999999;
}

void init_amounts(vector<uint64_t> &array)
{
    for (int i = array.size(); i < TOKENNUM; i++)
    {
        array.push_back(0);
    }
}

void add_amounts(vector<uint64_t> &array1, vector<uint64_t> &array2)
{
    init_amounts(array1);
    for (int i = 0; i < array2.size(); i++)
    {
        array1[i] += array2[i];
    }
}

void reset_amounts(vector<uint64_t> &array)
{
    for (int i = 0; i < array.size(); i++)
    {
        array[i] = 0;
    }
    init_amounts(array);
}
void onesgame::transfer(name from, name to, asset quantity, string memo)
{
    ACCOUNT_CHECK(from);
    ACCOUNT_CHECK(to);

    require_auth(from);

    if (from == get_self() || to != get_self())
    {
        return;
    }

    if (quantity.symbol == BOX_TOKEN_SYMBOL)
    {
        st_defi_config defi_config = _defi_config.get();
        defi_config.market_quantity[1] += quantity.amount;
        _defi_config.set(defi_config, _self);
    }
}

void onesgame::init()
{
    require_auth(eosio::name(ONES_PLAY_ACCOUNT));

    return;
}

void onesgame::upgrade()
{
    require_auth(eosio::name(ONES_PLAY_ACCOUNT));

    return;
}

void onesgame::oauth(name account, uint8_t status)
{
    require_auth(account);

    auto it = _defi_account.find(account.value);
    if (it != _defi_account.end())
    {
        _defi_account.modify(it, _self, [&](auto &t) {
            t.oauth = status > 0 ? 1 : 0;
        });
    }

    return;
}

void onesgame::_transfer_to(name to, uint64_t amount, symbol coin_code,
                            string memo)
{
    if (amount == 0)
    {
        return;
    }

    if (coin_code == ONES_TOKEN_SYMBOL)
    {
        eosio::action(
            permission_level{get_self(), "active"_n},
            eosio::name(ONES_TOKEN_ACCOUNT), "transfer"_n,
            make_tuple(get_self(), to, asset(amount, ONES_TOKEN_SYMBOL), memo))
            .send();
    }
    else if (coin_code == BOX_TOKEN_SYMBOL)
    {
        eosio::action(
            permission_level{get_self(), "active"_n},
            eosio::name(BOX_TOKEN_ACCOUNT), "transfer"_n,
            make_tuple(get_self(), to, asset(amount, BOX_TOKEN_SYMBOL), memo))
            .send();
    }
    else if (coin_code == DFS_TOKEN_SYMBOL)
    {
        eosio::action(
            permission_level{get_self(), "active"_n},
            eosio::name(DFS_TOKEN_ACCOUNT), "transfer"_n,
            make_tuple(get_self(), to, asset(amount, DFS_TOKEN_SYMBOL), memo))
            .send();
    }
}

void onesgame::issue(name to, asset quantity, string memo)
{
    require_auth(eosio::name(ONES_PLAY_ACCOUNT));
    // eosio_assert(false, "onesgamemine contact is upgrading");

    std::vector<std::string> params;
    split(memo, ',', params);

    std::string action = params.at(0);

    if (action == "issuemarket")
    {
        eosio_assert(quantity.amount == 360000, "invalid quantity");

        eosio_assert(params.size() == 3, "invalid memo");
        uint64_t round = atoll(params.at(1).c_str());
        uint64_t total = atoll(params.at(2).c_str());

        uint64_t cur_time = now();
        st_defi_config defi_config = _defi_config.get();

        defi_config.market_time = (cur_time / 3600) * 3600;
        defi_config.market_issue += quantity.amount;

        defi_config.last_swap_suply = defi_config.swap_suply;

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
    // eosio_assert(false, "onesgamemine contact is upgrading");

    eosio_assert(quantity.symbol == EOS_TOKEN_SYMBOL, "invalid symbol");

    if (quantity.amount < 10000)
    {
        return;
    }
    uint64_t counter = (quantity.amount / 10000);

    st_defi_config defi_config = _defi_config.get();

    uint64_t cur_swap_quantity = defi_config.swap_quantity;
    uint64_t cur_time = now();
    uint64_t increased_swap_quantity =
        (cur_time - defi_config.swap_time) * 0.02 * 10000;

    uint64_t total_swap_quantity = increased_swap_quantity + cur_swap_quantity;

    uint64_t airdrop_swap_quantity = 0;

    uint64_t t = total_swap_quantity;
    for (uint64_t i = 0; i < counter; i++)
    {
        airdrop_swap_quantity += t * 0.0001;
        t = total_swap_quantity - airdrop_swap_quantity;
    }
    defi_config.swap_counter += counter;
    defi_config.swap_quantity =
        total_swap_quantity - airdrop_swap_quantity;
    defi_config.swap_suply += airdrop_swap_quantity;
    uint64_t market_quantity = (airdrop_swap_quantity / 2);
    defi_config.market_quantity[0] += market_quantity;
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
            init_amounts(t.market_quantity);
            init_amounts(t.quantity);
            t.swap_quantity = asset(airdrop_swap_quantity, ONES_TOKEN_SYMBOL);
            t.mine_quantity = asset(quantity.amount, EOS_TOKEN_SYMBOL);
            t.timestamp = now();
        });
    }

    this->_transfer_to(account, airdrop_swap_quantity, ONES_TOKEN_SYMBOL,
                       "swap mine");
}

void onesgame::minemarkets()
{
    require_auth(name(ONES_PLAY_ACCOUNT));
    st_defi_config defi_config = _defi_config.get();
    eosio_assert(defi_config.market_quantity[0] > 10000, "market quantity is zero");

    uint64_t round_id = 1;
    auto it = _defi_market.rbegin();
    if (it != _defi_market.rend())
    {
        round_id = it->round + 1;
    }
    auto itr = _defi_market.begin();
    while (itr != _defi_market.end() && itr->round < (round_id - 100))
    {
        _defi_market.erase(itr);
        itr = _defi_market.begin();
    }
    uint64_t total_amount = 0;
    uint64_t total_user = 0;

    _syncmarket(round_id, total_amount, total_user);

    uint64_t cur_time = now();

    _defi_market.emplace(get_self(), [&](auto &t) {
        t.round = round_id;
        t.amount = total_amount;
        t.quantity = defi_config.market_quantity;
        t.total = total_user;
        t.executed = 0;
        t.timestamp = cur_time;
    });

    add_amounts(defi_config.market_suply, defi_config.market_quantity);
    reset_amounts(defi_config.market_quantity);

    _defi_config.set(defi_config, _self);

    eosio::action(eosio::permission_level{get_self(), "active"_n},
                  get_self(), "minemarket"_n,
                  make_tuple(round_id))
        .send();
}

void onesgame::_syncmarket(uint64_t round_id, uint64_t &total_amount, uint64_t &total_user)
{
    tb_defi_liquidity _defi_liquidity(name(ONES_DEFI_ACCOUNT),
                                      name(ONES_DEFI_ACCOUNT).value);
    auto lit = _defi_liquidity.begin();
    map<uint64_t, float> liquidities;

    while (lit != _defi_liquidity.end())
    {
        if (lit->liquidity_weight > 0)
        {
            liquidities[lit->liquidity_id] = lit->liquidity_weight;
        }
        lit++;
    }

    tb_defi_pool _defi_pool(name(ONES_DEFI_ACCOUNT),
                            name(ONES_DEFI_ACCOUNT).value);
    auto pit = _defi_pool.begin();

    tb_defi_round _defi_round(get_self(), get_self().value);

    std::map<uint64_t, float>::iterator mit;
    uint64_t id = 1;
    auto idx = _defi_round.rbegin();
    if (idx != _defi_round.rend())
    {
        id = idx->id + 1;
    }

    while (pit != _defi_pool.end())
    {
        mit = liquidities.find(pit->liquidity_id);
        if (mit != liquidities.end())
        {
            auto ait = _defi_round.find(pit->account.value);
            uint64_t amount = pit->quantity1.amount * mit->second;
            total_amount += amount;
            _defi_round.emplace(get_self(), [&](auto &t) {
                t.id = id++;
                t.account = pit->account;
                t.round_id = round_id;
                t.amount = amount;
            });
            total_user++;
        }
        pit++;
    }
}
void onesgame::minemarket(uint64_t round_id)
{
    require_auth(get_self());
    // eosio_assert(false, "onesgamemine contact is upgrading");

    auto market = _defi_market.find(round_id);
    eosio_assert(market != _defi_market.end(), "invalid round");
    eosio_assert(market->total > market->executed, "has been executed");

    tb_defi_round _defi_round(get_self(), get_self().value);
    auto round_index = _defi_round.get_index<"byroundkey"_n>();

    auto it = round_index.find(round_id);
    while (it != round_index.end())
    {
        float factor = (1.0 * it->amount / market->amount);
        this->_minemarket(round_id, it->account, market->quantity, factor);
        it = round_index.erase(it);
    }

    _defi_market.modify(market, _self, [&](auto &t) { t.executed = market->total; });
}

void onesgame::_minemarket(uint64_t round_id, name account, const vector<uint64_t> &quantity, float factor)
{
    auto it = _defi_account.find(account.value);

    if (it != _defi_account.end())
    {
        eosio_assert(round_id >= it->market_round, "invalid round");

        _defi_account.modify(it, _self, [&](auto &t) {
            init_amounts(t.quantity);
            for (uint64_t i = 0; i < quantity.size(); i++)
            {
                uint64_t amount = quantity[i] * factor;
                t.quantity[i] += amount;
            }
            t.market_round = round_id;
        });
    }
    else
    {
        eosio_assert(it->market_round != round_id, "market has been mined");

        _defi_account.emplace(get_self(), [&](auto &t) {
            t.account = account;
            init_amounts(t.quantity);
            init_amounts(t.market_quantity);
            for (uint64_t i = 0; i < quantity.size(); i++)
            {
                uint64_t amount = quantity[i] * factor;
                t.quantity[i] += amount;
            }
            t.swap_quantity = asset(0, ONES_TOKEN_SYMBOL);
            t.mine_quantity = asset(0, EOS_TOKEN_SYMBOL);
            t.market_round = round_id;
            t.timestamp = now();
        });
    }
}

void onesgame::claim(name account)
{
    require_auth(account);
    // eosio_assert(false, "onesgamemine contact is upgrading");

    auto it = _defi_account.find(account.value);
    eosio_assert((it != _defi_account.end()), "invalid account");

    vector<uint64_t> quantity = it->quantity;
    _defi_account.modify(it, _self, [&](auto &t) {
        reset_amounts(t.quantity);
        add_amounts(t.market_quantity, quantity);
    });

    for (uint64_t i = 0; i < quantity.size(); i++)
    {
        this->_transfer_to(account, quantity[i], token_symbols[i],
                           "market mine");
    }
}

extern "C"
{
    void apply(uint64_t receiver, uint64_t code, uint64_t action)
    {
        if (action == eosio::name("onerror").value)
        {
            eosio_assert(code == eosio::name("eosio").value,
                         "onerror action’s are only valid from the eosio");
        }

        if (code == receiver)
        {
            switch (action)
            {
                EOSIO_DISPATCH_HELPER(onesgame, (mineswap)(minemarket)(minemarkets)(
                                                    claim)(init)(upgrade)(oauth))
            }
            return;
        }

        if ((code == eosio::name(ONES_TOKEN_ACCOUNT).value) &&
            action == eosio::name("issue").value)
        {
            execute_action(eosio::name(receiver), eosio::name(receiver),
                           &onesgame::issue);
            return;
        }

        if (code == eosio::name(BOX_TOKEN_ACCOUNT).value &&
            action == eosio::name("transfer").value)
        {
            execute_action(eosio::name(receiver), eosio::name(receiver),
                           &onesgame::transfer);
            return;
        }
        eosio_exit(0);
    }
}
