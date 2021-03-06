Feature: Transact
AS a member
I WANT to transfer rCredits from my account to someone else's or vice versa
SO I will get a rebate or bonus that I can spend once I am an active participant

Setup:
  Given members:
  | id   | fullName   | number | flags      |*
  | .ZZA | Abe One    | +20001 | ok,bona  |
  | .ZZB | Bea Two    | +20002 | ok,bona  |
  | .ZZC | Corner Pub | +20003 | co,ok,bona |
  # later and elsewhere: name, country, minimum, community
  And relations:
  | id   | main | agent | permission |*
  | :ZZA | .ZZC | .ZZB  | buy        |
  | :ZZB | .ZZC | .ZZA  | sell       |
  And transactions: 
  | created   | type   | amount | from | to   | purpose |*
  | %today-1d | signup | 250    | ctty | .ZZA | signup  |
  | %today-1d | signup | 250    | ctty | .ZZB | signup  |
  | %today-1d | signup | 250    | ctty | .ZZC | signup  |

Scenario: The caller asks to pay a member id
  When phone +20001 says "123.45 to .ZZB for pie"
  Then we say to phone +20001 "confirm payment|please confirm" with subs:
  | amount  | otherName |*
  | $123.45 | Bea Two    |
  # "Pay Bea Two $123.45 for goods and services? Type MANGO to confirm."

Scenario: The caller asks to pay a player by name
  When phone +20001 says "123.45 to cornerpub for groceries"
  Then we say to phone +20001 "confirm payment|please confirm" with subs:
  | amount  | otherName |*
  | $123.45 | Corner Pub |
  # "Pay Corner Pub $123.45 for goods and services? Type MANGO to confirm."

Scenario: The caller asks to charge a member id
  When phone +20001 says "123.45 from .ZZC for labor"
  Then we say to phone +20001 "confirm charge|please confirm" with subs:
  | amount  | otherName |*
  | $123.45 | Corner Pub |
  # "Charge Corner Pub $123.45 for goods and services? Type MANGO to confirm."

Scenario: The caller confirms a payment
  When phone +20001 confirms "100 to .ZZC for groceries"
  Then the community has r$-765
  And phone +20003 has r$360
  And we say to phone +20001 "report transaction" with subs:
  | did    | otherName  | amount | rewardType | rewardAmount  | balance | tid |*
  | paid   | Corner Pub | $100   | reward     | $5            | $155    | 2   |
  # "You paid Corner Pub $100 (rebate: $5). Your new balance is $155. Transaction #2"

Scenario: The caller confirms a request to charge
  When phone +20001 confirms "100 from .ZZC for labor"
  Then phone +20003 has r$250
  And phone +20001 has r$250
  And we say to phone +20001 "report invoice" with subs:
  | did     | otherName | amount | tid |*
  | charged | Corner Pub | $100   | 2   |
  # "You charged Corner Pub $100 (bonus: $10). Your balance is unchanged, pending payment. Invoice #2"
  And we notice "new invoice" to member "c@" with subs:
  | created | fullName   | otherName | amount | purpose |*
  | %today  | Corner Pub | Abe One   | $100   | labor   |

Scenario: The caller requests a payment with insufficient balance
  When phone +20001 says "300 to .ZZC for groceries"
  And we say to phone +20001 "short to" with subs:
  | short |*
  | $50   |

Scenario: The caller asks to pay the latest invoice
  Given invoices:
  | created   | status      | amount | from | to   | purpose|*
  | %today-3d | %TX_PENDING | 100    | .ZZC | .ZZA | labor  |
  And the expected nonce for phone +20003 is "NONCY"
  When phone +20003 says "pay"
  Then we say to phone +20003 "confirm pay invoice|please confirm" with subs:
  | amount | otherName | created | nonce |*
  | $100   | Abe One   | %dmy-3d | NONCY |
  # "Pay Abe One $100 for goods and services (invoice 14-May-2013)? Type MANGO to confirm."

Scenario: The caller confirms payment of the latest invoice
  Given invoices:
  | created   | status      | amount | from | to   | purpose|*
  | %today-3d | %TX_PENDING | 100    | .ZZC | .ZZA | labor  |
  When phone +20003 confirms "pay"
  Then the community has r$-765
  And phone +20001 has r$360
  And we say to phone +20003 "report transaction" with subs:
  | did   | otherName | amount | rewardType | rewardAmount | balance | tid |*
  | paid  | Abe One    | $100   | rebate      | $5            | $155    | 3   |
  # "You paid Abe One $100 (rebate: $5). Your new balance is $55. Transaction #3" (#3 not #2, because the [deleted] invoice was #2)

Scenario: The caller asks to pay the latest invoice from a particular member
  Given invoices:
  | created   | status      | amount | from | to   | purpose|*
  | %today-3d | %TX_PENDING | 100    | .ZZC | .ZZA | labor  |
  | %today-1d | %TX_PENDING | 100    | .ZZC | .ZZB | labor  |
  When phone +20003 says "pay .ZZA"
  Then we say to phone +20003 "confirm pay invoice|please confirm" with subs:
  | amount | otherName | created   |*
  | $100   | Abe One    | %today-3d |
  # "Pay Abe One $100 for goods and services (invoice 14-May-2013)? Type MANGO to confirm."

Scenario: The caller asks to pay a company agent
  When phone +20001 confirms "123.45 to :ZZA for pie"
  Then we say to phone +20001 "confirm payment|please confirm" with subs:
  | amount  | otherName |*
  | $123.45 | Corner Pub |
