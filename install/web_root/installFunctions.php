<?php

function changeWLANModeAndRestart($mode){
	global $wlanConfigChangeModeScript;
	$data = array();
	$returncode = -1;
	exec($wlanConfigChangeModeScript . ' "'.$mode.'"', $data, $returncode);
//	echo "returncode was:".$returncode."\r\n";
//	print_r($data);
	return $data[count($data)-1];
}

function reconnectWLAN(){
	global $wlanReconnectScript;
	$data = array();
	$returncode = -1;
	exec($wlanReconnectScript, $data, $returncode);
//	echo "returncode was:".$returncode."\r\n";
//	print_r($data);
	return $data[count($data)-1];
}

/*
function getWlanMode(){
	global $wlanModeCommand_iwconfig;
	$data = array();
	$returncode = -1;
	exec($wlanModeCommand, $data, $returncode);
//	echo "returncode was:".$returncode."\r\n";
//	print_r($data);
	if ($data[0] == "Master") {
		return "ap";
	} else if ($data[0] == "Managed") {
		return "wpa";
	} else {
		return $data[0];
	}
}
*/

function getWlanMode(){
	global $wlanModeCommand;
	$data = array();
	$returncode = -1;
	exec($wlanModeCommand, $data, $returncode);
//	echo "returncode was:".$returncode."\r\n";
//	print_r($data);
	if ($data[0] == "_wlan_ap") {
		return "ap";
	} else if ($data[0] == "_wlan_wpa") {
		return "wpa";
	} else if ($data[0] == "") {
		return "invalide Mode";
	} else {
		return $data[0];
	}
}

function checkWlanConnected(){
	global $checkWlanConnectedCommand;
	global $wlanMode;
	global $wlanIP;
	$data = array();
	$returncode = -1;
	exec($checkWlanConnectedCommand, $data, $returncode);
//	echo "returncode was:".$returncode."\r\n";
//	print_r($data);
	if (count($data)>0){
		$wlanIP = $data[0];
	}
		
	if ($wlanMode == "wpa"){
		return ($returncode === 0);
	} else {
		return false;
	}
}

function showWlanInfos(){
	global $wlanMode;
	global $wlanIP;
	if ($wlanMode == "wpa"){
		echo '<span style="font-weight:bold;">Wlan Mode is Normal</span><br>';
		echo '<a href="?wlanAccessPoint=ap">change WLAN config to AccessPoint</a><br>';
	} else if ($wlanMode == "ap"){
		echo '<span style="font-weight:bold;">Wlan Mode is AccessPoint</span><br>';
		echo '<a href="?wlanAccessPoint=wpa">change WLAN config to normal</a><br>';
	} else {
		echo '<span style="font-weight:bold;">Wlan Mode is '.$wlanMode.'</span><br>';
		echo '<a href="?wlanAccessPoint=ap">change WLAN config to AccessPoint</a><br>';
		echo '<a href="?wlanAccessPoint=wpa">change WLAN config to normal</a><br>';
	}
	echo "<br>\r\n";
	if ($wlanIP == ''){
		echo '<span style="font-weight:bold;">Wlan has not yet a IP</span><br>';
	} else {
		echo '<span style="font-weight:bold;">Wlan IP is:' . $wlanIP . '</span><br>';
	}
	echo "<br>\r\n";
	
	$ssids = getAvailableWlan();
	$configuredWlans = getConfiguredWlans();
	$configuredAmount = count($configuredWlans);
	if (count($ssids)>0){
		echo "available wlans: <br>\r\n";
		for ($i=0; $i<count($ssids);++$i){
			$alreadyConfigured = false;
			for ($j=0; $j<count($configuredWlans);++$j){
				if ($configuredWlans[$j] == $ssids[$i]){
					$alreadyConfigured = true;
					unset ($configuredWlans[$j]);
					break;
				}
			}
			
			echo "<a href=\"?wlan_ssid=".$ssids[$i]."\">".$ssids[$i]."</a>" . (($alreadyConfigured)?" (a config already exist)":"") . "<br>\r\n";
		}
		echo "<br>\r\n";
	}
	if (count($configuredWlans)>0){
		if ($configuredAmount > count($configuredWlans)){
			echo "other configured wlans: <br>\r\n";
		} else {
			echo "configured wlans: <br>\r\n";
		}
		for ($j=0; $j<count($configuredWlans);++$j){
			echo $configuredWlans[$j]. "<br>\r\n";
		}
		echo "<br>\r\n";
	}
	if (count($ssids)>0){
		if ($configuredAmount > count($configuredWlans)){
			echo '<a href="?reconnectWLAN=true">retry to connect</a><br>';
		}
	}
}

