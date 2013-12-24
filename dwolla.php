<?php

use rCredits\Util as u; // cgf
// CGF: replace "if ($" with "if (@$" and "if (!$" with "if (!@$"
/**
 * Dwolla REST API Library for PHP
 *
 * MIT LICENSE
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 * Dwolla REST API Library for PHP
 *
 * @package   Dwolla
 * @author    Michael Schonfeld <michael@dwolla.com>
 * @copyright Copyright (c) 2012 Dwolla Inc. (http://www.dwolla.com)
 * @mods      Modifications copyright (c) 2013 William Spademan and Society to Benefit Everyone, Inc.
 * @license   http://opensource.org/licenses/MIT MIT
 * @version   1.5.1
 * @link      http://www.dwolla.com
 */
class DwollaRestClient {
  private $apiKey;
  private $oauthToken;
  private $permissions; // array oauth authentication scopes
  private $redirectUri; // URL to return the user to after the authentication request
  private $mode; // Transaction mode. Can be 'live' or 'test'
  private $gatewaySession; // array Off-Site Gateway order items
  private $errorMessage = FALSE; // string error messages returned from Dwolla
  private $debugMode = FALSE;
  // const API_SERVER = DW_API; // cgf
  private $step = FALSE; // string what to do next (cgf)

  /**
   * Sets the initial state of the client
   * 
   * @param string $apiKey
   * @param string $apiSecret
   * @param string $redirectUri
   * @param array $permissions
   * @param string $mode 
   * @throws InvalidArgumentException
   */
  public function __construct($apiKey = FALSE, $apiSecret = FALSE, $redirectUri = FALSE, $permissions = array("send", "transactions", "balance", "request", "contacts", "accountinfofull", "funding", "manageaccount"), $mode = 'live', $debugMode = FALSE) {
      $this->apiKey = $apiKey;
      $this->apiSecret = $apiSecret;
      $this->redirectUri = $redirectUri;
      $this->permissions = $permissions;
      $this->apiServerUrl = DW_SITE . '/oauth/rest/'; // cgf (to enable sandbox)
      $this->setMode($mode);
  }

  /**
   * Return the current step
   */
  public function step() {
    if (!$this->step and $response = $this->get("register/step")) {
      $result = $this->parse($response);
      $this->step = @$result['CurrentStep'];
    }
    return $this->step;
  }
  
  /**
   * Get oauth authenitcation URL
   * 
   * @return string URL
   */
  public function getAuthUrl() {
      $params = array(
          'client_id' => $this->apiKey,
          'response_type' => 'code',
          'scope' => implode('|', $this->permissions)
      );

      // Only append a redirectURI if one was explicitly specified
      if (@$this->redirectUri) { // cgf
          $params['redirect_uri'] = $this->redirectUri;
      }

      $url = DW_SITE . '/oauth/v2/authenticate?' . http_build_query($params);
      return $url;
  }

  /**
   * Request oauth token from Dwolla
   * 
   * @param string $code Temporary code returned from Dwolla
   * @return string oauth token
   */
  public function requestToken($code) {
      if (!@$code) { // cgf
          return $this->setError('Please pass an oauth code.');
      }

      $params = array(
          'client_id' => $this->apiKey,
          'client_secret' => $this->apiSecret,
          'redirect_uri' => $this->redirectUri,
          'grant_type' => 'authorization_code',
          'code' => $code
      );
      $url = DW_SITE . '/oauth/v2/token?' . http_build_query($params);
      $response = $this->curl($url, 'GET');
/*        if (isset($response['error'])) {
          $this->errorMessage = $response['error_description'];
          return FALSE;
      }
*/    
      if (!@$response['access_token']) { // mod by CGF
        return $this->setError(@$response['Message'] ?: ('failed, no Message: ' . print_r($response, 1)));
      }

      return $response['access_token'];
  }

  /**
   * Grabs the account information for the
   * authenticated user
   *
   * @return array Authenticated user's account information
   */
  public function me() {
      $response = $this->get('users/');
      return $this->parse($response);
  }

  /**
   * Grabs the basic account information for
   * the provided Dwolla account Id
   * 
   * @param string $userId Dwolla Account Id
   * @return array Basic account information 
   */
  public function getUser($userId) {
      $params = array(
          'client_id' => $this->apiKey,
          'client_secret' => $this->apiSecret
      );

      $response = $this->get("users/{$userId}", $params);
      $user = $this->parse($response);

      return $user;
  }

  /**
   * Get a list of users nearby a
   * given geo location
   *
   * @return array Users
   */
  public function usersNearby($lat, $long) {
      $params = array(
          'client_id' => $this->apiKey,
          'client_secret' => $this->apiSecret,
          'latitude' => $lat,
          'longitude' => $long
      );

      $response = $this->get("users/nearby", $params);
      $users = $this->parse($response);

      return $users;
  }

