<?php
namespace rCredits\Web; // typically abbreviated as "rWeb"
use rCredits\Backend as be;
use rCredits\Util as u;
use rCredits\Testing as t;
use rCredits as r;

/**
 * @file
 * Forms for managing transactions, and related functions
 */

define('TX_OPT_KEYS' , 'agents channels boxes exchanges pexchanges');
define('TX_URL_OPTS', 'currency states ' . TX_OPT_KEYS);
define('TX_OPT_AGENTS', 1);
define('TX_OPT_CHANNELS', 2);
define('TX_OPT_BOXES', 3);
define('TX_OPT_EXCHANGES', 4);
define('TX_OPT_PEXCHANGES', 5);

define('STATES_DONE', 0); // show just completed transactions
define('STATES_PENDING', 1); // show just open invoices
define('STATES_BOTH', 2); // show both completed and pending transactions

define('_N', 0); // used in tests
define('_XCH', 1);
define('_VPAY', 1);

define('RUSD_R', 0);
define('RUSD_USD', 1);
define('RUSD_BOTH', 2); // show both rCredits (0) and USD (1)

/**
 * List Transactions
 */
function formTxs($form, &$sta, $args = '') {
  global $agtViaBox;
  $agtViaBox = t('Agent is who did the transaction on your behalf.|Channel (Via) is where the transaction originated -- what type of device: Web, Smartphone, SMS, or Automatic.|Box is where the transaction originated -- which specific device.');
  $optHelp = $agtViaBox . '|' . t('Exchanges are 1-to-1 trades of US Dollars for rCredits (usually automatic).|PExchanges are daily automatic payment exchanges of rCredits between company and employee. The company pays the employee in US Dollars, then the employee trades back some US Dollars 1-to-1 for rCredits. The net effect is that the employee gets paid partly in US Dollars, partly in rCredits.');

  global $base_url;
  $mya = r\acct();
  
  extract(u\just('options period starting ending do xid download downloaded variety', $args)); 
  $report = $mya->can(B_CTTY_ADMIN) ? (@$variety == 'stats') : FALSE; // no community reports yet for non-admins
  if (!$report and @$do) if ($result = handleTx($do, $xid, $sta, $args)) return $result;

  $page = $report ? 'stats' : 'transactions';
  if (!@$period) $period = TX_DEFAULT_PERIOD;
  if (strlen(@$options) != count($urlOpts = u\ray(TX_URL_OPTS))) $options = $mya->person() ? '2200001' : '2210001'; // defaults for TX_OPT_KEYS options (2=both)
  $options = array_combine($urlOpts, str_split($options));
//  u\setDft($states, STATES_BOTH); // both  
//  u\setDft($currency, 2); // both
  if (@$download) $options['currency']  = RUSD_BOTH; // always both for download
  extract($options);
  $options = join('', array_values($options));
  
  $optDefaults = $opts = array();
  $optHelp = array_combine($keys = u\ray(TX_OPT_KEYS), u\ray($optHelp));
  foreach ($keys as $one) {
    $value = $opts[u\consta('tx_opt', $one)] = popHelp($one, $optHelp[$one]); // set TX_OPT_CHANNELS etc.
    if (@$$one) $optDefaults[] = $value; // set up option field values
  }
  list ($classes, $header, $downloadHeader) = txFields($report, @$download, @$agents, @$channels, @$boxes);
 
  $txDays = array('1' => 'today', '4' => '4 days', '30' => '30 days', '90' => '90 days', '180' => '6 months', '365' => '12 months', '-1' => 'Year to date');
  list ($starting, $ending) = txDates($txDays, @$period, @$starting, @$ending);
  $url0 = @"$base_url/$page/period=$period&starting=$starting&ending=$ending&options=$options&variety=$variety";

  $maxRows = @$download ? PHP_INT_MAX : TX_MAX_ROWS; // don't overwhelm the screen
  if (@$download) {
    $csv = setupDownload($starting, $ending);
    csv($csv, $downloadHeader);
    $totDone = $totPending = array_combine($classes, array_fill(0, count($classes), FALSE));
  } else $csv = FALSE;

  // build the transaction list
  $dones = ($states != STATES_PENDING);
  $pendings = ($states != STATES_DONE);
  $txOpts = compact(u\ray('dones pendings exchanges pexchanges'));
  $rows = array();
  
  foreach (u\ray('bankTransfers myTransactions') as $func) { // transfers first (usually not many)
    $func = "rCredits\\Backend\\$func";
    $result = $func($starting, $ending, $report ? 'ctty' : $mya->id, $txOpts);
    while (count($rows) < $maxRows) {
      if (!$tx = $result->fetchAssoc()) break;
      $tot = $tx['completed'] ? 'totDone' : 'totPending';
      $rows[] = doRow($tx, $report, $csv, $classes, @$agents, @$channels, @$boxes, $currency, $url0, $$tot);
    }
  }
  if ($tx and $result->fetchAssoc()) say('transaction list truncated'); // there was more than we could show
  if (@$download) {
    if ($states == STATES_BOTH) { // get proper totals in $totDone
      foreach ($totPending as $key => $value) if ($totDone[$key] !== FALSE) $totDone[$key] += $value;
    } elseif ($states == STATES_PENDING) $totDone = $totPending;
    
    u\splice($totDone, 'date', 1, u\ray('txid created date', FALSE, FALSE, FALSE));
    u\splice($totDone, 'buttons', 1); // don't download buttons
    u\splice($totDone, 'fromyou', 0, u\ray('frombank', 0));
    $totDone['name'] = 'TOTALS';
    foreach(u\just(TX_MONEY_FIELDS, $totDone) as $key => $value) $totDone[$key] = round($value, 2);
    //if ($value === FALSE) $totDone[$key] = '-';
    csv($csv, array_values($totDone)); // not STDOUT
    if (t\est()) return; else exit();
  }

  // prepare screen display
  $classes = u\prefixValues('tx-', $classes);
  $attributes = array('id' => 'txlist');
  $cols = array();
  foreach ($classes as $class) $cols[] = compact('class');
  $colgroups = array($cols);
  $caption = $sticky = '';
  $empty = tt('no txs');
  $list = theme_table(compact(u\ray('header rows attributes caption colgroups sticky empty')));
  $transactions = item("<div id=\"txs-title\">TRANSACTIONS</div>$list <br>&nbsp;");
  
  if ($report) {
    $agents = $summary = NULL;
  } else $summary = formField('markup', '', '', array('markup' => txSummary($starting, $ending, $currency)));

  $radios = array(t('rCredits'), t('USD'), t('both'));
  $currency = hidField(2); //formField('radios', t('Currency:'), '', dft($currency), $radios); 
  $radios = array(t('completed'), t('open invoices'), t('both'));
  $states = formField('radios', t('Types:'), '', dft($states), $radios);
  $opts2Desc = TRUE ? '' : t('Agent is who did the transaction on your behalf. Channel ("Via") is Web, Smartphone, SMS, or Automatic. Exchanges are 1-to-1 trades of rCredits for US Dollars. PExchanges are 1-to-1 trades that earn incentive rewards.');
  $opts2 = boxesField('opts', t('Show:'), $opts2Desc, $optDefaults, $opts);
  
  $divider = item('<hr>');
  $period = formField('select', t('For the past:'), '', dft($period), $txDays);
  $submitPeriod = submit(t('Show ')); // label has to be different from submitDates (see below)
  $downloadPeriod = submit(t('Download '));
  $starting = textField(t('OR Starting:'), '', dft(u\formatDate($starting)));
  $ending = textField(t('Ending:'), '', dft(u\formatDate($ending)));
  $submitDates = submit(t(' Show')); // label has to be different (Drupal bug -- else can't tell which is clicked)
  $downloadDates = submit(t(' Download'));
  $title = "";
  $title = item(t('Transaction History'));
//  $subtext = item(t('The "r%" column tells what percentage of the total transaction amount is rCredits. Hover over that figure to see the breakdown.'));

  $form = compact(u\ray('title subtext currency states opts2 divider period submitPeriod downloadPeriod starting ending submitDates downloadDates summary transactions'));
  if (@$downloaded) js("window.open(\"$url0&download=1\")", 'inline', 'footer');
  return labeled($form);
}

