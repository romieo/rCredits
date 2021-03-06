Feature: Undo Transaction
AS a managing agent for an rCredits member company
I WANT to undo the last transaction completed on the POS device I am using
SO I can easily correct a mistake made by another company agent or by me

Summary:
  An agent asks to undo a charge
  An agent asks to undo a refund
  An agent asks to undo a cash in payment
  An agent asks to undo a cash out charge
  An agent asks to undo a charge, with insufficient balance  
  An agent asks to undo a refund, with insufficient balance
  
Setup:
  Given members:
  | id   | fullName   | email | city  | state | cc  | cc2  | rebate | flags      |*
  | .ZZA | Abe One    | a@    | Atown | AK    | ccA | ccA2 |      5 | ok,bona    |
  | .ZZB | Bea Two    | b@    | Btown | UT    | ccB | ccB2 |      5 | ok,bona    |
  | .ZZC | Corner Pub | c@    | Ctown | CA    | ccC |      |     10 | ok,co,bona |
  | .ZZD | Dee Four   | d@    | Dtown | DE    | ccD | ccD2 |      5 | ok,bona    |
  | .ZZE | Eve Five   | e@    | Etown | IL    | ccE | ccE2 |      5 | ok,bona,secret |
  | .ZZF | Far Co     | f@    | Ftown | FL    | ccF |      |      5 | ok,co,bona |
  And devices:
  | id   | code |*
  | .ZZC | devC |
  And selling:
  | id   | selling         |*
  | .ZZC | this,that,other |
  And company flags:
  | id   | flags        |*
  | .ZZC | refund,r4usd |
  And relations:
  | id   | main | agent | permission |*
  | :ZZA | .ZZC | .ZZA  | scan       |
  | :ZZB | .ZZC | .ZZB  | refund     |
  | :ZZD | .ZZC | .ZZD  | read       |
  | :ZZE | .ZZF | .ZZE  | sell       |
  And transactions: 
  | xid | created   | type     | amount | from | to   | purpose      | taking |*
  | 1   | %today-7m | signup   |    250 | ctty | .ZZA | signup       |      0 |
  | 2   | %today-6m | signup   |    250 | ctty | .ZZB | signup       |      0 |
  | 3   | %today-6m | signup   |    250 | ctty | .ZZC | signup       |      0 |
  Then balances:
  | id   | balance |*
  | ctty |    -750 |
  | .ZZA |     250 |
  | .ZZB |     250 |
  | .ZZC |     250 |

#Variants: with/without an agent
#  | ".ZZA" asks device "devC" | ".ZZC" asks device "codeC" | ".ZZA" $ | ".ZZC" $ | # member to member (pro se) |
#  | ".ZZB" asks device "devC" | ".ZZB" asks device "codeC" | ".ZZA" $ | ".ZZC" $ | # agent to member           |
#  | ".ZZA" asks device "devC" | ".ZZC" asks device "codeC" | ".ZZA" $ | ".ZZC" $ | # member to agent           |
#  | ".ZZB" asks device "devC" | ".ZZB" asks device "codeC" | ".ZZA" $ | ".ZZC" $ | # agent to agent            |

Scenario: An agent asks to undo a charge
  Given transactions: 
  | xid | created   | type     | amount | from | to   | purpose      | taking |*
  | 4   | %today-1d | transfer |     80 | .ZZA | .ZZC | whatever     |      1 |
  | 5   | %today-1d | rebate   |      4 | ctty | .ZZA | rebate on #2 |      0 |
  | 6   | %today-1d | bonus    |      8 | ctty | .ZZC | bonus on #2  |      0 |
  When agent ":ZZB" asks device "devC" to undo transaction 4
  Then we respond ok txid 7 created %now balance 250 rewards 250
  And with message "report undo|report tx|reward" with subs:
  | solution | did      | otherName | amount | why                | rewardAmount |*
  | reversed | refunded | Abe One   | $80    | goods and services | $-8          |
  And with did ""
  And with undo "4"
  And we notice "new refund|reward other" to member ".ZZA" with subs:
  | created | otherName  | amount | payerPurpose | otherRewardAmount |*
  | %today  | Corner Pub | $80    | reverses #2  | $-4               |