  /**
   * Register new Dwolla user (version 2)
   *  
   * @param string $email
   * @param string $password
   * @param int $pin
   * @param string $firstName
   * @param string $lastName
   * @param string $dateOfBirth: the responsible person's date of birth mm-dd-yyyy
   * @param string $type Dwolla account type
   * @param string $organization
   * @param string $ein
   * @param string $businessStructure: SoleProprietorship, Corporation, or Partnership
   * @param bool $acceptTerms
   * @return assoc array [OAuthToken, CurrentStep, Errors] 
   */
  public function register2($email, $password, $pin, $firstName, $lastName, $dateOfBirth = '', $type = 'Personal', $organization = '', $ein = '', $businessStructure = '', $acceptTerms = FALSE) {
      $client_id = $this->apiKey;
      $client_secret = $this->apiSecret;
      $version = 2;
      $scope = join('|', $this->permissions);
      $params = compact(explode(' ', 'client_id client_secret email password pin firstName lastName dateOfBirth type organization ein businessStructure acceptTerms scope version'));
      $response = $this->post('register/', $params, FALSE); // FALSE = don't include oAuth token

      return $this->parse($response);
  }

  /**
   * Resend the Dwolla verification email to the user.
   * @return TRUE if success.
   */
  function resendEmail() {
    $response = $this->POST('register/resendVerificationEmail');
    $this->parse($response);
    return @$response['Success'];
  }
  
  /**
   * Add primary phone.
   * @param string $phone: primary phone number (no punctuation)
   * @param string $verificationMethod: SMS or Voice
   * @return string $PhoneId or FALSE
   */
  function addPhone($phone, $verificationMethod) {
    $response = $this->post('register/phones', compact('phone', 'verificationMethod'));
    $result = $this->parse($response);
    return @$result['PhoneId'];
  }

  /**
   * Resend phone verification code.
   * @param string $phoneId: phone identifier
   * @param string $err: (returned) the error message, if any
   * @return TRUE if success
   */
  function reAddPhone($phoneId) {
    $response = $this->get("register/phones/$phoneId/resend");
    $this->parse($response);
    return @$response['Success'];
  }
  
  /**
   * Verify primary phone.
   * @param string $phoneId: phone identifier
   * @param integer $code: 5-digit verification code (sent earlier to phone by SMS or voice)
   * @return TRUE if success
   */
  function verifyPhone($phoneId, $code) {
    $response = $this->post("register/phones/$phoneId/verify", compact('code'));
    $this->parse($response);
    return @$response['Success'];
  }

  /**
   * Specify user's address.
   * @param string $address: Line one of user address. Precise data & format critical to geo-location data.
   * @param string $city: User's City.
   * @param string $state: USA state or territory two character code (2 Char Abbrv)
   * @param string $zip: Postal code or zip code.
   * @param $string $address2: Line two of user address. Precise data & format critical to geo-location data.
   */
  function address($address, $city, $state, $zip, $address2 = '') {
    $response = $this->post('register/address', compact(explode(' ', 'address address2 city state zip')));
    $this->parse($response);
    return @$response['Success'];
  }

  /**
   * Verify the user's social security number.
   * @param string $ssn: the user's social security number
   * @return TRUE if success, else FALSE (in which case next step is normally "AccountInfo")
   */
  function ssn($ssn) {
    $response = $this->post('register/verification/ssn', compact('ssn'));
    $this->parse($response);
    return ($this->step == 'Finished');
  }

  /**
   * Submit corrected personal information (because SSN could not be verified by ssn())
   * @param string $dateOfBirth: person's birth date (empty for companies)
   * @param string $ein: organization's Employer Id Number (empty for individuals)
   * @param string $firstName: person's first name (empty for companies)
   * @param string $lastName: person's last name (empty for companies)
   * @param string $organization: organization name (empty for individuals)
   * @param string $businessStructure: SoleProprietorship, Corporation, or Partnership
   * @return TRUE if success, else FALSE (in which case next step is normally "Kba")
   */
  function accountInfo($dateOfBirth = '', $ein = '', $firstName = '', $lastName = '', $organization = '', $businessStructure = '') {
    $response = $this->post('register/general', compact(explode(' ', 'dateOfBirth ein firstName lastName organization businessStructure')));
    $this->parse($response);
    return ($this->step == 'Finished');
  }
  
  /**
   * Get KBA verification questions for the user.
   * @return assoc array: [TransactionId, Questions, CurrentStep, Errors], where
   *   Questions is a simple array of [Id, Text, Choices] and Choices is array of [answer id => answer text]
   */
  function kba() {
    $response = $this->get('register/verification/kba');
    return $this->parse($response);
  }

  /**
   * Verify the KBA questions.
   * @param int $transactionId: the ID of the set of questions being answered
   * @param assoc array $answers: [answerid, questionid]
   * @return TRUE if success
   */
  function verifyKba($transactionId, $answers) {
    $response = $this->post('register/verification/kba', compact('transactionId', 'answers'));
    $this->parse($response);
    return @$response['Success'];
  }
  
