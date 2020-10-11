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

unlockmain:
	cleos wallet unlock -n onesmain --password PW5KJP1STVLLTJeNaX9kaQEGnVCHSoAEjLZLvARrktJADMKi1nDRG 

unlocktest:
	cleos wallet unlock -n onestest --password PW5JBAmM8mX3V5NpszdrjASxpmomj3hp1R1wsxxZm2YrJZHc33QSD
	
buyram:
	cleos --url=https://jungle3.cryptolions.io system buyram "${Account}" "${Account}" "10.0000 EOS"

transfertest:
	cleos --url=https://jungle3.cryptolions.io push action eosio.token transfer '[ "onesgamedefi", "onesgameplay", "1.0000 EOS", "swap,1,1" ]' -p onesgameplay@active

luckydraw:
	cleos --url=https://jungle3.cryptolions.io push action eosio.token transfer '[ "onesgamehero", "onesgamedefi", "101.0000 EOS", "luckydraw" ]' -p onesgamehero@active

remove:
	cleos --url=https://jungle3.cryptolions.io push action "${Account}" remove '[ "100" ]' -p onesgamehero@active

addcode:
	cleos --url=https://jungle3.cryptolions.io set account permission onesgamedefi active onesgamedefi --add-code

deposit:
	cleos --url=https://jungle3.cryptolions.io push action eosio.token transfer '[ "onesgamehero", "onesgamedefi", "1.0000 EOS", "0x5e05b0402348a44f256b3343" ]' -p onesgamehero@active

withdraw:
	cleos --url=https://jungle3.cryptolions.io push action "${Account}" withdraw '[ "onesgamehero",1,2000 ]' -p onesgamehero@active


upgrade:
	cleos --url=https://eos.newdex.one push action "${Account}" upgrade '[  ]' -p onesgameplay@active

marketexit:
	cleos --url=https://eos.newdex.one push action "${Account}" marketexit '[ "17", 433]' -p onesgameplay@active

marketclaim:
	cleos --url=https://eos.newdex.one push action "${Account}" marketclaim '[  ]' -p onesgameplay@active

marketsettle:
	cleos --url=https://eos.newdex.one push action "${Account}" marketsettle '[  ]' -p onesgameplay@active

marketminebox:
	cleos --url=https://eos.newdex.one push action "${Account}" marketmine '[ "swap.defi",1,12,1 ]' -p onesgameplay@active

marketminedfs:
	cleos --url=https://eos.newdex.one push action "${Account}" marketmine '[ "defisswapcnt",1,17,1 ]' -p onesgameplay@active

# transfer:
	# cleos --url=https://eos.newdex.one push action tethertether transfer '[ "onesgamedefi", "woaichiyouzi", "12.0850 USDT", "refund" ]' -p onesgamedefi@active


# mutli
	# cleos --url=https://jungle3.cryptolions.io set account permission onesgamedefi owner '{"threshold":21,"keys":[],"accounts":[{"permission":{"actor":"onesgameplay","permission":"owner"},"weight":20},{"permission":{"actor":"tethertether","permission":"owner"},"weight":1},{"permission":{"actor":"onesgametest","permission":"owner"},"weight":1}],"waits":[]}' -p onesgamedefi@owner