function setupDownload($starting, $ending) {
  global $testCSV, $testCSVname;
  $testCSV = array();
  header('Content-type: text/plain');
  $dt1 = strftime('%Y%m%d', $starting);
  $dt2 = strftime('%Y%m%d', $ending);
  $testCSVname = $filename = "rcredits$dt1-$dt2.csv";
  header("Content-Disposition: attachment; filename=\"$filename\"");
  return fopen('php://output', 'w');
}

/**
 * Return classes and headers for the appropriate set of fields.
 */
function txFields($report, $download, $agents, $channels, $boxes) {
  global $agtViaBox;
  $headHelp = t('Transaction number|Date transaction was initiated or completed|Payer name|Payee name|Transaction amount|What percentage of the amount transferred was rCredits (hover over a percentage to see the breakdown)|rCredits amount|US Dollars amount|Current state of transaction -- check mark means completed, pending means awaiting approval by payer.|What is the purpose of the transaction (usually the item or service purchased)|How much incentive rewards were awarded for this transaction|Who did you pay or receive from|Amounts you spent|Amounts you received|Click on a symbol to approve, deny, or reverse a transaction or to edit the purpose|The net change in account balance|') . $agtViaBox;
  $classes = 'tid date from to amount rpct r usd state purpose reward';
  $headHelp = array_combine(u\ray($classes . ' name fromyou toyou buttons net agent channel box'), u\ray($headHelp));
  if ($report) { // community transaction history
    $headers = t('#,Date,From,To,Total,r%,rCredits,USD,Status,Purpose,Rewards');
  } else { // individual account transaction history
    $classes = 'tid date name fromyou toyou rpct r usd state buttons purpose reward net';
    $headers = t('#,Date,Name,From you,To you,r%,rCredits,USD,Status,Action,Purpose,Reward,Net');
  }

  $header = array_combine(u\ray($classes), u\ray($headers)); // make splices and additions easy and clear
  
  if (@$agents) $header['agent'] = t('Agent'); // (not applicable for reports)
  if (@$channels) $header['channel'] = t('Via');
  if (@$boxes) $header['box'] = t('Box');

  if (@$download) { // downloading -- modify standard individual or community field list
    $downloadHeader = $header;
    u\splice($downloadHeader, 'buttons', 1); // don't download buttons
    u\splice($downloadHeader, 'date', 0, u\ray('txid created', t('Tx#'), t('Created'))); // insert fields
    u\splice($downloadHeader, 'fromyou', 0, u\ray('frombank', t('From Bank')));
  }

  foreach ($header as $key => $value) $header[$key] = popHelp($value, $headHelp[$key]);
  
  return array(array_keys($header), array_values($header), @array_values($downloadHeader));
}

