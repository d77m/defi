#include "onesgamedefi.hpp"
#include <eosiolib/transaction.hpp>
#include <cmath>

#define EOS_TOKEN_SYMBOL symbol("EOS", 4)
#define EOS_TOKEN_ACCOUNT "eosio.token"

#define ONES_MINE_ACCOUNT "onesgamemine"
#define ONES_PLAY_ACCOUNT "onesgameplay"

#define DEFI_TYPE_LIQUIDITY 1
#define DEFI_TYPE_SWAP 2
#define DEFI_TYPE_PAIR 3

#define ACCOUNT_CHECK(account) \
    eosio_assert(is_account(account), "invalid account " #account);

#define ONES_FUND_ACCOUNT "onesgamefund"
#define ONES_DIVD_ACCOUNT "onesgamedivd"

float_t ONES_SWAP_FEE = 0.001;
float_t ONES_FUND_FEE = 0.001;
float_t ONES_DIVD_FEE = 0.001;

struct transfer_args
{
    name from;
    name to;
    asset quantity;
    string memo;
};

void onesgame::newliquidity(name account, token_t token1, token_t token2)
{
    require_auth(account);

    if (token1 == token2)
    {
        eosio_assert(false, "same token");
    }

    stats statstable1(token1.address, token1.symbol.code().raw());
    const auto &st1 = statstable1.find(token1.symbol.code().raw());
    std::string sterr1 = "invalid token address:" + token1.address.to_string();
    eosio_assert(st1 != statstable1.end(), sterr1.c_str());

    stats statstable2(token2.address, token2.symbol.code().raw());
    const auto &st2 = statstable2.find(token2.symbol.code().raw());
    std::string sterr2 = "invalid token address:" + token2.address.to_string();
    eosio_assert(st2 != statstable2.end(), sterr2.c_str());

    uint64_t liquidity_id = get_liquidity_id();

    this->_newpair(liquidity_id, token1, token2);

    bool iseos = false;
    if (token2.address == name(EOS_TOKEN_ACCOUNT) && token2.symbol == EOS_TOKEN_SYMBOL)
    {
        iseos = true;
    }
    _defi_liquidity.emplace(get_self(), [&](auto &t) {
        t.liquidity_id = liquidity_id;
        t.token1 = iseos ? token2 : token1;
        t.token2 = iseos ? token1 : token2;
        t.liquidity_token = 0;
        t.swap_weight = 0;
        t.liquidity_weight = 0;
        t.timestamp = now();
    });
}

void onesgame::_newpair(uint64_t liquidity_id, const token_t &token1, const token_t &token2)
{
    bool reverse = token1.address.value < token2.address.value;

    name _contract1 = reverse ? token1.address : token2.address;
    name _contract2 = reverse ? token2.address : token1.address;
    symbol _sym1 = reverse ? token1.symbol : token2.symbol;
    symbol _sym2 = reverse ? token2.symbol : token1.symbol;

    std::string uni_key = _contract1.to_string();
    uni_key += "-";
    uni_key += _sym1.code().to_string();
    uni_key += ":";
    uni_key += _contract2.to_string();
    uni_key += "-";
    uni_key += _sym2.code().to_string();

    const char *uni_key_cstr = uni_key.c_str();
    checksum256 digest = sha256(uni_key_cstr, strlen(uni_key_cstr));

    tb_defi_pair _defi_pair(_self, _self.value);

    auto itr = _defi_pair.find(utils::uint64_hash(digest));
    check(itr == _defi_pair.end(), "Liquidity already exists");

    _defi_pair.emplace(get_self(), [&](auto &t) {
        t.digest = digest;
        t.liquidity_id = liquidity_id;
    });
}

void onesgame::transfer(name from, name to, asset quantity, string memo)
{
    require_auth(from);

    ACCOUNT_CHECK(from);
    ACCOUNT_CHECK(to);

    if (from == get_self() || to != get_self())
    {
        return;
    }

    eosio_assert(quantity.is_valid(), "invalid quantity");
    eosio_assert(quantity.amount > 0, "must transfer positive quantity");

    std::vector<std::string> params;
    utils::split(memo, ',', params);

    if (params.size() == 0)
    {
        return;
    }
    std::string action = params.at(0);

    if (action == "swap")
    {
        this->swap(from, quantity, params);
    }
    return;
}

void onesgame::_swaplog(name account, uint64_t third_id, uint64_t liquidity_id, token_t in_token, token_t out_token, asset in_asset, asset out_asset, asset fee, float_t price)
{
    uint64_t swap_id = this->get_swap_id();
    if (swap_id > 200)
    {
        auto it = _swap_log.begin();
        while (it != _swap_log.end() && it->swap_id < (swap_id - 200))
        {
            it = _swap_log.erase(it);
        }
    }
    auto tx_size = transaction_size();
    char tx[tx_size];
    auto read_size = read_transaction(tx, tx_size);
    eosio_assert(tx_size == read_size, "read_transaction failed");
    auto trx = unpack<transaction>(tx, read_size);

    checksum256 trx_id = sha256(tx, tx_size);

    _swap_log.emplace(get_self(), [&](auto &t) {
        t.swap_id = swap_id;
        t.third_id = third_id;
        t.account = account;
        t.liquidity_id = liquidity_id;
        t.in_token = in_token;
        t.out_token = out_token;
        t.in_asset = in_asset;
        t.out_asset = out_asset;
        t.price = price;
        t.trx_id = trx_id;
        t.timestamp = now();
    });
}

void onesgame::_liquiditylog(name account, uint64_t liquidity_id, string type, token_t in_token, token_t out_token, asset in_asset, asset out_asset, uint64_t liquidity_token)
{
    uint64_t log_id = 0;
    auto it = _liquidity_log.rbegin();
    if (it != _liquidity_log.rend())
    {
        log_id = it->log_id;
    }
    log_id++;
    auto tx_size = transaction_size();
    char tx[tx_size];
    auto read_size = read_transaction(tx, tx_size);
    eosio_assert(tx_size == read_size, "read_transaction failed");
    auto trx = unpack<transaction>(tx, read_size);

    checksum256 trx_id = sha256(tx, tx_size);

    _liquidity_log.emplace(get_self(), [&](auto &t) {
        t.log_id = log_id;
        t.account = account;
        t.liquidity_id = liquidity_id;
        t.in_token = in_token;
        t.out_token = out_token;
        t.in_asset = in_asset;
        t.out_asset = out_asset;
        t.liquidity_token = liquidity_token;
        t.type = type;
        t.trx_id = trx_id;
        t.timestamp = now();
    });
}

void onesgame::swap(name account, asset quantity, std::vector<std::string> &params)
{
    //2预留
    eosio_assert(params.size() == 4, "invalid memo");

    std::vector<std::string> liquidity_ids;
    utils::split(params.at(3), '-', liquidity_ids);
    uint64_t liquidity_id = 0;
    uint64_t third_id = atoll(params.at(1).c_str());

    swap_t swapdata;
    swapdata.quantity = quantity;
    swapdata.code = this->code;

    for (uint64_t i = 0; i < liquidity_ids.size(); i++)
    {
        swapdata.original_quantity = asset(quantity.amount, quantity.symbol);

        asset fund_fee = asset(swapdata.quantity.amount * ONES_FUND_FEE, swapdata.quantity.symbol);
        this->_transfer_to(name(ONES_FUND_ACCOUNT), swapdata.code, fund_fee, "swap fund fee");

        asset divd_fee = asset(swapdata.quantity.amount * ONES_DIVD_FEE, swapdata.quantity.symbol);
        this->_transfer_to(name(ONES_DIVD_ACCOUNT), swapdata.code, divd_fee, "swap divd fee");

        swapdata.quantity.amount -= swapdata.quantity.amount * ONES_FUND_FEE;
        swapdata.quantity.amount -= swapdata.quantity.amount * ONES_DIVD_FEE;
        // name,quantity
        liquidity_id = atoll(liquidity_ids.at(i).c_str());
        swapdata = this->_swap(account, swapdata, liquidity_id, third_id);
    }

    this->_transfer_to(account, swapdata.code, swapdata.quantity, "swap");
}

onesgame::swap_t onesgame::_swap(name account, swap_t &in, uint64_t liquidity_id, uint64_t third_id)
{
    auto it = _defi_liquidity.find(liquidity_id);
    eosio_assert(it != _defi_liquidity.end(), "Liquidity does not exist");

    onesgame::swap_t out;

    token_t token1;
    token_t token2;

    eosio_assert(in.code == it->token1.address.value || in.code == it->token2.address.value, "token address error");

    if (in.code == it->token1.address.value && in.quantity.symbol == it->token1.symbol)
    {
        token1 = it->token1;
        token2 = it->token2;
        float_t alpha = (1.0 * in.quantity.amount) / it->quantity1.amount;
        float_t r = 1 - ONES_SWAP_FEE;
        uint64_t amount = (1.0 * (alpha * r) / (1 + alpha * r)) * it->quantity2.amount;
        // eosio_assert(amount != 0, "amount is zero");
        out.quantity = asset(amount, it->quantity2.symbol);
        out.code = it->token2.address.value;

        asset quantity1 = it->quantity1 + in.quantity;
        asset quantity2 = it->quantity2 - out.quantity;

        _defi_liquidity.modify(it, _self, [&](auto &t) {
            t.quantity1 += in.quantity;
            t.quantity2 -= out.quantity;
            t.price1 = 1.0 * quantity2.amount / quantity1.amount;
            t.price2 = 1.0 * quantity1.amount / quantity2.amount;
        });
    }
    else if (in.code == it->token2.address.value && in.quantity.symbol == it->token2.symbol)
    {
        token1 = it->token2;
        token2 = it->token1;

        float_t alpha = (1.0 * in.quantity.amount) / it->quantity2.amount;
        float_t r = 1 - ONES_SWAP_FEE;
        uint64_t amount = (((alpha * r) / (1 + alpha * r)) * it->quantity1.amount);
        out.quantity = asset(amount, it->quantity1.symbol);
        out.code = it->token1.address.value;

        asset quantity1 = it->quantity1 - out.quantity;
        asset quantity2 = it->quantity2 + in.quantity;

        _defi_liquidity.modify(it, _self, [&](auto &t) {
            t.quantity1 -= out.quantity;
            t.quantity2 += in.quantity;
            t.price1 = 1.0 * quantity2.amount / quantity1.amount;
            t.price2 = 1.0 * quantity1.amount / quantity2.amount;
        });
    }
    else
    {
        eosio_assert(false, "token address error");
    }

    float_t price = (1.0 * out.quantity.amount) / in.original_quantity.amount;
    asset fee(in.original_quantity.amount * (ONES_DIVD_FEE + ONES_FUND_FEE + ONES_SWAP_FEE), in.quantity.symbol);

    this->_swaplog(account, third_id, liquidity_id, token1, token2, in.original_quantity, out.quantity, fee, price);

    this->mine(account, in.original_quantity, liquidity_id);
    return out;
}

void onesgame::addliquidity(name account, uint64_t liquidity_id)
{
    require_auth(account);

    auto defi_liquidity = _defi_liquidity.find(liquidity_id);

    eosio_assert(defi_liquidity != _defi_liquidity.end(), "Liquidity does not exist");

    auto tx_size = transaction_size();
    char tx[tx_size];
    auto read_size = read_transaction(tx, tx_size);
    eosio_assert(tx_size == read_size, "read_transaction failed");
    auto trx = unpack<transaction>(tx, read_size);
    std::vector<eosio::action> actions = trx.actions;
    eosio_assert(actions.size() == 3, "You need transfer both tokens");

    eosio::action action1 = eosio::get_action(1, 0);
    eosio::action action2 = eosio::get_action(1, 1);

    eosio_assert(action1.name == eosio::name("transfer"), "You need transfer both tokens"); //action1.name.to_string().c_str());
    eosio_assert(action2.name == eosio::name("transfer"), "You need transfer both tokens"); //action1.name.to_string().c_str());

    size_t size1 = action1.data.size();
    auto transfer_data1 = unpack<transfer_args>(&action1.data[0], size1);

    size_t size2 = action1.data.size();
    auto transfer_data2 = unpack<transfer_args>(&action2.data[0], size2);

    //Invalid deposit.
    std::vector<std::string> params1;
    utils::split(transfer_data1.memo, ',', params1);

    std::vector<std::string> params2;
    utils::split(transfer_data2.memo, ',', params2);

    eosio_assert(params1.size() == 2, "Invalid add liquidity");
    eosio_assert(params2.size() == 2, "Invalid add liquidity");

    eosio_assert(params1[0] == "addliquidity", "Invalid add liquidity.");
    eosio_assert(params2[0] == "addliquidity", "Invalid add liquidity.");

    eosio_assert(params1[1] == params1[1], "Invalid add liquidity.");
    eosio_assert(atoll(params1[1].c_str()) == liquidity_id, "Invalid add liquidity.");

    // eosio_assert(false, this->action.c_str()); //action1.name.to_string().c_str());
    token_t token1 = defi_liquidity->token1;
    token_t token2 = defi_liquidity->token2;

    asset quantity1;
    asset quantity2;
    if (token1.address == action1.account && token2.address == action2.account)
    {
        quantity1 = transfer_data1.quantity;
        quantity2 = transfer_data2.quantity;
    }
    else if (token1.address == action2.account && token2.address == action1.account)
    {
        quantity1 = transfer_data2.quantity;
        quantity2 = transfer_data1.quantity;
    }
    else
    {
        eosio_assert(false, "You need transfer both tokens"); //action1.name.to_string().c_str());
    }
    eosio_assert(quantity1.symbol == token1.symbol, "You need transfer both tokens");
    eosio_assert(quantity2.symbol == token2.symbol, "You need transfer both tokens");

    uint64_t pool_id = this->get_pool_id();

    uint64_t myliquidity_token = 0;
    uint64_t liquidity_token = defi_liquidity->liquidity_token;

    uint64_t hasSurplus = 0;
    asset surplusQuantity;

    if (liquidity_token == 0)
    {
        myliquidity_token = std::pow(quantity1.amount * quantity2.amount, 0.5);
    }
    else
    {

        float_t price = defi_liquidity->price1;
        if (quantity1.amount * price > quantity2.amount)
        {
            hasSurplus = 1;
            surplusQuantity = asset(quantity1.amount - quantity2.amount / price, quantity1.symbol);
            quantity1 -= surplusQuantity;
        }
        else if (quantity1.amount * price < quantity2.amount)
        {
            hasSurplus = 2;
            surplusQuantity = asset(quantity2.amount - quantity1.amount * price, quantity2.symbol);
            quantity2 -= surplusQuantity;
        }

        float_t alpha = (1.00 * quantity1.amount) / (quantity1.amount + defi_liquidity->quantity1.amount);
        myliquidity_token = (alpha / (1 - alpha)) * defi_liquidity->liquidity_token;
    }
    liquidity_token += myliquidity_token;

    auto pool_index = _defi_pool.get_index<"byaccountkey"_n>();
    auto pool_itr = pool_index.find(account.value);
    bool haspool = false;
    while (pool_itr != pool_index.end())
    {
        if (pool_itr->account != account)
        {
            break;
        }

        if (pool_itr->liquidity_id == liquidity_id)
        {
            haspool = true;
            break;
        }
        pool_itr++;
    }

    if (haspool)
    {
        pool_index.modify(pool_itr, _self, [&](auto &t) {
            t.quantity1 += quantity1;
            t.quantity2 += quantity2;
            t.liquidity_token += myliquidity_token;
        });
    }
    else
    {
        _defi_pool.emplace(get_self(), [&](auto &t) {
            t.pool_id = pool_id;
            t.account = account;
            t.liquidity_id = liquidity_id;
            t.quantity1 = quantity1;
            t.quantity2 = quantity2;
            t.liquidity_token = myliquidity_token;
            t.timestamp = now();
        });
    }

    //判断amount1,amount2 需要符合比例

    if (defi_liquidity->liquidity_token == 0)
    {
        _defi_liquidity.modify(defi_liquidity, _self, [&](auto &t) {
            t.quantity1 = quantity1;
            t.quantity2 = quantity2;
            t.price1 = 1.0 * quantity2.amount / quantity1.amount;
            t.price2 = 1.0 * quantity1.amount / quantity2.amount;
            t.liquidity_token = liquidity_token;
        });
    }
    else
    {
        _defi_liquidity.modify(defi_liquidity, _self, [&](auto &t) {
            t.quantity1 += quantity1;
            t.quantity2 += quantity2;
            t.liquidity_token = liquidity_token;
        });
    }

    this->_liquiditylog(account, liquidity_id, "deposit", token1, token2, quantity1, quantity2, liquidity_token);

    if (hasSurplus == 1)
    {
        this->_transfer_to(account, defi_liquidity->token1.address.value, surplusQuantity, "refund");
    }
    else if (hasSurplus == 2)
    {
        this->_transfer_to(account, defi_liquidity->token2.address.value, surplusQuantity, "refund");
    }
    return;
}

void onesgame::subliquidity(name account, uint64_t liquidity_id, uint64_t liquidity_token)
{
    require_auth(account);

    auto defi_liquidity = _defi_liquidity.find(liquidity_id);
    eosio_assert(defi_liquidity != _defi_liquidity.end(), "Liquidity does not exist");

    auto pool_index = _defi_pool.get_index<"byaccountkey"_n>();
    auto pool_itr = pool_index.find(account.value);
    bool haspool = false;
    while (pool_itr != pool_index.end())
    {
        if (pool_itr->account != account)
        {
            break;
        }

        if (pool_itr->liquidity_id == liquidity_id)
        {
            haspool = true;
            break;
        }
        pool_itr++;
    }

    eosio_assert(haspool, "User liquidity does not exist.");
    eosio_assert(pool_itr->liquidity_token >= liquidity_token, " Insufficient liquidity");

    uint64_t amount1 = (1.00 * liquidity_token / defi_liquidity->liquidity_token) * defi_liquidity->quantity1.amount;
    uint64_t amount2 = (1.00 * liquidity_token / defi_liquidity->liquidity_token) * defi_liquidity->quantity2.amount;

    asset quantity1(amount1, pool_itr->quantity1.symbol);
    asset quantity2(amount2, pool_itr->quantity2.symbol);
    eosio_assert(amount1 > 0 && amount2 > 0, "Zero");

    if (liquidity_token == pool_itr->liquidity_token)
    {
        pool_index.erase(pool_itr);
    }
    else
    {
        pool_index.modify(pool_itr, _self, [&](auto &t) {
            t.quantity1 -= quantity1;
            t.quantity2 -= quantity2;
            t.liquidity_token -= liquidity_token;
        });
    }
    if (defi_liquidity->liquidity_token == liquidity_token)
    {
        _defi_liquidity.modify(defi_liquidity, _self, [&](auto &t) {
            t.quantity1 -= quantity1;
            t.quantity2 -= quantity2;
            t.price1 = 0;
            t.price2 = 0;
            t.liquidity_token = 0;
        });
    }
    else
    {
        _defi_liquidity.modify(defi_liquidity, _self, [&](auto &t) {
            t.quantity1 -= quantity1;
            t.quantity2 -= quantity2;
            t.price1 = 1.0 * (defi_liquidity->quantity2.amount - quantity2.amount) / (defi_liquidity->quantity1.amount - quantity1.amount);
            t.price2 = 1.0 * (defi_liquidity->quantity1.amount - quantity1.amount) / (defi_liquidity->quantity2.amount - quantity2.amount);
            t.liquidity_token -= liquidity_token;
        });
    }

    _transfer_to(account, defi_liquidity->token1.address.value, quantity1, "withdraw");
    _transfer_to(account, defi_liquidity->token2.address.value, quantity2, "withdraw");

    this->_liquiditylog(account, liquidity_id, "withdraw", defi_liquidity->token1, defi_liquidity->token2, quantity1, quantity2, liquidity_token);
}

void onesgame::mine(name account, asset quantity, uint64_t liquidity_id)
{
    auto defi_liquidity = _defi_liquidity.find(liquidity_id);

    float weight = defi_liquidity->swap_weight;

    asset mine_quantity = asset(0, EOS_TOKEN_SYMBOL);

    if (this->code == name(EOS_TOKEN_ACCOUNT).value && quantity.symbol == EOS_TOKEN_SYMBOL)
    {
        weight = weight > 0 ? weight : 1.0;
        mine_quantity.amount = quantity.amount * weight;
    }
    else
    {
        mine_quantity.amount = quantity.amount * weight * defi_liquidity->price2;
    }

    if (mine_quantity.amount >= 10000)
    {
        eosio::action(eosio::permission_level{get_self(), "active"_n},
                      eosio::name(ONES_MINE_ACCOUNT),
                      "mineswap"_n,
                      make_tuple(account, mine_quantity))
            .send();
    }
}

void onesgame::updateweight(uint64_t liquidity_id, uint64_t type, float_t weight)
{
    require_auth(name(ONES_PLAY_ACCOUNT));

    auto it = _defi_liquidity.find(liquidity_id);
    eosio_assert(it != _defi_liquidity.end(), "Liquidity does not exist");
    eosio_assert(type == DEFI_TYPE_LIQUIDITY || type == DEFI_TYPE_SWAP, "type invalid");

    if (type == DEFI_TYPE_LIQUIDITY)
    {
        _defi_liquidity.modify(it, _self, [&](auto &t) {
            t.liquidity_weight = weight;
        });
    }
    else
    {
        _defi_liquidity.modify(it, _self, [&](auto &t) {
            t.swap_weight = weight;
        });
    }
}

void onesgame::upgrade()
{
    require_auth(name(ONES_PLAY_ACCOUNT));
}

void onesgame::remove(uint64_t type, uint64_t id)
{
    require_auth(name(ONES_PLAY_ACCOUNT));

    uint64_t num = 0;
    if (type == DEFI_TYPE_LIQUIDITY)
    {
        auto it = _liquidity_log.begin();
        while (it != _liquidity_log.end())
        {
            num++;
            if (num > 200 || id < it->log_id)
            {
                break;
            }
            it = _liquidity_log.erase(it);
        }
    }
    else if (type == DEFI_TYPE_SWAP)
    {
        auto it = _swap_log.begin();
        while (it != _swap_log.end())
        {

            num++;
            if (num > 200 || id < it->swap_id)
            {
                break;
            }
            it = _swap_log.erase(it);
        }
    }
    else if (type == DEFI_TYPE_PAIR)
    {
        tb_defi_pair _defi_pair(_self, _self.value);

        auto itr = _defi_liquidity.find(id);
        eosio_assert(itr != _defi_liquidity.end(), "liquidity isn't exist");
        eosio_assert(itr->liquidity_token > 0, "liquidity has token");
        _defi_liquidity.erase(itr);

        auto index = _defi_pair.get_index<"byliquidity"_n>();
        auto it = index.find(id);
        eosio_assert(it != index.end(), "pair isn't exist");

        index.erase(it);
    }
    else
    {
        eosio_assert(false, "type invalid");
    }

    if (num > 200)
    {
        eosio::transaction txn{};
        txn.actions.emplace_back(
            action(eosio::permission_level{get_self(), "active"_n},
                   eosio::name(get_self()),
                   "remove"_n,
                   make_tuple(type, id)));
        txn.delay_sec = 0;
        txn.send(now(), get_self());
    }
}

void onesgame::_transfer_to(name to, uint64_t code, asset quantity, string memo)
{
    if (quantity.amount == 0)
    {
        return;
    }

    eosio::action(permission_level{get_self(), "active"_n},
                  eosio::name(code),
                  "transfer"_n,
                  make_tuple(get_self(), to, quantity, memo))
        .send();
}

uint64_t onesgame::get_swap_id()
{
    uint64_t cur_time = now();
    st_defi_config defi_config = _defi_config.get();
    defi_config.swap_id++;
    _defi_config.set(defi_config, _self);
    return defi_config.swap_id;
}

uint64_t onesgame::get_liquidity_id()
{
    uint64_t cur_time = now();
    st_defi_config defi_config = _defi_config.get();
    defi_config.liquidity_id++;
    _defi_config.set(defi_config, _self);
    return defi_config.liquidity_id;
}

uint64_t onesgame::get_pool_id()
{
    uint64_t cur_time = now();
    st_defi_config defi_config = _defi_config.get();
    defi_config.pool_id++;
    _defi_config.set(defi_config, _self);
    return defi_config.pool_id;
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
                EOSIO_DISPATCH_HELPER(onesgame, (newliquidity)(addliquidity)(subliquidity)(remove)(updateweight)(upgrade))
            }
            return;
        }

        if (action == eosio::name("transfer").value)
        {
            onesgame::code = code;
            execute_action(eosio::name(receiver), eosio::name(receiver), &onesgame::transfer);
            return;
        }

        eosio_exit(0);
    }
}