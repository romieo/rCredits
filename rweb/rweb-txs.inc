<?php
namespace rCredits\Web; // typically abbreviated as "rWeb"
use \rCredits\SMS as rSMS;
use \rCredits\API as api;
use \rCredits\Utilities as u;
use \rCredits as r; // get rid of this

/**
 * @file
 * Form for managing transactions, and related functions
 */

/**
 * Manage Transactions
 * @todo put transaction query in rcredits.inc instead (no reference to specific tables here)
 */
function txs_form($form, &$form_state, $tx_period = '', $do = '', $xid = '') {
  global $cid;
  global $base_path;

  //  if ($confirm = sure_form($form_state)) return $confirm;
  if ($do == 'edit') return tx_edit($xid);
  if ($do == 'ok' or $do == 'no') {
    if ($confirm_form = tx_confirm($do, $xid, $form_state)) return $confirm_form;
    if (!$form_state['rebuild']) tx_okorno($do, $xid); // don't do anything if canceling
    drupal_goto("members/transactions/$tx_period"); // back to the normal form
  }

  if (!$tx_period) $tx_period = 30;
  $tx_days = array('4' => '4 days', '30' => '30 days', '90' => '90 days', '180' => '6 months', '365' => '12 months', '-1' => 'Year to date');
  $ending = time();
  $starting = strtotime($tx_period == '-1' ? '1jan' : ((1 - $tx_days[$tx_period]) . ' days'), $ending);
  $can_create = api\access('can create transactions');

  $states = array(TX_DONE => '&#10004;', TX_PENDING => 'ok?', TX_DENIED => 'denied', TX_CONTESTED => 'contested');
  $colors = array('&#10004;' => 'green', 'ok?' => 'orange', 'pending' => 'darkgreen', 'denied' => 'red', 'contested' => 'red');
  $sql = <<<EOF
    SELECT t.*,
      (SELECT full_name FROM users WHERE uid = IF(t.payer = $cid, t.payee, t.payer)) AS name,
      IF(t.payer = $cid, t.payer_for, t.payee_for) AS purpose
    FROM r_txs t 
    WHERE t.type NOT IN (:rebate, :bonus)
    AND $cid IN (t.payer, t.payee) 
    AND (t.state IN(:done, :pending, :contested) OR (t.state = :denied AND t.payee = $cid))
    AND t.created >= $starting AND t.created <= $ending
    ORDER BY t.xid DESC
EOF;
//    AND ((t.state IN(:done, :pending, :contested) OR (t.state = :denied AND t.payee = $cid)) XOR t.amount < 0)
  $tx_map = array(
    'done' => TX_DONE, 'pending' => TX_PENDING, 'denied' => TX_DENIED, 'rebate' => TX_REBATE, 'bonus' => TX_BONUS, 'contested' => TX_CONTESTED,
  );
  $txs = r\db_q($sql, $tx_map);
  $rows = array();
  while ($row = $txs->fetchAssoc()) {
    extract($row, EXTR_PREFIX_ALL, 'one');
    $tid = api\tid_from_xid($cid, $one_xid);
    $data = unserialize($one_data);
    $is_reward = ($one_type != TX_TRANSFER);
    $reward = @$data[$one_payer == $cid ? 'rebate' : 'bonus'];
    $reward = $reward ? number_format($reward, 2) : '';
    $amount = number_format($one_amount, 2);
    if ($is_reward) list ($amount, $reward) = array('', $amount); // show interest and signup bonus as rewards
    list ($from_you, $to_you) = $one_payer == $cid ? array($amount, '') : array('', $amount);
//    $state = ($one_state == TX_PENDING and ($one_amount < 0 xor $one_payee == $cid)) ? 'pending' : $states[$one_state];
    $state = ($one_state == TX_PENDING and $one_payee == $cid) ? 'pending' : $states[$one_state];
    $color = $colors[$state];
    $state = "<span style='color:$color;'>$state</span>";
//    $button1 = (($one_payer == $cid xor $one_amount < 0) and $one_state == TX_PENDING) ? button('OK', "$base_path/members/transactions/$tx_period/ok/$one_xid", 'Approve') : '';
    $button1 = ($one_payer == $cid and $one_state == TX_PENDING) ? button('OK', "$base_path/members/transactions/$tx_period/ok/$one_xid", 'Approve') : '';
    $button2 = (!$is_reward and !@$data['undone']) ? button(' X ', "$base_path/members/transactions/$tx_period/no/$one_xid", 'Dispute/Cancel/Undo') : '';
    $button3 = ($can_create and !$is_reward) ? "<a href='$base_path/members/transactions/$tx_period/edit/$one_xid' title='Edit Purpose'><img src='{$base_path}images/edit-icon.png' width=20 height=20 border=0 /></a>" : '';
    $buttons = "<div class='txs-buttons'><div>$button1</div><div>$button2</div><div>$button3</div></div>";
    $rows[] = array($tid, date('d-M', $one_created), $one_name, $from_you, $to_you, $state, $buttons, $one_purpose, $reward);
  }

  $title = "<div id='txs-title'>TRANSACTIONS</div>";
  $classes = u\array_prefix('tx-', u\ray('tid date name fromyou toyou state buttons purpose reward'));
  $attributes = array('id' => 'txlist');
  $header = u\ray('#,Date,Name,From you,To you,Status,,Purpose,Rewards');
  $cols = array();
  foreach ($classes as $class) $cols[] = compact('class');
  $colgroups = array($cols);
  $caption = $sticky = '';
  $empty = tt('no txs');
  $list = theme_table(compact(u\ray('header rows attributes caption colgroups sticky empty')));

  $form = u\prefix_keys('tx_', array(
    'period' => form_field('select', t('Show transactions for the past'), '', dft($tx_period), $tx_days),
    'show' => form_field('submit', NULL, t('Submit button'), 'Show'),
    'do' => form_field('hidden', $do),
    'summary' => form_field('markup', '', '', array('markup' => tx_summary($starting, $ending))),
    'transactions' => form_field('markup', '', '', array('markup' => $title . $list . '<br>&nbsp;')),
  ));
  return $form;
}

