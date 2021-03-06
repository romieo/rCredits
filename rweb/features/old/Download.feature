Feature: Download
AS a member
I WANT to download my transactions
SO I can see what happened and possbily integrate with an accounting program.

Setup:
  Given members:
  | id   | fullName | floor | acctType      | flags                        |
  | .ZZA | Abe One  | -100  | %R_PERSONAL   | dft,ok,person,bona         |
  | .ZZB | Bea Two  | -200  | %R_PERSONAL   | dft,ok,person,company,bona |
  | .ZZC | Our Pub  | -300  | %R_COMMERCIAL | dft,ok,company,bona          |
  And relations:
  | id   | main | agent | permission |
  | .ZZA | .ZZA | .ZZB  | buy        |
  | .ZZB | .ZZB | .ZZA  | read       |
  | .ZZC | .ZZC | .ZZB  | buy        |
  | .ZZD | .ZZC | .ZZA  | sell       |
  And balances:
  | id   | usd   |
  | ctty | 10000 |
  | .ZZA |  1000 |
  | .ZZB |  2000 |
  | .ZZC |  3000 |
  And transactions: 
  | xid | created   | type     | state    | amount | r   | from | to   | purpose  | taking |
  |   1 | %today-7m | signup   | done     |    250 | 250 | ctty | .ZZA | signup   | 0      |
  |   2 | %today-6m | signup   | done     |    250 | 250 | ctty | .ZZB | signup   | 0      |
  |   3 | %today-6m | signup   | done     |    250 | 250 | ctty | .ZZC | signup   | 0      |
  |   4 | %today-5m | transfer | done     |     10 |  10 | .ZZB | .ZZA | cash E   | 0      |
  |   5 | %today-4m | transfer | done     |    100 |  20 | .ZZC | .ZZA | usd F    | 1      |
  |   6 | %today-3m | transfer | done     |    240 |  40 | .ZZA | .ZZB | what G   | 0      |
  |   7 | %today-3m | rebate   | done     |      2 |   2 | ctty | .ZZA | rebate   | 0      |
  |   8 | %today-3m | bonus    | done     |      4 |   4 | ctty | .ZZB | bonus    | 0      |
  |   9 | %today-3w | transfer | pending  |    100 | 100 | .ZZA | .ZZB | pie N    | 1      |
  |  10 | %today-3w | rebate   | pending  |      5 |   5 | ctty | .ZZA | rebate   | 0      |
  |  11 | %today-3w | bonus    | pending  |     10 |  10 | ctty | .ZZB | bonus    | 0      |
  |  12 | %today-2w | transfer | pending  |    100 | 100 | .ZZC | .ZZA | labor M  | 0      |
  |  13 | %today-2w | rebate   | pending  |      5 |   5 | ctty | .ZZC | rebate   | 0      |
  |  14 | %today-2w | bonus    | pending  |     10 |  10 | ctty | .ZZA | bonus    | 0      |
  |  15 | %today-2w | transfer | done     |     50 |   5 | .ZZB | .ZZC | cash P   | 0      |
  |  16 | %today-1w | transfer | done     |    120 |  80 | .ZZA | .ZZC | this Q   | 1      |
  |  17 | %today-1w | rebate   | done     |      4 |   4 | ctty | .ZZA | rebate   | 0      |
  |  18 | %today-1w | bonus    | done     |      8 |   8 | ctty | .ZZC | bonus    | 0      |
  |  19 | %today-6d | transfer | pending  |    100 | 100 | .ZZA | .ZZB | cash T   | 0      |
  |  20 | %today-6d | transfer | pending  |    100 | 100 | .ZZB | .ZZA | cash U   | 1      |
  |  21 | %today-6d | transfer | done     |    100 |   0 | .ZZA | .ZZB | cash V   | 0      |
  |  22 | %today-5d | transfer | denied   |    100 |  40 | .ZZC | .ZZA | labor CA | 0      |
  |  23 | %today-5d | rebate   | denied   |      2 |   2 | ctty | .ZZC | rebate   | 0      |
  |  24 | %today-5d | bonus    | denied   |      4 |   4 | ctty | .ZZA | bonus    | 0      |
  |  25 | %today-5d | transfer | denied   |      5 |   2 | .ZZA | .ZZC | cash CE  | 1      |
  |  26 | %today-5d | transfer | disputed |     80 |  20 | .ZZA | .ZZC | this CF  | 1      |
  |  27 | %today-5d | rebate   | disputed |      1 |   1 | ctty | .ZZA | rebate   | 0      |
  |  28 | %today-5d | bonus    | disputed |      2 |   2 | ctty | .ZZC | bonus    | 0      |
  |  29 | %today-5d | transfer | deleted  |    200 |  50 | .ZZA | .ZZC | USD nope | 1      |
  |  30 | %today-5d | transfer | disputed |    100 |  80 | .ZZC | .ZZA | cash CJ  | 1      |
  Then balances:
  | id   | r    | usd   | rewards |
  | ctty | -771 | 10000 |       0 |
  | .ZZA |  227 |   700 |     257 |
  | .ZZB |  279 |  2255 |     254 |
  | .ZZC |  265 |  3045 |     260 |
  When cron runs ""
  Then balances:
  | id   | r       | usd      | rewards    |
  | ctty | -772.75 | 10000.00 |          0 |
  | .ZZA |  227.50 |   699.50 |     257.50 |
  | .ZZB |  279.50 |  2254.50 |     254.50 |
  | .ZZC |  265.75 |  3044.25 |     260.75 |
  And transactions:
  | xid | created   | type     | state    | amount | r    | from | to   | purpose                 |
  |  31 | %today    | refund   | done     |      0 | 0.25 | ctty | .ZZA | Dwolla fee (reimbursed) |
  |  32 | %today    | refund   | done     |      0 | 0.25 | ctty | .ZZB | Dwolla fee (reimbursed) |
  |  33 | %today    | refund   | done     |      0 | 0.25 | ctty | .ZZC | Dwolla fee (reimbursed) |
  |  34 | %today    | refund   | done     |      0 | 0.25 | ctty | .ZZC | Dwolla fee (reimbursed) |
  |  35 | %today    | refund   | done     |      0 | 0.25 | ctty | .ZZB | Dwolla fee (reimbursed) |
  |  36 | %today    | refund   | done     |      0 | 0.25 | ctty | .ZZC | Dwolla fee (reimbursed) |
  |  37 | %today    | refund   | done     |      0 | 0.25 | ctty | .ZZA | Dwolla fee (reimbursed) |