/**
 * Prepare one transaction row for output (or download).
 * @param assoc $tx: the transaction record
 * @param file $csv: the (open) download file or FALSE if not downloading
 *
 * @return: the row
 */
function doRow($tx, $report, $csv, $classes, $agents, $channels, $boxes, $currency, $url0, &$tot) {
  global $base_url, $rUrl;
  $mya = r\acct();
  $myid = $mya->id;
  $txChannels = u\ray(TX_CHANNELS);

  extract($tx, EXTR_PREFIX_ALL, 'tx');
  $xid = $tx_xid;

  if ($banking = ($tx_type == TX_BANK)) {
    $tx_purpose = ($tx_amount < 0 ? t('from') : t('to')) . ' ' . t('bank account');
    if ($tx_amount < 0) list ($tx_amount, $tx_payee) = array(-$tx_amount, $myid);
  } else $data = unserialize($tx_data);
  
  $is_reward = ($tx_type != TX_TRANSFER and $tx_type != TX_BANK);
  $r = number_format(@$tx_r, 2);
  $total = number_format($tx_amount, 2);
  $usd = number_format($tx_amount - $r, 2);
  $rpct = $tx_amount == 0 ? R_NONE : number_format(100 * $r / $tx_amount, 1);
  if ($rpct == '100.0') $rpct = '100'; // keep the field narrow (4 chars max)
  $rpct = sprintf('<span title="$%sr + $%sus">%s</span>', $r, $usd, $rpct);
  $date = @$tx_completed ?: $tx_created;
  $amount = $currency == RUSD_R ? $r : ($currency == RUSD_USD ? $usd : $total);
  
  if ($report) { // community transaction history
    $reward = $banking ? 0 : (@$data['bonus'] + @$data['rebate']);
    txNeatAmounts($amount, $reward, $is_reward);
    $state = txColoredState($tx_state);
    $tid = $xid; // for community use the actual transaction numbers
    $payeeName = $banking ? '' : r\acct($tx_payee)->fullName;
    $row = array($tid, strftime('%d-%b-%Y', $date), r\acct($tx_payer)->fullName, $payeeName, $total, $rpct, $amount, $usd, $state, $tx_purpose, $reward);
  } else { // individual account transaction history
    $url = $url0 . "&xid=$xid";
    u\setDft($toMe, $tx_payee == $myid);
    $byMe = $banking ?: (@$tx_taking == $toMe);
    $tid = $toMe ? $tx_payeeTid : @$tx_payerTid;
    $agent = $toMe ? @$tx_payeeAgent : @$tx_payerAgent;
    $state = txColoredState($tx_state, $byMe);
    $reward = $banking ? 0 : @$data[$toMe ? 'bonus' : 'rebate'];
    $net = number_format(($toMe ? $total : -$total) + $reward, 2);
    txNeatAmounts($amount, $reward, $is_reward);
    list ($toYou, $fromYou) = $toMe ? array($amount, R_NONE) : array(R_NONE, $amount);
    if ($total + 0 == 0) $toYou = $fromYou = "<span class=\"virtual\">$r</span>"; // make payment exchanges clear
    if ($tx_type == TX_REFUND) list ($reward, $toYou) = array($toYou, R_NONE);
    $button1 = (!$byMe and okableState($tx_state)) ? button('OK', "$url&do=ok", txAct('ok', $tx_state, $toMe, $byMe)) : '';
    $button2 = ((!$is_reward or $mya->can(B_CTTY_ADMIN)) and !@$data['undoneBy']) ? button(' X ', "$url&do=no", txAct('no', $tx_state, $toMe, $byMe)) : '';
    $button3 = ($mya->can(@$tx_taking ? B_SELL : B_BUY) and !$is_reward) ? "<a href='$url&do=edit' title='Edit Purpose'><img src='$rUrl/images/icons/edit.png' width=20 height=20 border=0 /></a>" : '';
    $buttons = ($banking or $toYou == $fromYou) ? '' : "<div class='txs-buttons'><div>$button1</div><div>$button2</div><div>$button3</div></div>";
    $row = array($tid, strftime('%d-%b', $date), @$tx_name, $fromYou, $toYou, $rpct, $amount, $usd, $state, $buttons, $tx_purpose, $reward, $net);
  }

  if (@$agents) $row[] = (!@$agent or $agent == $myid or $agent < 0) ? R_NONE 
  : ($csv ? r\quid($agent) : str_replace(R_SERVER_ID . R_MEMBER_MARK, R_MEMBER_MARK, r\quid($agent)));
  // might optionally show both agents for reports someday
  if (@$channels) $row[] = $txChannels[$tx_channel];
  if (@$boxes) $row[] = r\dbLookup('boxName', 'r_boxnames', 'id=:tx_box', compact('tx_box'));

  return $csv ? finishCsvRow($csv, array_combine($classes, $row), $tx, $amount, $r, $usd, $toMe, $tot) : $row;
}