Scenario: An agent asks to undo a charge when balance is secret
  Given transactions: 
  | xid | created   | type     | amount | from | to   | purpose      | taking |*
  | 4   | %today-6m | signup   |    250 | ctty | .ZZE | signup       |      0 |
  | 5   | %today-1d | transfer |     80 | .ZZE | .ZZC | whatever     |      1 |
  | 6   | %today-1d | rebate   |      4 | ctty | .ZZE | rebate on #2 |      0 |
  | 7   | %today-1d | bonus    |      8 | ctty | .ZZC | bonus on #2  |      0 |
  When agent ":ZZB" asks device "devC" to undo transaction 5
  Then we respond ok txid 8 created %now balance "*250" rewards 250
  And with message "report undo|report tx|reward" with subs:
  | solution | did      | otherName | amount | why                | rewardAmount |*
  | reversed | refunded | Eve Five  | $80    | goods and services | $-8          |
  And with did ""
  And with undo "5"
  And we notice "new refund|reward other" to member ".ZZE" with subs:
  | created | otherName  | amount | payerPurpose | otherRewardAmount |*
  | %today  | Corner Pub | $80    | reverses #2  | $-4               |

Scenario: An agent asks to undo a refund
  Given transactions: 
  | xid | created   | type     | amount | from | to   | purpose      | taking |*
  | 4   | %today-1d | transfer |    -80 | .ZZA | .ZZC | refund       |      1 |
  | 5   | %today-1d | rebate   |     -4 | ctty | .ZZA | rebate on #2 |      0 |
  | 6   | %today-1d | bonus    |     -8 | ctty | .ZZC | bonus on #2  |      0 |
  When agent ":ZZB" asks device "devC" to undo transaction 4
  Then we respond ok txid 7 created %now balance 250 rewards 250
  And with message "report undo|report tx|reward" with subs:
  | solution | did        | otherName | amount | why                | rewardAmount |*
  | reversed | re-charged | Abe One   | $80    | goods and services | $8           |
  And with did ""
  And with undo "4"
  And we notice "new charge|reward other" to member ".ZZA" with subs:
  | created | otherName  | amount | payerPurpose | otherRewardAmount |*
  | %today  | Corner Pub | $80    | reverses #2  | $4                |

Scenario: An agent asks to undo a cash-out charge
  Given transactions: 
  | xid | created   | type     | amount | from | to   | purpose  | goods | taking |*
  | 4   | %today-1d | transfer |     80 | .ZZA | .ZZC | cash out |     0 |      1 |
  When agent ":ZZB" asks device "devC" to undo transaction 4
  Then we respond ok txid 5 created %now balance 250 rewards 250
  And with message "report undo|report tx" with subs:
  | solution | did      | otherName | amount | why         |*
  | reversed | credited | Abe One   | $80    | other money |
  And with did ""
  And with undo "4"
  And we notice "new payment" to member ".ZZA" with subs:
  | created | fullName | otherName  | amount | payeePurpose |*
  | %today  | Abe One  | Corner Pub | $80    | reverses #2  |

Scenario: An agent asks to undo a cash-in payment
  Given transactions: 
  | xid | created   | type     | amount | from | to   | purpose | goods | taking |*
  | 4   | %today-1d | transfer |    -80 | .ZZA | .ZZC | cash in |     0 |      1 |
  When agent ":ZZB" asks device "devC" to undo transaction 4
  Then we respond ok txid 5 created %now balance 250 rewards 250
  And with message "report undo|report tx" with subs:
  | solution | did        | otherName | amount | why         |*
  | reversed | re-charged | Abe One   | $80    | other money |
  And with did ""
  And with undo "4"
  And we notice "new charge" to member ".ZZA" with subs:
  | created | fullName | otherName  | amount | payerPurpose |*
  | %today  | Abe One  | Corner Pub | $80    | reverses #2  |

Scenario: An agent asks to undo a charge, with insufficient balance  
  Given transactions: 
  | xid | created   | type     | amount | from | to   | purpose      | goods | taking |*
  | 4   | %today-1d | transfer |     80 | .ZZA | .ZZC | whatever     |     2 |      1 |
  | 5   | %today-1d | rebate   |      4 | ctty | .ZZA | rebate on #2 |     0 |      0 |
  | 6   | %today-1d | bonus    |      8 | ctty | .ZZC | bonus on #2  |     0 |      0 |
  | 7   | %today    | transfer |    300 | .ZZC | .ZZB | labor        |     0 |      0 |
  When agent ":ZZB" asks device "devC" to undo transaction 4
  Then we respond ok txid 8 created %now balance 250 rewards 250
  And with message "report undo|report tx|reward" with subs:
  | solution | did      | otherName | amount | why                | rewardAmount |*
  | reversed | refunded | Abe One   | $80    | goods and services | $-8          |
  And with did ""
  And with undo "4"
  And we notice "new refund|reward other" to member ".ZZA" with subs:
  | created | otherName  | amount | payerPurpose | otherRewardAmount |*
  | %today  | Corner Pub | $80    | reverses #2  | $-4               |
  And balances:
  | id   | balance |*
  | ctty |    -750 |
  | .ZZA |     250 |
  | .ZZB |     550 |
  | .ZZC |     -50 |