function tx_confirm($do, $xid, &$form_state) {
  $verb = $do == 'ok' ? 'Approve' : 'Dispute/Cancel/Undo';
  if (confirming_v($form_state)) return FALSE;
  extract(tx_details($xid), EXTR_PREFIX_ALL, 'my');
  confirm("Are you sure you want to $verb this transaction?<br>($my_summary)", $form_state); // sets 'confirm'
  return sure_form($form_state);
}

function txs_form_validate($form, &$form_state) {
/*
  if ($op == 'Go') {
    extract($form_state['values']);
    if (no_selection($tx_transactions)) return say('nothing selected');
    if (confirming_v($form_state)) return;
    if ($tx_task != 'edit') confirm("Are you sure you want to $tx_task the selected transactions?", $form_state);
  }
*/
}

function txs_form_submit($form, &$form_state) {
  global $cid, $base_url;
  if (confirming_s($form_state)) return;
  extract($form_state['values']);
  if ($op == 'Show') { // changing the period to show
    $form_state['redirect'] = "members/transactions/$tx_period"; //\current_path();
    return;
  }
}

function tx_okorno($tx_task, $xid) {
  global $cid;
  $result = api\last_tx('xid=:xid', compact('xid'), TRUE);
  extract($result, EXTR_PREFIX_ALL, 'my');
  $atid = api\tid_from_xid($cid, $xid);
  $xname = "tx_transactions[x$xid]";
  if ($tx_task == 'ok') {
    if ($my_payer != $cid) {
      say("atid|can't ok for other", compact('atid'), $xname);
    } elseif ($my_state != TX_PENDING) {
      say("atid|can't ok if done", compact('atid'), $xname);
    } else {
      list ($message, $args) = api\pay_invoice($xid, TX_WEB);
      say("atid|$message", $args);
    }
  }
  if ($tx_task == 'no') {
    list ($message, $args) = api\undo_tx($xid, $cid, TX_WEB);
    say("atid|$message", array_merge($args, compact('atid')));
  }
/*  if ($tx_task == 'edit') {
    if ($my_state != TX_DONE and $my_payer != $cid) {
      say("atid|can't change what", compact('atid'), $xname);
    } else \drupal_set_message('Editing descriptions is not yet possible');
  }*/
}

