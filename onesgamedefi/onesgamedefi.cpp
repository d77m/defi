#include "onesgamedefi.hpp"

#include <cmath>
#include <eosiolib/transaction.hpp>

#define EOS_TOKEN_SYMBOL symbol("EOS", 4)
#define EOS_TOKEN_ACCOUNT "eosio.token"
#define USDT_TOKEN_SYMBOL symbol("USDT", 4)
#define USDT_TOKEN_ACCOUNT "tethertether"

#define ONES_MINE_ACCOUNT "onesgamemine"
#define ONES_PLAY_ACCOUNT "onesgameplay"

#define DEFI_TYPE_LIQUIDITY 1
#define DEFI_TYPE_SWAP 2
#define DEFI_TYPE_PAIR 3

#define ACCOUNT_CHECK(account) \
    eosio_assert(is_account(account), "invalid account " #account);

#define ONES_FUND_ACCOUNT "onesgamefund"
#define ONES_DIVD_ACCOUNT "onesgamedivd"

#define BOX_DEFI_ACCOUNT "swap.defi"
#define BOX_TOKEN_ACCOUNT "token.defi"
#define BOX_LPTOKEN_ACCOUNT "lptoken.defi"

#define BOX_TOKEN_SYMBOL symbol("BOX", 6)
#define BOXL_TOKEN_SYMBOL symbol("BOXL", 0)

#define DFS_DEFI_ACCOUNT "defisswapcnt"
#define DFS_TOKEN_ACCOUNT "minedfstoken"
#define DFS_TOKEN_SYMBOL symbol("DFS", 4)

float_t ONES_SWAP_FEE = 0.001;
float_t ONES_FUND_FEE = 0.001;
float_t ONES_DIVD_FEE = 0.001;

void onesgame::newliquidity(name account, token_t token1, token_t token2) {
    require_auth(account);

    if (token1 == token2) eosio_assert(false, "same token");

    stats statstable1(token1.address, token1.symbol.code().raw());
    const auto &st1 = statstable1.find(token1.symbol.code().raw());
    std::string sterr1 = "invalid token address:" + token1.address.to_string();
    eosio_assert(st1 != statstable1.end(), sterr1.c_str());

    stats statstable2(token2.address, token2.symbol.code().raw());
    const auto &st2 = statstable2.find(token2.symbol.code().raw());
    std::string sterr2 = "invalid token address:" + token2.address.to_string();
    eosio_assert(st2 != statstable2.end(), sterr2.c_str());

    uint64_t liquidity_id = _get_liquidity_id();

    this->_newpair(liquidity_id, token1, token2);

    bool iseos = (token2.address == name(EOS_TOKEN_ACCOUNT) && token2.symbol == EOS_TOKEN_SYMBOL)? true: false;

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

void onesgame::_newpair(uint64_t liquidity_id, const token_t &token1, const token_t &token2) {
    
    checksum256 digest = this->_getpair_digest(token1, token2);
    tb_defi_pair _defi_pair(_self, _self.value);

    auto itr = _defi_pair.find(utils::uint64_hash(digest));
    check(itr == _defi_pair.end(), "Liquidity already exists");

    _defi_pair.emplace(get_self(), [&](auto &t) {
        t.digest = digest;
        t.liquidity_id = liquidity_id;
    });
}

checksum256 onesgame::_getpair_digest( const token_t &token1, const token_t &token2){
    bool reverse = token1.address.value < token2.address.value;

    name _contract1 = reverse ? token1.address : token2.address;
    name _contract2 = reverse ? token2.address : token1.address;
    symbol _sym1 = reverse ? token1.symbol : token2.symbol;
    symbol _sym2 = reverse ? token2.symbol : token1.symbol;

    std::string uni_key =
        _contract1.to_string() + "-" + _sym1.code().to_string();
    uni_key += ":" + _contract2.to_string() + "-" + _sym2.code().to_string();

    const char *uni_key_cstr = uni_key.c_str();
    return sha256(uni_key_cstr, strlen(uni_key_cstr));
}

void onesgame::transfer(name from, name to, asset quantity, string memo) {
    require_auth(from);

    ACCOUNT_CHECK(to);

    if (from == get_self() || to != get_self()) return;

    eosio_assert(quantity.is_valid(), "invalid quantity");
    eosio_assert(quantity.amount > 0, "must transfer positive quantity");

    if (from == name(BOX_DEFI_ACCOUNT) || from == name(BOX_LPTOKEN_ACCOUNT))
        return _handle_box(from, to, quantity, memo);

    if (from == name(DFS_DEFI_ACCOUNT) || from == name(DFS_TOKEN_ACCOUNT))
        return _handle_dfs(from, to, quantity, memo);

    std::vector<std::string> params;
    utils::split(memo, ',', params);

    if (params.size() == 0)
        return this->_transfer_to(name(ONES_PLAY_ACCOUNT), this->code, quantity, memo);

    std::string action = params.at(0);

    if (action == "swap") return this->swap(from, quantity, params);
    if (action == "addliquidity") return this->_addliquidity(from, to, quantity, memo);
    return this->_transfer_to(name(ONES_PLAY_ACCOUNT), this->code, quantity, memo);
}

void onesgame::_swaplog(name account, uint64_t third_id, uint64_t liquidity_id,
                        token_t in_token, token_t out_token, asset in_asset,
                        asset out_asset, asset fee, float_t price) {
    uint64_t swap_id = this->_get_swap_id();
    if (swap_id > 200) {
        auto it = _swap_log.begin();
        while (it != _swap_log.end() && it->swap_id < (swap_id - 200)) {
            it = _swap_log.erase(it);
        }
    }
    
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
        t.trx_id = _get_trx_id();
        t.timestamp = now();
    });
}

