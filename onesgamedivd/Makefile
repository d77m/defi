# Makefile for an eosio smart contact

CC=eosio-cpp
Contract=onesgamedivd
Account=onesgamedivd

build:
	@echo "Building"
	$(CC) -abigen $(Contract).cpp -o $(Contract).wasm -I ./
 	#$(CC) $(Contract).cpp -o $(Contract).wasm

clean:
	rm -f *.abi
	rm -f $(Contract).wasm

test: 
	cleos --url=https://jungle3.cryptolions.io set contract $(Account) ../$(Contract) --abi ./$(Contract).abi  -p $(Account)@active

testabi: 
	cleos --url=https://jungle3.cryptolions.io set abi $(Account) ./$(Contract).abi  -p $(Account)@active

deploy:
	cleos --url=https://eospush.tokenpocket.pro set contract $(Account) ../$(Contract) --abi ./$(Contract).abi -p $(Account)@active

deployabi:
	cleos --url=https://eospush.tokenpocket.pro set abi $(Account) ./$(Contract).abi -p $(Account)@active

deployall:
	cleos --url=https://eospush.tokenpocket.pro set contract $(Account) ../$(Contract) --abi ./$(Contract).abi  -p $(Account)@active

unlockmain:
	cleos wallet unlock -n onesmain --password PW5KJP1STVLLTJeNaX9kaQEGnVCHSoAEjLZLvARrktJADMKi1nDRG 

unlocktest:
	cleos wallet unlock -n onestest --password PW5JBAmM8mX3V5NpszdrjASxpmomj3hp1R1wsxxZm2YrJZHc33QSD
	
buyram:
	cleos --url=https://jungle3.cryptolions.io system buyram "${Account}" "${Account}" "10.0000 EOS"

# transfer:
	# cleos --url=https://eospush.tokenpocket.pro push action eosio.token transfer '[ "onesgamedivd", "54321.x", "0.8887 EOS", "bonus" ]' -p onesgamedivd@active
transfer2:
	cleos --url=https://jungle3.cryptolions.io push action eosonestoken transfer '[ "onesgameplay", "onesgamedivd", "2.0000 ONES", "stake" ]' -p onesgameplay@active
transfer3:
	cleos --url=https://jungle3.cryptolions.io push action eosonestoken transfer '[ "onesgamedefi", "onesgamedivd", "5.0000 ONES", "stake" ]' -p onesgamedefi@active
transfer4:
	cleos --url=https://jungle3.cryptolions.io push action eosio.token transfer '[ "onesgamedefi", "onesgamedivd", "5.0000 EOS", "swap divd fee" ]' -p onesgamedefi@active

bonustest:
	cleos --url=https://jungle3.cryptolions.io push action onesgamedivd bonus '[]' -p onesgamedivd@active

init:
	cleos --url=https://jungle3.cryptolions.io push action onesgamedivd init '[]' -p onesgamedivd@active

initmain:
	cleos --url=https://eospush.tokenpocket.pro push action onesgamedivd init '[]' -p onesgamedivd@active

remove:
	cleos --url=https://jungle3.cryptolions.io push action "${Account}" remove '[ "100" ]' -p onesgamehero@active

addcode:
	cleos --url=https://jungle3.cryptolions.io set account permission onesgamedivd active onesgamedivd --add-code

bonus:
	cleos --url=https://eospush.tokenpocket.pro push action onesgamedivd bonus '[]' -p onesgamedivd@active