Scenario: An agent asks to undo a refund, with insufficient balance  
  Given transactions: 
  | xid | created   | type     | amount | from | to   | purpose      | goods | taking |*
  | 4   | %today-1d | transfer |    -80 | .ZZA | .ZZC | refund       |     2 |      1 |
  | 5   | %today-1d | rebate   |     -4 | ctty | .ZZA | rebate on #2 |     0 |      0 |
  | 6   | %today-1d | bonus    |     -8 | ctty | .ZZC | bonus on #2  |     0 |      0 |
  | 7   | %today    | transfer |    300 | .ZZA | .ZZB | labor        |     0 |      0 |
  When agent ":ZZB" asks device "devC" to undo transaction 4
  Then we respond ok txid 8 created %now balance -50 rewards 250
  And with message "report undo|report tx|reward" with subs:
  | solution | did        | otherName | amount | why                | rewardAmount |*
  | reversed | re-charged | Abe One   | $80    | goods and services | $8           |
  And with did ""
  And with undo "4"
  And we notice "new charge|reward other" to member ".ZZA" with subs:
  | created | otherName  | amount | payerPurpose | otherRewardAmount |*
  | %today  | Corner Pub | $80    | reverses #2  | $4                     |
  And balances:
  | id   | balance |*
  | ctty |    -750 |
  | .ZZA |     -50 |
  | .ZZB |     550 |
  | .ZZC |     250 |

Scenario: An agent asks to undo a charge, without permission
  Given transactions: 
  | xid | created   | type     | amount | from | to   | purpose      | goods | taking |*
  | 4   | %today-1d | transfer |     80 | .ZZB | .ZZC | whatever     |     2 |      1 |
  | 5   | %today-1d | rebate   |      4 | ctty | .ZZB | rebate on #2 |     0 |      0 |
  | 6   | %today-1d | bonus    |      8 | ctty | .ZZC | bonus on #2  |     0 |      0 |
  When agent ":ZZA" asks device "devC" to undo transaction 4
  Then we return error "no perm" with subs:
  | what    |*
  | refunds |

Scenario: An agent asks to undo a refund, without permission
  Given transactions: 
  | xid | created   | type     | amount | from | to   | purpose      | goods | taking |*
  | 4   | %today-1d | transfer |    -80 | .ZZB | .ZZC | refund       |     2 |      1 |
  | 5   | %today-1d | rebate   |     -4 | ctty | .ZZB | rebate on #2 |     0 |      0 |
  | 6   | %today-1d | bonus    |     -8 | ctty | .ZZC | bonus on #2  |     0 |      0 |
  When agent ":ZZD" asks device "devC" to undo transaction 4
  Then we return error "no perm" with subs:
  | what  |*
  | sales |

Scenario: An agent asks to undo a non-existent transaction
  When agent ":ZZA" asks device "devC" to undo transaction 99
  Then we respond ok txid 0 created "" balance 250 rewards 250
  And with did ""
  And with undo ""

Scenario: A cashier reverses a transaction with insufficient funds
  Given transactions: 
  | xid | created   | type  | amount | from | to   | purpose |*
  | 4   | %today-1m | grant |    100 | ctty | .ZZC | jnsaqwa |
  And agent ":ZZB" asks device "devC" to charge ".ZZA-ccA" $-100 for "cash": "cash in" at "%now-1hour" force 0
  Then transactions: 
  | xid | created | type     | amount | from | to   | purpose |*
  | 5   | %today  | transfer |   -100 | .ZZA | :ZZB | cash in |
  Given transactions:
  | xid | created | type     | amount | from | to   | purpose |*
  | 6   | %today  | transfer |      1 | .ZZA | .ZZB | cash    |
  When agent ":ZZB" asks device "devC" to charge ".ZZA-ccA" $-100 for "cash": "cash in" at "%now-1hour" force -1
  Then we respond ok txid 7 created %now balance 249 rewards 250
  And with undo "5"
  And we notice "new charge" to member ".ZZA" with subs:
  | created | fullName | otherName  | amount | payerPurpose |*
  | %today  | Bea Two  | Corner Pub | $100   | reverses #2  |
  And balances:
  | id   | balance |*
  | ctty |    -850 |
  | .ZZA |     249 |
  | .ZZB |     251 |
  | .ZZC |     350 |
  