  /**
   * Upload a photo ID
   * @param string $file: full path of file to upload
   * @return TRUE if success
   */
  function sendPhotoId($file) {
    $response = $this->post('register/verification/photo', compact('file'));
    $this->parse($response);
    return @$response['Success'];
  }
  
  /**
   * Specify an authorized representative for a company account (commercial or nonprofit).
   * @param string $firstName
   * @param string $lastName
   * @param string $address
   * @param string $city
   * @param string $state
   * @param string $zip
   * @param string $phone
   * @param string $dateOfBirth: the responsible person's date of birth mm-dd-yyyy
   * @param string $ssn: the responsible person's social security number
   * @return TRUE if success
  */
  function authorizedRep($firstName, $lastName, $address, $city, $state, $zip, $phone, $dateOfBirth, $ssn) {
    $info = compact(explode(' ' , 'firstName lastName address city state zip phone dateOfBirth ssn'));
    $response = $this->post('register/verification/authorized_representative', $info);
    $this->parse($response);
    return @$response['Success'];
  }
  
  /**
   * Register new Dwolla user
   *  
   * @param string $email
   * @param string $password
   * @param int $pin
   * @param string $firstName
   * @param string $lastName
   * @param string $address
   * @param string $city
   * @param string $state
   * @param int $zip
   * @param string $phone
   * @param string $dateOfBirth: the responsible person's date of birth mm-dd-yyyy
   * @param bool $acceptTerms
   * @param string $address2
   * @param string $type Dwolla account type
   * @param string $organization
   * @param string $ein
   * @return array New user information 
   */
  public function register($email, $password, $pin, $firstName, $lastName, $address, $city, $state, $zip, $phone, $dateOfBirth, $acceptTerms, $address2 = '', $type = 'Personal', $organization = '', $ein = ''
  ) {
      $params = array(
          'client_id' => $this->apiKey,
          'client_secret' => $this->apiSecret,
          'email' => $email,
          'password' => $password,
          'pin' => $pin,
          'firstName' => $firstName,
          'lastName' => $lastName,
          'address' => $address,
          'address2' => $address2,
          'city' => $city,
          'state' => $state,
          'zip' => $zip,
          'phone' => $phone,
          'dateOfBirth' => $dateOfBirth,
          'type' => $type,
          'organization' => $organization,
          'ein' => $ein,
          'acceptTerms' => $acceptTerms
      );
      $response = $this->post('register/', $params, FALSE); // FALSE = don't include oAuth token

      $user = $this->parse($response);

      return $user;
  }

  /**
   * Search contacts
   * 
   * @param string $search Search term(s)
   * @param array $types Types of contacts (Dwolla, Facebook . . .)
   * @param int $limit Number of contacts to retrieve between 1 and 200. 
   * @return array
   */
  public function contacts($search = FALSE, $types = array('Dwolla'), $limit = 10) {
      $params = array(
          'search' => $search,
          'types' => implode(',', $types),
          'limit' => $limit
      );
      $response = $this->get('contacts', $params);

      $contacts = $this->parse($response);

      return $contacts;
  }

  /**
   * Use this method to retrieve nearby Dwolla spots within the range of the 
   * provided latitude and longitude. 
   * 
   * Half of the limit are returned as spots with closest proximity. The other 
   * half of the spots are returned as random spots within the range.
   * This call can return nearby venues on Foursquare but not Dwolla, they will have an Id of "null"
   * 
   * @param float $latitude
   * @param float $longitude
   * @param int $range Range to search in miles
   * @param int $limit Limit search to this number results
   * @return array Search results 
   */
  public function nearbyContacts($latitude, $longitude, $range = 10, $limit = 10) {
    $params = array(
      'latitude' => $latitude,
      'longitude' => $longitude,
      'limit' => $limit,
      'range' => $range,
      'client_id' => $this->apiKey,
      'client_secret' => $this->apiSecret,
    );

    $response = $this->get('contacts/nearby', $params);
    $contacts = $this->parse($response);

    return $contacts;
  }

  /**
   * Retrieve a list of verified funding sources for the user associated 
   * with the authorized access token.
   *
   * @return array Funding Sources
   */
  public function fundingSources() {
      $response = $this->get('fundingsources');
      return $this->parse($response);
  }

  /**
   * Retrieve a verified funding source by identifier for the user associated 
   * with the authorized access token.
   *
   * @param string Funding Source ID
   * @return array Funding Source Details
   */
  public function fundingSource($fundingSourceId) {
      $response = $this->get("fundingsources/{$fundingSourceId}");
      return $this->parse($response);
  }