void onesgame::_liquiditylog(name account, uint64_t liquidity_id, string type,
                             token_t in_token, token_t out_token,
                             asset in_asset, asset out_asset,
                             uint64_t liquidity_token) {

    uint64_t log_id = 0;
    auto it = _liquidity_log.rbegin();
    if (it != _liquidity_log.rend()) {
        log_id = it->log_id;
    }
    if (log_id > 200) {
        auto it = _liquidity_log.begin();
        while (it != _liquidity_log.end() && it->log_id < (log_id - 200)) {
            it = _liquidity_log.erase(it);
        }
    }
    
    log_id++;
    
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
        t.trx_id = _get_trx_id();
        t.timestamp = now();
    });
}
checksum256 onesgame::_get_trx_id(){
    auto tx_size = transaction_size();
    char tx[tx_size];
    auto read_size = read_transaction(tx, tx_size);
    eosio_assert(tx_size == read_size, "read_transaction failed");
    auto trx = unpack<transaction>(tx, read_size);

    return sha256(tx, tx_size);
}

std::vector<eosio::action> onesgame::_get_actions(){
    auto tx_size = transaction_size();
    char tx[tx_size];
    auto read_size = read_transaction(tx, tx_size);
    eosio_assert(tx_size == read_size, "read_transaction failed");
    auto trx = unpack<transaction>(tx, read_size);

    return trx.actions;
}
void onesgame::swap(name account, asset quantity, std::vector<std::string> &params) {
    eosio_assert(params.size() == 4, "invalid memo");

    std::vector<std::string> liquidity_ids;
    utils::split(params.at(3), '-', liquidity_ids);
    uint64_t liquidity_id = 0;
    uint64_t third_id = atoll(params.at(1).c_str());

    swap_t swapdata;
    swapdata.quantity = quantity;
    swapdata.code = this->code;

    for (uint64_t i = 0; i < liquidity_ids.size(); i++) {
        swapdata.original_quantity =
            asset(swapdata.quantity.amount, swapdata.quantity.symbol);

        asset fund_fee = asset(swapdata.quantity.amount * ONES_FUND_FEE, swapdata.quantity.symbol);
        this->_transfer_to(name(ONES_FUND_ACCOUNT), swapdata.code, fund_fee, "swap fund fee");

        asset divd_fee = asset(swapdata.quantity.amount * ONES_DIVD_FEE, swapdata.quantity.symbol);
        this->_transfer_to(name(ONES_DIVD_ACCOUNT), swapdata.code, divd_fee, "swap divd fee");

        swapdata.quantity.amount -= swapdata.quantity.amount * ONES_FUND_FEE;
        swapdata.quantity.amount -= swapdata.quantity.amount * ONES_DIVD_FEE;

        liquidity_id = atoll(liquidity_ids.at(i).c_str());
        swapdata = this->_swap(account, swapdata, liquidity_id, third_id);
    }

    this->_transfer_to(account, swapdata.code, swapdata.quantity, "swap");
}