function addWLANAndRestart($name, $pw){
	global $wlanConfigAddScript;
	$data = array();
	$returncode = -1;
	exec($wlanConfigAddScript . ' "'.$name.'" "'.$pw.'"', $data, $returncode);
//	echo "returncode was:".$returncode."\r\n";
//	print_r($data);
	return $data[count($data)-1];
}

function getConfiguredWlans(){
	global $wlanConfiguredWlansScript;
	$data = array();
	$returncode = -1;
	exec($wlanConfiguredWlansScript, $data, $returncode);
//	echo "returncode was:".$returncode."\r\n";
//	print_r($data);
	return $data;
}


function getAvailableWlan(){
	global $availableWlanCommand;
	$data = array();
	$returncode = -1;
	exec($availableWlanCommand, $data, $returncode);
//	echo "returncode was:".$returncode."\r\n";
//	print_r($data);
	return $data;
}

function addWLAN(){
	global $userSet;
	$showForm = true;
	if (isset($_POST['name']) && isset($_POST['passphrase'])){
		$name = $_POST['name'];
		$pw = $_POST['passphrase'];
		if ($name != '' && $pw != ''){
			$result = addWLANAndRestart($name, $pw);
			if (substr($result,0,5) == 'Error'){
				echo $result . "<br>\r\n";
			} else {
				$_SESSION['wlanAdded'] = true;
				echo "wlan ip is: " . $result . "<br>\r\n";
				if ($userSet){
					header("Location: http://".$result."/db/");
					exit;
				}
			}
			$showForm = false;
		}
	}
	return $showForm;
}

function updateUserData(){
	$data = array();
	$returncode = -1;
	exec('/opt/EveryCook/sync/sync_privdata.sh', $data, $returncode);
//	echo "returncode was:".$returncode."\r\n";
//	print_r($data);
	if ($returncode != 0 || count($data)==0 || substr($data[0],0,5) == 'Error'){
		return $data[0];
	} else {
		return true;
	}
}

function setUserToSynch($check){
	global $validateUrl, $syncCredentialsFile;
	$showForm = true;
	if (isset($_POST['user']) && isset($_POST['pw'])){
		$user = $_POST['user'];
		$pw = $_POST['pw'];
		if ($user != '' && $pw != ''){
			if ($check){
				require_once("remotefileinfo.php");
				$inhalt=remote_file($validateUrl . '?user='.$user.'&pw='.$pw);
				$jsonValue=json_decode($inhalt);
				if (isset($jsonValue->error)){
					echo "error on validate user: " . $jsonValue->error;
				} else if (isset($jsonValue->success) && $jsonValue->success){
					exec('echo "user='.$user.'&pw='.$pw.'" > ' . $syncCredentialsFile);
					$result = updateUserData();
					if ($result === true){
						echo 'UserData loaded successful.';
						$showForm = false;
					} else {
						echo $result;
					}
				} else {
					echo "error on read validate: " . $inhalt;
				}
			} else {
				exec('echo "user='.$user.'&pw='.$pw.'" > ' . $syncCredentialsFile);
				$result = updateUserData();
				if ($result === true){
					echo 'UserData updated.';
					$showForm = false;
				} else {
					echo $result;
				}
				$showForm = false;
			}
		}
	}
	return $showForm;
}
?>