Feature: Statistics
AS a member
I WANT accurate, up-to-date system statistics
SO I can see how well the rCredits system is doing for myself, for my ctty, and for the world.
#   r floor rewards usd minimum maximum 
#   signup rebate bonus inflation grant loan fine maxRebate
#   balance (-r) demand (minimum - r)

Setup:
  Given members:
  | id   | fullName   | email | flags         | minimum | floor | share | created   |
  | .ZZA | Abe One    | a@    | ok,dw,bona    |       5 |     0 |    10 | %today-2m |
  | .ZZB | Bea Two    | b@    | ok,dw,bona    |    1000 |   -20 |    20 | %today-3w |
  | .ZZC | Corner Pub | c@    | ok,dw,co,bona |    2000 |    10 |    30 | %today-2w |
  And relations:
  | id   | main | agent | permission |
  | .ZZA | .ZZA | .ZZB  | buy        |
  | .ZZB | .ZZB | .ZZA  | read       |
  | .ZZC | .ZZC | .ZZB  | buy        |
  | .ZZD | .ZZC | .ZZA  | sell       |
  And usd transfers:
  | txid | payer | payee | amount | completed |
  |  100 | .ZZA  |     0 |  -1000 | %today-3d |
  |  101 | .ZZB  |     0 |  -2000 | %today-4d |
  |  102 | .ZZC  |     0 |  -3050 | %today-5d |
  |  103 | .ZZC  |     0 |     50 | %today-2d |
  And balances:
  | id   | usd   |
  | ctty |     0 |
  | .ZZA |  1000 |
  | .ZZB |  2000 |
  | .ZZC |  3000 |
  And transactions: 
  | xid | created   | type      | state   | amount | from | to   | purpose | goods |
  |   1 | %today-4m | signup    | done    |    250 | ctty | .ZZA | signup  | 0     |
  |   2 | %today-4m | signup    | done    |    250 | ctty | .ZZB | signup  | 0     |
  |   3 | %today-4m | signup    | done    |    250 | ctty | .ZZC | signup  | 0     |
  |   4 | %today-3m | transfer  | done    |     10 | .ZZB | .ZZA | cash E  | 0     |
  |   5 | %today-3m | transfer  | done    |    100 | .ZZC | .ZZA | usd F   | 0     |
  |   6 | %today-3m | transfer  | done    |    240 | .ZZA | .ZZB | what G  | 1     |
  |   9 | %today-3m | transfer  | pending |    100 | .ZZA | .ZZB | pie N   | 1     |
  |  12 | %today-2m | transfer  | pending |    100 | .ZZC | .ZZA | labor M | 1     |
  And statistics get set "%tomorrow-1m"
  And transactions: 
  | xid | created   | type      | state   | amount | from | to   | purpose | goods | channel  |
  |  15 | %today-2w | transfer  | done    |     50 | .ZZB | .ZZC | p2b     | 1     | %TX_WEB  |
  |  18 | %today-1w | transfer  | done    |    120 | .ZZA | .ZZC | this Q  | 1     | %TX_WEB  |
  |  21 | %today-6d | transfer  | pending |    100 | .ZZA | .ZZB | cash T  | 0     | %TX_WEB  |
  |  22 | %today-6d | transfer  | pending |    100 | .ZZB | .ZZA | cash U  | 0     | %TX_WEB  |
  |  23 | %today-6d | transfer  | done    |    100 | .ZZA | .ZZB | cash V  | 0     | %TX_WEB  |
  |  24 | %today-2d | inflation | done    |      1 | ctty | .ZZA | inflate | 0     | %TX_WEB  |
  |  25 | %today-2d | inflation | done    |      2 | ctty | .ZZB | inflate | 0     | %TX_WEB  |
  |  26 | %today-2d | inflation | done    |      3 | ctty | .ZZC | inflate | 0     | %TX_WEB  |
  |  27 | %today-2d | grant     | done    |      4 | ctty | .ZZA | grant   | 0     | %TX_WEB  |
  |  28 | %today-2d | loan      | done    |      5 | ctty | .ZZB | loan    | 0     | %TX_WEB  |
  |  29 | %today-2d | fine      | done    |      6 | .ZZC | ctty | fine    | 0     | %TX_WEB  |
  |  30 | %today-1d | transfer  | done    |    100 | .ZZC | .ZZA | payroll | 1     | %TX_WEB  |
  |  33 | %today-1d | transfer  | done    |      1 | .ZZC | .AAB | sharing rewards with CGF | 1 | %TX_CRON |
  And usd transfers:
  | completed | amount | payer | payee |
  | %today-1d |     10 | .ZZA  | ctty  |
  | %today-1d |    100 | .ZZC  | .ZZB  |
  Then balances:
  | id   | r       | usd   | rewards | committed |
  | ctty | -845.65 |    10 |    0.00 |         0 |
  | .ZZA |   43.00 |   990 |  279.00 |      2.80 |
  | .ZZB |  463.50 |  2100 |  278.50 |      5.30 |
  | .ZZC |  338.05 |  2900 |  275.05 |      6.62 |
  | .AAB |    1.10 |     0 |    0.10 |         0 |
  # total rewards < total r, because we made a grant, a loan, and a fine.
  When cron runs ""
  # causes coverFee() to run
  Then balances:
  | id   | r       | usd      | rewards |
  | ctty | -845.90 |    10.00 |    0.00 |
  | .ZZA |   43.00 |   990.00 |  279.00 |
  | .ZZB |  463.75 |  2099.75 |  278.50 |
  | .ZZC |  338.05 |  2900.00 |  275.05 |
  | .AAB |    1.10 |     0.00 |    0.10 |
  
Scenario: cron calculates the statistics
  Given cron runs "stats"
  When member ".ZZA" visits page "community"
  Then we show "Statistics" with:
  | | for %R_REGION_NAME |
  | Accounts        | 5 (3 personal, 2 companies) � up 5 from a month ago |
  | rCredits issued | $835.90r � up $835.90r from a month ago |
  | | signup: $750r, inflation adjustments: $6r, rebates/bonuses: $70.90r, grants: $4r, loans: $5r, fees: -$6r |
  | Demand          | $5,999.75 � up $5,999.75 from a month ago |
  | Total funds     | $835.90r + $5,999.75us = $6,835.65 |
  | | including about $6,564.65 in savings = 109.4% of demand (important why?) |
  | Banking / mo    | $6,050us (in) - $50us (out) - $0.25 (fees) = +$5,999.75us (net) |
  | Purchases / mo  | 4 ($271) / mo = $54.20 / acct |
  | p2p             | 0 ($0) / mo = $0 / acct |
  | p2b             | 2 ($170) / mo = $56.67 / acct |
  | b2b             | 1 ($1) / mo = $0.50 / acct |
  | b2p             | 1 ($100) / mo = $50 / acct |
  | Velocity        | 4.0% per month |