onesgame::swap_t onesgame::_swap(name account, swap_t &in,
                                 uint64_t liquidity_id, uint64_t third_id) {
    auto it = _defi_liquidity.find(liquidity_id);
    eosio_assert(it != _defi_liquidity.end(), "Liquidity does not exist");

    onesgame::swap_t out;

    token_t token1, token2;

    eosio_assert(in.code == it->token1.address.value || in.code == it->token2.address.value, "token address error");

    if (in.code == it->token1.address.value && in.quantity.symbol == it->token1.symbol) {
        token1 = it->token1;
        token2 = it->token2;
        float_t alpha = (1.0 * in.quantity.amount) / it->quantity1.amount;
        float_t r = 1 - ONES_SWAP_FEE;
        uint64_t amount = (1.0 * (alpha * r) / (1 + alpha * r)) * it->quantity2.amount;

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
    } else if (in.code == it->token2.address.value && in.quantity.symbol == it->token2.symbol) {
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
    } else {
        eosio_assert(false, "token address error");
    }

    float_t price = (1.0 * out.quantity.amount) / in.original_quantity.amount;
    asset fee(in.original_quantity.amount *
                  (ONES_DIVD_FEE + ONES_FUND_FEE + ONES_SWAP_FEE),
              in.quantity.symbol);

    this->_swaplog(account, third_id, liquidity_id, token1, token2,
                   in.original_quantity, out.quantity, fee, price);

    this->swapmine(account, in.code, in.original_quantity, liquidity_id);
    return out;
}

