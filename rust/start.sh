#!/bin/bash

source "$HOME/.cargo/env" 
export RUST_BACKTRACE=1

sleep 1
cargo run --release