function finishCsvRow($csv, $row, $tx, $amount, $r, $usd, $toMe, &$tot) {
  extract(u\just('xid type created completed state', $tx));

  $banking = ($type == TX_BANK);
  $txid = u\n2a(abs($xid), -4); // world-unique transaction id
  $created = strftime('%Y-%m-%d', $created);
  if ($banking) {
    $frombank = $toMe ? $amount : -$amount;
    $row['fromyou'] = $row['toyou'] = '';
  } else {
    $frombank = '';
    $txid = R_SERVER_ID . '-' . $txid;
  }
  
  u\splice($row, 'date', 0, compact(u\ray('txid created')));
  u\splice($row, 'buttons', 1); // don't download buttons
  u\splice($row, 'fromyou', 0, compact('frombank'));

  $date = @$completed ? strftime('%Y-%m-%d', $completed) : '';
  $txStates = u\ray(TX_STATES);
  $state = $txStates[$state - TX_STATE0]; // transaction state (human-readable)

  if ($amount == R_NONE) {
    $r = ''; // showing refunds and other incentives (other than rebate/bonus) as rewards
  } elseif (!$toMe) {$r = '-' . $r; $usd = '-' . $usd;}
  foreach (u\ray('date state r usd') as $key) $row[$key] = $$key; // don't compact()+$row (maintain order!)
///  if ($xid == 37) debug(compact('row','amount','r','usd','toMe','tot'));
  
  foreach ($row as $key => $value) $row[$key] = $value == R_NONE ? '' : strip_tags($value);
  if ($row['fromyou'] == $row['toyou']) $row['fromyou'] = $row['toyou'] = ''; // don't report gross on payex
  foreach (u\ray(TX_MONEY_FIELDS) as $key) {
    if (@$row[$key]) $tot[$key] = ($tot[$key] ?: 0) + $row[$key]; // formatting value is useless
  }
  csv($csv, array_values($row)); // not STDOUT
  return TRUE;
}