void onesgame::addliquidity(name account, uint64_t liquidity_id) {
    require_auth(account);

    auto defi_liquidity = _defi_liquidity.find(liquidity_id);

    eosio_assert(defi_liquidity != _defi_liquidity.end(), "Liquidity does not exist");

    tb_defi_transfer _defi_transfer(_self, _self.value);
    eosio_assert(_defi_transfer.exists(), "You need transfer both tokens");
    st_defi_transfer defi_transfer = _defi_transfer.get();
    
    auto trx_id = this->_get_trx_id();
    eosio_assert(defi_transfer.trx_id == trx_id && defi_transfer.status == 2, "You need transfer both tokens");

    auto transfer_data1 = defi_transfer.args1;
    auto transfer_data2 = defi_transfer.args2;

    std::vector<std::string> params1;
    utils::split(transfer_data1.memo, ',', params1);

    eosio_assert(atoll(params1[1].c_str()) == liquidity_id, "Invalid add liquidity.");

    token_t token1 = defi_liquidity->token1;
    token_t token2 = defi_liquidity->token2;

    asset quantity1, quantity2;
    if (token1.address == defi_transfer.action1 && token2.address == defi_transfer.action2) {
        quantity1 = transfer_data1.quantity;
        quantity2 = transfer_data2.quantity;
    } else if (token1.address == defi_transfer.action2 && token2.address == defi_transfer.action1) {
        quantity1 = transfer_data2.quantity;
        quantity2 = transfer_data1.quantity;
    } else {
        eosio_assert(false, "You need transfer both tokens"); 
    }
    eosio_assert(quantity1.symbol == token1.symbol, "You need transfer both tokens");
    eosio_assert(quantity2.symbol == token2.symbol, "You need transfer both tokens");

    uint64_t pool_id = this->_get_pool_id();

    uint64_t myliquidity_token = 0;
    uint64_t liquidity_token = defi_liquidity->liquidity_token;

    uint64_t hasSurplus = 0;
    asset surplusQuantity;

    if (liquidity_token == 0) {
        myliquidity_token = std::pow(quantity1.amount * quantity2.amount, 0.5);
    } else {
        float_t price = defi_liquidity->price1;
        if (quantity1.amount * price > quantity2.amount) {
            hasSurplus = 1;
            surplusQuantity = asset(quantity1.amount - quantity2.amount / price, quantity1.symbol);
            quantity1 -= surplusQuantity;
        } else if (quantity1.amount * price < quantity2.amount) {
            hasSurplus = 2;
            surplusQuantity = asset(quantity2.amount - quantity1.amount * price, quantity2.symbol);
            quantity2 -= surplusQuantity;
        }

        float_t alpha = (1.00 * quantity1.amount) /
                        (quantity1.amount + defi_liquidity->quantity1.amount);
        myliquidity_token =
            (alpha / (1 - alpha)) * defi_liquidity->liquidity_token;
    }
    liquidity_token += myliquidity_token;

    auto pool_index = _defi_pool.get_index<"byaccountkey"_n>();
    auto pool_itr = pool_index.find(account.value);
    bool haspool = false;
    while (pool_itr != pool_index.end()) {
        if (pool_itr->account != account) break;

        if (pool_itr->liquidity_id == liquidity_id) {
            haspool = true;
            break;
        }
        pool_itr++;
    }

    if (haspool) {
        pool_index.modify(pool_itr, _self, [&](auto &t) {
            t.quantity1 += quantity1;
            t.quantity2 += quantity2;
            t.liquidity_token += myliquidity_token;
        });
    } else {
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

    if (defi_liquidity->liquidity_token == 0) {
        _defi_liquidity.modify(defi_liquidity, _self, [&](auto &t) {
            t.quantity1 = quantity1;
            t.quantity2 = quantity2;
            t.price1 = 1.0 * quantity2.amount / quantity1.amount;
            t.price2 = 1.0 * quantity1.amount / quantity2.amount;
            t.liquidity_token = liquidity_token;
        });
    } else {
        _defi_liquidity.modify(defi_liquidity, _self, [&](auto &t) {
            t.quantity1 += quantity1;
            t.quantity2 += quantity2;
            t.liquidity_token = liquidity_token;
        });
    }

    this->_liquiditylog(account, liquidity_id, "deposit", token1, token2,
                        quantity1, quantity2, liquidity_token);

    if (hasSurplus == 1) {
        this->_transfer_to(account, defi_liquidity->token1.address.value, surplusQuantity, "refund");
    } else if (hasSurplus == 2) {
        this->_transfer_to(account, defi_liquidity->token2.address.value, surplusQuantity, "refund");
    }
}

void onesgame::_addliquidity(name from, name to, asset quantity, string memo) {
    tb_defi_transfer _defi_transfer(_self, _self.value);

    std::vector<std::string> params;
    utils::split(memo, ',', params);

    eosio_assert(params.size() == 2 && params[0] == "addliquidity", "Invalid add liquidity");
    uint64_t liquidity_id = atoll(params[1].c_str());
    
    auto defi_liquidity = _defi_liquidity.find(liquidity_id);
    eosio_assert(defi_liquidity != _defi_liquidity.end(), "Liquidity does not exist");

    auto trx_id = this->_get_trx_id();

    st_defi_transfer defi_transfer = _defi_transfer.get_or_default(st_defi_transfer{
        .trx_id = trx_id,
        .status = 0
    });

    transfer_args args({
        .from = from,
        .to = to,
        .quantity = quantity,
        .memo = memo
    });
    
    if(defi_transfer.trx_id != trx_id || (defi_transfer.trx_id==trx_id && defi_transfer.status  ==0) ){
        defi_transfer.args1 = args;
        defi_transfer.status = 1;
        defi_transfer.action1 = name(this->code);
        defi_transfer.trx_id = trx_id;
    }else if( defi_transfer.trx_id==trx_id && defi_transfer.status == 1){
        defi_transfer.args2 = args;
        defi_transfer.action2 = name(this->code);
        defi_transfer.status = 2;

        eosio_assert(defi_transfer.args1.memo == defi_transfer.args2.memo, "Invalid add liquidity");
    }else{
        eosio_assert(false, "You need transfer both tokens");
    }
    
    _defi_transfer.set(defi_transfer, _self);
}

void onesgame::subliquidity(name account, uint64_t liquidity_id, uint64_t liquidity_token) {
    require_auth(account);

    auto defi_liquidity = _defi_liquidity.find(liquidity_id);
    eosio_assert(defi_liquidity != _defi_liquidity.end(), "Liquidity does not exist");

    auto pool_index = _defi_pool.get_index<"byaccountkey"_n>();
    auto pool_itr = pool_index.find(account.value);
    bool haspool = false;
    while (pool_itr != pool_index.end()) {
        if (pool_itr->account != account) 
            break;

        if (pool_itr->liquidity_id == liquidity_id) {
            haspool = true;
            break;
        }
        pool_itr++;
    }

    eosio_assert(haspool, "User liquidity does not exist.");
    eosio_assert(pool_itr->liquidity_token >= liquidity_token, "Insufficient liquidity");

    uint64_t amount1 = (1.00 * liquidity_token / defi_liquidity->liquidity_token) *
        defi_liquidity->quantity1.amount;
    uint64_t amount2 = (1.00 * liquidity_token / defi_liquidity->liquidity_token) *
        defi_liquidity->quantity2.amount;

    asset quantity1(amount1, pool_itr->quantity1.symbol);
    asset quantity2(amount2, pool_itr->quantity2.symbol);
    eosio_assert(amount1 > 0 && amount2 > 0, "Zero");

    if (liquidity_token == pool_itr->liquidity_token) {
        pool_index.erase(pool_itr);
    } else {
        pool_index.modify(pool_itr, _self, [&](auto &t) {
            t.quantity1 -= quantity1;
            t.quantity2 -= quantity2;
            t.liquidity_token -= liquidity_token;
        });
    }
    if (defi_liquidity->liquidity_token == liquidity_token) {
        _defi_liquidity.modify(defi_liquidity, _self, [&](auto &t) {
            t.quantity1 -= quantity1;
            t.quantity2 -= quantity2;
            t.price1 = 0;
            t.price2 = 0;
            t.liquidity_token = 0;
        });
    } else {
        _defi_liquidity.modify(defi_liquidity, _self, [&](auto &t) {
            t.quantity1 -= quantity1;
            t.quantity2 -= quantity2;
            t.price1 = 1.0 * (defi_liquidity->quantity2.amount - quantity2.amount) /
                       (defi_liquidity->quantity1.amount - quantity1.amount);
            t.price2 = 1.0 * (defi_liquidity->quantity1.amount - quantity1.amount) /
                       (defi_liquidity->quantity2.amount - quantity2.amount);
            t.liquidity_token -= liquidity_token;
        });
    }

    _transfer_to(account, defi_liquidity->token1.address.value, quantity1, "withdraw");
    _transfer_to(account, defi_liquidity->token2.address.value, quantity2, "withdraw");

    this->_liquiditylog(account, liquidity_id, "withdraw",
                        defi_liquidity->token1, defi_liquidity->token2,
                        quantity1, quantity2, liquidity_token);
}

void onesgame::swapmine(name account, uint64_t code, asset quantity, uint64_t liquidity_id) {
    auto defi_liquidity = _defi_liquidity.find(liquidity_id);

    float weight = defi_liquidity->swap_weight;

    asset mine_quantity = asset(0, EOS_TOKEN_SYMBOL);
    if(defi_liquidity->token1.address == name(EOS_TOKEN_ACCOUNT)){
        if (code == name(EOS_TOKEN_ACCOUNT).value && quantity.symbol == EOS_TOKEN_SYMBOL) {
            mine_quantity.amount = quantity.amount * weight;
        } else {
            mine_quantity.amount = quantity.amount * weight * defi_liquidity->price2;
        }
    } else {
        token_t token1,token2;
        token1.address = name(EOS_TOKEN_ACCOUNT);
        token1.symbol = EOS_TOKEN_SYMBOL;
        token2.address = name(code);
        token2.symbol = quantity.symbol;

        checksum256 digest = this->_getpair_digest(token1, token2);
        tb_defi_pair _defi_pair(_self, _self.value);

        auto itr = _defi_pair.find(utils::uint64_hash(digest));
        if(itr == _defi_pair.end()) return;
        
        auto eos_liquidity = _defi_liquidity.find(itr->liquidity_id);
        if( eos_liquidity == _defi_liquidity.end()) return;
        
        mine_quantity.amount = quantity.amount * weight * eos_liquidity->price2;
    }

    if (mine_quantity.amount >= 10000) {
        eosio::action(eosio::permission_level{get_self(), "active"_n},
                      eosio::name(ONES_MINE_ACCOUNT), "mineswap"_n,
                      make_tuple(account, mine_quantity)).send();
    }
}

void onesgame::updateweight(uint64_t liquidity_id, uint64_t type, float_t weight) {
    require_auth(name(ONES_PLAY_ACCOUNT));

    auto it = _defi_liquidity.find(liquidity_id);
    eosio_assert(it != _defi_liquidity.end(), "Liquidity does not exist");
    eosio_assert(type == DEFI_TYPE_LIQUIDITY || type == DEFI_TYPE_SWAP, "type invalid");

    if (type == DEFI_TYPE_LIQUIDITY) {
        _defi_liquidity.modify(it, _self, [&](auto &t) { t.liquidity_weight = weight; });
    } else {
        _defi_liquidity.modify(it, _self, [&](auto &t) { t.swap_weight = weight; });
    }
}

void onesgame::remove(uint64_t type, uint64_t id) {
    require_auth(name(ONES_PLAY_ACCOUNT));

    if (type == DEFI_TYPE_PAIR) {
        tb_defi_pair _defi_pair(_self, _self.value);

        auto itr = _defi_liquidity.find(id);
        eosio_assert(itr != _defi_liquidity.end(), "liquidity isn't exist");
        eosio_assert(itr->liquidity_token == 0, "liquidity has token");
        _defi_liquidity.erase(itr);

        auto index = _defi_pair.get_index<"byliquidity"_n>();
        auto it = index.find(id);
        eosio_assert(it != index.end(), "pair isn't exist");

        index.erase(it);
    } else {
        eosio_assert(false, "type invalid");
    }
}

void onesgame::_transfer_to(name to, uint64_t code, asset quantity, string memo) {
    if (quantity.amount == 0) return;

    eosio::action(permission_level{get_self(), "active"_n}, eosio::name(code),
                  "transfer"_n, make_tuple(get_self(), to, quantity, memo)).send();
}

uint64_t onesgame::_get_swap_id() {
    st_defi_config defi_config = _defi_config.get();
    defi_config.swap_id++;
    _defi_config.set(defi_config, _self);
    return defi_config.swap_id;
}

uint64_t onesgame::_get_liquidity_id() {
    st_defi_config defi_config = _defi_config.get();
    defi_config.liquidity_id++;
    _defi_config.set(defi_config, _self);
    return defi_config.liquidity_id;
}

uint64_t onesgame::_get_pool_id() {
    st_defi_config defi_config = _defi_config.get();
    defi_config.pool_id++;
    _defi_config.set(defi_config, _self);
    return defi_config.pool_id;
}

void onesgame::marketmine(name account, uint64_t liquidity_id,
                          uint64_t to_liquidity_id, float percent) {
    require_auth(name(ONES_PLAY_ACCOUNT));

    eosio_assert(percent <= 0.8 && percent > 0, "percent must be less 0.8");

    eosio_assert( account == name(BOX_DEFI_ACCOUNT) || account == name(DFS_DEFI_ACCOUNT),
        "account invalid");

    tb_market_info _market_info(get_self(), get_self().value);
    auto info = _market_info.begin();
    eosio_assert(info == _market_info.end(), "mine has runing");

    auto it = _defi_liquidity.find(liquidity_id);
    eosio_assert(it != _defi_liquidity.end(), "Liquidity does not exist");

    eosio_assert(it->token1.address == name(EOS_TOKEN_ACCOUNT), "must be eos");
    eosio_assert(it->token2.address == name(USDT_TOKEN_ACCOUNT), "must be usdt");

    asset eos_quantity = it->quantity1 * percent;
    asset usdt_quantity = it->quantity1 * percent;

    _market_info.emplace(get_self(), [&](auto &t) {
        t.liquidity_id = liquidity_id;
        t.account = account;

        t.in_eos = eos_quantity;
        t.in_usdt = usdt_quantity;

        t.in_eos = asset(0, EOS_TOKEN_SYMBOL);
        t.in_usdt = asset(0, USDT_TOKEN_SYMBOL);

        t.liquidity_token = 0;
        t.timestamp = now();
        t.status = 0;
    });

    string memo = "deposit";
    if (account == name(BOX_DEFI_ACCOUNT))
        memo += "," + std::to_string(to_liquidity_id);

    eosio::action action1 = action(
        eosio::permission_level(get_self(), "active"_n), it->token1.address,
        "transfer"_n, make_tuple(get_self(), account, eos_quantity, memo));

    eosio::action action2 = action(
        eosio::permission_level(get_self(), "active"_n), it->token2.address,
        "transfer"_n, make_tuple(get_self(), account, usdt_quantity, memo));

    eosio::action action3 =
        action(eosio::permission_level(get_self(), "active"_n), account,
               "deposit"_n, make_tuple(get_self(), to_liquidity_id));
    if (account == name(BOX_DEFI_ACCOUNT)) {
        action1.send();
        action2.send();
        action3.send();
    } else if (account == name(DFS_DEFI_ACCOUNT)) {
        action3.send();
        action1.send();
        action2.send();
    }
}

void onesgame::marketexit(string memo, uint64_t amount) {
    require_auth(name(ONES_PLAY_ACCOUNT));

    tb_market_info _market_info(get_self(), get_self().value);
    auto info = _market_info.begin();
    eosio_assert(info != _market_info.end(), "mine does not exist");

    if (info->account == name(BOX_DEFI_ACCOUNT)) {
        _marketexit_box(info->liquidity_token, memo);
    } else if (info->account == name(DFS_DEFI_ACCOUNT)) {
        _market_info.modify(info, _self, [&](auto &t) { t.liquidity_token = amount; });
        _marketexit_dfs(amount, atoll(memo.c_str()));
    }
}

void onesgame::marketclaim() {
    require_auth(name(ONES_PLAY_ACCOUNT));

    tb_market_info _market_info(get_self(), get_self().value);
    auto info = _market_info.begin();
    eosio_assert(info != _market_info.end(), "mine does not exist");

    eosio_assert(info->status == 2 && info->out_eos.amount > 0 &&
                     info->out_usdt.amount > 0, "mine does not exist");

    if (info->account == name(BOX_DEFI_ACCOUNT)) _marketclaim_box();
}

void onesgame::marketsettle() {
    require_auth(name(ONES_PLAY_ACCOUNT));

    tb_market_info _market_info(get_self(), get_self().value);
    auto info = _market_info.begin();
    eosio_assert(info != _market_info.end(), "mine does not exist");

    uint64_t size = 1;
    if ((info->out_eos.amount - info->in_eos.amount) < 0) size++;

    if ((info->out_usdt.amount - info->in_usdt.amount) < 0) size++;

    std::vector<eosio::action> actions = _get_actions();
    eosio_assert(actions.size() == size, "You need transfer tokens");

    if (size == 2) {
        eosio::action action1 = eosio::get_action(1, 0);
        eosio_assert(action1.name == eosio::name("transfer"), "You need transfer tokens");

        size_t size1 = action1.data.size();
        auto transfer_data1 = unpack<transfer_args>(&action1.data[0], size1);
        eosio_assert(transfer_data1.memo == "marketsettle", "Invalid marketsettle.");

        if ((info->out_eos.amount - info->in_eos.amount) < 0) {
            eosio_assert(action1.account == name(EOS_TOKEN_ACCOUNT) &&
                             transfer_data1.quantity.symbol == EOS_TOKEN_SYMBOL,
                         "You need transfer eos token");
            eosio_assert((info->in_eos.amount - info->out_eos.amount) ==
                             transfer_data1.quantity.amount,
                         "You need transfer enough eos token ");
        }

        if ((info->out_usdt.amount - info->in_usdt.amount) < 0) {
            eosio_assert(action1.account == name(USDT_TOKEN_ACCOUNT) &&
                    transfer_data1.quantity.symbol == USDT_TOKEN_SYMBOL, "You need transfer usdt token");
            eosio_assert((info->in_usdt.amount - info->out_usdt.amount) ==
                             transfer_data1.quantity.amount, "You need transfer enough usdt token");
        }
    } else if (size == 3) {
        eosio::action action1 = eosio::get_action(1, 0);
        eosio::action action2 = eosio::get_action(1, 1);

        eosio_assert(action1.name == eosio::name("transfer") && action2.name == eosio::name("transfer"), "You need transfer both tokens");

        size_t size1 = action1.data.size();
        auto transfer_data1 = unpack<transfer_args>(&action1.data[0], size1);
        eosio_assert(transfer_data1.memo == "marketsettle", "Invalid marketsettle.");

        size_t size2 = action1.data.size();
        auto transfer_data2 = unpack<transfer_args>(&action2.data[0], size2);
        eosio_assert(transfer_data2.memo == "marketsettle", "Invalid marketsettle.");

        eosio_assert(action1.account == name(EOS_TOKEN_ACCOUNT) && transfer_data1.quantity.symbol == EOS_TOKEN_SYMBOL, "You need transfer eos token");
        eosio_assert((info->in_eos.amount - info->out_eos.amount) == transfer_data1.quantity.amount, "You need transfer enough eos token");

        eosio_assert(action2.account == name(USDT_TOKEN_ACCOUNT) && transfer_data2.quantity.symbol == USDT_TOKEN_SYMBOL, "You need transfer usdt token");
        eosio_assert((info->in_usdt.amount - info->out_usdt.amount) == transfer_data2.quantity.amount, "You need transfer enough usdt token");
    }

    if ((info->out_eos.amount - info->in_eos.amount) > 0)
        _transfer_to(name(ONES_PLAY_ACCOUNT), name(EOS_TOKEN_ACCOUNT).value,
                     info->out_eos - info->in_eos, "market");

    if ((info->out_usdt.amount - info->in_usdt.amount) > 0)
        _transfer_to(name(ONES_PLAY_ACCOUNT), name(USDT_TOKEN_ACCOUNT).value,
                     info->out_usdt - info->in_usdt, "market");

    tb_market_log _market_log(get_self(), get_self().value);
    auto log = _market_log.rbegin();
    uint64_t mine_id = 1;
    if (log != _market_log.rend()) mine_id = log->mine_id + 1;

    _market_log.emplace(get_self(), [&](auto &t) {
        t.mine_id = mine_id;
        t.liquidity_id = info->liquidity_id;
        t.account = info->account;

        t.in_eos = info->in_eos;
        t.in_usdt = info->in_usdt;

        t.out_eos = info->out_eos;
        t.out_usdt = info->out_usdt;

        t.profit = info->profit;

        t.liquidity_token = info->liquidity_token;
        t.begin_timestamp = info->timestamp;
        t.end_timestamp = now();
    });

    _market_info.erase(info);

    auto it = _market_log.begin();
    while (it->mine_id < (mine_id - 100)) {
        it = _market_log.erase(it);
    }
}

void onesgame::_marketexit_box(uint64_t liquidity_token, string memo) {
    asset quantity(liquidity_token, BOXL_TOKEN_SYMBOL);
    this->_transfer_to(name(BOX_DEFI_ACCOUNT), name(BOX_LPTOKEN_ACCOUNT).value, quantity, memo);
}

void onesgame::_marketexit_dfs(uint64_t liquidity_token, uint64_t liquidity_id) {
    eosio::action(permission_level{get_self(), "active"_n},
                  eosio::name(DFS_DEFI_ACCOUNT), "withdraw"_n,
                  make_tuple(get_self(), liquidity_id, liquidity_token)).send();
}

void onesgame::_marketclaim_box() {
    eosio::action(permission_level{get_self(), "active"_n},
                  eosio::name(BOX_LPTOKEN_ACCOUNT), "claim"_n,
                  make_tuple(get_self())).send();
}

void onesgame::_handle_box(name from, name to, asset quantity, string memo) {
    if (from == name(BOX_DEFI_ACCOUNT)) {
        if (memo.find("refund") != std::string::npos) {
            _handle_refund(quantity);
        } else if (memo.find("withdraw") != std::string::npos) {
            _handle_withdraw(quantity);
        } else if (memo.find("issue") != std::string::npos) {
            if (this->code == name(BOX_LPTOKEN_ACCOUNT).value) {
                tb_market_info _market_info(get_self(), get_self().value);
                auto info = _market_info.begin();
                eosio_assert(info != _market_info.end(), "mine is not exist");
                _market_info.modify(info, _self, [&](auto &t) {
                    t.liquidity_token = quantity.amount;
                });
            }
        }
    } else if (from == name(BOX_LPTOKEN_ACCOUNT)) {
        if (this->code == name(BOX_TOKEN_ACCOUNT).value) {
            tb_market_info _market_info(get_self(), get_self().value);
            auto info = _market_info.begin();
            eosio_assert(info != _market_info.end(), "mine is not exist");
            _market_info.modify(info, _self, [&](auto &t) { t.profit = quantity; });

            this->_transfer_to(name(ONES_MINE_ACCOUNT), name(BOX_TOKEN_ACCOUNT).value, quantity, "market mine reward");
        }
    }
}

void onesgame::_handle_dfs(name from, name to, asset quantity, string memo) {
    if (from == name(DFS_DEFI_ACCOUNT)) {
        if (memo.find("refund") != std::string::npos) {
            _handle_refund(quantity);
        } else if (memo.find("withdraw") != std::string::npos) {
            _handle_withdraw(quantity);
        }
    } else if (from == name(DFS_TOKEN_ACCOUNT)) {
        if (this->code == name(DFS_TOKEN_ACCOUNT).value) {
            tb_market_info _market_info(get_self(), get_self().value);
            auto info = _market_info.begin();
            eosio_assert(info != _market_info.end(), "mine is not exist");
            _market_info.modify(info, _self, [&](auto &t) { t.profit = quantity; });

            this->_transfer_to(name(ONES_MINE_ACCOUNT), name(DFS_TOKEN_ACCOUNT).value, quantity, "market mine reward");
        }
    }
}

void onesgame::_handle_refund(asset quantity) {
    tb_market_info _market_info(get_self(), get_self().value);
    auto info = _market_info.begin();
    eosio_assert(info != _market_info.end(), "mine is not exist");

    if (this->code == name(USDT_TOKEN_ACCOUNT).value) {
        _market_info.modify(info, _self, [&](auto &t) {
            t.in_usdt -= quantity;
            t.status = 1;
        });
    } else if (this->code == name(EOS_TOKEN_ACCOUNT).value) {
        _market_info.modify(info, _self, [&](auto &t) {
            t.in_eos -= quantity;
            t.status = 1;
        });
    }
}

void onesgame::_handle_withdraw(asset quantity) {
    tb_market_info _market_info(get_self(), get_self().value);
    auto info = _market_info.begin();
    eosio_assert(info != _market_info.end(), "mine is not exist");
    if (this->code == name(USDT_TOKEN_ACCOUNT).value) {
        _market_info.modify(info, _self, [&](auto &t) {
            t.out_usdt = quantity;
            t.status = 2;
        });
    } else if (this->code == name(EOS_TOKEN_ACCOUNT).value) {
        _market_info.modify(info, _self, [&](auto &t) {
            t.out_eos = quantity;
            t.status = 2;
        });
    }
}
extern "C" {
void apply(uint64_t receiver, uint64_t code, uint64_t action) {
    if (action == eosio::name("onerror").value) {
        eosio_assert(code == eosio::name("eosio").value, "onerror action’s are only valid from the eosio");
    }

    if (code == receiver) {
        switch (action) {
            EOSIO_DISPATCH_HELPER( onesgame, (newliquidity)(addliquidity)(subliquidity)(remove)(
                              updateweight)(marketmine)(marketexit)(marketclaim)(marketsettle))
        }
        return;
    }

    if (action == eosio::name("transfer").value) {
        onesgame::code = code;
        execute_action(eosio::name(receiver), eosio::name(receiver),
                       &onesgame::transfer);
        return;
    }

    eosio_exit(0);
}
}