Scenario: A member downloads transactions for the past year
  When member ".ZZA" visits page "transactions/period=365&download=1&options=%RUSD_BOTH%STATES_BOTH%_N%_N%_N%_XCH%_VPAY"
  Then we download "rcredits%todayn-12m-%todayn.csv" with:
  # For example rcredits20120525-20130524.csv
  | t# | Created | Name    | From you | To you | rCredits | USD     | Status   | Purpose    | Reward | Net  |
  | 17 | %ymd    | %ctty   |     0.25 |        |        0 |   -0.25 | done     | Dwolla fee (reimbursed) |   0.25 |    0 |
  | 16 | %ymd    | %ctty   |     0.25 |        |        0 |   -0.25 | done     | Dwolla fee (reimbursed) |   0.25 |    0 |
  | 15 | %ymd-5d | Our Pub |          |    100 |       80 |      20 | disputed | cash CJ    |        |  100 |
  | 13 | %ymd-5d | Our Pub |       80 |        |      -20 |     -60 | disputed | this CF    |      1 |  -79 |
  | 11 | %ymd-5d | Our Pub |          |    100 |       40 |      60 | denied   | labor CA   |      4 |  104 |
  | 10 | %ymd-6d | Bea Two |      100 |        |        0 |    -100 | done     | cash V     |        | -100 |
  | 9  | %ymd-6d | Bea Two |          |    100 |      100 |       0 | pending  | cash U     |        |  100 |
  | 8  | %ymd-6d | Bea Two |      100 |        |     -100 |       0 | pending  | cash T     |        | -100 |
  | 7  | %ymd-1w | Our Pub |      120 |        |      -80 |     -40 | done     | this Q     |      4 | -116 |
  | 6  | %ymd-2w | Our Pub |          |    100 |      100 |       0 | pending  | labor M    |     10 |  110 |
  | 5  | %ymd-3w | Bea Two |      100 |        |     -100 |       0 | pending  | pie N      |      5 |  -95 |
  | 4  | %ymd-3m | Bea Two |      240 |        |      -40 |    -200 | done     | what G     |      2 | -238 |
  | 3  | %ymd-4m | Our Pub |          |    100 |       20 |      80 | done     | usd F      |        |  100 |
  | 2  | %ymd-5m | Bea Two |          |     10 |       10 |       0 | done     | cash E     |        |   10 |
  | 1  | %ymd-7m | %ctty   |          |        |        0 |       0 | done     | signup     |    250 |  250 |
  |    |         | TOTALS  |   740.50 |    510 |       10 | -240.50 |          |            | 276.50 |   46 |
  And with download columns:
  | column |
  | Date   |

Scenario: A member downloads completed transactions for the past year
  When member ".ZZA" visits page "transactions/period=365&download=1&options=%RUSD_BOTH%STATES_DONE%_N%_N%_N%_XCH%_VPAY"
  Then we download "rcredits%todayn-12m-%todayn.csv" with:
  | Tx#      | t# | Created | Name    | From you | To you | rCredits | USD | Status   | Purpose | Reward | Net  |
  | NEW-AABL | 17 | %ymd    | %ctty   |     0.25 |        |    0 |   -0.25 | done     | Dwolla fee (reimbursed) |   0.25 |    0 |
  | NEW-AABF | 16 | %ymd    | %ctty   |     0.25 |        |    0 |   -0.25 | done     | Dwolla fee (reimbursed) |   0.25 |    0 |
  | NEW-AABE | 15 | %ymd-5d | Our Pub |          |    100 |   80 |      20 | disputed | cash CJ |        |  100 |
  | NEW-AABA | 13 | %ymd-5d | Our Pub |       80 |        |  -20 |     -60 | disputed | this CF |      1 |  -79 |
  | NEW-AAAV | 10 | %ymd-6d | Bea Two |      100 |        |    0 |    -100 | done     | cash V  |        | -100 |
  | NEW-AAAQ | 7  | %ymd-1w | Our Pub |      120 |        |  -80 |     -40 | done     | this Q  |      4 | -116 |
  | NEW-AAAG | 4  | %ymd-3m | Bea Two |      240 |        |  -40 |    -200 | done     | what G  |      2 | -238 |
  | NEW-AAAF | 3  | %ymd-4m | Our Pub |          |    100 |   20 |      80 | done     | usd F   |        |  100 |
  | NEW-AAAE | 2  | %ymd-5m | Bea Two |          |     10 |   10 |       0 | done     | cash E  |        |   10 |
  | NEW-AAAB | 1  | %ymd-7m | %ctty   |          |        |    0 |       0 | done     | signup  |    250 |  250 |
  |          |    |         | TOTALS  |   540.50 |    210 |  -30 | -300.50 |          |         | 257.50 |  -73 |