/**
 * Return a form to confirm the transaction operation.
 * @param string $do: the action to confirm
 * @param int $xid: record id of transaction to confirm an operation on
 * @param assoc $sta: Drupal's status array (gets member 'confirm' set)
 * @return: the form (FALSE on error or if already confirming)
 */
function txConfirm($do, $xid, &$sta) {
  $mya = r\acct();
  $myid = $mya->id;
  if (t\est() and @$sta['confirm']) return FALSE;
  if (confirming_v($sta)) return FALSE;
  if (!$details = be\lastTx('xid=:xid', compact('xid'), TRUE)) return 'bad tx id'; // bail on hack attempt
  $subs = r\txReportArgs($mya, $details);
  $summary = u\tt('tx summary', $subs);
  extract(u\just('state toMe byMe', $subs));
  $txAction = txAct($do, $state, $toMe, $byMe);
  confirm(u\tt('tx summary|confirm tx action', $subs + compact('txAction')), $sta); // sets 'confirm'
  return sureForm($sta);
}

function handleTx($do, $xid, &$sta, $args) {
  if (!$xid = (int) @$xid) return FALSE;
  if ($do == 'edit') return tx_edit($xid);
  if ($do == 'ok' or $do == 'no') {
    if ($confirm = txConfirm($do, $xid, $sta)) return $confirm;
    if (!$sta['rebuild']) txOKorNO($do, $xid); // don't do anything if canceling
    $url = str_replace('&do=', '&did=', "transactions/$args"); // don't do it again
    return r\go($url); // back to the normal form
  }
  return FALSE;
}
 
function formTxs_validate($form, &$sta) {
  $txDays = @$form['period']['#options'];
  extract(u\just('period starting ending opts', $sta['input']));
  $op = op($sta);
  list ($starting, $ending, $period) = ($op == 'submitPeriod' or $op == 'downloadPeriod') ? txDates($txDays, @$period) : txDates($txDays, '', @$starting, @$ending);
  u\preray(compact(u\ray('starting ending period')), $sta['input']);
}

function formTxs_submit($form, &$sta, $page = 'transactions') {
  if (confirming_s($sta)) return;
  extract(u\just('currency states period starting ending opts options do xid', $sta['input']));
  if (isset($states)) { // $options summarizes everything when clicking on OK or NO or EDIT
    $options = $currency . $states;
    foreach (u\ray(TX_OPT_KEYS) as $one) $options .= (@$opts[u\consta('TX_OPT', $one)] ? 1 : 0);
  }
  $url = @"$page/period=$period&starting=$starting&ending=$ending&options=$options";
  $op = op($sta);
  if (u\abbreviates('download', $op)) {
    say('downloading');
    if ($currency != 2) say('download always both');
    $url .= '&downloaded=1';    
  }
  $sta['redirect'] = $url;
}

/**
 * Return usable starting and ending dates.
 * @param assoc $txDays: descriptions of time periods relative to now, for example "[the past] 30 days"
 * @param int $period: index into $txDays -- for calculating starting and ending if none explicitly specified
 * @param int $starting: explicit starting date (optional)
 * @param int $ending: explicit ending date (optional)
 * @return array($starting, $ending, $period), where $starting is adjusted to start of day and $ending to end of day
 */
function txDates($txDays, $period, $starting = '', $ending = '') {
  if (@$starting and !is_numeric($starting) and !$starting = strtotime($starting)) say('bad date', 'starting');
  if (@$starting > REQUEST_TIME) say('beyond today', 'starting');
  if (@$ending and !is_numeric($ending) and !$ending = strtotime($ending)) say('bad date', 'ending');
  if (!@$ending or $ending > REQUEST_TIME) $ending = REQUEST_TIME;

  $period = (int) @$starting ? ($ending - $starting) / DAY_SECS : (@$period ?: TX_DEFAULT_PERIOD);
  $ago = @$txDays[$period] ?: ($period . ' days');
  if (!@$starting) $starting = strtotime($period == '-1' ? '1jan' : "$ago ago");
  return array(strtotime('today', $starting), strtotime('tomorrow', $ending) - 1, $period);
}  

