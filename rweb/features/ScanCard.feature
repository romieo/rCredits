Feature: Scan Card
AS a non-member
I WANT to scan someone's rCard
SO I can find out about rCredits.
AND
AS a member company
I WANT to have someone scan my Company Agent rCard
SO I can show them my company's rCredits web page.

Setup:
  Given members:
  | id   | fullName   | flags      | cc  | cc2  | address | city | phone        |*
  | .ZZA | Abe One    | ok,bona    | ccA | ccA2 | 1 A St. | Aton | +12000000001 |
  | .ZZB | Bea Two    | ok,bona    | ccB | ccB2 | 2 B St. | Bton | +12000000002 |
  | .ZZC | Corner Pub | ok,co,bona |     |      | 3 C St. | Cton | +12000000003 |
  And relations:
  | id   | main | agent | permission |*
  | :ZZB | .ZZC | .ZZB  | read       |

Scenario: Someone scans a member card
  When member "?" visits page "I/ZZB.ccB"
  Then we redirect to "http://rCredits.org"

Scenario: Someone scans a company agent card
  When member "?" visits page "I/ZZB-ccB2"
  Then we show "Corner Pub" with:
#  |_Address  |_phone        | _Button |*
#  | Cton     | 200.000.0003 | Pay     |
  |_Address  |_phone        |
  | Cton     | 200 000 0003 |

Scenario: Someone scans an old member card
  When member "?" visits page "I/NEW.ZZB-ccB"
  Then we redirect to "http://rCredits.org"

Scenario: Someone scans an old company agent card
  When member "?" visits page "I/NEW-ZZB-ccB2"
  Then we show "Corner Pub" with:
#  |_Address  |_phone        | _Button |*
#  | Cton     | 200.000.0003 | Pay     |
  |_Address  |_phone        |
  | Cton     | 200 000 0003 |
