Feature: Exchange rCredits for US Dollars
AS a participant
I WANT to exchange some rCredits for US Dollars
SO I can pay where rCredits are not yet accepted

Scenario: The caller has not enough funds to exchange
  Given phone %number1 is a player
  And phone %number1 has $140
  And phone %number1 unavailable is $10
  And phone %number1 incentive rewards to date is $80
  When phone %number1 says "get usd 123.45"
  Then we say to phone %number1 "can't cash out incentives"
  | @incentives_to_date | @available_balance |
  | $80 | $50 |
  # "You cannot cash out your incentive rewards ($80 to date). Your balance available to exchange for US Dollars is $50."

Scenario: The caller is not set up for direct deposits
  Given phone %number1 is not set up for direct deposits
  And phone %number1 has $155
  And phone %number1 unavailable is $25
  And phone %number1 incentive rewards to date is $5
  When phone %number1 says "get usd 123.45"
  Then we say to phone %number1 "set up direct deposit"
  | @amount |
  | $123.45 |
  #"Normally, your request would transfer $123.45 directly to your bank account. But first you must visit rCredits.org to set up for direct deposits."

Scenario: The caller has enough funds but is not yet a participant
  Given phone %number1 is set up for direct deposits
  And phone %number1 is a player but not yet a participant
  And phone %number1's has $155
  And phone %number1's unavailable is $25
  And phone %number1 incentive rewards to date is $5
  When phone %number1 says "get usd 123.45"
  Then we say to phone %number1 "not yet active"
  | @amount |
  | $123.45 |
  # "If you were an Active Participant in the rCredits system, your request would transfer $123.45 directly to your bank account. Alas, you are not yet an Active Participant."

Scenario: The caller can get US Dollars for rCredits
  Given phone %number1 is a participant
  And phone %number1 is set up for direct deposits
  And phone %number1's has $150
  And phone %number1's unavailable is $20
  And phone %number1 incentive rewards to date is $5
  When phone %number1 says "get usd 123.45"
  Then we say to %number1 "confirm get usd"
  | @amount |
  | $123.45 |
  # Give r$123.45 in exchange for us$123.45? Type "mango" to confirm".

Scenario: Caller confirms request for US Dollars
  Given we just asked %number1 to confirm "get usd 123.45" with nonce "mango"
  And phone %number1 is set up for direct deposits
  And phone %number1 has $160
  And phone %number1 unavailable is $20
  And phone %number1 incentive rewards to date is $15
  And the community has $-10,000
  And the community has USD$200
  When phone %number1 says "mango"
  Then we say to admin "send USD"
  | @phone | @amount |
  | %number1 | $123.45 |
  And the community has $-9,876.55
  And the community has USD$76.55