function txNeatAmounts(&$amount, &$reward, $is_reward) {
  $reward = $reward ? number_format($reward, 2) : R_NONE;
  if ($is_reward) list ($amount, $reward) = array(R_NONE, $amount); // show refunds, inflation, grants, fines, and signup bonus as rewards
}
function txColoredState($state, $sayPending = TRUE) {
  $states = array(TX_DONE => '&#10004;', TX_PENDING => 'ok?', TX_DENIED => 'denied', TX_DISPUTED => 'disputed');
  $colors = array('&#10004;' => 'green', 'ok?' => 'orange', 'pending' => 'darkgreen', 'denied' => 'red', 'disputed' => 'red');
  $state = ($state == TX_PENDING and $sayPending) ? 'pending' : $states[$state];
  $color = $colors[$state];
  return "<span style='color:$color;'>$state</span>";
}

/**
 * Return the action option for a transaction
 * (This has to match what be\undoTx() actually does.)
 * @param string $do: which major type of action ('ok' or 'no')
 * @param int $state: the current transaction state
 * @param bool $toMe: user is the payee
 * @param bool $byMe: user originated the transaction
 * @return string: how to describe the ok or no
 */
function txAct($do, $state, $toMe, $byMe) {
  if ($do == 'ok') return $toMe ? t('ACCEPT this payment') : t('APPROVE this charge');
  
  $state += 0; // otherwise string != int
  if ($toMe) {
    if ($state == TX_DISPUTED) return t('REVERSE this disputed charge'); // let the customer win
    if ($state == TX_PENDING) return $byMe ? t('CANCEL this invoice') : t('REFUSE this payment');
    return t('REVERSE this charge'); // let the customer win
  } else {
    if ($state == TX_PENDING) return $byMe ? t('CANCEL this payment') : t('DECLINE to pay this invoice');
    return $byMe ? t('REVERSE this payment') : t('DISPUTE this charge');
  }
}

/**
 * Handle click on 'OK' or 'NO' (approving or undoing a transaction)
 */
function txOKorNO($task, $xid) {
  $myid = r\acct()->id;
  if (!$result = be\lastTx('xid=:xid', compact('xid'), TRUE)) return FALSE; // bail on hack attempt
  extract(u\just('state amount payer toMe', $result));

  if ($task == 'ok') { // OK
    u\EXPECT($state == TX_PENDING or $state == TX_DISPUTED, "bad state for #$xid: $state");
    if ($toMe) { // accepting payment exchange offer (UNUSED)
      u\EXPECT(FALSE, 'payment offer');
      r\setTxState(TX_DONE, $xid); // accept the payment
      list ($message, $args) = array('payment accepted', compact('amount'));
    } elseif ($state != TX_PENDING) { // already done (probably disputed), so just change the state
      r\setTxState(TX_DONE, $xid); // accept the charge
      $who = r\acct($payer)->fullName;
      list ($message, $args) = array('charge accepted', compact('who'));
    } else list ($message, $args) = be\payInvoice($xid);
  } else list ($message, $args) = be\undoTx($xid); // NO
  say($message, $args);
}

/**
 * Return a form to edit the given transaction.
 * @param int $xid: the transaction id
 * @return the form (FALSE on error)
 */
function tx_edit($xid) {
  if (!$details = be\lastTx('xid=:xid', compact('xid'), TRUE)) return FALSE; // bail on hack attempt
  extract(u\just('taking purpose role', r\txReportArgs(r\acct(), $details)));
  
  if (!r\acct()->can($taking ? B_SELL : B_BUY)) return FALSE;
  
  $form = array(
    'title' => formField('item', t('Edit Description')),
//    'review' => formField('item', $summary),
    'purpose' => textField(t('Purpose'), '', dft($purpose)),
    'go' => formField('submit', NULL, t('Submit button'), 'Update'),
    'xid' => hidField($xid),
    'field' => hidField($role . 'For'),
    
    '#validate' => array('rCredits\\Web\\tx_edit_validate'),
    '#submit' => array('rCredits\\Web\\tx_edit_submit'),
  );
  return $form;
}

function tx_edit_validate($form, &$sta) {
}

function tx_edit_submit($form, &$sta) {
  extract($sta['input']);
  r\setTxField($field, $purpose, $xid);
  $sta['redirect'] = 'transactions';
}

function txBal($bal, $plus = '', $minus = '- ') {return ($bal < 0 ? $minus : $plus) . '$' . number_format(abs($bal), 2);}