  /**
   * Add a funding source for the user associated 
   * with the authorized access token.
   *
   * @return array Funding Sources
   */
  public function addFundingSource($accountNumber, $routingNumber, $accountType, $accountName) {
      // Verify required paramteres
      if (!@$accountNumber) { // cgf
        return $this->setError('Please enter a bank account number.');
      } else if (!@$routingNumber) { // cgf
        return $this->setError('Please enter a bank routing number.');
      } else if (!@$accountType) { // cgf
        return $this->setError('Please enter an account type.');
      } else if (!@$accountName) { // cgf
        return $this->setError('Please enter an account name.');
      }

      // Build request, and send it to Dwolla
      $params = array(
        'account_number' => $accountNumber,
        'routing_number' => $routingNumber,
        'account_type' => $accountType,
        'name' => $accountName
      );

      $response = $this->post('fundingsources/', $params);
      return $this->parse($response);
  }

  /**
   * Verify a funding source for the user associated 
   * with the authorized access token.
   *
   * @return array Funding Sources
   */
  public function verifyFundingSource($fundingSourceId, $deposit1, $deposit2) {
      // Verify required paramteres
      if (!@$deposit1) { // cgf
        return $this->setError('Please enter an amount for deposit1.');
      } else if (!@$deposit2) { // cgf
        return $this->setError('Please enter an amount for deposit2.');
      } else if (!@$fundingSourceId) { // cgf
        return $this->setError('Please enter a funding source ID.');
      }

      // Build request, and send it to Dwolla
      $params = array(
        'deposit1' => $deposit1,
        'deposit2' => $deposit2
      );

      $response = $this->post("fundingsources/{$fundingSourceId}/verify", $params);
      return $this->parse($response);
  }
  
  /**
   * Verify a funding source for the user associated 
   * with the authorized access token.
   *
   * @return array Funding Sources
   */
  public function withdraw($fundingSourceId, $pin, $amount) {
      // Verify required paramteres
      if (!@$pin) { // cgf
        return $this->setError('Please enter a PIN.');
      } else if (!@$fundingSourceId) { // cgf
        return $this->setError('Please enter a funding source ID.');
      } else if (!@$amount) { // cgf
        return $this->setError('Please enter an amount.');
      }

      // Build request, and send it to Dwolla
      $params = array(
        'pin' => $pin,
        'amount' => $amount
      );

      $response = $this->post("fundingsources/{$fundingSourceId}/withdraw", $params);
      return $this->parse($response);
  }

  /**
   * Verify a funding source for the user associated 
   * with the authorized access token.
   *
   * @return array Funding Sources
   */
  public function deposit($fundingSourceId, $pin, $amount) {
      // Verify required paramteres
      if (!@$pin) { // cgf
        return $this->setError('Please enter a PIN.');
      } else if (!@$fundingSourceId) { // cgf
        return $this->setError('Please enter a funding source ID.');
      } else if (!@$amount) { // cgf
        return $this->setError('Please enter an amount.');
      }

      // Build request, and send it to Dwolla
      $params = array(
        'pin' => $pin,
        'amount' => $amount
      );

      $response = $this->post("fundingsources/{$fundingSourceId}/deposit", $params);
      return $this->parse($response);
  }    

  /**
   * Retrieve the account balance for the user with the given authorized 
   * access token. 
   * 
   * @return float Balance in USD 
   */
  public function balance() {
      $response = $this->get('balance/');
      return $this->parse($response);
  }

  /**
   * Send funds to a destination user from the user associated with the 
   * authorized access token.
   * 
   * @param int $pin
   * @param string $destinationId Dwolla identifier, Facebook identifier, Twitter identifier, phone number, or email address
   * @param float $amount
   * @param string $destinationType Type of destination user. Can be Dwolla, Facebook, Twitter, Email, or Phone. Defaults to Dwolla.
   * @param string $notes Note to attach to the transaction. Limited to 250 characters.
   * @param float $facilitatorAmount
   * @param bool $assumeCosts Will sending user assume the Dwolla fee?
   * @param string $fundsSource Funding source ID to use. Defaults to Dwolla balance.
   * @return string Transaction Id 
   */
  public function send($pin = FALSE, $destinationId = FALSE, $amount = FALSE, $destinationType = 'Dwolla', $notes = '', $facilitatorAmount = 0, $assumeCosts = FALSE, $fundsSource = 'balance'
  ) {
      // Verify required paramteres
      if (!@$pin) { // cgf
          return $this->setError('Please enter a PIN.');
      } else if (!@$destinationId) { // cgf
          return $this->setError('Please enter a destination ID.');
      } else if (!@$amount) { // cgf
          return $this->setError('Please enter a transaction amount.');
      }

      // Build request, and send it to Dwolla
      $params = array(
          'pin' => $pin,
          'destinationId' => $destinationId,
          'destinationType' => $destinationType,
          'amount' => $amount,
          'facilitatorAmount' => $facilitatorAmount,
          'assumeCosts' => $assumeCosts,
          'notes' => $notes,
          'fundsSource' => $fundsSource,
      );
      $response = $this->post('transactions/send', $params);

      // Parse Dwolla's response
      $transactionId = $this->parse($response);

      return $transactionId;
  }

