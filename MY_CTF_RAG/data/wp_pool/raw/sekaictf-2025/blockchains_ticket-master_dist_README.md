# blockchains_ticket-master_dist_README

# TON CTF Framework

## Project structure

- `contracts` - source code of all the smart contracts of the project and their dependencies.
- `tests` - integration test that allows you to test your exploit locally.
- `checker` - server running on remote that will test your exploit and client that will compile and submit your exploit.

## How to solve the challenge

Your goal is to exploit the challenge contract (`contracts/service_counter.fc`) to make it set the solver to your account.

You need to write your exploit in `contracts/exploit.fc`. In this contract, you will receive a message containing the service counter address, the coin minter address, and the ticket minter address.

### Install dependencies

`yarn install`

### Test exploit locally

`npm run test` or `yarn test`

### Submit exploit to remote server

`npm run submit http://remote.example.com:1337` or `yarn submit http://remote.example.com:1337`