/**
 * Return the transaction summary table
 * @param int $starting: starting time (the start of some day)
 * @param int $ending: ending time (the end of some day)
 */
function txSummary($starting, $ending, $currency) {
  $mya = r\acct();
  $myid = $mya->id;
  $previous_ending = $starting - 1;
  extract($nowInfo = (array) be\creditInfo(array('asof' => $ending)), EXTR_PREFIX_ALL, 'n');
  extract($pendingInfo = (array) be\creditInfo(u\ray('pending asof', TRUE, $ending)), EXTR_PREFIX_ALL, 'p');
  extract($oldInfo = (array) be\creditInfo(array('asof' => $previous_ending)), EXTR_PREFIX_ALL, 'o');
///  debug(compact('nowInfo','oldInfo'));

  $dif = array();
  foreach (u\ray('inProper outProper rInProper rOutProper usdIn usdOut rewards bank fees') as $key) $dif[$key] = $nowInfo[$key] - $oldInfo[$key];
  extract($dif, EXTR_PREFIX_ALL, 'd');
  $p_committed = $mya->committed; // this will include all pending commitments
  
  $fields = 'old bank in out fees rewards new';
  $values =
    ($currency == 2 ? array($o_balance, $d_bank, $d_inProper, $d_outProper, $d_fees, $d_rewards, $n_balance) // both
  : ($currency == 0 ? array($o_r, 0, $d_rInProper, $d_rOutProper, 0, $d_rewards, $n_r) // r
  : ($currency == 1 ? array($o_usd, $d_bank, $d_usdIn, $d_usdOut, $d_fees, 0, $n_usd) : ''))); // usd
  $values = array_combine(u\ray($fields), $values);

  $p_balance = $p_bank + $p_rInProper - $p_rOutProper + $p_rewards - $p_committed;
  $values += compact(u\ray('p_inProper p_bank p_outProper p_rewards p_committed p_balance'));
  foreach ($values as $key => $value) $$key = number_format($value, 2);
 
  $p_balance = txBal($p_balance, '+ '); // add +/-/$ to these, as appropriate
  $new = txBal($new);
  $old = txBal($old);

  $starting = u\formatDate($starting);
  $ending = u\formatDate($ending);
  $summary_title = "<div id='txsum-title'>SUMMARY</div>";
  
  $classes = u\prefixValues('txsum-', u\ray('one banksign bank fromsign fromyou tosign toyou rewardssign rewards sharesign share balancesign balance'));
  $heads = u\ray(t('Starting balance,,From Bank,,From You,,To You,,Rewards,,To CGF,,Ending balance'));
  $headHelp = u\ray(t('Your account balance at the start of this time period||How much was transferred from your bank account to your rCredits account||How much you spent||How much you received||How much incentive rewards did you receive||Amount of rewards you committed to share with Common Good Finance||Your account balance at the end of this time period'));
  foreach ($heads as $key => $value) if ($value) $heads[$key] = popHelp($value, $headHelp[$key]);
  $PENDING = popHelp(t('PENDING'), t('Pending transfers, transaction requests, and commitments as of ') . $ending);
  
  $rows = array(
    array('id' => 'txsum-dates', 'data' => u\ray("$starting,,,,,,,,,,,,$ending")),
    array('id' => 'txsum-headers', 'data' => $heads),
    array(
      'id' => 'txsum-now', 
      'data' => u\ray("$old | + | $bank | - | $out | + | $in | + | $rewards |  |  | = | $new"),
    ),
    array(
      'id' => 'txsum-pending',
      'data' => u\ray("<b>$PENDING:</b> | + | $p_bank | - | $p_outProper | + | $p_inProper | + | $p_rewards | - | $p_committed | = | $p_balance"),
    ),
  );
  $attributes = array('id' => 'txsum');
  $header = $cols = array();
  foreach ($classes as $class) $cols[] = compact('class');
  $colgroups = array($cols);
  $caption = $sticky = $empty = '';
  return $summary_title . theme_table(compact(u\ray('header rows attributes caption colgroups sticky empty')));
}

function okableState($state) {return ($state == TX_PENDING or $state == TX_DISPUTED);}

function csv($csv, $a) {
  global $testCSV; if (!@$testCSV) $testCSV = array();
  fputcsv($csv, $a);
  $testCSV[] = $a;
}

function popHelp($text, $help) {return "<span title=\"$help\">$text</span>";}