  /**
   * Request funds from a source user, originating from the user associated 
   * with the authorized access token.
   * 
   * @param string $sourceId
   * @param float $amount
   * @param string $sourceType
   * @param string $notes
   * @param float $facilitatorAmount
   * @return int Request Id 
   */
  public function request($sourceId = FALSE, $amount = FALSE, $sourceType = 'Dwolla', $notes = '', $facilitatorAmount = 0) {
      // Verify required paramteres
      if (!@$sourceId) { // cgf
          return $this->setError('Please enter a source ID.');
      } else if (!@$amount) { // cgf
          return $this->setError('Please enter a transaction amount.');
      }

      // Build request, and send it to Dwolla
      $params = array(
          'sourceId' => $sourceId,
          'sourceType' => $sourceType,
          'amount' => $amount,
          'facilitatorAmount' => $facilitatorAmount,
          'notes' => $notes
      );
      $response = $this->post('requests/', $params);

      // Parse Dwolla's response
      $transactionId = $this->parse($response);

      return $transactionId;
  }
  
  /**
   * Get a request by its ID

   * @return array Request with the given ID
   */
  public function requestById($requestId) {
      // Verify required paramteres
      if (!@$requestId) { // cgf
        return $this->setError('Please enter a request ID.');
      }

      // Build request, and send it to Dwolla
      $response = $this->get("requests/{$requestId}");

      // Parse Dwolla's response
      $request = $this->parse($response);

      return $request;
  }

  /**
   * Fulfill (:send) a pending payment request

   * @return array Transaction information
   */
  public function fulfillRequest($requestId, $pin, $amount = FALSE, $notes = FALSE, $fundsSource = FALSE, $assumeCosts = FALSE) {
      // Verify required paramteres
      if (!@$pin) { // cgf
        return $this->setError('Please enter a PIN.');
      } else if (!@$requestId) { // cgf
        return $this->setError('Please enter a request ID.');
      }

      // Build request, and send it to Dwolla
      $params = array(
        'pin' => $pin
      );
      if($amount) { $params['amount'] = $amount; }
      if($notes) { $params['notes'] = $notes; }
      if($fundsSource) { $params['fundsSource'] = $fundsSource; }
      if($assumeCosts) { $params['assumeCosts'] = $assumeCosts; }

      $response = $this->post("requests/{$requestId}/fulfill", $params);
      return $this->parse($response);
  }
  
  /**
   * Cancel (:reject) a pending payment request

   * @return array Transaction information
   */
  public function cancelRequest($requestId) {
      // Verify required paramteres
      if (!@$requestId) { // cgf
        return $this->setError('Please enter a request ID.');
      }

      $response = $this->post("requests/{$requestId}/cancel", array());
      return $this->parse($response);
  }

  /**
   * Get a list of pending money requests

   * @return array Pending Requests
   */
  public function requests() {
      // Build request, and send it to Dwolla
      $response = $this->get("requests/");

      // Parse Dwolla's response
      $requests = $this->parse($response);

      return $requests;
  }

  /**
   * Grab information for the given transaction ID with 
   * app credentials (instead of oauth token)
   *
   * @param int Transaction ID to which information is pulled
   * @return array Transaction information
   */
  public function transaction($transactionId) {
      // Verify required paramteres
      if (!@$transactionId) {
          return $this->setError('Please enter a transaction ID.');
      }

      $params = array(
          'client_id' => $this->apiKey,
          'client_secret' => $this->apiSecret
      );

      // Build request, and send it to Dwolla
      $response = $this->get("transactions/{$transactionId}", $params);

      // Parse Dwolla's response
      $transaction = $this->parse($response);

      return $transaction;
  }

  /**
   * Retrieve a list of transactions for the user associated with the 
   * authorized access token.
   * 
   * @param string $sinceDate Earliest date and time for which to retrieve transactions.
   *        Defaults to 7 days prior to current date and time in UTC. Format: DD-MM-YYYY (cgf: actually MM-DD-YYYY)
   * @param array $types Types of transactions to retrieve.  Options are money_sent, money_received, deposit, withdrawal, and fee.
   * @param int $limit Number of transactions to retrieve between 1 and 200
   * @param int $skip Number of transactions to skip
   * @return array Transaction search results 
   */
  public function listings($sinceDate = FALSE, $types = FALSE, $limit = 10, $skip = 0) {
      $params = array(
          'limit' => $limit,
          'skip' => $skip
      );

      if($sinceDate) { $params['sinceDate'] = $sinceDate; }
      if($types) { $params['types'] = implode(',', $types); }

      // Build request, and send it to Dwolla
      $response = $this->get("transactions", $params);

      // Parse Dwolla's response
      $listings = $this->parse($response);

      return $listings;
  }