function tx_details($xid, $uid = '') {
  global $cid;
  if (!$uid) $uid = $cid;
  
  extract(api\last_tx('xid=:xid', compact('xid'), TRUE));
//  extract(r\db_q('SELECT * FROM r_txs WHERE xid=:xid', compact('xid'))->fetchAssoc());
  $amount = u\format_amount($amount);
  $tofrom = $payer == $uid ? 'to' : 'from';
  $who = api\user_field('full_name', $payer == $uid ? $payee : $payer);
  $role = $payer == $uid ? 'payer' : 'payee';
  $purpose = $payer == $uid ? $payer_for : $payee_for;
  $tid = api\tid_from_xid($uid, $xid);
  $created = date('m-d-Y', $created);
  $summary = "Transaction #$tid: $amount $tofrom $who for \"$purpose\" on $created";
  return compact(u\ray('amount tofrom who role purpose tid created summary'));
}

function tx_edit($xid) {
  if (!api\access('can create transactions')) return FALSE;

  extract(tx_details($xid), EXTR_PREFIX_ALL, 'my');
  
  $form = array(
    'title' => form_field('item', t('Edit Description')),
//    'review' => form_field('item', $tx_summary),
    'purpose' => form_field('textfield', t('Purpose'), '', dft($my_purpose)),
    'go' => form_field('submit', NULL, t('Submit button'), 'Update'),
    'xid' => form_field('hidden', $xid),
    'field' => form_field('hidden', $my_role . '_for'),
    
    '#validate' => array('rCredits\\Web\\tx_edit_validate'),
    '#submit' => array('rCredits\\Web\\tx_edit_submit'),
  );
  return $form;
}

function tx_edit_validate($form, &$form_state) {
}

function tx_edit_submit($form, &$form_state) {
  extract($form_state['values']);
  r\set_tx_field($field, $purpose, $xid);
  //r\db_q("UPDATE r_txs SET $field=:purpose WHERE xid=:xid", compact('purpose', 'xid'));
  $form_state['redirect'] = 'members/transactions';
}

function tx_bal($bal, $plus = '', $minus = '- ') {return ($bal < 0 ? $minus : $plus) . '$' . number_format(abs($bal), 2);}

function tx_summary($starting, $ending) {
  global $cid;
  $previous_ending = strtotime('-1 days', $starting);
  extract($now_info = (array) api\credit_info($cid, TX_DONE, $ending), EXTR_PREFIX_ALL, 'now');
  extract($pending_info = (array) api\credit_info($cid, TX_PENDING, $ending), EXTR_PREFIX_ALL, 'pending');
  extract($old_info = (array) api\credit_info($cid, TX_DONE, $previous_ending), EXTR_PREFIX_ALL, 'old');
  $dif = $pfancy = array();
  foreach (u\ray('gross_in_proper gross_out rewards') as $key) {
    $dif[$key] = number_format($now_info[$key] - $old_info[$key], 2);
    $pfancy[$key] = number_format($pending_info[$key], 2);
  }
  extract($dif, EXTR_PREFIX_ALL, 'dif');
  extract($pfancy, EXTR_PREFIX_ALL, 'pfancy');
  
  $starting = date('d-M-Y', $starting);
  $ending = date('d-M-Y', $ending);
  $pending_balance = tx_bal($pending_gross_in_proper - $pending_gross_out + $pending_rewards);
  $now_balance = tx_bal($now_balance);
  $old_balance = tx_bal($old_balance);
  $summary_title = "<div id='txsum-title'>SUMMARY</div>";
  
  $classes = u\array_prefix('txsum-', u\ray('one tosign toyou fromsign fromyou rewardssign rewards balancesign balance'));
  $rows = array(
    array('id' => 'txsum-dates', 'data' => u\ray("$starting,,,,,,,,$ending")),
    array('id' => 'txsum-headers', 'data' => u\ray('Starting balance,,To You,,From You,,Rewards,,Ending balance')),
    array(
      'id' => 'txsum-now', 
      'data' => u\ray("$old_balance | + | $dif_gross_in_proper | - | $dif_gross_out | + | $dif_rewards | = | $now_balance"),
    ),
    array(
      'id' => 'txsum-pending',
      'data' => u\ray("<b>PENDING:</b> | + | $pfancy_gross_in_proper | - | $pfancy_gross_out | + | $pfancy_rewards | = | $pending_balance"),
    ),
  );
  $attributes = array('id' => 'txsum');
  $header = $cols = array();
  foreach ($classes as $class) $cols[] = compact('class');
  $colgroups = array($cols);
  $caption = $sticky = $empty = '';
  return $summary_title . theme_table(compact(u\ray('header rows attributes caption colgroups sticky empty')));
}