<?php
/**
 * @file
 * Specialized rCredits settings for this rCredits server
 */

define('R_PILOT', TRUE); // pilot phase
define('ADMIN_NAME', 'William Spademan');
define('SIGNATORY', 'William Spademan, President');
define('R_REGION_NAME', 'rCredits Western Mass');
define('R_REGION_LEGALNAME', 'rCredits Western Massachusetts Region');
define('R_SERVER_ID', 'NEW'); // qid header for this region's server (New England / Western MA)
// other 3rd letters: M=ME H=NH V=VT C=CT R=RI P=PA B=Boston
define('R_REGION_SHORTNAME', R_SERVER_ID . '.');
define('R_REGION_ADDR', 'c/o CGF, PO Box 21, Ashfield, MA 01330');
define('R_REGION_PHONE', '+1 413 628 1723'); // was 376.5021 (and maybe again someday)
define('R_TESTER_EMAIL', 'wspademan@gmail.com'); // tester's personal email
define('R_REGION_EMAIL', strpos($_SERVER['HTTP_HOST'], '.') < 3 ? str_replace('@', '_cgf@', R_TESTER_EMAIL) : (strtolower(R_SERVER_ID) . '@rCredits.org'));
define('R_REGION_EIN', '46-1821792');
define('R_SITE_URL', 'https://' . (isPRODUCTION ? PRODUCTION : STAGE));

// (you don't get an ID card unless you are in this geographic area)
define('R_REGION_POSTAL_AREA', '^01.*'); // anyone with a postal code that matches this regular expression is in the region
date_default_timezone_set('America/New_York'); // server handles just one timezone
//define('DW_LAG', 1 * HOUR_SECS); // no longer used (Dwolla uses greenwich mean time, our PHP adjusts)
define('R_COUNTRY_ID', US_COUNTRY_ID); // this region's country: United States
define('R_STATE_ID', 1020); // Massachusetts
define('R_REBATE', 10); // standard initial rebate percentage
define('R_BONUS_FACTOR', 1); // standard bonus is this times standard rebate
define('R_MAX_REBATE', 500); // cap on rebate for a single transaction (max bonus is in proportion)
define('R_XFEE_CARD', .03); // percentage fee for exchanges of USD for rCredits using a credit/debit card
define('R_XFEE_CHECK', 3.00); // flat fee for exchanges of USD for rCredits with a check
define('R_MAX_AMOUNT', 100000); // transaction request size limit (some smaller limits also apply)
define('R_SIGNUP_GIFT', 5); // UNUSED how much to give people to play with, even before they are fully set up
define('R_SIGNUP_BONUS', 20); // how much people start with at this time
define('R_HELPER_BONUS', 20); // how much you get for inviting and assisting someone with account opening
define('R_COUNTED_BONUS', 10); // how much you get for each member employee of a company you invite/assist
define('R_FLOOR', 0); // how low can user's balance go, by default
define('R_DAILY_HOUR', isDEV ? '00' : '01'); // what time (2-digit 24-hour clock hour) to start intensive daily maintenance tasks
define('R_WEEKLY_DAY', 'Sunday'); // what day of the week to do weekly tasks
define('R_MONTHLY_DAY', '01'); // what day of the month to do monthly tasks (2-digits)
define('R_FEE_FOR_CASH', FALSE); // UNUSED does customer pay the Dwolla fee when trading rCredits for cash?
define('R_DEBT_FEE', 25); // fee for overdue reserve credit
define('R_DEBT_PERCENT', 1.5); // percentage fee for overdue reserve credit
define('R_DEBT_DAYS', 30); // how long to pay back reserve credit
define('R_STATEMENT_COST', 10); // how much member must pay (in rCredits dollars) for monthly and annual statements by mail
define('R_INFLATION_RATE', .05); // annual USD inflation rate (but never less than zero) (extra big is okay)
define('R_LAUNCH', strtotime('12/15/2013'));
define('R_ADMIN_QID', 'NEW.AAA'); // William's account code
define('R_USDTX_DAYS', 5); // how many days it takes for a transfer to clear from bank to rCredits account
define('ESCROW_ACCOUNT', '1326532739'); // escrow account number
define('ESCROW_ROUTING', '211070175'); // escrow account routing number
define('R_MAX_TX_REWARD', 500); // cap on incentive rewards for a single transaction