  /**
   * Retrieve transactions stats for the user associated with the authorized 
   * access token.
   * 
   * @param array $types Options are 'TransactionsCount' and 'TransactionsTotal'
   * @param string $startDate Starting date and time to for which to process transactions stats. Defaults to 0300 of the current day in UTC.
   * @param string $endDate Ending date and time to for which to process transactions stats. Defaults to current date and time in UTC.
   * @return array Transaction stats search results 
   */
  public function stats($types = array('TransactionsCount', 'TransactionsTotal'), $startDate = null, $endDate = null) {
      $params = array(
          'types' => implode(',', $types),
          'startDate' => $startDate,
          'endDate' => $endDate
      );

      // Build request, and send it to Dwolla
      $response = $this->get("transactions/stats", $params);

      // Parse Dwolla's response
      $stats = $this->parse($response);

      return $stats;
  }

  /**
   * Creates an empty Off-Site Gateway Order Items array 
   */
  public function startGatewaySession() {
      $this->gatewaySession = array();
  }

  /**
   * Adds new order item to gateway session
   * 
   * @param string $name
   * @param float $price Item price in USD
   * @param int $quantity Number of items
   * @param string $description Item description
   */
  public function addGatewayProduct($name, $price, $quantity = 1, $description = '') {
      $product = array(
          'Name' => $name,
          'Description' => $description,
          'Price' => $price,
          'Quantity' => $quantity
      );

      $this->gatewaySession[] = $product;
  }

  /**
   * Creates and executes Server-to-Server checkout request
   * @link http://developers.dwolla.com/dev/docs/gateway#server-to-server
   * 
   * @param string $destinationId
   * @param string $orderId
   * @param float $discount
   * @param float $shipping
   * @param float $tax
   * @param string $notes
   * @param string $callback
   * @param boolean $allowFundingSources
   * @return string Checkout URL 
   */
  public function getGatewayURL($destinationId, $orderId = null, $discount = 0, $shipping = 0, $tax = 0, $notes = '', $callback = null, $allowFundingSources = TRUE) {
      // TODO add validation? Throw exception if malformed?
      $destinationId = $this->parseDwollaID($destinationId);

      // Normalize optional parameters
      if (!@$shipping) {
          $shipping = 0;
      } else {
          $shipping = floatval($shipping);
      }
      if (!@$tax) {
          $tax = 0;
      } else {
          $tax = floatval($tax);
      }
      if (!@$discount) {
          $discount = 0;
      } else {
          $discount = abs(floatval($discount));
      }
      if (!@$notes) {
          $notes = '';
      }

      // Calcualte subtotal
      $subtotal = 0;

      foreach ($this->gatewaySession as $product) {
          $subtotal += floatval($product['Price']) * floatval($product['Quantity']);
      }

      // Calculate grand total
      $total = round($subtotal - $discount + $shipping + $tax, 2);

      // Create request body
      $request = array(
          'Key' => $this->apiKey,
          'Secret' => $this->apiSecret,
          'Test' => ($this->mode == 'test') ? 'true' : 'FALSE',
          'AllowFundingSources' => $allowFundingSources ? 'true' : 'FALSE',
          'PurchaseOrder' => array(
              'DestinationId' => $destinationId,
              'OrderItems' => $this->gatewaySession,
              'Discount' => -$discount,
              'Shipping' => $shipping,
              'Tax' => $tax,
              'Total' => $total,
              'Notes' => $notes
          )
      );

      // Append optional parameters
      if (@$this->redirectUri) {
          $request['Redirect'] = $this->redirectUri;
      }

      if (@$callback) {
          $request['Callback'] = $callback;
      }

      if (@$orderId) {
          $request['OrderId'] = $orderId;
      }

      // Send off the request
      $response = $this->curl(DW_SITE . '/payment/request', 'POST', $request);

      if (@$response['Result'] != 'Success') {
          $this->errorMessage = $response['Message'];
          return FALSE;
      }

      return DW_SITE . '/payment/checkout/' . $response['CheckoutId'];
  }

  /**
   * Verifiy the signature returned from Offsite-Gateway Redirect
   * 
   * @param string $signature: Proposed signature; (required)
   * @param string $checkoutId: Dwolla's checkout ID; (required)
   * @param float $amount: Dwolla's reported total amount; (required)
   * @return bool Is signature valid? 
   */
  public function verifyGatewaySignature($signature = FALSE, $checkoutId = FALSE, $amount = FALSE) {
      // Verify required paramteres
      if (!@$signature) {
          return $this->setError('Please pass a proposed signature.');
      }
      if (!@$checkoutId) {
          return $this->setError('Please pass a checkout ID.');
      }
      if (!@$amount) {
          return $this->setError('Please pass a total transaction amount.');
      }

      // Calculate an HMAC-SHA1 hexadecimal hash
      // of the checkoutId and amount ampersand separated
      // using the consumer secret of the application
      // as the hash key.
      //
      // @doc: http://developers.dwolla.com/dev/docs/gateway
      $hash = hash_hmac("sha1", "{$checkoutId}&{$amount}", $this->apiSecret);

      if($hash !== $signature) {
        return $this->setError('Dwolla signature verification failed.');
      }

      return TRUE;
  }
  
