# Makefile for an eosio smart contact

CC=eosio-cpp
Contract=onesgamedefi
Account=onesgamedefi

build:
	@echo "Building"
	$(CC) -abigen $(Contract).cpp -o $(Contract).wasm -I ./
 	#$(CC) $(Contract).cpp -o $(Contract).wasm

clean:
	rm -f *.abi
	rm -f $(Contract).wasm

test: 
	cleos --url=https://jungle3.cryptolions.io set contract $(Account) ../$(Contract) --abi ./$(Contract).abi  -p $(Account)@active
	# cleos --url=https://jungle3.cryptolions.io set contract $(Account) ../$(Contract) --abi ./$(Contract).abi  -p $(Account)@active

testabi: 
	cleos --url=https://jungle3.cryptolions.io set abi $(Account) ./$(Contract).abi  -p $(Account)@active


upgradetest:
	cleos --url=https://jungle3.cryptolions.io push action "${Account}" upgrade '[  ]' -p onesgamedefi@active

deploy:
	cleos --url=https://eos.newdex.one set contract $(Account) ../$(Contract) --abi ./$(Contract).abi -p $(Account)@active

deployabi:
	cleos --url=https://eos.newdex.one set abi $(Account) ./$(Contract).abi -p $(Account)@active

deployall:
	cleos --url=https://mainnet.meet.one set contract $(Account) ../$(Contract) --abi ./$(Contract).abi  -p $(Account)@active

buyram:
	cleos --url=https://jungle3.cryptolions.io system buyram "${Account}" "${Account}" "10.0000 EOS"

transfer:
	cleos --url=https://jungle3.cryptolions.io push action eosio.token transfer '[ "onesgamehero", "onesgamedefi", "1.0000 EOS", "swap,1,1" ]' -p onesgamehero@active

addcode:
	cleos --url=https://jungle3.cryptolions.io set account permission onesgamedefi active onesgamedefi --add-code