  /**
   * Verifiy the signature returned from Webhook notifications
   * 
   * @return bool Is signature valid?
   */
  public function verifyWebhookSignature() {
      if (!function_exists('getallheaders')) { 
          throw new Exception("This function can only be used in an Apache environment.");
      }

      // 1. Get the request body
      $body = file_get_contents('php://input');
      
      // 2. Get Dwolla's signature
      $headers = getallheaders();
      $signature = $headers['X-Dwolla-Signature'];

      // 3. Calculate hash, and compare to the signature
      $hash = hash_hmac('sha1', $body, $this->apiSecret);
      $validated = ($hash == $signature);
      
      if(!$validated) {
          return $this->setError('Dwolla signature verification failed.');
      }
      
      return TRUE;
  }    

  public function massPayCreate($pin, $email, $filedata, $assumeCosts = FALSE, $source = FALSE, $user_job_id = FALSE) {
      if (!@$pin) {
        return $this->setError('Please enter a PIN.');
      } else if (!@$email) {
        return $this->setError('Please pass an reporting email.');
      } else if (!@$filedata) {
        return $this->setError('Please pass the MassPay bulk data.');
      }

      // Create request body
      $params = array(
        'token' => $this->oauthToken,
        'pin' => $pin,
        'email' => $email,
        'filedata' => $filedata,
        'assumeCosts' => $assumeCosts ? 'true' : 'FALSE',
        'test' => ($this->mode == 'test') ? 'true' : 'FALSE'
      );
      if($source) { $params['source'] = $source; }
      if($user_job_id) { $params['user_job_id'] = $user_job_id; }

      // Send off the request
      $response = $this->curl('https://masspay.dwollalabs.com/api/create/', 'POST', $params);

      $job = $this->parseMassPay($response);

      return $job;
  }
  
  public function massPayDetails($uid, $job_id = FALSE, $user_job_id = FALSE) {
      if (!@$uid) {
        return $this->setError('Please pass the associated Dwolla ID.');
      } else if (!@$job_id && !$user_job_id) {
        return $this->setError('Please pass either a MassPay job ID, or a user assigned job ID.');
      }

      // Create request body
      $params = array(
        'uid' => $uid
      );
      if($job_id) { $params['job_id'] = $job_id; }
      if($user_job_id) { $params['user_job_id'] = $user_job_id; }

      // Send off the request
      $response = $this->curl('https://masspay.dwollalabs.com/api/status/', 'POST', $params);

      $job = $this->parseMassPay($response);

      return $job;
  }

  /**
   * @return string|bool Error message or FALSE if error message does not exist
   */
  public function getError() {
      if (!@$this->errorMessage) {
          return FALSE;
      }

      $error = $this->errorMessage;
      $this->errorMessage = FALSE;

      return $error;
  }

  /**
   * Returns properly formatted Dwolla Id
   * 
   * @param string|int $id
   * @return string Properly formatted Dwolla Id 
   */
  public function parseDwollaID($id) {
      $id = preg_replace("/[^0-9]/", "", $id);
      $id = preg_replace("/([0-9]{3})([0-9]{3})([0-9]{4})/", "$1-$2-$3", $id);

      return $id;
  }

  /**
   * @param string $message Error message
   */
  protected function setError($message) {
      $this->errorMessage = $message;
  }

  /**
   * Parse Dwolla API response
   * 
   * @param array $response
   * @return array
   */
  protected function parse($response) {
      if (!@$response['Success']) {
          $this->errorMessage = $response['Message'];

          // Exception for /register method
          if ($r = @$response['Response']) { // cgf (several lines here)
            $this->errorMessage .= '<ul>';
            foreach ($r['Errors'] as $key => $value) $this->errorMessage .= "<li>$value</li>";
            $this->errorMessage .= '</ul>';
            //" :: " . u\jsonEncode($response['Response']);
          }

          return FALSE;
      }

      if ($step = @$response['CurrentStep']) $this->step = $step; // cgf
      return @$response['Response'];
  }

  /**
   * Parse MassPay API response
   * 
   * @param array $response
   * @return array
   */
  protected function parseMassPay($response) {
      if (!@$response['success']) {
          $this->errorMessage = $response['message'];

          return FALSE;
      }

      return $response['job'];
  }

  /**
   * Executes POST request against API
   * 
   * @param string $request
   * @param array $params
   * @param bool $includeToken Include oauth token in request?
   * @return array|null 
   */
  protected function post($request, $params = FALSE, $includeToken = true) {
      $url = $this->apiServerUrl . $request . ($includeToken ? "?oauth_token=" . urlencode($this->oauthToken) : "");
/**/  if($this->debugMode) debug("Posting request to: {$url} :: With params: \n" . print_r($params, 1)); // cgf
      $rawData = $this->curl($url, 'POST', $params);
/**/ if($this->debugMode) debug("Got response:" . print_r($rawData, 1));
      return $rawData;
  }

  /**
   * Executes GET requests against API
   * 
   * @param string $request
   * @param array $params
   * @return array|null Array of results or null if json_decode fails in curl()
   */
  protected function get($request, $params = array()) {
      $params['oauth_token'] = $this->oauthToken;

      $delimiter = (strpos($request, '?') === FALSE) ? '?' : '&';
      $url = $this->apiServerUrl . $request . $delimiter . http_build_query($params);

/**/ if($this->debugMode) debug("Getting request from: {$url}");
      
      $rawData = $this->curl($url, 'GET');

/**/ if($this->debugMode) debug("Got response:" . print_r($rawData, 1));

      return $rawData;
  }

  /**
   * Execute curl request
   * 
   * @param string $url URL to send requests
   * @param string $method HTTP method
   * @param array $params request params
   * @return array|null Returns array of results or null if json_decode fails
   */
  protected function curl($url, $method = 'GET', $params = array()) {
      $ch = curl_init(); // Set up our CURL request
      $timeout = isDEV ? 20 : 10; // cgf (was 5)

      if ($fileName = @$params['file']) { // cgf: no json for image file uploads
        $data = array('fileName' => '@' . $fileName); // . ';type=image/jpeg'); // magic curl syntax
        //curl_setopt($ch, CURLOPT_POST, true);
        //$url .= '&fileName=' . ($fileName);
        $headers = array('Content-Type: image/jpeg'); // or multipart/form-data or image/jpeg
      } else {
        $data = u\jsonEncode($params); // Encode POST data
        $headers = array('Accept: application/json', 'Content-Type: application/json;charset=UTF-8');
        if ($method == 'POST') $headers[] = 'Content-Length: ' . strlen($data);
      }

      if (isDEV) { // cgf (for sandbox)
        curl_setopt($ch, CURLOPT_SSL_VERIFYHOST, FALSE);
        curl_setopt($ch, CURLOPT_SSL_VERIFYPEER, FALSE);
      }
      
      if( strtoupper (substr(PHP_OS, 0,3)) == 'WIN' ) { // Windows requires this certificate
        $ca = dirname(__FILE__);
        curl_setopt($ch, CURLOPT_CAINFO, $ca); // Set the location of the CA-bundle
        curl_setopt($ch, CURLOPT_CAINFO, $ca . '/cacert.crt'); // Set the location of the CA-bundle
      }     

      curl_setopt($ch, CURLOPT_URL, $url);
      curl_setopt($ch, CURLOPT_CUSTOMREQUEST, $method);
      curl_setopt($ch, CURLOPT_POSTFIELDS, $data);
      curl_setopt($ch, CURLOPT_CONNECTTIMEOUT, $timeout);
      curl_setopt($ch, CURLOPT_TIMEOUT, $timeout);
      curl_setopt($ch, CURLOPT_RETURNTRANSFER, true);
      curl_setopt($ch, CURLOPT_FOLLOWLOCATION, true);
      curl_setopt($ch, CURLOPT_HEADER, FALSE);
      curl_setopt($ch, CURLOPT_HTTPHEADER, $headers);
      
      $rawData = curl_exec($ch); // Initiate request

      $code = curl_getinfo($ch, CURLINFO_HTTP_CODE); // get HTTP response
      if ($code !== 200) {
        $curlErr = print_r(curl_error($ch), 1);
        $Success = FALSE;
        $Message = "Request failed. Server responded with: {$code} -- {$curlErr}";
        $response = compact('Success', 'Message');
      } else $response = json_decode($rawData, true); // if no error, assume we got a json response

      if ($this->debugMode and !(bool) @$response['Success']) {
/**/    debug(compact('data','headers','method'));
/**/    debug(curl_getinfo($ch));
      }
      curl_close($ch); // All done with CURL
      return $response;
  }

  /**
   * @param string $token oauth token
   */
  public function setToken($token) {
      $this->oauthToken = $token;
  }

  /**
   * @return string oauth token
   */
  public function getToken() {
      return $this->oauthToken;
  }

  /**
   * Sets client mode.  Appropriate values are 'live' and 'test'
   * 
   * @param string $mode
   * @throws InvalidArgumentException
   * @return void
   */
  public function setMode($mode = 'live') {
      $mode = strtolower($mode);
      
      if ($mode != 'live' && $mode != 'test') {
          throw new InvalidArgumentException('Appropriate mode values are live or test');
      }

      $this->mode = $mode;
  }

  /**
   * @return string Client mode
   */
  public function getMode() {
      return $this->mode;
  }

  /**
   * Set debug mode
   * 
   * @return boolean True
   */
/**/ public function setDebug($mode) {
    $this->debugMode = $mode;
    return true;
  }
}
